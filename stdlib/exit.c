/* Copyright (C) 1991,95,96,97,99,2001,2002,2005 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysdep.h>
#include "exit.h"

#include "set-hooks.h"
DEFINE_HOOK (__libc_atexit, (void))


/* 调用所有用`atexit'和`on_exit'注册的函数，
 * 按照它们注册的相反顺序执行stdio清理，并以STATUS终止程序执行。
 * */
void
exit (int status)
{
  /* 我们这样做是为了处理由`atexit'和`on_exit'注册的函数对exit()的递归调用。
   * 我们调用列表中的每个人，并使用最后一个exit()中的状态值。 */
  while (__exit_funcs != NULL)
    {
      struct exit_function_list *old;

      while (__exit_funcs->idx > 0)
	{
	  const struct exit_function *const f =
	    &__exit_funcs->fns[--__exit_funcs->idx];
	  switch (f->flavor)
	    {
	      void (*atfct) (void);
	      void (*onfct) (int status, void *arg);
	      void (*cxafct) (void *arg, int status);

	    case ef_free:
	    case ef_us:
	      break;
	    case ef_on:
	      onfct = f->func.on.fn;
#ifdef PTR_DEMANGLE
	      PTR_DEMANGLE (onfct);
#endif
	      onfct (status, f->func.on.arg);
	      break;
	    case ef_at:
	      atfct = f->func.at;
#ifdef PTR_DEMANGLE
	      PTR_DEMANGLE (atfct);
#endif
	      atfct ();
	      break;
	    case ef_cxa:
	      cxafct = f->func.cxa.fn;
#ifdef PTR_DEMANGLE
	      PTR_DEMANGLE (cxafct);
#endif
	      cxafct (f->func.cxa.arg, status);
	      break;
	    }
	}

      old = __exit_funcs;
      __exit_funcs = __exit_funcs->next;
      if (__exit_funcs != NULL)
	/* Don't free the last element in the chain, this is the statically
	   allocate element.  */
	free (old);
    }

  RUN_HOOK (__libc_atexit, ());             // 在这里冲洗并关闭标准IO

  _exit (status);
}
libc_hidden_def (exit)
