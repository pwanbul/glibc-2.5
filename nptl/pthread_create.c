/* Copyright (C) 2002, 2003, 2004, 2005, 2006 Free Software Foundation, Inc.
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

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "pthreadP.h"
#include <hp-timing.h>
#include <ldsodefs.h>
#include <atomic.h>
#include <libc-internal.h>
#include <resolv.h>

#include <shlib-compat.h>


/* Local function to start thread and handle cleanup.  */
static int start_thread (void *arg);


/* Nozero if debugging mode is enabled.  */
int __pthread_debug;

/* Globally enabled events.  */
static td_thr_events_t __nptl_threads_events;

/* Pointer to descriptor with the last event.  */
static struct pthread *__nptl_last_event;

/* Number of threads running.  */
unsigned int __nptl_nthreads = 1;


/* Code to allocate and deallocate a stack.  */
#include "allocatestack.c"

/* Code to create the thread.  */
#include <createthread.c>


struct pthread *
internal_function
__find_in_stack_list (pd)
     struct pthread *pd;
{
  list_t *entry;
  struct pthread *result = NULL;

  lll_lock (stack_cache_lock);

  list_for_each (entry, &stack_used)
    {
      struct pthread *curp;

      curp = list_entry (entry, struct pthread, list);
      if (curp == pd)
	{
	  result = curp;
	  break;
	}
    }

  if (result == NULL)
    list_for_each (entry, &__stack_user)
      {
	struct pthread *curp;

	curp = list_entry (entry, struct pthread, list);
	if (curp == pd)
	  {
	    result = curp;
	    break;
	  }
      }

  lll_unlock (stack_cache_lock);

  return result;
}


/* Deallocate POSIX thread-local-storage.  */
void
attribute_hidden
__nptl_deallocate_tsd (void)
{
  struct pthread *self = THREAD_SELF;

  /* Maybe no data was ever allocated.  This happens often so we have
     a flag for this.  */
  if (THREAD_GETMEM (self, specific_used))
    {
      size_t round;
      size_t cnt;

      round = 0;
      do
	{
	  size_t idx;

	  /* So far no new nonzero data entry.  */
	  THREAD_SETMEM (self, specific_used, false);

	  for (cnt = idx = 0; cnt < PTHREAD_KEY_1STLEVEL_SIZE; ++cnt)
	    {
	      struct pthread_key_data *level2;

	      level2 = THREAD_GETMEM_NC (self, specific, cnt);

	      if (level2 != NULL)
		{
		  size_t inner;

		  for (inner = 0; inner < PTHREAD_KEY_2NDLEVEL_SIZE;
		       ++inner, ++idx)
		    {
		      void *data = level2[inner].data;

		      if (data != NULL)
			{
			  /* Always clear the data.  */
			  level2[inner].data = NULL;

			  /* Make sure the data corresponds to a valid
			     key.  This test fails if the key was
			     deallocated and also if it was
			     re-allocated.  It is the user's
			     responsibility to free the memory in this
			     case.  */
			  if (level2[inner].seq
			      == __pthread_keys[idx].seq
			      /* It is not necessary to register a destructor
				 function.  */
			      && __pthread_keys[idx].destr != NULL)
			    /* Call the user-provided destructor.  */
			    __pthread_keys[idx].destr (data);
			}
		    }
		}
	      else
		idx += PTHREAD_KEY_1STLEVEL_SIZE;
	    }

	  if (THREAD_GETMEM (self, specific_used) == 0)
	    /* No data has been modified.  */
	    goto just_free;
	}
      /* We only repeat the process a fixed number of times.  */
      while (__builtin_expect (++round < PTHREAD_DESTRUCTOR_ITERATIONS, 0));

      /* Just clear the memory of the first block for reuse.  */
      memset (&THREAD_SELF->specific_1stblock, '\0',
	      sizeof (self->specific_1stblock));

    just_free:
      /* Free the memory for the other blocks.  */
      for (cnt = 1; cnt < PTHREAD_KEY_1STLEVEL_SIZE; ++cnt)
	{
	  struct pthread_key_data *level2;

	  level2 = THREAD_GETMEM_NC (self, specific, cnt);
	  if (level2 != NULL)
	    {
	      /* The first block is allocated as part of the thread
		 descriptor.  */
	      free (level2);
	      THREAD_SETMEM_NC (self, specific, cnt, NULL);
	    }
	}

      THREAD_SETMEM (self, specific_used, false);
    }
}


