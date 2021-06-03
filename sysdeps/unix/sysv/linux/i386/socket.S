/* Copyright (C) 1995-1998,2002,2003,2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#include <sysdep-cancel.h>
#include <socketcall.h>
#include <tls.h>

#define P(a, b) P2(a, b)
#define P2(a, b) a##b

	.text
/* The socket-oriented system calls are handled unusally in Linux.
   They are all gated through the single `socketcall' system call number.
   `socketcall' takes two arguments: the first is the subcode, specifying
   which socket function is being called; and the second is a pointer to
   the arguments to the specific function.

   The .S files for the other calls just #define socket and #include this.  */

#ifndef __socket
# ifndef NO_WEAK_ALIAS
#  define __socket P(__,socket)
# else
#  define __socket socket
# endif
#endif

.globl __socket
ENTRY (__socket)
#if defined NEED_CANCELLATION && defined CENABLE
	SINGLE_THREAD_P
	jne 1f
#endif

	/* Save registers.  */
	movl %ebx, %edx
	cfi_register (3, 2)

	movl $SYS_ify(socketcall), %eax	/* System call number in %eax.  */

	/* Use ## so `socket' is a separate token that might be #define'd.  */
	movl $P(SOCKOP_,socket), %ebx	/* Subcode is first arg to syscall.  */
	lea 4(%esp), %ecx		/* Address of args is 2nd arg.  */

        /* Do the system call trap.  */
	ENTER_KERNEL

	/* Restore registers.  */
	movl %edx, %ebx
	cfi_restore (3)

	/* %eax is < 0 if there was an error.  */
	cmpl $-125, %eax
	jae SYSCALL_ERROR_LABEL

	/* Successful; return the syscall's value.  */
L(pseudo_end):
	ret


#if defined NEED_CANCELLATION && defined CENABLE
	/* We need one more register.  */
1:	pushl %esi
	cfi_adjust_cfa_offset(4)

	/* Enable asynchronous cancellation.  */
	CENABLE
	movl %eax, %esi
	cfi_offset(6, -8)		/* %esi */

	/* Save registers.  */
	movl %ebx, %edx
	cfi_register (3, 2)

	movl $SYS_ify(socketcall), %eax	/* System call number in %eax.  */

	/* Use ## so `socket' is a separate token that might be #define'd.  */
	movl $P(SOCKOP_,socket), %ebx	/* Subcode is first arg to syscall.  */
	lea 8(%esp), %ecx		/* Address of args is 2nd arg.  */

        /* Do the system call trap.  */
	ENTER_KERNEL

	/* Restore registers.  */
	movl %edx, %ebx
	cfi_restore (3)

	/* Restore the cancellation.  */
	xchgl %esi, %eax
	CDISABLE

	/* Restore registers.  */
	movl %esi, %eax
	popl %esi
	cfi_restore (6)
	cfi_adjust_cfa_offset(-4)

	/* %eax is < 0 if there was an error.  */
	cmpl $-125, %eax
	jae SYSCALL_ERROR_LABEL

	/* Successful; return the syscall's value.  */
	ret
#endif
PSEUDO_END (__socket)

#ifndef NO_WEAK_ALIAS
weak_alias (__socket, socket)
#endif