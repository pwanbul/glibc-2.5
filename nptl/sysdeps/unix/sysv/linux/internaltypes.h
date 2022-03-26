/* Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
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

#ifndef _INTERNALTYPES_H
#define _INTERNALTYPES_H	1

#include <stdint.h>


// 线程属性
struct pthread_attr
{
  /* Scheduler parameters and priority.  */
  struct sched_param schedparam;
  int schedpolicy;
  /* Various flags like detachstate, scope, etc.  */
  int flags;
  /* 警戒区的大小。线程栈末尾的警戒缓冲区大小（字节数）  */
  size_t guardsize;
  /* 线程栈的最低地址  */
  void *stackaddr;
  /* 线程栈的大小（字节数） */
  size_t stacksize;
  /* Affinity map.  */
  cpu_set_t *cpuset;
  size_t cpusetsize;
};

#define ATTR_FLAG_DETACHSTATE		0x0001
#define ATTR_FLAG_NOTINHERITSCHED	0x0002
#define ATTR_FLAG_SCOPEPROCESS		0x0004
#define ATTR_FLAG_STACKADDR		0x0008
#define ATTR_FLAG_OLDATTR		0x0010
#define ATTR_FLAG_SCHED_SET		0x0020
#define ATTR_FLAG_POLICY_SET		0x0040


/* Mutex attribute data structure.  */
struct pthread_mutexattr
{
  /* Identifier for the kind of mutex.

     Bit 31 is set if the mutex is to be shared between processes.

     Bit 0 to 30 contain one of the PTHREAD_MUTEX_ values to identify
     the type of the mutex.  */
  int mutexkind;
};


/* Conditional variable attribute data structure.  */
struct pthread_condattr
{
  /* Combination of values:

     Bit 0  : flag whether coditional variable will be shareable between
	      processes.

     Bit 1-7: clock ID.  */
  int value;
};


/* The __NWAITERS field is used as a counter and to house the number
   of bits which represent the clock.  COND_CLOCK_BITS is the number
   of bits reserved for the clock.  */
#define COND_CLOCK_BITS	1


/* Read-write lock variable attribute data structure.  */
struct pthread_rwlockattr
{
  int lockkind;
  int pshared;
};


/* Barrier data structure.  */
struct pthread_barrier
{
  unsigned int curr_event;
  int lock;
  unsigned int left;
  unsigned int init_count;
};


/* Barrier variable attribute data structure.  */
struct pthread_barrierattr
{
  int pshared;
};


/* 线程本地数据处理。 */
struct pthread_key_struct
{
  /* 序号。偶数表示空置条目。请注意，零是偶数。
   * 我们使用uintptr_t在32位和64位机器上不需要填充。
   * 在64位机器上，它也有助于避免换行。 */
  uintptr_t seq;

  /* 数据的析构函数。  */
  void (*destr) (void *);
};

/* 检查key是否未使用。 */
#define KEY_UNUSED(p) (((p) & 1) == 0)
/* 检查key是否可用。
 * 如果序列计数器在下一次销毁调用后溢出，我们就不能重用分配的键。
 * 这意味着我们可能会为具有相同序列的键释放内存。
 * 这不太可能发生，程序必须创建和销毁密钥2^31次（在32位平台上，在64位平台上为2^63）。
 * 如果发生这种情况，我们就不再使用这个特定的key了。  */
#define KEY_USABLE(p) (((uintptr_t) (p)) < ((uintptr_t) ((p) + 2)))


/* Handling of read-write lock data.  */
// XXX For now there is only one flag.  Maybe more in future.
#define RWLOCK_RECURSIVE(rwlock) ((rwlock)->__data.__flags != 0)


/* Semaphore variable structure.  */
struct sem
{
  unsigned int count;
};


/* Compatibility type for old conditional variable interfaces.  */
typedef struct
{
  pthread_cond_t *cond;
} pthread_cond_2_0_t;

#endif	/* internaltypes.h */
