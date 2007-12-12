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

#ifndef __X86EMU_LIBX86_H
#define __X86EMU_LIBX86_H

#ifdef  __cplusplus
extern "C" {            			/* Use "C" linkage when in C++ mode */
#endif

#include <inttypes.h>

#define	X86API
#define	X86APIP	*

#define u8	uint8_t
#define u16	uint16_t
#define u32	uint32_t
#define u64	uint64_t
#define s8	int8_t
#define s16	int16_t
#define s32	int32_t
#define s64	int64_t
#define uint	uint32_t

typedef u16 X86EMU_pioAddr;



/*---------------------- Macros and type definitions ----------------------*/

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
  I32_reg_t   I32_reg;
  I16_reg_t   I16_reg;
  I8_reg_t    I8_reg;
} i386_general_register;

struct i386_general_regs {
  i386_general_register A, B, C, D;
};

struct i386_special_regs {
  i386_general_register SP, BP, SI, DI, IP;
  u32 FLAGS;
};


typedef struct {
  u32 base, limit;
  u16 sel;
  u16 acc;
} sel_t;

/*  
 * segment registers here represent 16 bit selectors & base/limit cache
 * ldt & tr are quite similar to segment selectors
 */
struct i386_segment_regs {
  sel_t CS, DS, SS, ES, FS, GS;
  sel_t LDT, TR;
};

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
#define R_CS		seg.CS.sel
#define R_CS_BASE	seg.CS.base
#define R_CS_LIMIT	seg.CS.limit
#define R_CS_ACC	seg.CS.acc
#define R_DS		seg.DS.sel
#define R_DS_BASE	seg.DS.base
#define R_DS_LIMIT	seg.DS.limit
#define R_DS_ACC	seg.DS.acc
#define R_SS		seg.SS.sel
#define R_SS_BASE	seg.SS.base
#define R_SS_LIMIT	seg.SS.limit
#define R_SS_ACC	seg.SS.acc
#define R_ES		seg.ES.sel
#define R_ES_BASE	seg.ES.base
#define R_ES_LIMIT	seg.ES.limit
#define R_ES_ACC	seg.ES.acc
#define R_FS		seg.FS.sel
#define R_FS_BASE	seg.FS.base
#define R_FS_LIMIT	seg.FS.limit
#define R_FS_ACC	seg.FS.acc
#define R_GS		seg.GS.sel
#define R_GS_BASE	seg.GS.base
#define R_GS_LIMIT	seg.GS.limit
#define R_GS_ACC	seg.GS.acc

/* other registers: tr, ldt, gdt, idt */
#define R_TR		seg.TR.sel
#define R_TR_BASE	seg.TR.base
#define R_TR_LIMIT	seg.TR.limit
#define R_TR_ACC	seg.TR.acc
#define R_LDT		seg.LDT.sel
#define R_LDT_BASE	seg.LDT.base
#define R_LDT_LIMIT	seg.LDT.limit
#define R_LDT_ACC	seg.LDT.acc
#define R_GDT_BASE	gdt.base
#define R_GDT_LIMIT	gdt.limit
#define R_IDT_BASE	idt.base
#define R_IDT_LIMIT	idt.limit

/* machine status & debug registers: CRx, DRx, TRx */
#define R_CR0		cr[0]
#define R_CR1		cr[1]
#define R_CR2		cr[2]
#define R_CR3		cr[3]
#define R_CR4		cr[4]
#define R_CR5		cr[5]
#define R_CR6		cr[6]
#define R_CR7		cr[7]
#define R_DR0		dr[0]
#define R_DR1		dr[1]
#define R_DR2		dr[2]
#define R_DR3		dr[3]
#define R_DR4		dr[4]
#define R_DR5		dr[5]
#define R_DR6		dr[6]
#define R_DR7		dr[7]
#define R_TR0		tr[0]
#define R_TR1		tr[1]
#define R_TR2		tr[2]
#define R_TR3		tr[3]
#define R_TR4		tr[4]
#define R_TR5		tr[5]
#define R_TR6		tr[6]
#define R_TR7		tr[7]

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
  if (COND) SET_FLAG(FLAG); else CLEAR_FLAG(FLAG)

#define F_PF_CALC 0x010000      /* PARITY flag has been calced    */
#define F_ZF_CALC 0x020000      /* ZERO flag has been calced      */
#define F_SF_CALC 0x040000      /* SIGN flag has been calced      */

#define F_ALL_CALC      0xff0000        /* All have been calced   */

/*
 * Emulator machine state.
 * Segment usage control.
 */