/* 在可选地确保线程描述符仍然有效之后，释放线程的堆栈。  */
void
internal_function
__free_tcb (struct pthread *pd)
{
  /* The thread is exiting now.  */
  if (__builtin_expect (atomic_bit_test_set (&pd->cancelhandling,
					     TERMINATED_BIT) == 0, 1))
    {
      /* Remove the descriptor from the list.  */
      if (DEBUGGING_P && __find_in_stack_list (pd) == NULL)
	/* Something is really wrong.  The descriptor for a still
	   running thread is gone.  */
	abort ();

      /* Free TPP data.  */
      if (__builtin_expect (pd->tpp != NULL, 0))
	{
	  struct priority_protection_data *tpp = pd->tpp;

	  pd->tpp = NULL;
	  free (tpp);
	}

      /* Queue the stack memory block for reuse and exit the process.  The
	 kernel will signal via writing to the address returned by
	 QUEUE-STACK when the stack is available.  */
      __deallocate_stack (pd);
    }
}


static int
start_thread (void *arg)
{
  struct pthread *pd = (struct pthread *) arg;

#if HP_TIMING_AVAIL
  /* Remember the time when the thread was started.  */
  hp_timing_t now;
  HP_TIMING_NOW (now);
  THREAD_SETMEM (pd, cpuclock_offset, now);
#endif

  /* Initialize resolver state pointer.  */
  __resp = &pd->res;

#ifdef __NR_set_robust_list
# ifndef __ASSUME_SET_ROBUST_LIST
  if (__set_robust_list_avail >= 0)
# endif
    {
      INTERNAL_SYSCALL_DECL (err);
      /* This call should never fail because the initial call in init.c
	 succeeded.  */
      INTERNAL_SYSCALL (set_robust_list, err, 2, &pd->robust_head,
			sizeof (struct robust_list_head));
    }
#endif

  /* If the parent was running cancellation handlers while creating
     the thread the new thread inherited the signal mask.  Reset the
     cancellation signal mask.  */
  if (__builtin_expect (pd->parent_cancelhandling & CANCELING_BITMASK, 0))
    {
      INTERNAL_SYSCALL_DECL (err);
      sigset_t mask;
      __sigemptyset (&mask);
      __sigaddset (&mask, SIGCANCEL);
      (void) INTERNAL_SYSCALL (rt_sigprocmask, err, 4, SIG_UNBLOCK, &mask,
			       NULL, _NSIG / 8);
    }

  /* This is where the try/finally block should be created.  For
     compilers without that support we do use setjmp.  */
  struct pthread_unwind_buf unwind_buf;

  /* No previous handlers.  */
  unwind_buf.priv.data.prev = NULL;
  unwind_buf.priv.data.cleanup = NULL;

  int not_first_call;
  not_first_call = setjmp ((struct __jmp_buf_tag *) unwind_buf.cancel_jmp_buf);
  if (__builtin_expect (! not_first_call, 1))
    {
      /* Store the new cleanup handler info.  */
      THREAD_SETMEM (pd, cleanup_jmp_buf, &unwind_buf);

      if (__builtin_expect (pd->stopped_start, 0))
	{
	  int oldtype = CANCEL_ASYNC ();

	  /* Get the lock the parent locked to force synchronization.  */
	  lll_lock (pd->lock);
	  /* And give it up right away.  */
	  lll_unlock (pd->lock);

	  CANCEL_RESET (oldtype);
	}

      /* Run the code the user provided.  */
#ifdef CALL_THREAD_FCT
      THREAD_SETMEM (pd, result, CALL_THREAD_FCT (pd));
#else
      THREAD_SETMEM (pd, result, pd->start_routine (pd->arg));
#endif
    }

  /* Run the destructor for the thread-local data.  */
  __nptl_deallocate_tsd ();

  /* Clean up any state libc stored in thread-local variables.  */
  __libc_thread_freeres ();

  /* If this is the last thread we terminate the process now.  We
     do not notify the debugger, it might just irritate it if there
     is no thread left.  */
  if (__builtin_expect (atomic_decrement_and_test (&__nptl_nthreads), 0))
    /* This was the last thread.  */
    exit (0);

  /* Report the death of the thread if this is wanted.  */
  if (__builtin_expect (pd->report_events, 0))
    {
      /* See whether TD_DEATH is in any of the mask.  */
      const int idx = __td_eventword (TD_DEATH);
      const uint32_t mask = __td_eventmask (TD_DEATH);

      if ((mask & (__nptl_threads_events.event_bits[idx]
		   | pd->eventbuf.eventmask.event_bits[idx])) != 0)
	{
	  /* Yep, we have to signal the death.  Add the descriptor to
	     the list but only if it is not already on it.  */
	  if (pd->nextevent == NULL)
	    {
	      pd->eventbuf.eventnum = TD_DEATH;
	      pd->eventbuf.eventdata = pd;

	      do
		pd->nextevent = __nptl_last_event;
	      while (atomic_compare_and_exchange_bool_acq (&__nptl_last_event,
							   pd, pd->nextevent));
	    }

	  /* Now call the function to signal the event.  */
	  __nptl_death_event ();
	}
    }

  /* The thread is exiting now.  Don't set this bit until after we've hit
     the event-reporting breakpoint, so that td_thr_get_info on us while at
     the breakpoint reports TD_THR_RUN state rather than TD_THR_ZOMBIE.  */
  atomic_bit_set (&pd->cancelhandling, EXITING_BIT);

#ifndef __ASSUME_SET_ROBUST_LIST
  /* If this thread has any robust mutexes locked, handle them now.  */
# if __WORDSIZE == 64
  void *robust = pd->robust_head.list;
# else
  __pthread_slist_t *robust = pd->robust_list.__next;
# endif
  /* We let the kernel do the notification if it is able to do so.
     If we have to do it here there for sure are no PI mutexes involved
     since the kernel support for them is even more recent.  */
  if (__set_robust_list_avail < 0
      && __builtin_expect (robust != (void *) &pd->robust_head, 0))
    {
      do
	{
	  struct __pthread_mutex_s *this = (struct __pthread_mutex_s *)
	    ((char *) robust - offsetof (struct __pthread_mutex_s,
					 __list.__next));
	  robust = *((void **) robust);

# ifdef __PTHREAD_MUTEX_HAVE_PREV
	  this->__list.__prev = NULL;
# endif
	  this->__list.__next = NULL;

	  lll_robust_mutex_dead (this->__lock);
	}
      while (robust != (void *) &pd->robust_head);
    }
#endif

  /* If the thread is detached free the TCB.  */
  if (IS_DETACHED (pd))
    /* Free the TCB.  */
    __free_tcb (pd);
  else if (__builtin_expect (pd->cancelhandling & SETXID_BITMASK, 0))
    {
      /* Some other thread might call any of the setXid functions and expect
	 us to reply.  In this case wait until we did that.  */
      do
	lll_futex_wait (&pd->setxid_futex, 0);
      while (pd->cancelhandling & SETXID_BITMASK);

      /* Reset the value so that the stack can be reused.  */
      pd->setxid_futex = 0;
    }

  /* We cannot call '_exit' here.  '_exit' will terminate the process.

     The 'exit' implementation in the kernel will signal when the
     process is really dead since 'clone' got passed the CLONE_CLEARTID
     flag.  The 'tid' field in the TCB will be set to zero.

     The exit code is zero since in case all threads exit by calling
     'pthread_exit' the exit status must be 0 (zero).  */
  __exit_thread_inline (0);

  /* NOTREACHED */
  return 0;
}


