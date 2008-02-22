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
* Description:  This file includes subroutines which are related to
*				programmed I/O and memory access. Included in this module
*				are default functions with limited usefulness. For real
*				uses these functions will most likely be overriden by the
*				user library.
*
****************************************************************************/

#include "include/x86emu_int.h"

/*----------------------------- Implementation ----------------------------*/

/****************************************************************************
PARAMETERS:
addr	- Emulator memory address to read

RETURNS:
Byte value read from emulator memory.

REMARKS:
Reads a byte value from the emulator memory. 
****************************************************************************/
u8 rdb(u32 addr)
{
  if(addr <= M.mem_size - 1) return M.mem_base[addr];

  return 0xff;
}

/****************************************************************************
PARAMETERS:
addr	- Emulator memory address to read

RETURNS:
Word value read from emulator memory.

REMARKS:
Reads a word value from the emulator memory.
****************************************************************************/
u16 rdw(u32 addr)
{
  if(addr <= M.mem_size - 2) return M.mem_base[addr] + (M.mem_base[addr + 1] << 8);

  if(addr == M.mem_size - 1) return M.mem_base[addr] + 0xff00;

  return 0xffff;
}

/****************************************************************************
PARAMETERS:
addr	- Emulator memory address to read

RETURNS:
Long value read from emulator memory.
REMARKS:
Reads a long value from the emulator memory. 
****************************************************************************/
u32 rdl(u32 addr)
{
  if(addr <= M.mem_size - 4) return
    M.mem_base[addr] +
    (M.mem_base[addr + 1] << 8) +
    (M.mem_base[addr + 2] << 16) +
    (M.mem_base[addr + 3] << 24);

  if(addr == M.mem_size - 3) return
    M.mem_base[addr] +
    0xffffff00;

  if(addr == M.mem_size - 2) return
    M.mem_base[addr] +
    (M.mem_base[addr + 1] << 8) +
    0xffff0000;

  if(addr == M.mem_size - 1) return
    M.mem_base[addr] +
    (M.mem_base[addr + 1] << 8) +
    (M.mem_base[addr + 2] << 16) +
    0xff000000;

  return 0xffffffff;
}

/****************************************************************************
PARAMETERS:
addr	- Emulator memory address to read
val		- Value to store

REMARKS:
Writes a byte value to emulator memory.
****************************************************************************/
void wrb(u32 addr, u8 val)
{
  if(addr <= M.mem_size - 1) M.mem_base[addr] = val;
}

/****************************************************************************
PARAMETERS:
addr	- Emulator memory address to read
val		- Value to store

REMARKS:
Writes a word value to emulator memory.
****************************************************************************/
void wrw(u32 addr, u16 val)
{
  if(addr <= M.mem_size - 2) {
    M.mem_base[addr] = val;
    M.mem_base[addr + 1] = val >> 8;
  }
  else if(addr == M.mem_size - 1) {
    M.mem_base[addr] = val;
  }
}

/****************************************************************************
PARAMETERS:
addr	- Emulator memory address to read
val		- Value to store

REMARKS:
Writes a long value to emulator memory. 
****************************************************************************/
void wrl(u32 addr, u32 val)
{
  if(addr <= M.mem_size - 4) {
    M.mem_base[addr] = val;
    M.mem_base[addr + 1] = val >> 8;
    M.mem_base[addr + 2] = val >> 16;
    M.mem_base[addr + 3] = val >> 24;
  }
  else if(addr == M.mem_size - 3) {
    M.mem_base[addr] = val;
    M.mem_base[addr + 1] = val >> 8;
    M.mem_base[addr + 2] = val >> 16;
  }
  else if(addr == M.mem_size - 2) {
    M.mem_base[addr] = val;
    M.mem_base[addr + 1] = val >> 8;
  }
  else if(addr == M.mem_size - 1) {
    M.mem_base[addr] = val;
  }
}

/*----------------------------- Setup -------------------------------------*/

/****************************************************************************
PARAMETERS:
funcs	- New memory function pointers to make active

REMARKS:
This function is used to set the pointers to functions which access
memory space, allowing the user application to override these functions
and hook them out as necessary for their application.
****************************************************************************/
void x86emu_set_mem_funcs(x86emu_t *emu, x86emu_mem_funcs_t *funcs)
{
  if(!funcs) return;

  emu->mem.rdb = funcs->rdb;
  emu->mem.rdw = funcs->rdw;
  emu->mem.rdl = funcs->rdl;
  emu->mem.wrb = funcs->wrb;
  emu->mem.wrw = funcs->wrw;
  emu->mem.wrl = funcs->wrl;
}

/****************************************************************************
PARAMETERS:
funcs	- New programmed I/O function pointers to make active

REMARKS:
This function is used to set the pointers to functions which access
I/O space, allowing the user application to override these functions
and hook them out as necessary for their application.
****************************************************************************/
void x86emu_set_io_funcs(x86emu_t *emu, x86emu_io_funcs_t *funcs)
{
  if(!funcs) return;

  emu->io.inb = funcs->inb;
  emu->io.inw = funcs->inw;
  emu->io.inl = funcs->inl;
  emu->io.outb = funcs->outb;
  emu->io.outw = funcs->outw;
  emu->io.outl = funcs->outl;
}

/****************************************************************************
PARAMETERS:
funcs	- New interrupt vector table to make active

REMARKS:
This function is used to set the pointers to functions which handle
interrupt processing in the emulator, allowing the user application to
hook interrupts as necessary for their application. Any interrupts that
are not hooked by the user application, and reflected and handled internally
in the emulator via the interrupt vector table. This allows the application
to get control when the code being emulated executes specific software
interrupts.
****************************************************************************/
void x86emu_set_intr_func(x86emu_t *emu, unsigned num, x86emu_intr_func_t handler)
{
  if(num < sizeof emu->intr_table / sizeof *emu->intr_table) emu->intr_table[num] = handler;
}


void x86emu_set_code_check(x86emu_t *emu, x86emu_code_check_t func)
{
  emu->code_check = func;
}


void x86emu_set_log(x86emu_t *emu, char *buffer, unsigned buffer_size)
{
  emu->log.size = buffer_size;
  emu->log.buf = buffer;
  emu->log.ptr = emu->log.buf;
  *emu->log.buf = 0;
}


void x86emu_clear_log()
{
  M.log.ptr = M.log.buf;
  *M.log.ptr = 0;
}


char *x86emu_get_log()
{
  return M.log.buf;
}


void x86emu_log(const char *format, ...)
{
  va_list args;
  int size = M.log.size - (M.log.ptr - M.log.buf);

  va_start(args, format);
  if(size > 0) {
    size = vsnprintf(M.log.ptr, size, format, args);
    if(size > 0) {
      M.log.ptr += size;
    }
    else {
      *M.log.ptr = 0;
    }
  }
  va_end(args);  
}