#define SYSMODE_SEG_DS_SS       0x00000001
#define SYSMODE_SEGOVR_CS       0x00000002
#define SYSMODE_SEGOVR_DS       0x00000004
#define SYSMODE_SEGOVR_ES       0x00000008
#define SYSMODE_SEGOVR_FS       0x00000010
#define SYSMODE_SEGOVR_GS       0x00000020
#define SYSMODE_SEGOVR_SS       0x00000040
#define SYSMODE_PREFIX_REPE     0x00000080
#define SYSMODE_PREFIX_REPNE    0x00000100
#define SYSMODE_PREFIX_DATA     0x00000200
#define SYSMODE_PREFIX_ADDR     0x00000400
#define SYSMODE_INTR_PENDING    0x10000000
#define SYSMODE_EXTRN_INTR      0x20000000
#define SYSMODE_HALTED          0x40000000

#define SYSMODE_SEGMASK (SYSMODE_SEG_DS_SS      | \
						 SYSMODE_SEGOVR_CS      | \
						 SYSMODE_SEGOVR_DS      | \
						 SYSMODE_SEGOVR_ES      | \
						 SYSMODE_SEGOVR_FS      | \
						 SYSMODE_SEGOVR_GS      | \
						 SYSMODE_SEGOVR_SS)
#define SYSMODE_CLRMASK (SYSMODE_SEG_DS_SS      | \
						 SYSMODE_SEGOVR_CS      | \
						 SYSMODE_SEGOVR_DS      | \
						 SYSMODE_SEGOVR_ES      | \
						 SYSMODE_SEGOVR_FS      | \
						 SYSMODE_SEGOVR_GS      | \
						 SYSMODE_SEGOVR_SS      | \
						 SYSMODE_PREFIX_DATA    | \
						 SYSMODE_PREFIX_ADDR)

#define  SYSMODE_DATA32		(M.x86.mode & SYSMODE_PREFIX_DATA)

#define  INTR_SYNCH           0x1
#define  INTR_ASYNCH          0x2
#define  INTR_HALTED          0x4
#define  INTR_ILLEGAL_OP      0x8

typedef struct {
    struct i386_general_regs    gen;
    struct i386_special_regs    spc;
    struct i386_segment_regs    seg;
    u32 cr[8];
    u32 dr[8];
    u32 tr[8];
    struct {
      u32 base, limit;
    } gdt;
    struct {
      u32 base, limit;
    } idt;
    /*
     * MODE contains information on:
     *  REPE prefix             2 bits  repe,repne
     *  SEGMENT overrides       5 bits  normal,DS,SS,CS,ES
     *  Delayed flag set        3 bits  (zero, signed, parity)
     *  reserved                6 bits
     *  interrupt #             8 bits  instruction raised interrupt
     *  BIOS video segregs      4 bits  
     *  Interrupt Pending       1 bits  
     *  Extern interrupt        1 bits
     *  Halted                  1 bits
     */
    u32                         mode;
    volatile int                intr;   /* mask of pending interrupts */
    int                         debug;
    int                         check;
    u16                         saved_ip;
    u16                         saved_cs;
    int                         enc_pos;
    int                         enc_str_pos;
    char                        decode_buf[32]; /* encoded byte stream  */
    char                        decoded_buf[256]; /* disassembled strings */
    u8                          intno;
    u8                          __pad[3];
} X86EMU_regs;

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
	unsigned long	mem_base;
	unsigned long	mem_size;
	void*        	private;
	X86EMU_regs		x86;
	} X86EMU_sysEnv;

/*----------------------------- Global Variables --------------------------*/

/* Global emulator machine state.
 *
 * We keep it global to avoid pointer dereferences in the code for speed.
 */

extern    X86EMU_sysEnv	_X86EMU_env;
#define   M             _X86EMU_env

/*-------------------------- Function Prototypes --------------------------*/

/* Function to log information at runtime */

void	printk(const char *fmt, ...);






/*---------------------- Macros and type definitions ----------------------*/

/*  #pragma	pack(1) */  /* Don't pack structs with function pointers! */

/****************************************************************************
REMARKS:
Data structure containing ponters to programmed I/O functions used by the
emulator. This is used so that the user program can hook all programmed
I/O for the emulator to handled as necessary by the user program. By
default the emulator contains simple functions that do not do access the
hardware in any way. To allow the emualtor access the hardware, you will
need to override the programmed I/O functions using the X86EMU_setupPioFuncs
function.

HEADER:
libx86.h

MEMBERS:
inb		- Function to read a byte from an I/O port
inw		- Function to read a word from an I/O port
inl     - Function to read a dword from an I/O port
outb	- Function to write a byte to an I/O port
outw    - Function to write a word to an I/O port
outl    - Function to write a dword to an I/O port
****************************************************************************/
typedef struct {
	u8  	(X86APIP inb)(X86EMU_pioAddr addr);
	u16 	(X86APIP inw)(X86EMU_pioAddr addr);
	u32 	(X86APIP inl)(X86EMU_pioAddr addr);
	void 	(X86APIP outb)(X86EMU_pioAddr addr, u8 val);
	void 	(X86APIP outw)(X86EMU_pioAddr addr, u16 val);
	void 	(X86APIP outl)(X86EMU_pioAddr addr, u32 val);
	} X86EMU_pioFuncs;