/* 用户未提供任何情况下的默认线程属性。  */
static const struct pthread_attr default_attr =
  {
    /* 只是一些 > 0 的值四舍五入到最接近的页面大小。  */
    .guardsize = 1,
  };


int
__pthread_create_2_1 (newthread, attr, start_routine, arg)
     pthread_t *newthread;
     const pthread_attr_t *attr;
     void *(*start_routine) (void *);
     void *arg;
{
  STACK_VARIABLES;

  const struct pthread_attr *iattr = (struct pthread_attr *) attr;
  if (iattr == NULL)
    /* 这是最好的主意吗？在 NUMA 机器上，这可能意味着访问远处的内存。  */
    iattr = &default_attr;

  struct pthread *pd = NULL;
  int err = ALLOCATE_STACK (iattr, &pd);        //
  if (__builtin_expect (err != 0, 0))
    /* 出了些问题。也许属性的参数无效或我们无法分配内存。  */
    return err;


  /* 初始化 TCB。所有零初始化都应在“get_cached_stack”中执行。
   * 这样，如果堆栈新分配了“mmap”，我们就可以避免这样做。  */

#ifdef TLS_TCB_AT_TP
  /* Reference to the TCB itself.  */
  pd->header.self = pd;

  /* Self-reference for TLS.  */
  pd->header.tcb = pd;
#endif

  /* 存储启动例程的地址和参数。
   * 由于我们不直接启动函数，死线程将从其线程描述符中获取信息。
   * */
  pd->start_routine = start_routine;
  pd->arg = arg;

  /* Copy the thread attribute flags.  */
  struct pthread *self = THREAD_SELF;
  pd->flags = ((iattr->flags & ~(ATTR_FLAG_SCHED_SET | ATTR_FLAG_POLICY_SET))
	       | (self->flags & (ATTR_FLAG_SCHED_SET | ATTR_FLAG_POLICY_SET)));

  /* 初始化正在等待我们的线程的 ID 字段。
   * 这是一个自引用，以防线程被分离创建。
   * */
  pd->joinid = iattr->flags & ATTR_FLAG_DETACHSTATE ? pd : NULL;

  /* 调试事件是从父级继承的。  */
  pd->eventbuf = self->eventbuf;


  /* 复制父级的调度参数。标志将说明什么是有效的，什么是无效的。
   * */
  pd->schedpolicy = self->schedpolicy;
  pd->schedparam = self->schedparam;

  /* 复制堆栈保护金丝雀。  */
#ifdef THREAD_COPY_STACK_GUARD
  THREAD_COPY_STACK_GUARD (pd);
#endif

  /*复制指针保护值。  */
#ifdef THREAD_COPY_POINTER_GUARD
  THREAD_COPY_POINTER_GUARD (pd);
#endif

  /* 确定线程的调度参数。  */
  if (attr != NULL
      && __builtin_expect ((iattr->flags & ATTR_FLAG_NOTINHERITSCHED) != 0, 0)
      && (iattr->flags & (ATTR_FLAG_SCHED_SET | ATTR_FLAG_POLICY_SET)) != 0)
    {
      INTERNAL_SYSCALL_DECL (scerr);

      /* Use the scheduling parameters the user provided.  */
      if (iattr->flags & ATTR_FLAG_POLICY_SET)
	pd->schedpolicy = iattr->schedpolicy;
      else if ((pd->flags & ATTR_FLAG_POLICY_SET) == 0)
	{
	  pd->schedpolicy = INTERNAL_SYSCALL (sched_getscheduler, scerr, 1, 0);
	  pd->flags |= ATTR_FLAG_POLICY_SET;
	}

      if (iattr->flags & ATTR_FLAG_SCHED_SET)
	memcpy (&pd->schedparam, &iattr->schedparam,
		sizeof (struct sched_param));
      else if ((pd->flags & ATTR_FLAG_SCHED_SET) == 0)
	{
	  INTERNAL_SYSCALL (sched_getparam, scerr, 2, 0, &pd->schedparam);
	  pd->flags |= ATTR_FLAG_SCHED_SET;
	}

      /* Check for valid priorities.  */
      int minprio = INTERNAL_SYSCALL (sched_get_priority_min, scerr, 1,
				      iattr->schedpolicy);
      int maxprio = INTERNAL_SYSCALL (sched_get_priority_max, scerr, 1,
				      iattr->schedpolicy);
      if (pd->schedparam.sched_priority < minprio
	  || pd->schedparam.sched_priority > maxprio)
	{
	  err = EINVAL;
	  goto errout;
	}
    }

  /* 将描述符传递给调用者。  */
  *newthread = (pthread_t) pd;

  /* 记住线程是否分离。如果出现错误，我们必须释放未分离的死线程堆栈。*/
  bool is_detached = IS_DETACHED (pd);

  /* Start the thread.  */
  err = create_thread (pd, iattr, STACK_VARIABLES_ARGS);
  if (err != 0)
    {
      /* Something went wrong.  Free the resources.  */
      if (!is_detached)
	{
	errout:
	  __deallocate_stack (pd);
	}
      return err;
    }

  return 0;
}
versioned_symbol (libpthread, __pthread_create_2_1, pthread_create, GLIBC_2_1);


