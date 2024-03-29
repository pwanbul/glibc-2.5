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

#ifndef _LOWLEVELLOCK_H
#define _LOWLEVELLOCK_H	1

#include <time.h>
#include <sys/param.h>
#include <bits/pthreadtypes.h>

#ifndef LOCK_INSTR
# ifdef UP
#  define LOCK_INSTR	/* nothing */
# else
#  define LOCK_INSTR "lock;"
# endif
#endif

#define SYS_futex		240
#define FUTEX_WAIT		0
#define FUTEX_WAKE		1
#define FUTEX_LOCK_PI		6
#define FUTEX_UNLOCK_PI		7
#define FUTEX_TRYLOCK_PI	8


/* Initializer for compatibility lock.  */
#define LLL_MUTEX_LOCK_INITIALIZER		(0)
#define LLL_MUTEX_LOCK_INITIALIZER_LOCKED	(1)
#define LLL_MUTEX_LOCK_INITIALIZER_WAITERS	(2)


#ifdef PIC
# define LLL_EBX_LOAD	"xchgl %2, %%ebx\n"
# define LLL_EBX_REG	"D"
#else
# define LLL_EBX_LOAD
# define LLL_EBX_REG	"b"
#endif

#ifdef I386_USE_SYSENTER
# ifdef SHARED
#  define LLL_ENTER_KERNEL	"call *%%gs:%P6\n\t"
# else
#  define LLL_ENTER_KERNEL	"call *_dl_sysinfo\n\t"
# endif
#else
# define LLL_ENTER_KERNEL	"int $0x80\n\t"
#endif

/* Delay in spinlock loop.  */
#define BUSY_WAIT_NOP          asm ("rep; nop")


#define LLL_STUB_UNWIND_INFO_START \
	".section	.eh_frame,\"a\",@progbits\n"		\
"5:\t"	".long	7f-6f	# Length of Common Information Entry\n"	\
"6:\t"	".long	0x0	# CIE Identifier Tag\n\t"		\
	".byte	0x1	# CIE Version\n\t"			\
	".ascii \"zR\\0\"	# CIE Augmentation\n\t"		\
	".uleb128 0x1	# CIE Code Alignment Factor\n\t"	\
	".sleb128 -4	# CIE Data Alignment Factor\n\t"	\
	".byte	0x8	# CIE RA Column\n\t"			\
	".uleb128 0x1	# Augmentation size\n\t"		\
	".byte	0x1b	# FDE Encoding (pcrel sdata4)\n\t"	\
	".byte	0xc	# DW_CFA_def_cfa\n\t"			\
	".uleb128 0x4\n\t"					\
	".uleb128 0x0\n\t"					\
	".align 4\n"						\
"7:\t"	".long	17f-8f	# FDE Length\n"				\
"8:\t"	".long	8b-5b	# FDE CIE offset\n\t"			\
	".long	1b-.	# FDE initial location\n\t"		\
	".long	4b-1b	# FDE address range\n\t"		\
	".uleb128 0x0	# Augmentation size\n\t"		\
	".byte	0x16	# DW_CFA_val_expression\n\t"		\
	".uleb128 0x8\n\t"					\
	".uleb128 10f-9f\n"					\
"9:\t"	".byte	0x78	# DW_OP_breg8\n\t"			\
	".sleb128 3b-1b\n"
#define LLL_STUB_UNWIND_INFO_END \
	".byte	0x16	# DW_CFA_val_expression\n\t"		\
	".uleb128 0x8\n\t"					\
	".uleb128 12f-11f\n"					\
"11:\t"	".byte	0x78	# DW_OP_breg8\n\t"			\
	".sleb128 3b-2b\n"					\
"12:\t"	".byte	0x40 + (3b-2b-1) # DW_CFA_advance_loc\n\t"	\
	".byte	0x16	# DW_CFA_val_expression\n\t"		\
	".uleb128 0x8\n\t"					\
	".uleb128 16f-13f\n"					\
"13:\t"	".byte	0x78	# DW_OP_breg8\n\t"			\
	".sleb128 15f-14f\n\t"					\
	".byte	0x0d	# DW_OP_const4s\n"			\
"14:\t"	".4byte	3b-.\n\t"					\
	".byte	0x1c	# DW_OP_minus\n\t"			\
	".byte	0x0d	# DW_OP_const4s\n"			\
"15:\t"	".4byte	18f-.\n\t"					\
	".byte	0x22	# DW_OP_plus\n"				\
