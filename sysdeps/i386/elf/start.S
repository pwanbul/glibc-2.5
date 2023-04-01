/* Startup code compliant to the ELF i386 ABI.
   Copyright (C) 1995, 1996, 1997, 1998, 2000, 2001, 2002, 2003, 2004
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   In addition to the permissions in the GNU Lesser General Public
   License, the Free Software Foundation gives you unlimited
   permission to link the compiled version of this file with other
   programs, and to distribute those programs without any restriction
   coming from the use of this file. (The GNU Lesser General Public
   License restrictions do apply in other respects; for example, they
   cover modification of the file, and distribution when not linked
   into another program.)

   Note that people who make modified versions of this file are not
   obligated to grant this special exception for their modified
   versions; it is their choice whether to do so. The GNU Lesser
   General Public License gives permission to release a modified
   version without this exception; this exception also makes it
   possible to release a modified version which carries forward this
   exception.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* This is the canonical entry point, usually the first thing in the text
   segment.  The SVR4/i386 ABI (pages 3-31, 3-32) says that when the entry
   point runs, most registers' values are unspecified, except for:

   %edx		Contains a function pointer to be registered with `atexit'.
		This is how the dynamic linker arranges to have DT_FINI
		functions called for shared libraries that have been loaded
		before this code runs.

   %esp		The stack contains the arguments and environment:
		0(%esp)			argc
		4(%esp)			argv[0]
		...
		(4*argc)(%esp)		NULL
		(4*(argc+1))(%esp)	envp[0]
		...
					NULL
*/

#include "bp-sym.h"

	.text
	.globl _start
	.type _start,@function
_start:
	/* 清除帧指针。ABI建议这样做，以明显地标记最外层的框架。*/
	xorl %ebp, %ebp

	/* 提取堆栈上编码的参数，并为“main”设置参数：argc、argv。 envp 稍后将在 __libc_start_main 中确定。 */
	popl %esi		/* 弹出参数计数。*/
	movl %esp, %ecx		/* argv从当前栈顶开始。*/

	/* 在推送参数之前，将堆栈对齐到 16 字节（SSE 需要 16 字节对齐）边界以避免未对齐访问的惩罚。
	 * 感谢 Edward Seidl <seidl@janed.com> 指出这一点。
	 * */
	andl $0xfffffff0, %esp
	pushl %eax		/* 推送垃圾，因为我们分配了 28 个字节。  */

	/* 向用户代码提供最高堆栈地址（对于向下增长的堆栈）。 */
	pushl %esp

	pushl %edx		/* 共享库终止函数的推送地址。 */

#ifdef SHARED
	/* Load PIC register.  */
	call 1f
	addl $_GLOBAL_OFFSET_TABLE_, %ebx

	/* Push address of our own entry points to .fini and .init.  */
	leal __libc_csu_fini@GOTOFF(%ebx), %eax
	pushl %eax
	leal __libc_csu_init@GOTOFF(%ebx), %eax
	pushl %eax

	pushl %ecx		/* Push second argument: argv.  */
	pushl %esi		/* Push first argument: argc.  */

	pushl BP_SYM (main)@GOT(%ebx)

	/* Call the user's main function, and exit with its value.
	   But let the libc call main.    */
	call BP_SYM (__libc_start_main)@PLT
#else
	/* 将我们自己的入口点地址推送到.fini和.init。 */
	pushl $__libc_csu_fini
	pushl $__libc_csu_init

	pushl %ecx		/* Push second argument: argv.  */
	pushl %esi		/* Push first argument: argc.  */

	pushl $BP_SYM (main)

	/* 调用用户的main函数，并以其值退出。
	 * 但是让libc调用main。    */
	call BP_SYM (__libc_start_main)
#endif

	hlt			/* Crash if somehow `exit' does return.  */

#ifdef SHARED
1:	movl	(%esp), %ebx
	ret
#endif

/* To fulfill the System V/i386 ABI we need this symbol.  Yuck, it's so
   meaningless since we don't support machines < 80386.  */
	.section .rodata
	.globl _fp_hw
_fp_hw:	.long 3
	.size _fp_hw, 4
	.type _fp_hw,@object

/* Define a symbol for the first piece of initialized data.  */
	.data
	.globl __data_start
__data_start:
	.long 0
	.weak data_start
	data_start = __data_start
