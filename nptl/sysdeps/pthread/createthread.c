/* Copyright (C) 2002, 2003, 2004, 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <sched.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <atomic.h>
#include <ldsodefs.h>
#include <tls.h>

#include "kernel-features.h"


#define CLONE_SIGNAL    	(CLONE_SIGHAND | CLONE_THREAD)

/* Unless otherwise specified, the thread "register" is going to be
   initialized with a pointer to the TCB.  */
#ifndef TLS_VALUE
# define TLS_VALUE pd
#endif

#ifndef ARCH_CLONE
# define ARCH_CLONE __clone
#endif


#ifndef TLS_MULTIPLE_THREADS_IN_TCB
/* Pointer to the corresponding variable in libc.  */
int *__libc_multiple_threads_ptr attribute_hidden;
#endif


static int
do_clone (struct pthread *pd, const struct pthread_attr *attr,
	  int clone_flags, int (*fct) (void *), STACK_VARIABLES_PARMS,
	  int stopped)
{
#ifdef PREPARE_CREATE
  PREPARE_CREATE;
#endif

  if (stopped)
    /* We Make sure the thread does not run far by forcing it to get a
       lock.  We lock it here too so that the new thread cannot continue
       until we tell it to.  */
    lll_lock (pd->lock);

  /* One more thread.  We cannot have the thread do this itself, since it
     might exist but not have been scheduled yet by the time we've returned
     and need to check the value to behave correctly.  We must do it before
     creating the thread, in case it does get scheduled first and then
     might mistakenly think it was the only thread.  In the failure case,
     we momentarily store a false value; this doesn't matter because there
     is no kosher thing a signal handler interrupting us right here can do
     that cares whether the thread count is correct.  */
  atomic_increment (&__nptl_nthreads);

  if (ARCH_CLONE (fct, STACK_VARIABLES_ARGS, clone_flags,
		  pd, &pd->tid, TLS_VALUE, &pd->tid) == -1)
    {
      atomic_decrement (&__nptl_nthreads); /* Oops, we lied for a second.  */

      /* Failed.  If the thread is detached, remove the TCB here since
	 the caller cannot do this.  The caller remembered the thread
	 as detached and cannot reverify that it is not since it must
	 not access the thread descriptor again.  */
      if (IS_DETACHED (pd))
	__deallocate_stack (pd);

      return errno;
    }

  /* Now we have the possibility to set scheduling parameters etc.  */
  if (__builtin_expect (stopped != 0, 0))
    {
      INTERNAL_SYSCALL_DECL (err);
      int res = 0;

      /* Set the affinity mask if necessary.  */
      if (attr->cpuset != NULL)
	{
	  res = INTERNAL_SYSCALL (sched_setaffinity, err, 3, pd->tid,
				  sizeof (cpu_set_t), attr->cpuset);

	  if (__builtin_expect (INTERNAL_SYSCALL_ERROR_P (res, err), 0))
	    {
	      /* The operation failed.  We have to kill the thread.  First
		 send it the cancellation signal.  */
	      INTERNAL_SYSCALL_DECL (err2);
	    err_out:
#if __ASSUME_TGKILL
	      (void) INTERNAL_SYSCALL (tgkill, err2, 3,
				       THREAD_GETMEM (THREAD_SELF, pid),
				       pd->tid, SIGCANCEL);
#else
	      (void) INTERNAL_SYSCALL (tkill, err2, 2, pd->tid, SIGCANCEL);
#endif

	      return (INTERNAL_SYSCALL_ERROR_P (res, err)
		      ? INTERNAL_SYSCALL_ERRNO (res, err)
		      : 0);
	    }
	}

      /* Set the scheduling parameters.  */
      if ((attr->flags & ATTR_FLAG_NOTINHERITSCHED) != 0)
	{
	  res = INTERNAL_SYSCALL (sched_setscheduler, err, 3, pd->tid,
				  pd->schedpolicy, &pd->schedparam);

	  if (__builtin_expect (INTERNAL_SYSCALL_ERROR_P (res, err), 0))
	    goto err_out;
	}
    }

  /* We now have for sure more than one thread.  The main thread might
     not yet have the flag set.  No need to set the global variable
     again if this is what we use.  */
  THREAD_SETMEM (THREAD_SELF, header.multiple_threads, 1);

  return 0;
}


