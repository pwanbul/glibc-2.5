/* Copyright (C) 2002, 2003 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include "pthreadP.h"


void *
__pthread_getspecific (key)
     pthread_key_t key;
{
  struct pthread_key_data *data;

  /* 访问第一个2级块的特殊情况。这是通常的情况。  */
  if (__builtin_expect (key < PTHREAD_KEY_2NDLEVEL_SIZE, 1))
    data = &THREAD_SELF->specific_1stblock[key];
  else
    {
      /* 验证key是否正常。  */
      if (key >= PTHREAD_KEYS_MAX)
		/* 无效。  */
		return NULL;

      unsigned int idx1st = key / PTHREAD_KEY_2NDLEVEL_SIZE;
      unsigned int idx2nd = key % PTHREAD_KEY_2NDLEVEL_SIZE;

      /* 如果序列号不匹配或无法为该线程定义键，因为未分配第二级数组，则也返回NULL。 */
      struct pthread_key_data *level2 = THREAD_GETMEM_NC (THREAD_SELF,
							  specific, idx1st);
      if (level2 == NULL)
		/* 未分配，因此没有数据。  */
		return NULL;

      /* 有数据。 */
      data = &level2[idx2nd];
    }

  void *result = data->data;
  if (result != NULL)
    {
      uintptr_t seq = data->seq;

      if (__builtin_expect (seq != __pthread_keys[key].seq, 0))
	result = data->data = NULL;
    }

  return result;
}
strong_alias (__pthread_getspecific, pthread_getspecific)
strong_alias (__pthread_getspecific, __pthread_getspecific_internal)
