/* Macros for copying by pages; used in memcpy, memmove.  Generic macros.
   Copyright (C) 1995, 1997 Free Software Foundation, Inc.
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

/* 该文件定义了宏:

   PAGE_COPY_FWD_MAYBE (dstp, srcp, nbytes_left, nbytes)

	它的调用方式类似于 WORD_COPY_FWD 等。指针至少应该是字对齐的。
	这将检查按页进行虚拟复制是否可以并且应该完成，如果可以，则执行此操作。

  系统特定的 pagecopy.h 文件应定义这些宏，然后包含此文件：

   PAGE_COPY_THRESHOLD
   -- 值得按页进行虚拟复制的最小大小。

   PAGE_SIZE
   -- 页面大小.

   PAGE_COPY_FWD (dstp, srcp, nbytes_left, nbytes)
   -- 执行虚拟复制操作的宏。指针将与 PAGE_SIZE 字节对齐。
*/


#if PAGE_COPY_THRESHOLD

#include <assert.h>

#define PAGE_COPY_FWD_MAYBE(dstp, srcp, nbytes_left, nbytes)		      \
  do									      \
    {									      \
      if ((nbytes) >= PAGE_COPY_THRESHOLD &&				      \
	  PAGE_OFFSET ((dstp) - (srcp)) == 0) 				      \
	{								      \
	  /* 复制的数量超过了使用内核 VM 操作虚拟复制页的阈值，
 		* 并且源地址和目标地址具有相同的对齐方式。  */    \
	  size_t nbytes_before = PAGE_OFFSET (-(dstp));			      \
	  if (nbytes_before != 0)					      \
	    {								      \
	      /* 首先复制第一页边界之前的单词。  */     \
	      WORD_COPY_FWD (dstp, srcp, nbytes_left, nbytes_before);	      \
	      assert (nbytes_left == 0);				      \
	      nbytes -= nbytes_before;					      \
	    }								      \
	  PAGE_COPY_FWD (dstp, srcp, nbytes_left, nbytes);		      \
	}								      \
    } while (0)

/* 页大小始终是 2 的幂，因此我们可以避免模除。  */
#define PAGE_OFFSET(n)	((n) & (PAGE_SIZE - 1))

#else

#define PAGE_COPY_FWD_MAYBE(dstp, srcp, nbytes_left, nbytes) /* nada */

#endif
