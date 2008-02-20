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

#include "include/x86emui.h"
#include <string.h>
#include <stdarg.h>

/*------------------------- Global Variables ------------------------------*/

X86EMU_sysEnv		_X86EMU_env;		/* Global emulator machine state */

/*----------------------------- Implementation ----------------------------*/

/****************************************************************************
PARAMETERS:
addr	- Emulator memory address to read

RETURNS:
Byte value read from emulator memory.

REMARKS:
Reads a byte value from the emulator memory. 
****************************************************************************/
u8 X86API rdb(
    u32 addr)
{
	u8 val;

	if (addr > M.mem_size - 1) {
		// printk("mem_read: address %#lx out of range!\n", addr);
		HALT_SYS();
		}
	val = *(u8*)(M.mem_base + addr);
	return val;
}

/****************************************************************************
PARAMETERS:
addr	- Emulator memory address to read

RETURNS:
Word value read from emulator memory.

REMARKS:
Reads a word value from the emulator memory.
****************************************************************************/
u16 X86API rdw(
	u32 addr)
{
	u16 val = 0;

	if (addr > M.mem_size - 2) {
		// printk("mem_read: address %#lx out of range!\n", addr);
		HALT_SYS();
		}
#ifdef __BIG_ENDIAN__
	if (addr & 0x1) {
		val = (*(u8*)(M.mem_base + addr) |
			  (*(u8*)(M.mem_base + addr + 1) << 8));
		}
	else
#endif
#if defined(__alpha__) || defined(__alpha)
		val = ldw_u((u16*)(M.mem_base + addr));
#else
		val = *(u16*)(M.mem_base + addr);
#endif
    return val;
}

/****************************************************************************
PARAMETERS:
addr	- Emulator memory address to read

RETURNS:
Long value read from emulator memory.
REMARKS:
Reads a long value from the emulator memory. 
****************************************************************************/
u32 X86API rdl(
	u32 addr)
{
	u32 val = 0;

	if (addr > M.mem_size - 4) {
		// printk("mem_read: address %#lx out of range!\n", addr);
		HALT_SYS();
		}
#ifdef __BIG_ENDIAN__
	if (addr & 0x3) {
		val = (*(u8*)(M.mem_base + addr + 0) |
			  (*(u8*)(M.mem_base + addr + 1) << 8) |
			  (*(u8*)(M.mem_base + addr + 2) << 16) |
			  (*(u8*)(M.mem_base + addr + 3) << 24));
		}
	else
#endif
#if defined(__alpha__) || defined(__alpha)
		val = ldl_u((u32*)(M.mem_base + addr));
#else
		val = *(u32*)(M.mem_base + addr);
#endif
	return val;
}

/****************************************************************************
PARAMETERS:
addr	- Emulator memory address to read
val		- Value to store

REMARKS:
Writes a byte value to emulator memory.
****************************************************************************/
void X86API wrb(
	u32 addr,
	u8 val)
{
    if (addr > M.mem_size - 1) {
		// printk("mem_write: address %#lx out of range!\n", addr);
		HALT_SYS();
		}
	*(u8*)(M.mem_base + addr) = val;
}

/****************************************************************************
PARAMETERS:
addr	- Emulator memory address to read
val		- Value to store

REMARKS:
Writes a word value to emulator memory.
****************************************************************************/
void X86API wrw(
	u32 addr,
	u16 val)
{
	if (addr > M.mem_size - 2) {
		// printk("mem_write: address %#lx out of range!\n", addr);
		HALT_SYS();
		}
#ifdef __BIG_ENDIAN__
	if (addr & 0x1) {
		*(u8*)(M.mem_base + addr + 0) = (val >> 0) & 0xff;
		*(u8*)(M.mem_base + addr + 1) = (val >> 8) & 0xff;
		}
	else
#endif
#if defined(__alpha__) || defined(__alpha)
	 stw_u(val,(u16*)(M.mem_base + addr));
#else
	 *(u16*)(M.mem_base + addr) = val;
#endif
}

/****************************************************************************
PARAMETERS:
addr	- Emulator memory address to read
val		- Value to store

REMARKS:
Writes a long value to emulator memory. 
****************************************************************************/
void X86API wrl(
	u32 addr,
	u32 val)
{
	if (addr > M.mem_size - 4) {
		// printk("mem_write: address %#lx out of range!\n", addr);
		HALT_SYS();
		}
#ifdef __BIG_ENDIAN__
	if (addr & 0x1) {
		*(u8*)(M.mem_base + addr + 0) = (val >>  0) & 0xff;
		*(u8*)(M.mem_base + addr + 1) = (val >>  8) & 0xff;
		*(u8*)(M.mem_base + addr + 2) = (val >> 16) & 0xff;
		*(u8*)(M.mem_base + addr + 3) = (val >> 24) & 0xff;
		}
	else
#endif
#if defined(__alpha__) || defined(__alpha)
	 stl_u(val,(u32*)(M.mem_base + addr));
#else
	 *(u32*)(M.mem_base + addr) = val;
#endif
}

