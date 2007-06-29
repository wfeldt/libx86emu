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
*				instruction decoding and accessess of immediate data via IP.  etc.
*
****************************************************************************/


#include "x86emu/x86emui.h"

/*----------------------------- Implementation ----------------------------*/

/****************************************************************************
REMARKS:
Handles any pending asychronous interrupts.
****************************************************************************/
static void x86emu_intr_handle(void)
{
	u8	intno;

	if (M.x86.intr & INTR_SYNCH) {
		intno = M.x86.intno;
		if (_X86EMU_intrTab[intno]) {
			(*_X86EMU_intrTab[intno])(intno);
		} else {
			push_word((u16)M.x86.R_FLG);
			CLEAR_FLAG(F_IF);
			CLEAR_FLAG(F_TF);
			push_word(M.x86.R_CS);
			M.x86.R_CS = mem_access_word(intno * 4 + 2);
			push_word(M.x86.R_IP);
			M.x86.R_IP = mem_access_word(intno * 4);
			M.x86.intr = 0;
		}
	}
}

/****************************************************************************
PARAMETERS:
intrnum - Interrupt number to raise

REMARKS:
Raise the specified interrupt to be handled before the execution of the
next instruction.
****************************************************************************/
void x86emu_intr_raise(
	u8 intrnum)
{
	M.x86.intno = intrnum;
	M.x86.intr |= INTR_SYNCH;
}

/****************************************************************************
REMARKS:
Main execution loop for the emulator. We return from here when the system
halts, which is normally caused by a stack fault when we return from the
original real mode call.
****************************************************************************/
void X86EMU_exec(void)
{
	u8 op1;
	static unsigned char is_prefix[0x100] = {
		[0x26] = 1, [0x2e] = 1, [0x36] = 1, [0x3e] = 1,
		[0x64 ... 0x67] = 1,
		[0xf0] = 1, [0xf2 ... 0xf3] = 1
	};

	M.x86.intr = 0;
	DB(x86emu_end_instr();)

	for (;;) {
DB(		M.x86.enc_pos = 0;)

DB(		X86EMU_trace_xregs();)

DB(		if (CHECK_IP_FETCH())
			x86emu_check_ip_access();)

		/* If debugging, save the IP and CS values. */
		SAVE_IP_CS(M.x86.R_CS, M.x86.R_IP);

		if (M.x86.intr) {
			if (M.x86.intr & INTR_HALTED) break;

			if (
				((M.x86.intr & INTR_SYNCH) && (M.x86.intno == 0 || M.x86.intno == 2)) ||
				!ACCESS_FLAG(F_IF)
			) {
				x86emu_intr_handle();
			}
		}

		INC_DECODED_INST_LEN(1);

		/* handle prefixes here */
		while (is_prefix[op1 = (*sys_rdb)(((u32)M.x86.R_CS << 4) + (M.x86.R_IP++))]) {
			INC_DECODED_INST_LEN(1);

			switch(op1) {
				case 0x26:
					DECODE_PRINTF("es: ");
					M.x86.mode |= SYSMODE_SEGOVR_ES;
					break;
				case 0x2e:
					DECODE_PRINTF("cs: ");
					M.x86.mode |= SYSMODE_SEGOVR_CS;
					break;
				case 0x36:
					DECODE_PRINTF("ss: ");
					M.x86.mode |= SYSMODE_SEGOVR_SS;
					break;
				case 0x3e:
					DECODE_PRINTF("ds: ");
					M.x86.mode |= SYSMODE_SEGOVR_DS;
					break;
				case 0x64:
					DECODE_PRINTF("fs: ");
					M.x86.mode |= SYSMODE_SEGOVR_FS;
					break;
				case 0x65:
					DECODE_PRINTF("gs: ");
					M.x86.mode |= SYSMODE_SEGOVR_GS;
					break;
				case 0x66:
					// DECODE_PRINTF("data32 ");
					M.x86.mode |= SYSMODE_PREFIX_DATA;
					break;
				case 0x67:
					// DECODE_PRINTF("addr32 ");
					M.x86.mode |= SYSMODE_PREFIX_ADDR;
					break;
				case 0xf0:
					DECODE_PRINTF("lock: ");
					break;
				case 0xf2:
					DECODE_PRINTF("repne ");
					M.x86.mode |= SYSMODE_PREFIX_REPNE;
					break;
				case 0xf3:
					DECODE_PRINTF("repe ");
					M.x86.mode |= SYSMODE_PREFIX_REPE;
					break;
			}
		}
		(*x86emu_optab[op1])(op1);

DB(		X86EMU_trace_code();)
	}

#if 0
DB(	X86EMU_trace_xregs();)
#endif
}