/****************************************************************************
REMARKS:
Data structure containing ponters to memory access functions used by the
emulator. This is used so that the user program can hook all memory
access functions as necessary for the emulator. By default the emulator
contains simple functions that only access the internal memory of the
emulator. If you need specialised functions to handle access to different
types of memory (ie: hardware framebuffer accesses and BIOS memory access
etc), you will need to override this using the X86EMU_setupMemFuncs
function.

HEADER:
libx86.h

MEMBERS:
rdb		- Function to read a byte from an address
rdw		- Function to read a word from an address
rdl     - Function to read a dword from an address
wrb		- Function to write a byte to an address
wrw    	- Function to write a word to an address
wrl    	- Function to write a dword to an address
****************************************************************************/
typedef struct {
	u8  	(X86APIP rdb)(u32 addr);
	u16 	(X86APIP rdw)(u32 addr);
	u32 	(X86APIP rdl)(u32 addr);
	void 	(X86APIP wrb)(u32 addr, u8 val);
	void 	(X86APIP wrw)(u32 addr, u16 val);
	void	(X86APIP wrl)(u32 addr, u32 val);
	} X86EMU_memFuncs;

/****************************************************************************
  Here are the default memory read and write
  function in case they are needed as fallbacks.
***************************************************************************/
extern u8 X86API rdb(u32 addr);
extern u16 X86API rdw(u32 addr);
extern u32 X86API rdl(u32 addr);
extern void X86API wrb(u32 addr, u8 val);
extern void X86API wrw(u32 addr, u16 val);
extern void X86API wrl(u32 addr, u32 val);
 
/*  #pragma	pack() */

typedef struct {
	void (* ip)(void);
} X86EMU_checkFuncs;


/*--------------------- type definitions -----------------------------------*/

typedef void (X86APIP X86EMU_intrFuncs)(int num);
extern X86EMU_intrFuncs _X86EMU_intrTab[256];


/*-------------------------- Function Prototypes --------------------------*/

#define	HALT_SYS()	X86EMU_halt_sys()
#define	ILLEGAL_OP()	X86EMU_illegal_op()

/* checks to be enabled for "runtime" */

#define CHECK_IP_FETCH_F                0x1
#define CHECK_SP_ACCESS_F               0x2
#define CHECK_MEM_ACCESS_F              0x4 /*using regular linear pointer */
#define CHECK_DATA_ACCESS_F             0x8 /*using segment:offset*/

/* debug options */

#define DEBUG_DECODE_F          0x000001 /* print decoded instruction  */
#define DEBUG_TRACE_F           0x000002 /* dump regs before/after execution */
#define DEBUG_STEP_F            0x000004
#define DEBUG_DISASSEMBLE_F     0x000008
#define DEBUG_BREAK_F           0x000010
#define DEBUG_SVC_F             0x000020
#define DEBUG_FS_F              0x000080
#define DEBUG_PROC_F            0x000100
#define DEBUG_SYSINT_F          0x000200 /* bios system interrupts. */
#define DEBUG_TRACECALL_F       0x000400
#define DEBUG_INSTRUMENT_F      0x000800
#define DEBUG_MEM_TRACE_F       0x001000 
#define DEBUG_IO_TRACE_F        0x002000 
#define DEBUG_TRACECALL_REGS_F  0x004000
#define DEBUG_DECODE_NOPRINT_F  0x008000 
#define DEBUG_SAVE_IP_CS_F      0x010000
#define DEBUG_SYS_F             (DEBUG_SVC_F|DEBUG_FS_F|DEBUG_PROC_F)

void	X86EMU_setupMemFuncs(X86EMU_memFuncs *funcs);
void	X86EMU_setupPioFuncs(X86EMU_pioFuncs *funcs);
void	X86EMU_setupIntrFuncs(X86EMU_intrFuncs funcs[]);
void	X86EMU_setupCheckFuncs(X86EMU_checkFuncs *funcs);
void	X86EMU_prepareForInt(int num);

void	X86EMU_trace_regs(void);
void	X86EMU_trace_code(void);
void	X86EMU_trace_xregs(void);
void	X86EMU_dump_memory(u16 seg, u16 off, u32 amt);
int	X86EMU_trace_on(void);
int	X86EMU_trace_off(void);

void	X86EMU_exec(void);
void	X86EMU_halt_sys(void);
void	X86EMU_illegal_op(void);

#ifdef  __cplusplus
}                       			/* End of "C" linkage for C++   	*/
#endif

#endif /* __X86EMU_LIBX86_H */
