/****************************************************************************
*
*						Realmode X86 Emulator Library
*
*            	Copyright (C) 1996-1999 SciTech Software, Inc.
* 				     Copyright (C) David Mosberger-Tang
* 					   Copyright (C) 1999 Egbert Eich
*
*  ========================================================================
*
*  Permission to use, copy, modify, distribute, and sell this software and
*  its documentation for any purpose is hereby granted without fee,
*  provided that the above copyright notice appear in all copies and that
*  both that copyright notice and this permission notice appear in
*  supporting documentation, and that the name of the authors not be used
*  in advertising or publicity pertaining to distribution of the software
*  without specific, written prior permission.  The authors makes no
*  representations about the suitability of this software for any purpose.
*  It is provided "as is" without express or implied warranty.
*
*  THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
*  EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
*  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
*  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
*  PERFORMANCE OF THIS SOFTWARE.
*
*  ========================================================================
*
* Language:		ANSI C
* Environment:	Any
* Developer:    Kendall Bennett
*
* Description:  Header file for system specific functions. These functions
*				are always compiled and linked in the OS depedent libraries,
*				and never in a binary portable driver.
*
****************************************************************************/


#ifndef __X86EMU_X86EMU_INT_H
#define __X86EMU_X86EMU_INT_H

#define M x86emu

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "x86emu.h"
#include "decode.h"
#include "ops.h"
#include "prim_ops.h"
#include "mem.h"


#define INTR_RAISE_DIV0		emu_intr_raise(0, INTR_TYPE_SOFT | INTR_MODE_RESTART, 0)
#define INTR_RAISE_SOFT(n)	emu_intr_raise(n, INTR_TYPE_SOFT, 0)
#define INTR_RAISE_GP(err)	emu_intr_raise(0x0d, INTR_TYPE_FAULT | INTR_MODE_RESTART | INTR_MODE_ERRCODE, err)
#define INTR_RAISE_UD		emu_intr_raise(0x06, INTR_TYPE_FAULT | INTR_MODE_RESTART, 0)


#if defined(__i386__) || defined (__x86_64__)
#define WITH_TSC	1
#define WITH_IOPL	1
#else
#define WITH_TSC	0
#define WITH_IOPL	0
#endif


#if WITH_TSC
#if defined(__i386__)
static inline u64 tsc()
{
  register u64 tsc asm ("%eax");

  asm (
    "rdtsc"
    : "=r" (tsc)
  );

  return tsc;
}
#endif

#if defined (__x86_64__)
static inline u64 tsc()
{
  register u64 tsc asm ("%rax");

  asm (
    "push %%rdx\n"
    "rdtsc\n"
    "xchg %%edx,%%eax\n"
    "shl $32,%%rax\n"
    "add %%rdx,%%rax\n"
    "pop %%rdx"
    : "=r" (tsc)
  );

  return tsc;
}
#endif

#endif


#if WITH_IOPL
#if defined(__i386__)
static inline unsigned getiopl() 
{
  register u32 i asm ("%eax");
  
  asm(
    "pushf\n"
    "pop %%eax"
    : "=r" (i)
  );

  i = (i >> 12) & 3;

  return i;
}
#endif

#if defined (__x86_64__)
static inline unsigned getiopl()
{
  register unsigned i asm ("%rax");

  asm(
    "pushf\n"
    "pop %%rax"
    : "=r" (i)
  );

  i = (i >> 12) & 3;

  return i;
}
#endif

#endif


#endif /* __X86EMU_X86EMU_INT_H */