/****************************************************************************
REMARKS:
Halts the system by setting the halted system flag.
****************************************************************************/
void X86EMU_halt_sys(void)
{
	M.x86.intr |= INTR_HALTED;
}

/****************************************************************************
PARAMETERS:
mod		- Mod value from decoded byte
regh	- Reg h value from decoded byte
regl	- Reg l value from decoded byte

REMARKS:
Raise the specified interrupt to be handled before the execution of the
next instruction.

NOTE: Do not inline this function, as (*sys_rdb) is already inline!
****************************************************************************/
void fetch_decode_modrm(
	int *mod,
	int *regh,
	int *regl)
{
	int fetched;

// DB(	if (CHECK_IP_FETCH()) x86emu_check_ip_access();)
	fetched = (*sys_rdb)(((u32)M.x86.R_CS << 4) + (M.x86.R_IP++));
	INC_DECODED_INST_LEN(1);
	*mod  = (fetched >> 6) & 0x03;
	*regh = (fetched >> 3) & 0x07;
    *regl = (fetched >> 0) & 0x07;
}

/****************************************************************************
RETURNS:
Immediate byte value read from instruction queue

REMARKS:
This function returns the immediate byte from the instruction queue, and
moves the instruction pointer to the next value.

NOTE: Do not inline this function, as (*sys_rdb) is already inline!
****************************************************************************/
u8 fetch_byte_imm(void)
{
	u8 fetched;

// DB(	if (CHECK_IP_FETCH()) x86emu_check_ip_access();)
	fetched = (*sys_rdb)(((u32)M.x86.R_CS << 4) + (M.x86.R_IP++));
	INC_DECODED_INST_LEN(1);
	return fetched;
}

/****************************************************************************
RETURNS:
Immediate word value read from instruction queue

REMARKS:
This function returns the immediate byte from the instruction queue, and
moves the instruction pointer to the next value.

NOTE: Do not inline this function, as (*sys_rdw) is already inline!
****************************************************************************/
u16 fetch_word_imm(void)
{
	u16	fetched;

// DB(	if (CHECK_IP_FETCH()) x86emu_check_ip_access();)
	fetched = (*sys_rdw)(((u32)M.x86.R_CS << 4) + (M.x86.R_IP));
	M.x86.R_IP += 2;
	INC_DECODED_INST_LEN(2);
	return fetched;
}

/****************************************************************************
RETURNS:
Immediate lone value read from instruction queue

REMARKS:
This function returns the immediate byte from the instruction queue, and
moves the instruction pointer to the next value.

NOTE: Do not inline this function, as (*sys_rdw) is already inline!
****************************************************************************/
u32 fetch_long_imm(void)
{
	u32 fetched;

// DB(	if (CHECK_IP_FETCH()) x86emu_check_ip_access();)
	fetched = (*sys_rdl)(((u32)M.x86.R_CS << 4) + (M.x86.R_IP));
	M.x86.R_IP += 4;
	INC_DECODED_INST_LEN(4);
	return fetched;
}