"16:\t"	".align 4\n"						\
"17:\t"	".previous\n"

/* Unwind info for
   1: lea ..., ...
   2: call ...
   3: jmp 18f
   4:
   snippet.  */
#define LLL_STUB_UNWIND_INFO_3 \
LLL_STUB_UNWIND_INFO_START					\
"10:\t"	".byte	0x40 + (2b-1b) # DW_CFA_advance_loc\n\t"	\
LLL_STUB_UNWIND_INFO_END

/* Unwind info for
   1: lea ..., ...
   0: movl ..., ...
   2: call ...
   3: jmp 18f
   4:
   snippet.  */
#define LLL_STUB_UNWIND_INFO_4 \
LLL_STUB_UNWIND_INFO_START					\
"10:\t"	".byte	0x40 + (0b-1b) # DW_CFA_advance_loc\n\t"	\
	".byte	0x16	# DW_CFA_val_expression\n\t"		\
	".uleb128 0x8\n\t"					\
	".uleb128 20f-19f\n"					\
"19:\t"	".byte	0x78	# DW_OP_breg8\n\t"			\
	".sleb128 3b-0b\n"					\
"20:\t"	".byte	0x40 + (2b-0b) # DW_CFA_advance_loc\n\t"	\
LLL_STUB_UNWIND_INFO_END


#define lll_futex_wait(futex, val) \
  ({									      \
    int __status;							      \
    register __typeof (val) _val asm ("edx") = (val);			      \
    __asm __volatile (LLL_EBX_LOAD					      \
		      LLL_ENTER_KERNEL					      \
		      LLL_EBX_LOAD					      \
		      : "=a" (__status)					      \
		      : "0" (SYS_futex), LLL_EBX_REG (futex), "S" (0),	      \
			"c" (FUTEX_WAIT), "d" (_val),			      \
			"i" (offsetof (tcbhead_t, sysinfo))		      \
		      : "memory");					      \
    __status;								      \
  })


#define lll_futex_timed_wait(futex, val, timeout)			      \
  ({									      \
    int __status;							      \
    register __typeof (val) _val asm ("edx") = (val);			      \
    __asm __volatile (LLL_EBX_LOAD					      \
		      LLL_ENTER_KERNEL					      \
		      LLL_EBX_LOAD					      \
		      : "=a" (__status)					      \
		      : "0" (SYS_futex), LLL_EBX_REG (futex), "S" (timeout),  \
			"c" (FUTEX_WAIT), "d" (_val),			      \
			"i" (offsetof (tcbhead_t, sysinfo))		      \
		      : "memory");					      \
    __status;								      \
  })


#define lll_futex_wake(futex, nr) \
  do {									      \
    int __ignore;							      \
    register __typeof (nr) _nr asm ("edx") = (nr);			      \
    __asm __volatile (LLL_EBX_LOAD					      \
		      LLL_ENTER_KERNEL					      \
		      LLL_EBX_LOAD					      \
		      : "=a" (__ignore)					      \
		      : "0" (SYS_futex), LLL_EBX_REG (futex),		      \
			"c" (FUTEX_WAKE), "d" (_nr),			      \
			"i" (0) /* phony, to align next arg's number */,      \
			"i" (offsetof (tcbhead_t, sysinfo)));		      \
  } while (0)


/* Does not preserve %eax and %ecx.  */
extern int __lll_mutex_lock_wait (int val, int *__futex)
     __attribute ((regparm (2))) attribute_hidden;
/* Does not preserve %eax, %ecx, and %edx.  */
extern int __lll_mutex_timedlock_wait (int val, int *__futex,
				       const struct timespec *abstime)
     __attribute ((regparm (3))) attribute_hidden;
/* Preserves all registers but %eax.  */
extern int __lll_mutex_unlock_wake (int *__futex)
     __attribute ((regparm (1))) attribute_hidden;


/* NB: in the lll_mutex_trylock macro we simply return the value in %eax
   after the cmpxchg instruction.  In case the operation succeded this
   value is zero.  In case the operation failed, the cmpxchg instruction
   has loaded the current value of the memory work which is guaranteed
   to be nonzero.  */
#define lll_mutex_trylock(futex) \
  ({ int ret;								      \
     __asm __volatile (LOCK_INSTR "cmpxchgl %2, %1"			      \
		       : "=a" (ret), "=m" (futex)			      \
		       : "r" (LLL_MUTEX_LOCK_INITIALIZER_LOCKED), "m" (futex),\
			 "0" (LLL_MUTEX_LOCK_INITIALIZER)		      \
		       : "memory");					      \
     ret; })


