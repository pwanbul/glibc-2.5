/* Copyright (C) 1991,94,95,96,97,99,2002 Free Software Foundation, Inc.
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

#include <unistd.h>
#include <stdlib.h>

/* 函数“_exit”应该接受一个状态参数并简单地终止程序执行，
 * 使用给定整数的低8位作为状态。 */
void
_exit (status)
     int status;
{
  status &= 0xff;
  abort ();
}
libc_hidden_def (_exit)
weak_alias (_exit, _Exit)

stub_warning (_exit)
#include <stub-tag.h>