/****************************************************************************
PARAMETERS:
addr	- PIO address to read
RETURN:
0
REMARKS:
Default PIO byte read function. Doesn't perform real inb.
****************************************************************************/
static u8 X86API p_inb(
	X86EMU_pioAddr addr)
{
	// printk("No real inb\n");

	return 0;
}

/****************************************************************************
PARAMETERS:
addr	- PIO address to read
RETURN:
0
REMARKS:
Default PIO word read function. Doesn't perform real inw.
****************************************************************************/
static u16 X86API p_inw(
	X86EMU_pioAddr addr)
{
	// printk("No real inw\n");
	return 0;
}

/****************************************************************************
PARAMETERS:
addr	- PIO address to read
RETURN:
0
REMARKS:
Default PIO long read function. Doesn't perform real inl.
****************************************************************************/
static u32 X86API p_inl(
	X86EMU_pioAddr addr)
{
	// printk("No real inl\n");
	return 0;
}

/****************************************************************************
PARAMETERS:
addr	- PIO address to write
val     - Value to store
REMARKS:
Default PIO byte write function. Doesn't perform real outb.
****************************************************************************/
static void X86API p_outb(
	X86EMU_pioAddr addr,
	u8 val)
{
	// printk("No real outb\n");
    return;
}

/****************************************************************************
PARAMETERS:
addr	- PIO address to write
val     - Value to store
REMARKS:
Default PIO word write function. Doesn't perform real outw.
****************************************************************************/
static void X86API p_outw(
	X86EMU_pioAddr addr,
	u16 val)
{
	// printk("No real outw\n");
	return;
}

/****************************************************************************
PARAMETERS:
addr	- PIO address to write
val     - Value to store
REMARKS:
Default PIO ;ong write function. Doesn't perform real outl.
****************************************************************************/
static void X86API p_outl(
	X86EMU_pioAddr addr,
	u32 val)
{
	// printk("No real outl\n");
    return;
}

/*------------------------- Global Variables ------------------------------*/

u8  	(X86APIP sys_rdb)(u32 addr) 			            = rdb;
u16 	(X86APIP sys_rdw)(u32 addr) 			            = rdw;
u32 	(X86APIP sys_rdl)(u32 addr) 			            = rdl;
void 	(X86APIP sys_wrb)(u32 addr,u8 val) 		            = wrb;
void 	(X86APIP sys_wrw)(u32 addr,u16 val) 	            = wrw;
void 	(X86APIP sys_wrl)(u32 addr,u32 val) 	            = wrl;
u8  	(X86APIP sys_inb)(X86EMU_pioAddr addr)	            = p_inb;
u16 	(X86APIP sys_inw)(X86EMU_pioAddr addr)	            = p_inw;
u32 	(X86APIP sys_inl)(X86EMU_pioAddr addr)              = p_inl;
void 	(X86APIP sys_outb)(X86EMU_pioAddr addr, u8 val) 	= p_outb;
void 	(X86APIP sys_outw)(X86EMU_pioAddr addr, u16 val)	= p_outw;
void 	(X86APIP sys_outl)(X86EMU_pioAddr addr, u32 val)	= p_outl;

/*----------------------------- Setup -------------------------------------*/

/****************************************************************************
PARAMETERS:
funcs	- New memory function pointers to make active

REMARKS:
This function is used to set the pointers to functions which access
memory space, allowing the user application to override these functions
and hook them out as necessary for their application.
****************************************************************************/
void X86EMU_setupMemFuncs(
	X86EMU_memFuncs *funcs)
{
    sys_rdb = funcs->rdb;
    sys_rdw = funcs->rdw;
    sys_rdl = funcs->rdl;
    sys_wrb = funcs->wrb;
    sys_wrw = funcs->wrw;
    sys_wrl = funcs->wrl;
}

/****************************************************************************
PARAMETERS:
funcs	- New programmed I/O function pointers to make active

REMARKS:
This function is used to set the pointers to functions which access
I/O space, allowing the user application to override these functions
and hook them out as necessary for their application.
****************************************************************************/
void X86EMU_setupPioFuncs(
	X86EMU_pioFuncs *funcs)
{
    sys_inb = funcs->inb;
    sys_inw = funcs->inw;
    sys_inl = funcs->inl;
    sys_outb = funcs->outb;
    sys_outw = funcs->outw;
    sys_outl = funcs->outl;
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
void x86emu_set_intr_handler(X86EMU_sysEnv *emu, unsigned num, x86emu_intr_handler_t handler)
{
  if(num < sizeof emu->intr_table / sizeof *emu->intr_table) emu->intr_table[num] = handler;
}


void x86emu_set_instr_check(X86EMU_sysEnv *emu, x86emu_instr_check_t instr_check)
{
  emu->instr_check = instr_check;
}


void x86emu_set_log(X86EMU_sysEnv *emu, char *buffer, unsigned buffer_size)
{
  emu->log.size = buffer_size;
  emu->log.data = buffer;
  emu->log.ptr = emu->log.data;
  *emu->log.data = 0;
}


void x86emu_clear_log()
{
  M.log.ptr = M.log.data;
  *M.log.ptr = 0;
}


char *x86emu_get_log()
{
  return M.log.data;
}


void x86emu_log(const char *format, ...)
{
  va_list args;
  int size = M.log.size - (M.log.ptr - M.log.data);

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


