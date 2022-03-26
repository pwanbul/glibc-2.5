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

#ifndef _DESCR_H
#define _DESCR_H	1

#include <limits.h>
#include <sched.h>
#include <setjmp.h>
#include <stdbool.h>
#include <sys/types.h>
#include <hp-timing.h>
#include <list.h>
#include <lowlevellock.h>
#include <pthreaddef.h>
#include <dl-sysdep.h>
#include "../nptl_db/thread_db.h"
#include <tls.h>
#ifdef HAVE_FORCED_UNWIND
# include <unwind.h>
#endif
#define __need_res_state
#include <resolv.h>

#ifndef TCB_ALIGNMENT
# define TCB_ALIGNMENT	sizeof (double)
#endif


/* 我们将线程特定的数据保存在一个特殊的数据结构中，一个两级数组。
 * 顶级数组包含指向动态分配的一定数量数据指针的数组的指针。
 * 所以我们可以实现一个稀疏数组。每个动态二级数组都有PTHREAD_KEY_2NDLEVEL_SIZE条目。
 * 这个值不应该太大。
 * */
#define PTHREAD_KEY_2NDLEVEL_SIZE       32

/* We need to address PTHREAD_KEYS_MAX key with PTHREAD_KEY_2NDLEVEL_SIZE
   keys in each subarray.  */
#define PTHREAD_KEY_1STLEVEL_SIZE \
  ((PTHREAD_KEYS_MAX + PTHREAD_KEY_2NDLEVEL_SIZE - 1) \
   / PTHREAD_KEY_2NDLEVEL_SIZE)




/* Internal version of the buffer to store cancellation handler
   information.  */
struct pthread_unwind_buf
{
  struct
  {
    __jmp_buf jmp_buf;
    int mask_was_saved;
  } cancel_jmp_buf[1];

  union
  {
    /* This is the placeholder of the public version.  */
    void *pad[4];

    struct
    {
      /* Pointer to the previous cleanup buffer.  */
      struct pthread_unwind_buf *prev;

      /* Backward compatibility: state of the old-style cleanup
	 handler at the time of the previous new-style cleanup handler
	 installment.  */
      struct _pthread_cleanup_buffer *cleanup;

      /* Cancellation type before the push call.  */
      int canceltype;
    } data;
  } priv;
};


/* Opcodes and data types for communication with the signal handler to
   change user/group IDs.  */
struct xid_command
{
  int syscall_no;
  long int id[3];
  volatile int cntr;
};


/* Data structure used by the kernel to find robust futexes.  */
struct robust_list_head
{
  void *list;
  long int futex_offset;
  void *list_op_pending;
};


/* Data strcture used to handle thread priority protection.  */
struct priority_protection_data
{
  int priomax;
  unsigned int priomap[];
};


/* 线程描述符数据结构。 */
struct pthread
{
  union
  {
#if !TLS_DTV_AT_TP
    /* 这与用于不带线程的TLS的TCB重叠（参见tls.h）。  */
    tcbhead_t header;
#else
    struct
    {
      int multiple_threads;
    } header;
#endif

    /* This extra padding has no special purpose, and this structure layout
       is private and subject to change without affecting the official ABI.
       We just have it here in case it might be convenient for some
       implementation-specific instrumentation hack or suchlike.  */
    void *__padding[16];
  };

  /* This descriptor's link on the `stack_used' or `__stack_user' list.  */
  list_t list;

  /* Thread ID - which is also a 'is this thread descriptor (and
     therefore stack) used' flag.  */
  pid_t tid;

  /* Process ID - thread group ID in kernel speak.  */
  pid_t pid;

  /* List of robust mutexes the thread is holding.  */
#ifdef __PTHREAD_MUTEX_HAVE_PREV
  void *robust_prev;
  struct robust_list_head robust_head;