#if SHLIB_COMPAT(libpthread, GLIBC_2_0, GLIBC_2_1)
int
__pthread_create_2_0 (newthread, attr, start_routine, arg)
     pthread_t *newthread;
     const pthread_attr_t *attr;
     void *(*start_routine) (void *);
     void *arg;
{
  /* ATTR 属性实际上不是“pthread_attr_t”类型。它具有旧大小并且访问新成员可能会使程序崩溃。我们现在转换结构。  */
  struct pthread_attr new_attr;

  if (attr != NULL)
    {
      struct pthread_attr *iattr = (struct pthread_attr *) attr;
      size_t ps = __getpagesize ();

      /* 从用户提供的属性中复制值。  */
      new_attr.schedparam = iattr->schedparam;
      new_attr.schedpolicy = iattr->schedpolicy;
      new_attr.flags = iattr->flags;

      /* 为旧实现中不存在的字段填写默认值。  */
      new_attr.guardsize = ps;
      new_attr.stackaddr = NULL;
      new_attr.stacksize = 0;
      new_attr.cpuset = NULL;

      /* 我们会将这个值传递给真正的实现。  */
      attr = (pthread_attr_t *) &new_attr;
    }

  return __pthread_create_2_1 (newthread, attr, start_routine, arg);
}
compat_symbol (libpthread, __pthread_create_2_0, pthread_create,
	       GLIBC_2_0);
#endif

/* Information for libthread_db.  */

#include "../nptl_db/db_info.c"

/* If pthread_create is present, libgcc_eh.a and libsupc++.a expects some other POSIX thread
   functions to be present as well.  */
PTHREAD_STATIC_FN_REQUIRE (pthread_mutex_lock)
PTHREAD_STATIC_FN_REQUIRE (pthread_mutex_unlock)

PTHREAD_STATIC_FN_REQUIRE (pthread_once)
PTHREAD_STATIC_FN_REQUIRE (pthread_cancel)

PTHREAD_STATIC_FN_REQUIRE (pthread_key_create)
PTHREAD_STATIC_FN_REQUIRE (pthread_setspecific)
PTHREAD_STATIC_FN_REQUIRE (pthread_getspecific)