/****************************************************************************
RETURNS:
Value of the default data segment

REMARKS:
Inline function that returns the default data segment for the current
instruction.

On the x86 processor, the default segment is not always DS if there is
no segment override. Address modes such as -3[BP] or 10[BP+SI] all refer to
addresses relative to SS (ie: on the stack). So, at the minimum, all
decodings of addressing modes would have to set/clear a bit describing
whether the access is relative to DS or SS.  That is the function of the
cpu-state-varible M.x86.mode. There are several potential states:

	repe prefix seen  (handled elsewhere)
	repne prefix seen  (ditto)

	cs segment override
	ds segment override
	es segment override
	fs segment override
	gs segment override
	ss segment override

	ds/ss select (in absense of override)

Each of the above 7 items are handled with a bit in the mode field.
****************************************************************************/
_INLINE u32 get_data_segment(void)
{
#define	GET_SEGMENT(segment)
	switch (M.x86.mode & SYSMODE_SEGMASK) {
	  case 0:					/* default case: use ds register */
	  case SYSMODE_SEGOVR_DS:
	  case SYSMODE_SEGOVR_DS | SYSMODE_SEG_DS_SS:
		return  M.x86.R_DS;
	  case SYSMODE_SEG_DS_SS:	/* non-overridden, use ss register */
		return  M.x86.R_SS;
	  case SYSMODE_SEGOVR_CS:
	  case SYSMODE_SEGOVR_CS | SYSMODE_SEG_DS_SS:
		return  M.x86.R_CS;
	  case SYSMODE_SEGOVR_ES:
	  case SYSMODE_SEGOVR_ES | SYSMODE_SEG_DS_SS:
		return  M.x86.R_ES;
	  case SYSMODE_SEGOVR_FS:
	  case SYSMODE_SEGOVR_FS | SYSMODE_SEG_DS_SS:
		return  M.x86.R_FS;
	  case SYSMODE_SEGOVR_GS:
	  case SYSMODE_SEGOVR_GS | SYSMODE_SEG_DS_SS:
		return  M.x86.R_GS;
	  case SYSMODE_SEGOVR_SS:
	  case SYSMODE_SEGOVR_SS | SYSMODE_SEG_DS_SS:
		return  M.x86.R_SS;
	  default:
#ifdef	DEBUG
		printk("error: should not happen:  multiple overrides.\n");
#endif
		HALT_SYS();
		return 0;
	}
}