static int
create_thread (struct pthread *pd, const struct pthread_attr *attr,
	       STACK_VARIABLES_PARMS)
{
#ifdef TLS_TCB_AT_TP
  assert (pd->header.tcb != NULL);
#endif

  /* We rely heavily on various flags the CLONE function understands:

     CLONE_VM, CLONE_FS, CLONE_FILES
	这些标志根据POSIX要求选择具有共享地址空间和文件描述符的语义。

     CLONE_SIGNAL
	此标志选择POSIX信号语义。

     CLONE_SETTLS
	CLONE的第六个参数确定新线程的TLS区域。

     CLONE_PARENT_SETTID
	内核将新创建的线程的线程ID写入CLONE的第五个参数指向的位置。

	请注意，它在语义上等同于使用CLONE_CHILD_SETTID，但在内核中成本更高。

     CLONE_CHILD_CLEARTID
	内核清除在第7个参数指向CLONE的位置调用sys_exit()的线程的线程 ID。

     CLONE_DETACHED
	如果线程存在并自动收割，则不会生成信号。

     终止信号被选择为零，这意味着没有信号被发送。
     */
  // 注意：#define CLONE_SIGNAL    	(CLONE_SIGHAND | CLONE_THREAD)
  int clone_flags = (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGNAL
		     | CLONE_SETTLS | CLONE_PARENT_SETTID
		     | CLONE_CHILD_CLEARTID | CLONE_SYSVSEM
#if __ASSUME_NO_CLONE_DETACHED == 0
		     | CLONE_DETACHED
#endif
		     | 0);

  if (__builtin_expect (THREAD_GETMEM (THREAD_SELF, report_events), 0))
    {
      /* The parent thread is supposed to report events.  Check whether
	 the TD_CREATE event is needed, too.  */
      const int _idx = __td_eventword (TD_CREATE);
      const uint32_t _mask = __td_eventmask (TD_CREATE);

      if ((_mask & (__nptl_threads_events.event_bits[_idx]
		    | pd->eventbuf.eventmask.event_bits[_idx])) != 0)
	{
	  /* We always must have the thread start stopped.  */
	  pd->stopped_start = true;

	  /* 创建线程。我们总是创建停止的线程，以便在我们告诉调试器之前它不会走得太远。 */
	  int res = do_clone (pd, attr, clone_flags, start_thread,
			      STACK_VARIABLES_ARGS, 1);
	  if (res == 0)
	    {
	      /* Now fill in the information about the new thread in
		 the newly created thread's data structure.  We cannot let
		 the new thread do this since we don't know whether it was
		 already scheduled when we send the event.  */
	      pd->eventbuf.eventnum = TD_CREATE;
	      pd->eventbuf.eventdata = pd;

	      /* Enqueue the descriptor.  */
	      do
		pd->nextevent = __nptl_last_event;
	      while (atomic_compare_and_exchange_bool_acq (&__nptl_last_event,
							   pd, pd->nextevent)
		     != 0);

	      /* Now call the function which signals the event.  */
	      __nptl_create_event ();

	      /* And finally restart the new thread.  */
	      lll_unlock (pd->lock);
	    }

	  return res;
	}
    }

#ifdef NEED_DL_SYSINFO
  assert (THREAD_SELF_SYSINFO == THREAD_SYSINFO (pd));
#endif

  /* Determine whether the newly created threads has to be started
     stopped since we have to set the scheduling parameters or set the
     affinity.  */
  bool stopped = false;
  if (attr != NULL && (attr->cpuset != NULL
		       || (attr->flags & ATTR_FLAG_NOTINHERITSCHED) != 0))
    stopped = true;
  pd->stopped_start = stopped;
  pd->parent_cancelhandling = THREAD_GETMEM (THREAD_SELF, cancelhandling);

  /* Actually create the thread.  */
  int res = do_clone (pd, attr, clone_flags, start_thread,
		      STACK_VARIABLES_ARGS, stopped);

  if (res == 0 && stopped)
    /* And finally restart the new thread.  */
    lll_unlock (pd->lock);

  return res;
}
