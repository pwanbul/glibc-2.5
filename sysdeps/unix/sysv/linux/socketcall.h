/* ID for functions called via socketcall system call.
   Copyright (C) 1995, 1996 Free Software Foundation, Inc.
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

#ifndef _SYS_SOCKETCALL_H

#define _SYS_SOCKETCALL_H	1

/* 为套接字上允许的操作定义唯一编号。 Linux 对所有这些功能使用单个系统调用。
 * 相关的代码文件是usr/include/linux/net.h。
   我们不能在这里使用枚举，因为这些值是在汇编代码中使用的。
   */

#define SOCKOP_socket		1
#define SOCKOP_bind		2
#define SOCKOP_connect		3
#define SOCKOP_listen		4
#define SOCKOP_accept		5
#define SOCKOP_getsockname	6
#define SOCKOP_getpeername	7
#define SOCKOP_socketpair	8
#define SOCKOP_send		9
#define SOCKOP_recv		10
#define SOCKOP_sendto		11
#define SOCKOP_recvfrom		12
#define SOCKOP_shutdown		13
#define SOCKOP_setsockopt	14
#define SOCKOP_getsockopt	15
#define SOCKOP_sendmsg		16
#define SOCKOP_recvmsg		17

#endif /* _SYS_SOCKETCALL_H */