  /* The list above is strange.  It is basically a double linked list
     but the pointer to the next/previous element of the list points
     in the middle of the object, the __next element.  Whenever
     casting to __pthread_list_t we need to adjust the pointer
     first.  */
# define QUEUE_PTR_ADJUST (offsetof (__pthread_list_t, __next))

# define ENQUEUE_MUTEX_BOTH(mutex, val)					      \
  do {									      \
    __pthread_list_t *next = (__pthread_list_t *)			      \
      ((((uintptr_t) THREAD_GETMEM (THREAD_SELF, robust_head.list)) & ~1ul)   \
       - QUEUE_PTR_ADJUST);						      \
    next->__prev = (void *) &mutex->__data.__list.__next;		      \
    mutex->__data.__list.__next = THREAD_GETMEM (THREAD_SELF,		      \
						 robust_head.list);	      \
    mutex->__data.__list.__prev = (void *) &THREAD_SELF->robust_head;	      \
    THREAD_SETMEM (THREAD_SELF, robust_head.list,			      \
		   (void *) (((uintptr_t) &mutex->__data.__list.__next)	      \
			     | val));					      \
  } while (0)
# define DEQUEUE_MUTEX(mutex) \
  do {									      \
    __pthread_list_t *next = (__pthread_list_t *)			      \
      ((char *) (((uintptr_t) mutex->__data.__list.__next) & ~1ul)	      \
       - QUEUE_PTR_ADJUST);						      \
    next->__prev = mutex->__data.__list.__prev;				      \
    __pthread_list_t *prev = (__pthread_list_t *)			      \
      ((char *) (((uintptr_t) mutex->__data.__list.__prev) & ~1ul)	      \
       - QUEUE_PTR_ADJUST);						      \
    prev->__next = mutex->__data.__list.__next;				      \
    mutex->__data.__list.__prev = NULL;					      \
    mutex->__data.__list.__next = NULL;					      \
  } while (0)
#else
  union
  {
    __pthread_slist_t robust_list;
    struct robust_list_head robust_head;
  };

# define ENQUEUE_MUTEX_BOTH(mutex, val)					      \
  do {									      \
    mutex->__data.__list.__next						      \
      = THREAD_GETMEM (THREAD_SELF, robust_list.__next);		      \
    THREAD_SETMEM (THREAD_SELF, robust_list.__next,			      \
		   (void *) (((uintptr_t) &mutex->__data.__list) | val));     \
  } while (0)
# define DEQUEUE_MUTEX(mutex) \
  do {									      \
    __pthread_slist_t *runp = (__pthread_slist_t *)			      \
      (((uintptr_t) THREAD_GETMEM (THREAD_SELF, robust_list.__next)) & ~1ul); \
    if (runp == &mutex->__data.__list)					      \
      THREAD_SETMEM (THREAD_SELF, robust_list.__next, runp->__next);	      \
    else								      \
      {									      \
	__pthread_slist_t *next = (__pthread_slist_t *)		      \
	  (((uintptr_t) runp->__next) & ~1ul);				      \
	while (next != &mutex->__data.__list)				      \
	  {								      \
	    runp = next;						      \
	    next = (__pthread_slist_t *) (((uintptr_t) runp->__next) & ~1ul); \
	  }								      \
									      \
	runp->__next = next->__next;					      \
	mutex->__data.__list.__next = NULL;				      \
      }									      \
  } while (0)
#endif
#define ENQUEUE_MUTEX(mutex) ENQUEUE_MUTEX_BOTH (mutex, 0)
#define ENQUEUE_MUTEX_PI(mutex) ENQUEUE_MUTEX_BOTH (mutex, 1)

  /* List of cleanup buffers.  */
  struct _pthread_cleanup_buffer *cleanup;

  /* Unwind information.  */
  struct pthread_unwind_buf *cleanup_jmp_buf;
#define HAVE_CLEANUP_JMP_BUF