#define lll_robust_mutex_trylock(futex, id) \
  ({ int ret;								      \
     __asm __volatile (LOCK_INSTR "cmpxchgl %2, %1"			      \
		       : "=a" (ret), "=m" (futex)			      \
		       : "r" (id), "m" (futex),				      \
			 "0" (LLL_MUTEX_LOCK_INITIALIZER)		      \
		       : "memory");					      \
     ret; })


#define lll_mutex_cond_trylock(futex) \
  ({ int ret;								      \
     __asm __volatile (LOCK_INSTR "cmpxchgl %2, %1"			      \
		       : "=a" (ret), "=m" (futex)			      \
		       : "r" (LLL_MUTEX_LOCK_INITIALIZER_WAITERS),	      \
			  "m" (futex), "0" (LLL_MUTEX_LOCK_INITIALIZER)	      \
		       : "memory");					      \
     ret; })


#define lll_mutex_lock(futex) \
  (void) ({ int ignore1, ignore2;					      \
	    __asm __volatile (LOCK_INSTR "cmpxchgl %1, %2\n\t"		      \
			      "jnz _L_mutex_lock_%=\n\t"		      \
			      ".subsection 1\n\t"			      \
			      ".type _L_mutex_lock_%=,@function\n"	      \
			      "_L_mutex_lock_%=:\n"			      \
			      "1:\tleal %2, %%ecx\n"			      \
			      "2:\tcall __lll_mutex_lock_wait\n"	      \
			      "3:\tjmp 18f\n"				      \
			      "4:\t.size _L_mutex_lock_%=, 4b-1b\n\t"	      \
			      ".previous\n"				      \
			      LLL_STUB_UNWIND_INFO_3			      \
			      "18:"					      \
			      : "=a" (ignore1), "=c" (ignore2), "=m" (futex)  \
			      : "0" (0), "1" (1), "m" (futex)		      \
			      : "memory"); })


#define lll_robust_mutex_lock(futex, id) \
  ({ int result, ignore;						      \
     __asm __volatile (LOCK_INSTR "cmpxchgl %1, %2\n\t"			      \
		       "jnz _L_robust_mutex_lock_%=\n\t"		      \
		       ".subsection 1\n\t"				      \
		       ".type _L_robust_mutex_lock_%=,@function\n"	      \
		       "_L_robust_mutex_lock_%=:\n"			      \
		       "1:\tleal %2, %%ecx\n"				      \
		       "2:\tcall __lll_robust_mutex_lock_wait\n"	      \
		       "3:\tjmp 18f\n"					      \
		       "4:\t.size _L_robust_mutex_lock_%=, 4b-1b\n\t"	      \
		       ".previous\n"					      \
		       LLL_STUB_UNWIND_INFO_3				      \
		       "18:"						      \
		       : "=a" (result), "=c" (ignore), "=m" (futex)	      \
		       : "0" (0), "1" (id), "m" (futex)			      \
		       : "memory");					      \
     result; })


/* Special version of lll_mutex_lock which causes the unlock function to
   always wakeup waiters.  */
#define lll_mutex_cond_lock(futex) \
  (void) ({ int ignore1, ignore2;					      \
	    __asm __volatile (LOCK_INSTR "cmpxchgl %1, %2\n\t"		      \
			      "jnz _L_mutex_cond_lock_%=\n\t"		      \
			      ".subsection 1\n\t"			      \
			      ".type _L_mutex_cond_lock_%=,@function\n"	      \
			      "_L_mutex_cond_lock_%=:\n"		      \
			      "1:\tleal %2, %%ecx\n"			      \
			      "2:\tcall __lll_mutex_lock_wait\n"	      \
			      "3:\tjmp 18f\n"				      \
			      "4:\t.size _L_mutex_cond_lock_%=, 4b-1b\n\t"    \
			      ".previous\n"				      \
			      LLL_STUB_UNWIND_INFO_3			      \
			      "18:"					      \
			      : "=a" (ignore1), "=c" (ignore2), "=m" (futex)  \
			      : "0" (0), "1" (2), "m" (futex)		      \
			      : "memory"); })


