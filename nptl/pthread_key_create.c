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
#include "pthreadP.h"
#include <atomic.h>


int
__pthread_key_create (key, destr)
     pthread_key_t *key;
     void (*destr) (void *);
{
  /* 在__pthread_kyes中找到一个未使用的插槽。
   * PTHREAD_KEYS_MAX为1024
   * */
  for (size_t cnt = 0; cnt < PTHREAD_KEYS_MAX; ++cnt)
    {
      uintptr_t seq = __pthread_keys[cnt].seq;

      if (KEY_UNUSED (seq) && KEY_USABLE (seq)
	  /* 我们找到了一个未使用的插槽。尝试分配它。 CAS语义 */
	  && ! atomic_compare_and_exchange_bool_acq (&__pthread_keys[cnt].seq,
						     seq + 1, seq))
	{
	  /* 记住析构函数.  */
	  __pthread_keys[cnt].destr = destr;

	  /* 将密钥返回给调用者，数组的下标 */
	  *key = cnt;

	  /* 通话成功。  */
	  return 0;
	}
    }

  return EAGAIN;
}
strong_alias (__pthread_key_create, pthread_key_create)
strong_alias (__pthread_key_create, __pthread_key_create_internal)