/****************************************************************************
PARAMETERS:
offset	- Offset to load data from

RETURNS:
Byte value read from the absolute memory location.

NOTE: Do not inline this function as (*sys_rdX) is already inline!
****************************************************************************/
u8 fetch_data_byte(
	uint offset)
{
#ifdef DEBUG
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access((u16)get_data_segment(), offset);
#endif
	return (*sys_rdb)((get_data_segment() << 4) + offset);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to load data from

RETURNS:
Word value read from the absolute memory location.

NOTE: Do not inline this function as (*sys_rdX) is already inline!
****************************************************************************/
u16 fetch_data_word(
	uint offset)
{
#ifdef DEBUG
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access((u16)get_data_segment(), offset);
#endif
	return (*sys_rdw)((get_data_segment() << 4) + offset);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to load data from

RETURNS:
Long value read from the absolute memory location.

NOTE: Do not inline this function as (*sys_rdX) is already inline!
****************************************************************************/
u32 fetch_data_long(
	uint offset)
{
#ifdef DEBUG
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access((u16)get_data_segment(), offset);
#endif
	return (*sys_rdl)((get_data_segment() << 4) + offset);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to load data from
offset	- Offset to load data from

RETURNS:
Byte value read from the absolute memory location.

NOTE: Do not inline this function as (*sys_rdX) is already inline!
****************************************************************************/
u8 fetch_data_byte_abs(
	uint segment,
	uint offset)
{
#ifdef DEBUG
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access(segment, offset);
#endif
	return (*sys_rdb)(((u32)segment << 4) + offset);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to load data from
offset	- Offset to load data from

RETURNS:
Word value read from the absolute memory location.

NOTE: Do not inline this function as (*sys_rdX) is already inline!
****************************************************************************/
u16 fetch_data_word_abs(
	uint segment,
	uint offset)
{
#ifdef DEBUG
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access(segment, offset);
#endif
	return (*sys_rdw)(((u32)segment << 4) + offset);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to load data from
offset	- Offset to load data from

RETURNS:
Long value read from the absolute memory location.

NOTE: Do not inline this function as (*sys_rdX) is already inline!
****************************************************************************/
u32 fetch_data_long_abs(
	uint segment,
	uint offset)
{
#ifdef DEBUG
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access(segment, offset);
#endif
	return (*sys_rdl)(((u32)segment << 4) + offset);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a word value to an segmented memory location. The segment used is
the current 'default' segment, which may have been overridden.

NOTE: Do not inline this function as (*sys_wrX) is already inline!
****************************************************************************/
void store_data_byte(
	uint offset,
	u8 val)
{
#ifdef DEBUG
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access((u16)get_data_segment(), offset);
#endif
	(*sys_wrb)((get_data_segment() << 4) + offset, val);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a word value to an segmented memory location. The segment used is
the current 'default' segment, which may have been overridden.

NOTE: Do not inline this function as (*sys_wrX) is already inline!
****************************************************************************/
void store_data_word(
	uint offset,
	u16 val)
{
#ifdef DEBUG
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access((u16)get_data_segment(), offset);
#endif
	(*sys_wrw)((get_data_segment() << 4) + offset, val);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a long value to an segmented memory location. The segment used is
the current 'default' segment, which may have been overridden.

NOTE: Do not inline this function as (*sys_wrX) is already inline!
****************************************************************************/
void store_data_long(
	uint offset,
	u32 val)
{
#ifdef DEBUG
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access((u16)get_data_segment(), offset);
#endif
	(*sys_wrl)((get_data_segment() << 4) + offset, val);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to store data at
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a byte value to an absolute memory location.

NOTE: Do not inline this function as (*sys_wrX) is already inline!
****************************************************************************/
void store_data_byte_abs(
	uint segment,
	uint offset,
	u8 val)
{
#ifdef DEBUG
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access(segment, offset);
#endif
	(*sys_wrb)(((u32)segment << 4) + offset, val);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to store data at
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a word value to an absolute memory location.

NOTE: Do not inline this function as (*sys_wrX) is already inline!
****************************************************************************/
void store_data_word_abs(
	uint segment,
	uint offset,
	u16 val)
{
#ifdef DEBUG
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access(segment, offset);
#endif
	(*sys_wrw)(((u32)segment << 4) + offset, val);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to store data at
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a long value to an absolute memory location.

NOTE: Do not inline this function as (*sys_wrX) is already inline!
****************************************************************************/
void store_data_long_abs(
	uint segment,
	uint offset,
	u32 val)
{
#ifdef DEBUG
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access(segment, offset);
#endif
	(*sys_wrl)(((u32)segment << 4) + offset, val);
}

/****************************************************************************
PARAMETERS:
reg	- Register to decode

RETURNS:
Pointer to the appropriate register

REMARKS:
Return a pointer to the register given by the R/RM field of the
modrm byte, for byte operands. Also enables the decoding of instructions.
****************************************************************************/
u8* decode_rm_byte_register(
	int reg)
{
	switch (reg) {
      case 0:
		DECODE_PRINTF("al");
		return &M.x86.R_AL;
	  case 1:
		DECODE_PRINTF("cl");
		return &M.x86.R_CL;
	  case 2:
		DECODE_PRINTF("dl");
		return &M.x86.R_DL;
	  case 3:
		DECODE_PRINTF("bl");
		return &M.x86.R_BL;
	  case 4:
		DECODE_PRINTF("ah");
		return &M.x86.R_AH;
	  case 5:
		DECODE_PRINTF("ch");
		return &M.x86.R_CH;
	  case 6:
		DECODE_PRINTF("dh");
		return &M.x86.R_DH;
	  case 7:
		DECODE_PRINTF("bh");
		return &M.x86.R_BH;
	}
	HALT_SYS();
	return NULL;                /* NOT REACHED OR REACHED ON ERROR */
}

/****************************************************************************
PARAMETERS:
reg	- Register to decode

RETURNS:
Pointer to the appropriate register

REMARKS:
Return a pointer to the register given by the R/RM field of the
modrm byte, for word operands.  Also enables the decoding of instructions.
****************************************************************************/
u16* decode_rm_word_register(
	int reg)
{
	switch (reg) {
	  case 0:
		DECODE_PRINTF("ax");
		return &M.x86.R_AX;
	  case 1:
		DECODE_PRINTF("cx");
		return &M.x86.R_CX;
	  case 2:
		DECODE_PRINTF("dx");
		return &M.x86.R_DX;
	  case 3:
		DECODE_PRINTF("bx");
		return &M.x86.R_BX;
	  case 4:
		DECODE_PRINTF("sp");
		return &M.x86.R_SP;
	  case 5:
		DECODE_PRINTF("bp");
		return &M.x86.R_BP;
	  case 6:
		DECODE_PRINTF("si");
		return &M.x86.R_SI;
	  case 7:
		DECODE_PRINTF("di");
		return &M.x86.R_DI;
	}
	HALT_SYS();
    return NULL;                /* NOTREACHED OR REACHED ON ERROR */
}

/****************************************************************************
PARAMETERS:
reg	- Register to decode

RETURNS:
Pointer to the appropriate register

REMARKS:
Return a pointer to the register given by the R/RM field of the
modrm byte, for dword operands.  Also enables the decoding of instructions.
****************************************************************************/
u32* decode_rm_long_register(
	int reg)
{
    switch (reg) {
      case 0:
		DECODE_PRINTF("eax");
		return &M.x86.R_EAX;
	  case 1:
		DECODE_PRINTF("ecx");
		return &M.x86.R_ECX;
	  case 2:
		DECODE_PRINTF("edx");
		return &M.x86.R_EDX;
	  case 3:
		DECODE_PRINTF("ebx");
		return &M.x86.R_EBX;
	  case 4:
		DECODE_PRINTF("esp");
		return &M.x86.R_ESP;
	  case 5:
		DECODE_PRINTF("ebp");
		return &M.x86.R_EBP;
	  case 6:
		DECODE_PRINTF("esi");
		return &M.x86.R_ESI;
	  case 7:
		DECODE_PRINTF("edi");
		return &M.x86.R_EDI;
	}
	HALT_SYS();
    return NULL;                /* NOTREACHED OR REACHED ON ERROR */
}

/****************************************************************************
PARAMETERS:
reg	- Register to decode

RETURNS:
Pointer to the appropriate register

REMARKS:
Return a pointer to the register given by the R/RM field of the
modrm byte, for word operands, modified from above for the weirdo
special case of segreg operands.  Also enables the decoding of instructions.
****************************************************************************/
u16* decode_rm_seg_register(
	int reg)
{
	switch (reg) {
	  case 0:
		DECODE_PRINTF("es");
		return &M.x86.R_ES;
	  case 1:
		DECODE_PRINTF("cs");
		return &M.x86.R_CS;
	  case 2:
		DECODE_PRINTF("ss");
		return &M.x86.R_SS;
	  case 3:
		DECODE_PRINTF("ds");
		return &M.x86.R_DS;
	  case 4:
		DECODE_PRINTF("fs");
		return &M.x86.R_FS;
	  case 5:
		DECODE_PRINTF("gs");
		return &M.x86.R_GS;
	  case 6:
	  case 7:
		DECODE_PRINTF("illegal segreg");
		break;
	}
	printk("reg %d\n", reg);
		//DECODE_PRINTF("cs");
		//return &M.x86.R_CS;
	/*HALT_SYS();*/
	return NULL;                /* NOT REACHED OR REACHED ON ERROR */
}

/*
 *
 * return offset from the SIB Byte
 */
u32 decode_sib_address(int sib, int mod)
{
    u32 base = 0, i = 0, scale = 1;

    switch(sib & 0x07) {
    case 0:
	DECODE_PRINTF("[eax]");
	base = M.x86.R_EAX;
	break;
    case 1:
	DECODE_PRINTF("[ecx]");
	base = M.x86.R_ECX;
	break;
    case 2:
	DECODE_PRINTF("[edx]");
	base = M.x86.R_EDX;
	break;
    case 3:
	DECODE_PRINTF("[ebx]");
	base = M.x86.R_EBX;
	break;
    case 4:
	DECODE_PRINTF("[esp]");
	base = M.x86.R_ESP;
	M.x86.mode |= SYSMODE_SEG_DS_SS;
	break;
    case 5:
	if (mod == 0) {
	    base = fetch_long_imm();
	    DECODE_PRINTF2("%08x", base);
	} else {
	    DECODE_PRINTF("[ebp]");
	    base = M.x86.R_ESP;
	    M.x86.mode |= SYSMODE_SEG_DS_SS;
	}
	break;
    case 6:
	DECODE_PRINTF("[esi]");
	base = M.x86.R_ESI;
	break;
    case 7:
	DECODE_PRINTF("[edi]");
	base = M.x86.R_EDI;
	break;
    }
    switch ((sib >> 3) & 0x07) {
    case 0:
	DECODE_PRINTF("[eax");
	i = M.x86.R_EAX;
	break;
    case 1:
	DECODE_PRINTF("[ecx");
	i = M.x86.R_ECX;
	break;
    case 2:
	DECODE_PRINTF("[edx");
	i = M.x86.R_EDX;
	break;
    case 3:
	DECODE_PRINTF("[ebx");
	i = M.x86.R_EBX;
	break;
    case 4:
	i = 0;
	break;
    case 5:
	DECODE_PRINTF("[ebp");
	i = M.x86.R_EBP;
	break;
    case 6:
	DECODE_PRINTF("[esi");
	i = M.x86.R_ESI;
	break;
    case 7:
	DECODE_PRINTF("[edi");
	i = M.x86.R_EDI;
	break;
    }
    scale = 1 << ((sib >> 6) & 0x03);
    if (((sib >> 3) & 0x07) != 4) {
	if (scale == 1) {
	    DECODE_PRINTF("]");
	} else {
	    DECODE_PRINTF2("*%d]", scale);
	}
    }
    return base + (i * scale);
}

/****************************************************************************
PARAMETERS:
rm	- RM value to decode

RETURNS:
Offset in memory for the address decoding

REMARKS:
Return the offset given by mod=00 addressing.  Also enables the
decoding of instructions.

NOTE: 	The code which specifies the corresponding segment (ds vs ss)
		below in the case of [BP+..].  The assumption here is that at the
		point that this subroutine is called, the bit corresponding to
		SYSMODE_SEG_DS_SS will be zero.  After every instruction
		except the segment override instructions, this bit (as well
		as any bits indicating segment overrides) will be clear.  So
		if a SS access is needed, set this bit.  Otherwise, DS access
		occurs (unless any of the segment override bits are set).
****************************************************************************/
u32 decode_rm00_address(
	int rm)
{
    u32 offset;
    int sib;

    if (M.x86.mode & SYSMODE_PREFIX_ADDR) {
        /* 32-bit addressing */
	switch (rm) {
	  case 0:
		DECODE_PRINTF("[eax]");
		return M.x86.R_EAX;
	  case 1:
		DECODE_PRINTF("[ecx]");
		return M.x86.R_ECX;
	  case 2:
		DECODE_PRINTF("[edx]");
		return M.x86.R_EDX;
	  case 3:
		DECODE_PRINTF("[ebx]");
		return M.x86.R_EBX;
	  case 4:
		sib = fetch_byte_imm();
		return decode_sib_address(sib, 0);
	  case 5:
		offset = fetch_long_imm();
		DECODE_PRINTF2("[%08x]", offset);
		return offset;
	  case 6:
		DECODE_PRINTF("[esi]");
		return M.x86.R_ESI;
	  case 7:
		DECODE_PRINTF("[edi]");
		return M.x86.R_EDI;
	}
	HALT_SYS();
    } else {
        /* 16-bit addressing */
	switch (rm) {
	  case 0:
		DECODE_PRINTF("[bx+si]");
            return (M.x86.R_BX + M.x86.R_SI) & 0xffff;
	  case 1:
		DECODE_PRINTF("[bx+di]");
            return (M.x86.R_BX + M.x86.R_DI) & 0xffff;
	  case 2:
		DECODE_PRINTF("[bp+si]");
		M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + M.x86.R_SI) & 0xffff;
	  case 3:
		DECODE_PRINTF("[bp+di]");
		M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + M.x86.R_DI) & 0xffff;
	  case 4:
		DECODE_PRINTF("[si]");
		return M.x86.R_SI;
	  case 5:
		DECODE_PRINTF("[di]");
		return M.x86.R_DI;
	  case 6:
		offset = fetch_word_imm();
		DECODE_PRINTF2("[%04x]", offset);
		return offset;
	  case 7:
		DECODE_PRINTF("[bx]");
		return M.x86.R_BX;
	}
	HALT_SYS();
    }
    return 0;
}

/****************************************************************************
PARAMETERS:
rm	- RM value to decode

RETURNS:
Offset in memory for the address decoding

REMARKS:
Return the offset given by mod=01 addressing.  Also enables the
decoding of instructions.
****************************************************************************/
u32 decode_rm01_address(
	int rm)
{
    int displacement = 0;
    int sib;

    /* Fetch disp8 if no SIB byte */
    if (!((M.x86.mode & SYSMODE_PREFIX_ADDR) && (rm == 4)))
	displacement = (s8)fetch_byte_imm();

    if (M.x86.mode & SYSMODE_PREFIX_ADDR) {
        /* 32-bit addressing */
	switch (rm) {
	  case 0:
		DECODE_PRINTF2("%d[eax]", displacement);
		return M.x86.R_EAX + displacement;
	  case 1:
		DECODE_PRINTF2("%d[ecx]", displacement);
		return M.x86.R_ECX + displacement;
	  case 2:
		DECODE_PRINTF2("%d[edx]", displacement);
		return M.x86.R_EDX + displacement;
	  case 3:
		DECODE_PRINTF2("%d[ebx]", displacement);
		return M.x86.R_EBX + displacement;
	  case 4:
		sib = fetch_byte_imm();
		displacement = (s8)fetch_byte_imm();
		DECODE_PRINTF2("%d", displacement);
		return decode_sib_address(sib, 1) + displacement;
	  case 5:
		DECODE_PRINTF2("%d[ebp]", displacement);
		return M.x86.R_EBP + displacement;
	  case 6:
		DECODE_PRINTF2("%d[esi]", displacement);
		return M.x86.R_ESI + displacement;
	  case 7:
		DECODE_PRINTF2("%d[edi]", displacement);
		return M.x86.R_EDI + displacement;
	}
	HALT_SYS();
    } else {
        /* 16-bit addressing */
	switch (rm) {
	  case 0:
		DECODE_PRINTF2("%d[bx+si]", displacement);
            return (M.x86.R_BX + M.x86.R_SI + displacement) & 0xffff;
	  case 1:
		DECODE_PRINTF2("%d[bx+di]", displacement);
            return (M.x86.R_BX + M.x86.R_DI + displacement) & 0xffff;
	  case 2:
		DECODE_PRINTF2("%d[bp+si]", displacement);
		M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + M.x86.R_SI + displacement) & 0xffff;
	  case 3:
		DECODE_PRINTF2("%d[bp+di]", displacement);
		M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + M.x86.R_DI + displacement) & 0xffff;
	  case 4:
		DECODE_PRINTF2("%d[si]", displacement);
            return (M.x86.R_SI + displacement) & 0xffff;
	  case 5:
		DECODE_PRINTF2("%d[di]", displacement);
            return (M.x86.R_DI + displacement) & 0xffff;
	  case 6:
		DECODE_PRINTF2("%d[bp]", displacement);
		M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + displacement) & 0xffff;
	  case 7:
		DECODE_PRINTF2("%d[bx]", displacement);
            return (M.x86.R_BX + displacement) & 0xffff;
	}
	HALT_SYS();
    }
    return 0;                   /* SHOULD NOT HAPPEN */
}

/****************************************************************************
PARAMETERS:
rm	- RM value to decode

RETURNS:
Offset in memory for the address decoding

REMARKS:
Return the offset given by mod=10 addressing.  Also enables the
decoding of instructions.
****************************************************************************/
u32 decode_rm10_address(
	int rm)
{
    u32 displacement = 0;
    int sib;

    /* Fetch disp16 if 16-bit addr mode */
    if (!(M.x86.mode & SYSMODE_PREFIX_ADDR))
	displacement = (u16)fetch_word_imm();
    else {
	/* Fetch disp32 if no SIB byte */
	if (rm != 4)
	    displacement = (u32)fetch_long_imm();
    }

    if (M.x86.mode & SYSMODE_PREFIX_ADDR) {
        /* 32-bit addressing */
      switch (rm) {
	  case 0:
		DECODE_PRINTF2("%08x[eax]", displacement);
		return M.x86.R_EAX + displacement;
	  case 1:
		DECODE_PRINTF2("%08x[ecx]", displacement);
		return M.x86.R_ECX + displacement;
	  case 2:
		DECODE_PRINTF2("%08x[edx]", displacement);
		M.x86.mode |= SYSMODE_SEG_DS_SS;
		return M.x86.R_EDX + displacement;
	  case 3:
		DECODE_PRINTF2("%08x[ebx]", displacement);
		return M.x86.R_EBX + displacement;
	  case 4:
		sib = fetch_byte_imm();
		displacement = (u32)fetch_long_imm();
		DECODE_PRINTF2("%08x", displacement);
		return decode_sib_address(sib, 2) + displacement;
		break;
	  case 5:
		DECODE_PRINTF2("%08x[ebp]", displacement);
		return M.x86.R_EBP + displacement;
	  case 6:
		DECODE_PRINTF2("%08x[esi]", displacement);
		return M.x86.R_ESI + displacement;
	  case 7:
		DECODE_PRINTF2("%08x[edi]", displacement);
		return M.x86.R_EDI + displacement;
	}
	HALT_SYS();
    } else {
        /* 16-bit addressing */
      switch (rm) {
	  case 0:
            DECODE_PRINTF2("%04x[bx+si]", displacement);
            return (M.x86.R_BX + M.x86.R_SI + displacement) & 0xffff;
	  case 1:
            DECODE_PRINTF2("%04x[bx+di]", displacement);
            return (M.x86.R_BX + M.x86.R_DI + displacement) & 0xffff;
	  case 2:
		DECODE_PRINTF2("%04x[bp+si]", displacement);
		M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + M.x86.R_SI + displacement) & 0xffff;
	  case 3:
		DECODE_PRINTF2("%04x[bp+di]", displacement);
		M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + M.x86.R_DI + displacement) & 0xffff;
	  case 4:
            DECODE_PRINTF2("%04x[si]", displacement);
            return (M.x86.R_SI + displacement) & 0xffff;
	  case 5:
            DECODE_PRINTF2("%04x[di]", displacement);
            return (M.x86.R_DI + displacement) & 0xffff;
	  case 6:
		DECODE_PRINTF2("%04x[bp]", displacement);
		M.x86.mode |= SYSMODE_SEG_DS_SS;
            return (M.x86.R_BP + displacement) & 0xffff;
	  case 7:
            DECODE_PRINTF2("%04x[bx]", displacement);
            return (M.x86.R_BX + displacement) & 0xffff;
	}
	HALT_SYS();
    }
    return 0;
    /*NOTREACHED */
}
