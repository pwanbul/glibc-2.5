/* Startup support for ELF initializers/finalizers in the main executable.
   Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation, Inc.
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

#include <stddef.h>


/* These magic symbols are provided by the linker.  */
extern void (*__preinit_array_start []) (int, char **, char **)
  attribute_hidden;
extern void (*__preinit_array_end []) (int, char **, char **)
  attribute_hidden;
extern void (*__init_array_start []) (int, char **, char **)
  attribute_hidden;
extern void (*__init_array_end []) (int, char **, char **)
  attribute_hidden;
extern void (*__fini_array_start []) (void) attribute_hidden;
extern void (*__fini_array_end []) (void) attribute_hidden;


/* These function symbols are provided for the .init/.fini section entry
   points automagically by the linker.  */
extern void _init (void);
extern void _fini (void);

/* 这些函数由启动代码传递给__libc_start_main。这些静态链接到每个程序中。
 * 对于动态链接的程序，此模块将来自libc_nonshared.a，
 * 与libc.a模块的不同之处在于它不调用preinit数组。
 * */

void
__libc_csu_init (int argc, char **argv, char **envp)
{
  /* 对于动态链接的可执行文件，preinit数组由动态链接器执行（在初始化任何共享对象之前）。  */

#ifndef LIBC_NONSHARED
  /* For static executables, preinit happens rights before init.  */
  {
    const size_t size = __preinit_array_end - __preinit_array_start;
    size_t i;
    for (i = 0; i < size; i++)
      (*__preinit_array_start [i]) (argc, argv, envp);
  }
#endif

  _init ();

  const size_t size = __init_array_end - __init_array_start;
  for (size_t i = 0; i < size; i++)
      (*__init_array_start [i]) (argc, argv, envp);
}

/* 不应再使用此功能。我们现在运行可执行文件的析构函数，就像其他的一样。但是，我们不能删除该功能。  */
void
__libc_csu_fini (void)
{
#ifndef LIBC_NONSHARED
  size_t i = __fini_array_end - __fini_array_start;
  while (i-- > 0)
    (*__fini_array_start [i]) ();

  _fini ();
#endif
}