#define lll_robust_mutex_cond_lock(futex, id) \
  ({ int result, ignore;						      \
     __asm __volatile (LOCK_INSTR "cmpxchgl %1, %2\n\t"			      \
		       "jnz _L_robust_mutex_cond_lock_%=\n\t"		      \
		       ".subsection 1\n\t"				      \
		       ".type _L_robust_mutex_cond_lock_%=,@function\n"	      \
		       "_L_robust_mutex_cond_lock_%=:\n"		      \
		       "1:\tleal %2, %%ecx\n"				      \
		       "2:\tcall __lll_robust_mutex_lock_wait\n"	      \
		       "3:\tjmp 18f\n"					      \
		       "4:\t.size _L_robust_mutex_cond_lock_%=, 4b-1b\n\t"    \
		       ".previous\n"					      \
		       LLL_STUB_UNWIND_INFO_3				      \
		       "18:"						      \
		       : "=a" (result), "=c" (ignore), "=m" (futex)	      \
		       : "0" (0), "1" (id | FUTEX_WAITERS), "m" (futex)	      \
		       : "memory");					      \
     result; })


#define lll_mutex_timedlock(futex, timeout) \
  ({ int result, ignore1, ignore2;					      \
     __asm __volatile (LOCK_INSTR "cmpxchgl %1, %3\n\t"			      \
		       "jnz _L_mutex_timedlock_%=\n\t"			      \
		       ".subsection 1\n\t"				      \
		       ".type _L_mutex_timedlock_%=,@function\n"	      \
		       "_L_mutex_timedlock_%=:\n"			      \
		       "1:\tleal %3, %%ecx\n"				      \
		       "0:\tmovl %7, %%edx\n"				      \
		       "2:\tcall __lll_mutex_timedlock_wait\n"		      \
		       "3:\tjmp 18f\n"					      \
		       "4:\t.size _L_mutex_timedlock_%=, 4b-1b\n\t"	      \
		       ".previous\n"					      \
		       LLL_STUB_UNWIND_INFO_4				      \
		       "18:"						      \
		       : "=a" (result), "=c" (ignore1), "=&d" (ignore2),      \
			 "=m" (futex)					      \
		       : "0" (0), "1" (1), "m" (futex), "m" (timeout)	      \
		       : "memory");					      \
     result; })


#define lll_robust_mutex_timedlock(futex, timeout, id) \
  ({ int result, ignore1, ignore2;					      \
     __asm __volatile (LOCK_INSTR "cmpxchgl %1, %3\n\t"			      \
		       "jnz _L_robust_mutex_timedlock_%=\n\t"		      \
		       ".subsection 1\n\t"				      \
		       ".type _L_robust_mutex_timedlock_%=,@function\n"	      \
		       "_L_robust_mutex_timedlock_%=:\n"		      \
		       "1:\tleal %3, %%ecx\n"				      \
		       "0:\tmovl %7, %%edx\n"				      \
		       "2:\tcall __lll_robust_mutex_timedlock_wait\n"	      \
		       "3:\tjmp 18f\n"					      \
		       "4:\t.size _L_robust_mutex_timedlock_%=, 4b-1b\n\t"    \
		       ".previous\n"					      \
		       LLL_STUB_UNWIND_INFO_4				      \
		       "18:"						      \
		       : "=a" (result), "=c" (ignore1), "=&d" (ignore2),      \
			 "=m" (futex)					      \
		       : "0" (0), "1" (id), "m" (futex), "m" (timeout)	      \
		       : "memory");					      \
     result; })


#define lll_mutex_unlock(futex) \
  (void) ({ int ignore;							      \
            __asm __volatile (LOCK_INSTR "subl $1, %0\n\t"		      \
			      "jne _L_mutex_unlock_%=\n\t"		      \
			      ".subsection 1\n\t"			      \
			      ".type _L_mutex_unlock_%=,@function\n"	      \
			      "_L_mutex_unlock_%=:\n"			      \
			      "1:\tleal %0, %%eax\n"			      \
			      "2:\tcall __lll_mutex_unlock_wake\n"	      \
			      "3:\tjmp 18f\n"				      \
			      "4:\t.size _L_mutex_unlock_%=, 4b-1b\n\t"	      \
			      ".previous\n"				      \
			      LLL_STUB_UNWIND_INFO_3			      \
			      "18:"					      \
			      : "=m" (futex), "=&a" (ignore)		      \
			      : "m" (futex)				      \
			      : "memory"); })


