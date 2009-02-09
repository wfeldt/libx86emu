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
* Description:  Header file for public specific functions.
*               Any application linking against us should only
*               include this header
*
****************************************************************************/

#ifndef __X86EMU_X86EMU_H
#define __X86EMU_X86EMU_H

#ifdef  __cplusplus
extern "C" {            			/* Use "C" linkage when in C++ mode */
#endif

#include <inttypes.h>


/*---------------------- Macros and type definitions ----------------------*/

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define s8 int8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int64_t


typedef int (* x86emu_intr_func_t)(u8 num, unsigned type);
typedef int (* x86emu_code_check_t)(void);

#define X86EMU_MEMIO_8	0
#define X86EMU_MEMIO_16	1
#define X86EMU_MEMIO_32	2
#define X86EMU_MEMIO_R	(0 << 8)
#define X86EMU_MEMIO_W	(1 << 8)
#define X86EMU_MEMIO_X	(2 << 8)
#define X86EMU_MEMIO_I	(3 << 8)
#define X86EMU_MEMIO_O	(4 << 8)

typedef unsigned (* x86emu_memio_func_t)(u32 addr, u32 *val, unsigned type);

/*
 * General EAX, EBX, ECX, EDX type registers.  Note that for
 * portability, and speed, the issue of byte swapping is not addressed
 * in the registers.  All registers are stored in the default format
 * available on the host machine.  The only critical issue is that the
 * registers should line up EXACTLY in the same manner as they do in
 * the 386.  That is:
 *
 * EAX & 0xff  === AL
 * EAX & 0xffff == AX
 *
 * etc.  The result is that alot of the calculations can then be
 * done using the native instruction set fully.
 */

#ifdef	__BIG_ENDIAN__

typedef struct {
  u32 e_reg;
} I32_reg_t;

typedef struct {
  u16 filler0, x_reg;
} I16_reg_t;

typedef struct {
  u8 filler0, filler1, h_reg, l_reg;
} I8_reg_t;

#else /* !__BIG_ENDIAN__ */

typedef struct {
  u32 e_reg;
} I32_reg_t;

typedef struct {
  u16 x_reg;
} I16_reg_t;

typedef struct {
  u8 l_reg, h_reg;
} I8_reg_t;

#endif /* BIG_ENDIAN */

typedef union {
  I32_reg_t I32_reg;
  I16_reg_t I16_reg;
  I8_reg_t I8_reg;
} i386_general_register;

struct i386_general_regs {
  i386_general_register A, B, C, D;
};

struct i386_special_regs {
  i386_general_register SP, BP, SI, DI, IP;
  u32 FLAGS;
};


/*  
 * segment registers here represent 16 bit selectors & base/limit cache
 * ldt & tr are quite similar to segment selectors
 */
typedef struct {
  u32 base, limit;
  u16 sel;
  u16 acc;
} sel_t;

#define ACC_G(a)	((a >> 11) & 1)		/* 0/1: granularity bytes/4k */
#define ACC_D(a)	((a >> 10) & 1)		/* 0/1: default size 16/32 bit */
#define ACC_P(a)	((a >> 7) & 1)		/* 0/1: present no/yes */
#define ACC_DPL(a)	((a >> 5) & 3)		/* 0..3: dpl */
#define ACC_S(a)	((a >> 4) & 1)		/* 0/1: system/normal  */
#define ACC_E(a)	((a >> 3) & 1)		/* 0/1: type data/code (ACC_S = normal) */
#define ACC_ED(a)	((a >> 2) & 1)		/* 0/1: expand up/down (ACC_E = data) */
#define ACC_C(a)	((a >> 2) & 1)		/* 0/1: conforming no/yes (ACC_E = code) */
#define ACC_W(a)	((a >> 1) & 1)		/* 0/1: writable no/yes (ACC_E = data) */
#define ACC_R(a)	((a >> 1) & 1)		/* 0/1: readable no/yes (ACC_E = code) */
#define ACC_A(a)	(a & 1)			/* 0/1: accessed no/yes */
#define ACC_TYPE(a)	(a & 0xf)		/* 0..0xf: system descr type (ACC_S = system) */

/* 8 bit registers */
#define R_AH		gen.A.I8_reg.h_reg
#define R_AL		gen.A.I8_reg.l_reg
#define R_BH		gen.B.I8_reg.h_reg
#define R_BL		gen.B.I8_reg.l_reg
#define R_CH		gen.C.I8_reg.h_reg
#define R_CL		gen.C.I8_reg.l_reg
#define R_DH		gen.D.I8_reg.h_reg
#define R_DL		gen.D.I8_reg.l_reg

/* 16 bit registers */
#define R_AX		gen.A.I16_reg.x_reg
#define R_BX		gen.B.I16_reg.x_reg
#define R_CX		gen.C.I16_reg.x_reg
#define R_DX		gen.D.I16_reg.x_reg

/* 32 bit extended registers */
#define R_EAX		gen.A.I32_reg.e_reg
#define R_EBX		gen.B.I32_reg.e_reg
#define R_ECX		gen.C.I32_reg.e_reg
#define R_EDX		gen.D.I32_reg.e_reg

/* special registers */
#define R_SP		spc.SP.I16_reg.x_reg
#define R_BP		spc.BP.I16_reg.x_reg
#define R_SI		spc.SI.I16_reg.x_reg
#define R_DI		spc.DI.I16_reg.x_reg
#define R_IP		spc.IP.I16_reg.x_reg
#define R_FLG		spc.FLAGS

/* special registers */
#define R_ESP		spc.SP.I32_reg.e_reg
#define R_EBP		spc.BP.I32_reg.e_reg
#define R_ESI		spc.SI.I32_reg.e_reg
#define R_EDI		spc.DI.I32_reg.e_reg
#define R_EIP		spc.IP.I32_reg.e_reg
#define R_EFLG		spc.FLAGS

/* segment registers */
#define R_ES_INDEX	0
#define R_CS_INDEX	1
#define R_SS_INDEX	2
#define R_DS_INDEX	3
#define R_FS_INDEX	4
#define R_GS_INDEX	5

#define R_ES		seg[0].sel
#define R_ES_BASE	seg[0].base
#define R_ES_LIMIT	seg[0].limit
#define R_ES_ACC	seg[0].acc
#define R_CS		seg[1].sel
#define R_CS_BASE	seg[1].base
#define R_CS_LIMIT	seg[1].limit
#define R_CS_ACC	seg[1].acc
#define R_SS		seg[2].sel
#define R_SS_BASE	seg[2].base
#define R_SS_LIMIT	seg[2].limit
#define R_SS_ACC	seg[2].acc
#define R_DS		seg[3].sel
#define R_DS_BASE	seg[3].base
#define R_DS_LIMIT	seg[3].limit
#define R_DS_ACC	seg[3].acc
#define R_FS		seg[4].sel
#define R_FS_BASE	seg[4].base
#define R_FS_LIMIT	seg[4].limit
#define R_FS_ACC	seg[4].acc
#define R_GS		seg[5].sel
#define R_GS_BASE	seg[5].base
#define R_GS_LIMIT	seg[5].limit
#define R_GS_ACC	seg[5].acc
#define R_NOSEG		seg[6].sel
#define R_NOSEG_BASE	seg[6].base
#define R_NOSEG_LIMIT	seg[6].limit
#define R_NOSEG_ACC	seg[6].acc

/* other registers: tr, ldt, gdt, idt */
#define R_TR		tr.sel
#define R_TR_BASE	tr.base
#define R_TR_LIMIT	tr.limit
#define R_TR_ACC	tr.acc
#define R_LDT		ldt.sel
#define R_LDT_BASE	ldt.base
#define R_LDT_LIMIT	ldt.limit
#define R_LDT_ACC	ldt.acc
#define R_GDT_BASE	gdt.base
#define R_GDT_LIMIT	gdt.limit
#define R_IDT_BASE	idt.base
#define R_IDT_LIMIT	idt.limit

/* machine status & debug registers: CRx, DRx, TRx */
#define R_CR0		crx[0]
#define R_CR1		crx[1]
#define R_CR2		crx[2]
#define R_CR3		crx[3]
#define R_CR4		crx[4]
#define R_CR5		crx[5]
#define R_CR6		crx[6]
#define R_CR7		crx[7]
#define R_DR0		drx[0]
#define R_DR1		drx[1]
#define R_DR2		drx[2]
#define R_DR3		drx[3]
#define R_DR4		drx[4]
#define R_DR5		drx[5]
#define R_DR6		drx[6]
#define R_DR7		drx[7]

/* flag conditions   */
#define FB_CF 0x0001            /* CARRY flag  */
#define FB_PF 0x0004            /* PARITY flag */
#define FB_AF 0x0010            /* AUX  flag   */
#define FB_ZF 0x0040            /* ZERO flag   */
#define FB_SF 0x0080            /* SIGN flag   */
#define FB_TF 0x0100            /* TRAP flag   */
#define FB_IF 0x0200            /* INTERRUPT ENABLE flag */
#define FB_DF 0x0400            /* DIR flag    */
#define FB_OF 0x0800            /* OVERFLOW flag */

/* 80286 and above always have bit#1 set */
#define F_ALWAYS_ON  (0x0002)   /* flag bits always on */

/*
 * Define a mask for only those flag bits we will ever pass back 
 * (via PUSHF) 
 */
#define F_MSK (FB_CF|FB_PF|FB_AF|FB_ZF|FB_SF|FB_TF|FB_IF|FB_DF|FB_OF)

/* following bits masked in to a 16bit quantity */

#define F_CF 0x0001             /* CARRY flag  */
#define F_PF 0x0004             /* PARITY flag */
#define F_AF 0x0010             /* AUX  flag   */
#define F_ZF 0x0040             /* ZERO flag   */
#define F_SF 0x0080             /* SIGN flag   */
#define F_TF 0x0100             /* TRAP flag   */
#define F_IF 0x0200             /* INTERRUPT ENABLE flag */
#define F_DF 0x0400             /* DIR flag    */
#define F_OF 0x0800             /* OVERFLOW flag */

#define TOGGLE_FLAG(flag)     	(M.x86.R_FLG ^= (flag))
#define SET_FLAG(flag)        	(M.x86.R_FLG |= (flag))
#define CLEAR_FLAG(flag)      	(M.x86.R_FLG &= ~(flag))
#define ACCESS_FLAG(flag)     	(M.x86.R_FLG & (flag))
#define CLEARALL_FLAG(m)    	(M.x86.R_FLG = 0)

#define CONDITIONAL_SET_FLAG(COND,FLAG) \
  if(COND) SET_FLAG(FLAG); else CLEAR_FLAG(FLAG)

#define F_PF_CALC 0x010000      /* PARITY flag has been calced    */
#define F_ZF_CALC 0x020000      /* ZERO flag has been calced      */
#define F_SF_CALC 0x040000      /* SIGN flag has been calced      */

#define F_ALL_CALC      0xff0000        /* All have been calced   */

/*
 * Emulator machine state.
 * Segment usage control.
 */
#define _MODE_SEG_DS_SS         0x00000001
#define _MODE_REPE              0x00000002
#define _MODE_REPNE             0x00000004
#define _MODE_DATA32            0x00000008
#define _MODE_ADDR32            0x00000010
#define _MODE_STACK32           0x00000020
#define _MODE_CODE32            0x00000040
#define _MODE_HALTED            0x00000080

#define MODE_REPE		(M.x86.mode & _MODE_REPE)
#define MODE_REPNE		(M.x86.mode & _MODE_REPNE)
#define MODE_REP		(M.x86.mode & (_MODE_REPE | _MODE_REPNE))
#define MODE_DATA32		(M.x86.mode & _MODE_DATA32)
#define MODE_ADDR32		(M.x86.mode & _MODE_ADDR32)
#define MODE_STACK32		(M.x86.mode & _MODE_STACK32)
#define MODE_CODE32		(M.x86.mode & _MODE_CODE32)
#define MODE_HALTED		(M.x86.mode & _MODE_HALTED)

#define MODE_PROTECTED		(M.x86.R_CR0 & 1)
#define MODE_REAL		(!MODE_PROTECTED)

#define INTR_TYPE_SOFT		1
#define INTR_TYPE_FAULT		2
#define INTR_MODE_RESTART	0x100
#define INTR_MODE_ERRCODE	0x200

#define INTR_RAISE_DIV0		x86emu_intr_raise(0, INTR_TYPE_SOFT | INTR_MODE_RESTART, 0)
#define INTR_RAISE_SOFT(n)	x86emu_intr_raise(n, INTR_TYPE_SOFT, 0)
#define INTR_RAISE_GP(err)	x86emu_intr_raise(0x0d, INTR_TYPE_FAULT | INTR_MODE_RESTART | INTR_MODE_ERRCODE, err)
#define INTR_RAISE_UD		x86emu_intr_raise(0x06, INTR_TYPE_FAULT | INTR_MODE_RESTART, 0)

typedef struct {
  struct i386_general_regs gen;
  struct i386_special_regs spc;
  sel_t seg[8];
  sel_t ldt;
  sel_t tr;
  u32 crx[8];
  u32 drx[8];
  struct {
    u32 base, limit;
  } gdt;
  struct {
    u32 base, limit;
  } idt;
  u64 msr[0x800];		/* MSRs */
  u32 tsc;			/* TSC */
  u64 real_tsc;
  u32 mode;
  sel_t *default_seg;
  u32 saved_eip;
  u16 saved_cs;
  char decode_seg[4];
  unsigned char instr_buf[32];	/* instruction bytes */
  unsigned instr_len;		/* bytes in instr_buf */
  char disasm_buf[256];
  char *disasm_ptr;
  u8 intr_nr;
  unsigned intr_type;
  unsigned intr_errcode;
} x86emu_regs_t;

/****************************************************************************
REMARKS:
Structure maintaining the emulator machine state.

MEMBERS:
mem_base		- Base real mode memory for the emulator
mem_size		- Size of the real mode memory block for the emulator
private			- private data pointer
x86			- X86 registers
****************************************************************************/
typedef struct {
  x86emu_regs_t x86;
  x86emu_code_check_t code_check;
  x86emu_memio_func_t memio;
  x86emu_intr_func_t intr_table[256];
  unsigned char *mem_base;
  u32 mem_size;
  struct {
    unsigned size;
    char *buf;
    char *ptr;
    unsigned full:1;

    unsigned regs:1;
    unsigned code:1;
    unsigned data:1;
    unsigned acc:1;
    unsigned io:1;
    unsigned intr:1;
  } log;
  void *private;
} x86emu_t;

/*----------------------------- Global Variables --------------------------*/

/* Global emulator machine state.
 *
 * We keep it global to avoid pointer dereferences in the code for speed.
 */

extern x86emu_t x86emu;

/*-------------------------- Function Prototypes --------------------------*/

void x86emu_set_memio_func(x86emu_t *emu, x86emu_memio_func_t func);
void x86emu_set_intr_func(x86emu_t *emu, unsigned num, x86emu_intr_func_t handler);
void x86emu_set_code_check(x86emu_t *emu, x86emu_code_check_t func);
void x86emu_set_log(x86emu_t *emu, char *buffer, unsigned buffer_size);
char *x86emu_get_log();
void x86emu_clear_log();
void x86emu_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void x86emu_reset(x86emu_t *emu);
void x86emu_exec(void);
void x86emu_stop(void);

void x86emu_intr_raise(u8 intr_nr, unsigned type, unsigned err);

#ifdef  __cplusplus
}                       			/* End of "C" linkage for C++   	*/
#endif

#endif /* __X86EMU_X86EMU_H */

