# Copyright (C) 1991-2002,2003,2004,2005,2006 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with the GNU C Library; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA.

#
#	Makefile for stdlib routines
#
subdir	:= stdlib

headers	:= stdlib.h bits/stdlib.h bits/stdlib-ldbl.h			      \
	   monetary.h bits/monetary-ldbl.h				      \
	   inttypes.h stdint.h bits/wordsize.h				      \
	   errno.h sys/errno.h bits/errno.h				      \
	   ucontext.h sys/ucontext.h					      \
	   alloca.h fmtmsg.h

routines	:=							      \
	atof atoi atol atoll						      \
	abort								      \
	bsearch qsort msort						      \
	getenv putenv setenv secure-getenv				      \
	exit on_exit atexit cxa_atexit cxa_finalize old_atexit		      \
	abs labs llabs							      \
	div ldiv lldiv							      \
	mblen mbstowcs mbtowc wcstombs wctomb				      \
	random random_r rand rand_r					      \
	drand48 erand48 lrand48 nrand48 mrand48 jrand48			      \
	srand48 seed48 lcong48						      \
	drand48_r erand48_r lrand48_r nrand48_r mrand48_r jrand48_r	      \
	srand48_r seed48_r lcong48_r					      \
	drand48-iter							      \
	strtol strtoul strtoll strtoull					      \
	strtol_l strtoul_l strtoll_l strtoull_l				      \
	strtof strtod strtold						      \
	strtof_l strtod_l strtold_l					      \
	system canonicalize						      \
	a64l l64a							      \
	rpmatch strfmon strfmon_l getsubopt xpg_basename fmtmsg		      \
	strtoimax strtoumax wcstoimax wcstoumax				      \
	getcontext setcontext makecontext swapcontext
aux =	grouping groupingwc

# These routines will be omitted from the libc shared object.
# Instead the static object files will be included in a special archive
# linked against when the shared library will be used.
static-only-routines = atexit

distribute	:= exit.h grouping.h abort-instr.h isomac.c tst-fmtmsg.sh   \
		   allocalim.h
test-srcs	:= tst-fmtmsg
tests		:= tst-strtol tst-strtod testmb testrand testsort testdiv   \
		   test-canon test-canon2 tst-strtoll tst-environ	    \
		   tst-xpg-basename tst-random tst-random2 tst-bsearch	    \
		   tst-limits tst-rand48 bug-strtod tst-setcontext	    \
		   test-a64l tst-qsort tst-system testmb2 bug-strtod2

include ../Makeconfig

ifeq ($(build-shared),yes)
tests		+= tst-putenv
endif

# Several mpn functions from GNU MP are used by the strtod function.
mpn-routines := inlines add_n addmul_1 cmp divmod_1 divrem udiv_qrnnd \
		lshift rshift mod_1 mul mul_1 mul_n sub_n submul_1
mpn-headers = longlong.h gmp.h gmp-impl.h gmp-mparam.h asm-syntax.h

routines := $(strip $(routines) $(mpn-routines))	\
	    dbl2mpn ldbl2mpn				\
	    mpn2flt mpn2dbl mpn2ldbl
aux += fpioconst mp_clz_tab
distribute := $(distribute) $(mpn-headers) fpioconst.h tst-putenvmod.c

tests-extras += tst-putenvmod
extra-test-objs += tst-putenvmod.os

generated += isomac isomac.out tst-putenvmod.so

CFLAGS-bsearch.c = $(uses-callbacks)
CFLAGS-msort.c = $(uses-callbacks)
CFLAGS-qsort.c = $(uses-callbacks)
CFLAGS-system.c = -fexceptions
CFLAGS-system.os = -fomit-frame-pointer
CFLAGS-fmtmsg.c = -fexceptions

ifneq (,$(filter %REENTRANT, $(defines)))
CFLAGS-strfmon.c = -D_IO_MTSAFE_IO
CFLAGS-strfmon_l.c = -D_IO_MTSAFE_IO
endif

ifeq (yes,$(have-protected))
CFLAGS-atexit.c = -DHAVE_DOT_HIDDEN
endif

CFLAGS-tst-bsearch.c = $(stack-align-test-flags)
CFLAGS-tst-qsort.c = $(stack-align-test-flags)

include ../Rules

# Testdir has to be named stdlib and needs to be writable
test-canon-ARGS = --test-dir=${common-objpfx}stdlib

tst-strtod-ENV = LOCPATH=$(common-objpfx)localedata
testmb2-ENV = LOCPATH=$(common-objpfx)localedata

# Run a test on the header files we use.
tests: $(objpfx)isomac.out

ifeq (no,$(cross-compiling))
tests: $(objpfx)tst-fmtmsg.out
endif

$(objpfx)isomac.out: $(objpfx)isomac
	$(dir $<)$(notdir $<) '$(CC)'  \
	'-I../include $(+sysdep-includes) $(sysincludes) -I..' > $<.out

isomac-CFLAGS = -O
$(objpfx)isomac: isomac.c
	$(native-compile)

$(objpfx)tst-fmtmsg.out: tst-fmtmsg.sh $(objpfx)tst-fmtmsg
	$(SHELL) -e $< $(common-objpfx) '$(run-program-prefix)' $(common-objpfx)stdlib/

$(objpfx)tst-putenv: $(objpfx)tst-putenvmod.so

$(objpfx)tst-putenvmod.so: $(objpfx)tst-putenvmod.os
	$(build-module)
CFLAGS-tst-putenvmod.c = -DNOT_IN_libc=1