#define lll_robust_mutex_unlock(futex) \
  (void) ({ int ignore;							      \
            __asm __volatile (LOCK_INSTR "andl %2, %0\n\t"		      \
			      "jne _L_robust_mutex_unlock_%=\n\t"	      \
			      ".subsection 1\n\t"			      \
			      ".type _L_robust_mutex_unlock_%=,@function\n"   \
			      "_L_robust_mutex_unlock_%=:\n\t"		      \
			      "1:\tleal %0, %%eax\n"			      \
			      "2:\tcall __lll_mutex_unlock_wake\n"	      \
			      "3:\tjmp 18f\n"				      \
			      "4:\t.size _L_robust_mutex_unlock_%=, 4b-1b\n\t"\
			      ".previous\n"				      \
			      LLL_STUB_UNWIND_INFO_3			      \
			      "18:"					      \
			      : "=m" (futex), "=&a" (ignore)		      \
			      : "i" (FUTEX_WAITERS), "m" (futex)	      \
			      : "memory"); })


#define lll_robust_mutex_dead(futex) \
  (void) ({ int __ignore;						      \
	    register int _nr asm ("edx") = 1;				      \
	    __asm __volatile (LOCK_INSTR "orl %5, (%2)\n\t"		      \
			      LLL_EBX_LOAD				      \
			      LLL_ENTER_KERNEL				      \
			      LLL_EBX_LOAD				      \
			      : "=a" (__ignore)				      \
			      : "0" (SYS_futex), LLL_EBX_REG (&(futex)),      \
				"c" (FUTEX_WAKE), "d" (_nr),		      \
				"i" (FUTEX_OWNER_DIED),			      \
				"i" (offsetof (tcbhead_t, sysinfo))); })


#define lll_futex_wake(futex, nr) \
  do {									      \
    int __ignore;							      \
    register __typeof (nr) _nr asm ("edx") = (nr);			      \
    __asm __volatile (LLL_EBX_LOAD					      \
		      LLL_ENTER_KERNEL					      \
		      LLL_EBX_LOAD					      \
		      : "=a" (__ignore)					      \
		      : "0" (SYS_futex), LLL_EBX_REG (futex),		      \
			"c" (FUTEX_WAKE), "d" (_nr),			      \
			"i" (0) /* phony, to align next arg's number */,      \
			"i" (offsetof (tcbhead_t, sysinfo)));		      \
  } while (0)


#define lll_mutex_islocked(futex) \
  (futex != 0)


/* 我们有一个独立的内部锁实现，它与二进制兼容性无关。 */

/* 锁定对象的类型。  */
typedef int lll_lock_t;

/* 锁的初始化器  */
#define LLL_LOCK_INITIALIZER		(0)
#define LLL_LOCK_INITIALIZER_LOCKED	(1)


extern int __lll_lock_wait (int val, int *__futex)
     __attribute ((regparm (2))) attribute_hidden;
extern int __lll_unlock_wake (int *__futex)
     __attribute ((regparm (1))) attribute_hidden;
extern int lll_unlock_wake_cb (int *__futex) attribute_hidden;


/* The states of a lock are:
    0  -  untaken
    1  -  taken by one user
    2  -  taken by more users */


#if defined NOT_IN_libc || defined UP
# define lll_trylock(futex) lll_mutex_trylock (futex)
# define lll_lock(futex) lll_mutex_lock (futex)
# define lll_unlock(futex) lll_mutex_unlock (futex)
#else
/* Special versions of the macros for use in libc itself.  They avoid
   the lock prefix when the thread library is not used.

   XXX In future we might even want to avoid it on UP machines.  */
# include <tls.h>

# define lll_trylock(futex) \
  ({ unsigned char ret;							      \
     __asm __volatile ("cmpl $0, %%gs:%P5\n\t"				      \
		       "je 0f\n\t"					      \
		       "lock\n"						      \
		       "0:\tcmpxchgl %2, %1; setne %0"			      \
		       : "=a" (ret), "=m" (futex)			      \
		       : "r" (LLL_MUTEX_LOCK_INITIALIZER_LOCKED), "m" (futex),\
			 "0" (LLL_MUTEX_LOCK_INITIALIZER),		      \
		         "i" (offsetof (tcbhead_t, multiple_threads))	      \
		       : "memory");					      \
     ret; })