  /* Flags determining processing of cancellation.  */
  int cancelhandling;
  /* Bit set if cancellation is disabled.  */
#define CANCELSTATE_BIT		0
#define CANCELSTATE_BITMASK	0x01
  /* Bit set if asynchronous cancellation mode is selected.  */
#define CANCELTYPE_BIT		1
#define CANCELTYPE_BITMASK	0x02
  /* Bit set if canceling has been initiated.  */
#define CANCELING_BIT		2
#define CANCELING_BITMASK	0x04
  /* Bit set if canceled.  */
#define CANCELED_BIT		3
#define CANCELED_BITMASK	0x08
  /* Bit set if thread is exiting.  */
#define EXITING_BIT		4
#define EXITING_BITMASK		0x10
  /* Bit set if thread terminated and TCB is freed.  */
#define TERMINATED_BIT		5
#define TERMINATED_BITMASK	0x20
  /* Bit set if thread is supposed to change XID.  */
#define SETXID_BIT		6
#define SETXID_BITMASK		0x40
  /* Mask for the rest.  Helps the compiler to optimize.  */
#define CANCEL_RESTMASK		0xffffff80

#define CANCEL_ENABLED_AND_CANCELED(value) \
  (((value) & (CANCELSTATE_BITMASK | CANCELED_BITMASK | EXITING_BITMASK	      \
	       | CANCEL_RESTMASK | TERMINATED_BITMASK)) == CANCELED_BITMASK)
#define CANCEL_ENABLED_AND_CANCELED_AND_ASYNCHRONOUS(value) \
  (((value) & (CANCELSTATE_BITMASK | CANCELTYPE_BITMASK | CANCELED_BITMASK    \
	       | EXITING_BITMASK | CANCEL_RESTMASK | TERMINATED_BITMASK))     \
   == (CANCELTYPE_BITMASK | CANCELED_BITMASK))

  /* 我们在这里分配一个引用块。这应该足以避免为大多数应用程序动态分配任何内存。 */
  struct pthread_key_data
  {
    /* 序列号。我们使用uintptr_t在32位和64位机器上不需要填充。
     * 在64位机器上，它也有助于避免换行。 */
    uintptr_t seq;

    /* 数据指针。  */
    void *data;
  } specific_1stblock[PTHREAD_KEY_2NDLEVEL_SIZE];		// 32

  /* 线程特定数据的两级数组。  */
  struct pthread_key_data *specific[PTHREAD_KEY_1STLEVEL_SIZE];			// 32

  /* 设置特定数据时设置的标志，当存在tsd时，改标记为True */
  bool specific_used;

  /* True if events must be reported.  */
  bool report_events;

  /* True if the user provided the stack.  */
  bool user_stack;

  /* True if thread must stop at startup time.  */
  bool stopped_start;

  /* The parent's cancel handling at the time of the pthread_create
     call.  This might be needed to undo the effects of a cancellation.  */
  int parent_cancelhandling;

  /* Lock to synchronize access to the descriptor.  */
  lll_lock_t lock;

  /* Lock for synchronizing setxid calls.  */
  lll_lock_t setxid_futex;

#if HP_TIMING_AVAIL
  /* Offset of the CPU clock at start thread start time.  */
  hp_timing_t cpuclock_offset;
#endif

  /* If the thread waits to join another one the ID of the latter is
     stored here.

     In case a thread is detached this field contains a pointer of the
     TCB if the thread itself.  This is something which cannot happen
     in normal operation.  */
  struct pthread *joinid;
  /* Check whether a thread is detached.  */
#define IS_DETACHED(pd) ((pd)->joinid == (pd))

  /* Flags.  Including those copied from the thread attribute.  */
  int flags;

  /* The result of the thread function.  */
  void *result;

  /* Scheduling parameters for the new thread.  */
  struct sched_param schedparam;
  int schedpolicy;

  /* Start position of the code to be executed and the argument passed
     to the function.  */
  void *(*start_routine) (void *);
  void *arg;

  /* Debug state.  */
  td_eventbuf_t eventbuf;
  /* Next descriptor with a pending event.  */
  struct pthread *nextevent;

#ifdef HAVE_FORCED_UNWIND
  /* Machine-specific unwind info.  */
  struct _Unwind_Exception exc;
#endif

  /* If nonzero pointer to area allocated for the stack and its
     size.  */
  void *stackblock;
  size_t stackblock_size;
  /* Size of the included guard area.  */
  size_t guardsize;
  /* This is what the user specified and what we will report.  */
  size_t reported_guardsize;

  /* Thread Priority Protection data.  */
  struct priority_protection_data *tpp;

  /* Resolver state.  */
  struct __res_state res;

  /* This member must be last.  */
  char end_padding[];

#define PTHREAD_STRUCT_END_PADDING \
  (sizeof (struct pthread) - offsetof (struct pthread, end_padding))
} __attribute ((aligned (TCB_ALIGNMENT)));


#endif	/* descr.h */
