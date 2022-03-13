/* Copyright (C) 1991, 93, 1995-1998, 2000 Free Software Foundation, Inc.
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

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysdep.h>		/* In case it wants to define anything.  */

/* The first piece of initialized data.  */
int __data_start = 0;
#ifdef HAVE_WEAK_SYMBOLS
weak_alias (__data_start, data_start)
#endif

#ifdef	DUMMIES
#define	ARG_DUMMIES	DUMMIES,
#define	DECL_DUMMIES	int DUMMIES;
#else
#define	ARG_DUMMIES
#define	DECL_DUMMIES
#endif

extern void __libc_init (int argc, char **argv, char **envp);
extern int main (int argc, char **argv, char **envp);


/* Not a prototype because it gets called strangely.  */
static void start1();

#ifndef	HAVE__start

/* N.B.：重要的是这是第一个函数。这个文件是文本部分的第一件事。  */
void
_start ()
{
  start1 ();
}

#ifndef NO_UNDERSCORES
/* 为“_start”创建一个名为“start”的别名（没有前导下划线，因此它不会与 C 符号冲突）。
 * 这是供应商 crt0.o 倾向于使用的名称，因此也是大多数链接器所期望的名称。
 * */
asm (".set start, __start");
#endif

#endif

/* ARGSUSED */
static void
start1 (ARG_DUMMIES argc, argp)
     DECL_DUMMIES
     int argc;
     char *argp;
{
  char **argv = &argp;

  /* 环境在ARGV之后开始。  */
  __environ = &argv[argc + 1];

  /* 如果ARGV之后的第一件事是参数本身，则没有环境。  */
  if ((char *) __environ == *argv)
    /* 环境是空的。使__environ指向ARGV[ARGC]，即NULL。  */
    --__environ;

  /* 进行C库初始化。  */
  __libc_init (argc, argv, __environ);

  /* 调用用户程序。  */
  exit (main (argc, argv, __environ));
}