# define lll_lock(futex) \
  (void) ({ int ignore1, ignore2;					      \
	    __asm __volatile ("cmpl $0, %%gs:%P6\n\t"			      \
			      "je 0f\n\t"				      \
			      "lock\n"					      \
			      "0:\tcmpxchgl %1, %2\n\t"			      \
			      "jnz _L_lock_%=\n\t"			      \
			      ".subsection 1\n\t"			      \
			      ".type _L_lock_%=,@function\n"		      \
			      "_L_lock_%=:\n"				      \
			      "1:\tleal %2, %%ecx\n"			      \
			      "2:\tcall __lll_mutex_lock_wait\n"	      \
			      "3:\tjmp 18f\n"				      \
			      "4:\t.size _L_lock_%=, 4b-1b\n\t"		      \
			      ".previous\n"				      \
			      LLL_STUB_UNWIND_INFO_3			      \
			      "18:"					      \
			      : "=a" (ignore1), "=c" (ignore2), "=m" (futex)  \
			      : "0" (0), "1" (1), "m" (futex),		      \
		                "i" (offsetof (tcbhead_t, multiple_threads))  \
			      : "memory"); })


# define lll_unlock(futex) \
  (void) ({ int ignore;							      \
            __asm __volatile ("cmpl $0, %%gs:%P3\n\t"			      \
			      "je 0f\n\t"				      \
			      "lock\n"					      \
			      "0:\tsubl $1,%0\n\t"			      \
			      "jne _L_unlock_%=\n\t"			      \
			      ".subsection 1\n\t"			      \
			      ".type _L_unlock_%=,@function\n"		      \
			      "_L_unlock_%=:\n"				      \
			      "1:\tleal %0, %%eax\n"			      \
			      "2:\tcall __lll_mutex_unlock_wake\n"	      \
			      "3:\tjmp 18f\n\t"				      \
			      "4:\t.size _L_unlock_%=, 4b-1b\n\t"	      \
			      ".previous\n"				      \
			      LLL_STUB_UNWIND_INFO_3			      \
			      "18:"					      \
			      : "=m" (futex), "=&a" (ignore)		      \
			      : "m" (futex),				      \
				"i" (offsetof (tcbhead_t, multiple_threads))  \
			      : "memory"); })
#endif


#define lll_islocked(futex) \
  (futex != LLL_LOCK_INITIALIZER)


/* The kernel notifies a process with uses CLONE_CLEARTID via futex
   wakeup when the clone terminates.  The memory location contains the
   thread ID while the clone is running and is reset to zero
   afterwards.

   The macro parameter must not have any side effect.  */
#define lll_wait_tid(tid) \
  do {									      \
    int __ignore;							      \
    register __typeof (tid) _tid asm ("edx") = (tid);			      \
    if (_tid != 0)							      \
      __asm __volatile (LLL_EBX_LOAD					      \
			"1:\tmovl %1, %%eax\n\t"			      \
			LLL_ENTER_KERNEL				      \
			"cmpl $0, (%%ebx)\n\t"				      \
			"jne 1b\n\t"					      \
			LLL_EBX_LOAD					      \
			: "=&a" (__ignore)				      \
			: "i" (SYS_futex), LLL_EBX_REG (&tid), "S" (0),	      \
			  "c" (FUTEX_WAIT), "d" (_tid),			      \
			  "i" (offsetof (tcbhead_t, sysinfo))		      \
			: "memory");					      \
  } while (0)

extern int __lll_timedwait_tid (int *tid, const struct timespec *abstime)
     __attribute__ ((regparm (2))) attribute_hidden;
#define lll_timedwait_tid(tid, abstime) \
  ({									      \
    int __result = 0;							      \
    if (tid != 0)							      \
      {									      \
	if (abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000)	      \
	  __result = EINVAL;						      \
	else								      \
	  __result = __lll_timedwait_tid (&tid, abstime);		      \
      }									      \
    __result; })


/* Conditional variable handling.  */

extern void __lll_cond_wait (pthread_cond_t *cond)
     __attribute ((regparm (1))) attribute_hidden;
extern int __lll_cond_timedwait (pthread_cond_t *cond,
				 const struct timespec *abstime)
     __attribute ((regparm (2))) attribute_hidden;
extern void __lll_cond_wake (pthread_cond_t *cond)
     __attribute ((regparm (1))) attribute_hidden;
extern void __lll_cond_broadcast (pthread_cond_t *cond)
     __attribute ((regparm (1))) attribute_hidden;


#define lll_cond_wait(cond) \
  __lll_cond_wait (cond)
#define lll_cond_timedwait(cond, abstime) \
  __lll_cond_timedwait (cond, abstime)
#define lll_cond_wake(cond) \
  __lll_cond_wake (cond)
#define lll_cond_broadcast(cond) \
  __lll_cond_broadcast (cond)


#endif	/* lowlevellock.h */
