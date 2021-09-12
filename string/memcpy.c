/* Copy memory to memory until the specified number of bytes
   has been copied.  Overlap is NOT handled correctly.
   Copyright (C) 1991, 1997, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Torbjorn Granlund (tege@sics.se).

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

#include <string.h>
#include <memcopy.h>
#include <pagecopy.h>

#undef memcpy

void *
memcpy (dstpp, srcpp, len)
     void *dstpp;
     const void *srcpp;
     size_t len;
{
  unsigned long int dstp = (long int) dstpp;
  unsigned long int srcp = (long int) srcpp;

  /* 从头复制到尾。  */

  /* 如果要复制的字节不是太少，请使用字复制。  */
  if (len >= OP_T_THRES)		// OP_T_THRES为16
    {
      /* 仅复制几个字节以使 DSTP 对齐. */
      len -= (-dstp) % OPSIZ;		// OPSIZ为8
      BYTE_COPY_FWD (dstp, srcp, (-dstp) % OPSIZ);			// 按字节拷贝

      /* 尽可能通过虚拟地址操作将整个页面从SRCP复制到DSTP。*/

      PAGE_COPY_FWD_MAYBE (dstp, srcp, len, len);		// 按虚拟内存页拷贝，i386不支持

      /* 利用已知的 DSTP 对齐方式从 SRCP 复制到 DSTP。
       * 剩余的字节数放在第三个参数中，即在 LEN 中。这个数字可能因机器而异。
       * */

      WORD_COPY_FWD (dstp, srcp, len, len);		// 按字拷贝

      /* 掉出来复制尾巴.  */
    }

  /* 只需复制几个字节。使用字节内存操作。  */
  BYTE_COPY_FWD (dstp, srcp, len);		// 按字节拷贝

  return dstpp;
}
libc_hidden_builtin_def (memcpy)
