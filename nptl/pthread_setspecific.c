/* Copyright (C) 2002, 2003, 2006 Free Software Foundation, Inc.
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

#include <errno.h>
#include <stdlib.h>
#include "pthreadP.h"


int
__pthread_setspecific (key, value)
     pthread_key_t key;
     const void *value;
{
  struct pthread *self;
  unsigned int idx1st;
  unsigned int idx2nd;
  struct pthread_key_data *level2;
  unsigned int seq;

  self = THREAD_SELF;		// 获取当前线程

  /*访问第一个2级块的特殊情况。这是通常的情况。 */
  if (__builtin_expect (key < PTHREAD_KEY_2NDLEVEL_SIZE, 1))
    {
      /* 验证key是否正常。  */
      if (KEY_UNUSED ((seq = __pthread_keys[key].seq)))
		/* 无效。  */
		return EINVAL;

      level2 = &self->specific_1stblock[key];

      /* 请记住，我们至少存储了一组数据。 */
      if (value != NULL)
		THREAD_SETMEM (self, specific_used, true);
    }
  else
    {
      if (key >= PTHREAD_KEYS_MAX
	  || KEY_UNUSED ((seq = __pthread_keys[key].seq)))
		/* 无效。 */
		return EINVAL;

      idx1st = key / PTHREAD_KEY_2NDLEVEL_SIZE;		// 1级数组的索引
      idx2nd = key % PTHREAD_KEY_2NDLEVEL_SIZE;		// 2级数组的索引

      /* 这是第二级数组。必要时分配它。 */
      level2 = THREAD_GETMEM_NC (self, specific, idx1st);
      if (level2 == NULL)
	{
	  if (value == NULL)
	    /* 我们不必做任何事情。在任何情况下，该值都将为NULL。我们可以节省内存分配。 */
	    return 0;

	  level2
	    = (struct pthread_key_data *) calloc (PTHREAD_KEY_2NDLEVEL_SIZE,
						  sizeof (*level2));
	  if (level2 == NULL)
	    return ENOMEM;

	  THREAD_SETMEM_NC (self, specific, idx1st, level2);		// 将2级数组和1级数组关联起来
	}

      /* 指向右数组元素的指针。 */
      level2 = &level2[idx2nd];

      /* 请记住，我们至少存储了一组数据。 */
      THREAD_SETMEM (self, specific_used, true);
    }

  /* 存储数据和序列号，以便我们可以识别过时的数据。  */
  level2->seq = seq;
  level2->data = (void *) value;

  return 0;
}
strong_alias (__pthread_setspecific, pthread_setspecific)
strong_alias (__pthread_setspecific, __pthread_setspecific_internal)
