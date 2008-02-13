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


#include "include/x86emui.h"

static void     print_encoded_bytes (u16 s, u32 o);
static void     print_decoded_instruction (void);


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
  x86emu_end_instr();

  for(;;) {
    M.x86.disasm_ptr = M.x86.disasm_buf;
    *M.x86.disasm_ptr = 0;	// for now

    *M.x86.decoded_buf = 0;	// dto.

    M.x86.instr_len = 0;

    M.x86.mode = 0;
    if(ACC_D(M.x86.R_CS_ACC)) {
      M.x86.mode |= _MODE_DATA32 | _MODE_ADDR32;
    }
    if(ACC_D(M.x86.R_SS_ACC)) {
      M.x86.mode |= _MODE_STACK32;
    }

    M.x86.default_seg = NULL;

    X86EMU_trace_regs();

    if(CHECK_IP_FETCH()) x86emu_check_ip_access();

    /* save EIP and CS values */
    M.x86.saved_cs = M.x86.R_CS;
    M.x86.saved_eip = M.x86.R_EIP;

    if(M.x86.intr) {
      if(M.x86.intr & INTR_HALTED) break;

      if(
        ((M.x86.intr & INTR_SYNCH) && (M.x86.intno == 0 || M.x86.intno == 2)) ||
        !ACCESS_FLAG(F_IF)
      ) {
        x86emu_intr_handle();
      }
    }

    memcpy(M.x86.decode_seg, "[", 1);

    /* handle prefixes here */
    while(is_prefix[op1 = fetch_byte_imm()]) {
      switch(op1) {
        case 0x26:
          memcpy(M.x86.decode_seg, "es:[", 4);
          M.x86.default_seg = M.x86.seg + R_ES_INDEX;
          break;
        case 0x2e:
          memcpy(M.x86.decode_seg, "cs:[", 4);
          M.x86.default_seg = M.x86.seg + R_CS_INDEX;
          break;
        case 0x36:
          memcpy(M.x86.decode_seg, "ss:[", 4);
          M.x86.default_seg = M.x86.seg + R_SS_INDEX;
          break;
        case 0x3e:
          memcpy(M.x86.decode_seg, "ds:[", 4);
          M.x86.default_seg = M.x86.seg + R_DS_INDEX;
          break;
        case 0x64:
          memcpy(M.x86.decode_seg, "fs:[", 4);
          M.x86.default_seg = M.x86.seg + R_FS_INDEX;
          break;
        case 0x65:
          memcpy(M.x86.decode_seg, "gs:[", 4);
          M.x86.default_seg = M.x86.seg + R_GS_INDEX;
          break;
        case 0x66:
          M.x86.mode ^= _MODE_DATA32;
          break;
        case 0x67:
          M.x86.mode ^= _MODE_ADDR32;
          break;
        case 0xf0:
          OP_DECODE("lock: ");
          break;
        case 0xf2:
          OP_DECODE("repne ");
          M.x86.mode |= SYSMODE_PREFIX_REPNE;
          break;
        case 0xf3:
          OP_DECODE("repe ");
          M.x86.mode |= SYSMODE_PREFIX_REPE;
          break;
      }
    }
    (*x86emu_optab[op1])(op1);

    *M.x86.disasm_ptr++ = 0;

    X86EMU_trace_code();
  }
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
REMARKS:
Flag illegal Opcode.
****************************************************************************/
void X86EMU_illegal_op(void)
{
  M.x86.intr |= INTR_HALTED + INTR_ILLEGAL_OP;
  M.x86.disasm_ptr = M.x86.disasm_buf;
  OP_DECODE("illegal opcode");
}

/****************************************************************************
PARAMETERS:
mod		- Mod value from decoded byte
regh	- Reg h value from decoded byte
regl	- Reg l value from decoded byte

REMARKS:
Raise the specified interrupt to be handled before the execution of the
next instruction.
****************************************************************************/
void fetch_decode_modrm(int *mod, int *regh, int *regl)
{
  u8 fetched;

  fetched = fetch_byte_imm();

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

// if (CHECK_IP_FETCH()) x86emu_check_ip_access();

  fetched = (*sys_rdb)(((u32)M.x86.R_CS << 4) + (M.x86.R_IP));

  M.x86.R_IP += 1;

  if(M.x86.instr_len < sizeof M.x86.instr_buf) {
    M.x86.instr_buf[M.x86.instr_len++] = fetched;
  }

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
  u16 fetched;

// if (CHECK_IP_FETCH()) x86emu_check_ip_access();
  fetched = (*sys_rdw)(((u32)M.x86.R_CS << 4) + (M.x86.R_IP));

  M.x86.R_IP += 2;

  if(M.x86.instr_len + 1 < sizeof M.x86.instr_buf) {
    M.x86.instr_buf[M.x86.instr_len++] = fetched;
    M.x86.instr_buf[M.x86.instr_len++] = fetched >> 8;
  }

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

// if (CHECK_IP_FETCH()) x86emu_check_ip_access();
  fetched = (*sys_rdl)(((u32)M.x86.R_CS << 4) + (M.x86.R_IP));

  M.x86.R_IP += 4;

  if(M.x86.instr_len + 3 < sizeof M.x86.instr_buf) {
    M.x86.instr_buf[M.x86.instr_len++] = fetched;
    M.x86.instr_buf[M.x86.instr_len++] = fetched >> 8;
    M.x86.instr_buf[M.x86.instr_len++] = fetched >> 16;
    M.x86.instr_buf[M.x86.instr_len++] = fetched >> 24;
  }

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
static u32 get_data_segment(void)
{
  sel_t *seg;

  if(!(seg = M.x86.default_seg)) {
    seg = M.x86.seg + (M.x86.mode & SYSMODE_SEG_DS_SS ? R_SS_INDEX : R_DS_INDEX);
  }

  return seg->sel;
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

	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access((u16)get_data_segment(), offset);

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

	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access((u16)get_data_segment(), offset);

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

	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access((u16)get_data_segment(), offset);

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

	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access(segment, offset);

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

	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access(segment, offset);

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
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access(segment, offset);

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
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access((u16)get_data_segment(), offset);

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
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access((u16)get_data_segment(), offset);

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
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access((u16)get_data_segment(), offset);

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
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access(segment, offset);

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
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access(segment, offset);

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
	if (CHECK_DATA_ACCESS())
		x86emu_check_data_access(segment, offset);

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
u16* decode_rm_word_register(int reg)
{
  switch(reg) {
    case 0:
      OP_DECODE("ax");
      return &M.x86.R_AX;

    case 1:
      OP_DECODE("cx");
      return &M.x86.R_CX;

    case 2:
      OP_DECODE("dx");
      return &M.x86.R_DX;

    case 3:
      OP_DECODE("bx");
      return &M.x86.R_BX;

    case 4:
      OP_DECODE("sp");
      return &M.x86.R_SP;

    case 5:
      OP_DECODE("bp");
      return &M.x86.R_BP;

    case 6:
      OP_DECODE("si");
      return &M.x86.R_SI;

    case 7:
      OP_DECODE("di");
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
u32* decode_rm_long_register(int reg)
{
  switch(reg) {
    case 0:
      OP_DECODE("eax");
      return &M.x86.R_EAX;

    case 1:
      OP_DECODE("ecx");
      return &M.x86.R_ECX;

    case 2:
      OP_DECODE("edx");
      return &M.x86.R_EDX;

    case 3:
      OP_DECODE("ebx");
      return &M.x86.R_EBX;

    case 4:
      OP_DECODE("esp");
      return &M.x86.R_ESP;

    case 5:
      OP_DECODE("ebp");
      return &M.x86.R_EBP;

    case 6:
      OP_DECODE("esi");
      return &M.x86.R_ESI;

    case 7:
      OP_DECODE("edi");
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
sel_t *decode_rm_seg_register(int reg)
{
  switch(reg) {
    case 0:
      OP_DECODE("es");
      break;

    case 1:
      OP_DECODE("cs");
      break;

    case 2:
      OP_DECODE("ss");
      break;

    case 3:
      OP_DECODE("ds");
      break;

    case 4:
      OP_DECODE("fs");
      break;

    case 5:
      OP_DECODE("gs");
      break;

    default:
      ILLEGAL_OP();
      reg = 6;
      break;
  }

  return M.x86.seg + reg;
}


void decode_hex2(u32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = M.x86.disasm_ptr;

  M.x86.disasm_ptr += 2;

  s[1] = h[ofs & 0xf];
  ofs >>= 4;
  s[0] = h[ofs & 0xf];
}


void decode_hex4(u32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = M.x86.disasm_ptr;

  M.x86.disasm_ptr += 4;

  s[3] = h[ofs & 0xf];
  ofs >>= 4;
  s[2] = h[ofs & 0xf];
  ofs >>= 4;
  s[1] = h[ofs & 0xf];
  ofs >>= 4;
  s[0] = h[ofs & 0xf];
}


void decode_hex8(u32 ofs)
{
  decode_hex4(ofs >> 16);
  decode_hex4(ofs & 0xffff);
}


void decode_hex2s(s32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = M.x86.disasm_ptr;

  M.x86.disasm_ptr += 3;

  if(ofs >= 0) {
    s[0] = '+';
  }
  else {
    s[0] = '-';
    ofs = -ofs;
  }

  s[2] = h[ofs & 0xf];
  ofs >>= 4;
  s[1] = h[ofs & 0xf];
}


void decode_hex4s(s32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = M.x86.disasm_ptr;

  M.x86.disasm_ptr += 5;

  if(ofs >= 0) {
    s[0] = '+';
  }
  else {
    s[0] = '-';
    ofs = -ofs;
  }

  s[4] = h[ofs & 0xf];
  ofs >>= 4;
  s[3] = h[ofs & 0xf];
  ofs >>= 4;
  s[2] = h[ofs & 0xf];
  ofs >>= 4;
  s[1] = h[ofs & 0xf];
}


void decode_hex8s(s32 ofs)
{
  if(ofs >= 0) {
    *M.x86.disasm_ptr++ = '+';
  }
  else {
    *M.x86.disasm_ptr++ = '-';
    ofs = -ofs;
  }

  decode_hex8(ofs);
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
u32 decode_rm_address(int mod, int rl)
{
  switch(mod) {
    case 0:
      return decode_rm00_address(rl);
      break;

    case 1:
      return decode_rm01_address(rl);
      break;

    case 2:
      return decode_rm10_address(rl);
      break;

    default:
      ILLEGAL_OP();
      break;
  }

  return 0;
}


u32 decode_rm00_address(int rm)
{
  u32 offset, base;
  int sib;

  if(M.x86.mode & SYSMODE_PREFIX_ADDR) {
    /* 32-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("eax]");
        return M.x86.R_EAX;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("ecx]");
        return M.x86.R_ECX;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("edx]");
        return M.x86.R_EDX;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("ebx]");
        return M.x86.R_EBX;

      case 4:
        sib = fetch_byte_imm();
        base = decode_sib_address(sib, 0);
        OP_DECODE("]");
        return base;

      case 5:
        offset = fetch_long_imm();
        SEGPREF_DECODE;
        decode_hex8(offset);
        OP_DECODE("]");
        return offset;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("esi]");
        return M.x86.R_ESI;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("edi]");
        return M.x86.R_EDI;
    }
    HALT_SYS();
  }
  else {
    /* 16-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("bx+si]");
        return (M.x86.R_BX + M.x86.R_SI) & 0xffff;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("bx+di]");
        return (M.x86.R_BX + M.x86.R_DI) & 0xffff;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("bp+si]");
        M.x86.mode |= SYSMODE_SEG_DS_SS;
        return (M.x86.R_BP + M.x86.R_SI) & 0xffff;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("bp+di]");
        M.x86.mode |= SYSMODE_SEG_DS_SS;
        return (M.x86.R_BP + M.x86.R_DI) & 0xffff;

      case 4:
        SEGPREF_DECODE;
        OP_DECODE("si]");
        return M.x86.R_SI;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("di]");
        return M.x86.R_DI;

      case 6:
        offset = fetch_word_imm();
        SEGPREF_DECODE;
        decode_hex4(offset);
        OP_DECODE("]");
        return offset;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("bx]");
        return M.x86.R_BX;
      }
    HALT_SYS();
  }

  HALT_SYS();

  return 0;
}


u32 decode_rm01_address(int rm)
{
  s32 displacement = 0;
  u32 base;
  int sib;

  /* Fetch disp8 if no SIB byte */
  if(!((M.x86.mode & SYSMODE_PREFIX_ADDR) && (rm == 4))) {
    displacement = (s8) fetch_byte_imm();
  }

  if(M.x86.mode & SYSMODE_PREFIX_ADDR) {
    /* 32-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("eax");
        decode_hex2s(displacement);
        OP_DECODE("]");
        return M.x86.R_EAX + displacement;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("ecx");
        decode_hex2s(displacement);
        OP_DECODE("]");
        return M.x86.R_ECX + displacement;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("edx");
        decode_hex2s(displacement);
        OP_DECODE("]");
        return M.x86.R_EDX + displacement;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("ebx");
        decode_hex2s(displacement);
        OP_DECODE("]");
        return M.x86.R_EBX + displacement;

      case 4:
        sib = fetch_byte_imm();
        base = decode_sib_address(sib, 1);
        displacement = (s8) fetch_byte_imm();
        decode_hex2s(displacement);
        OP_DECODE("]");
        return base + displacement;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("ebp");
        decode_hex2s(displacement);
        OP_DECODE("]");
        return M.x86.R_EBP + displacement;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("esi");
        decode_hex2s(displacement);
        OP_DECODE("]");
        return M.x86.R_ESI + displacement;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("edi");
        decode_hex2s(displacement);
        OP_DECODE("]");
        return M.x86.R_EDI + displacement;
    }
    HALT_SYS();
  }
  else {
    /* 16-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("bx+si");
        decode_hex2s(displacement);
        OP_DECODE("]");
        return (M.x86.R_BX + M.x86.R_SI + displacement) & 0xffff;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("bx+di");
        decode_hex2s(displacement);
        OP_DECODE("]");
        return (M.x86.R_BX + M.x86.R_DI + displacement) & 0xffff;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("bp+si");
        decode_hex2s(displacement);
        OP_DECODE("]");
        M.x86.mode |= SYSMODE_SEG_DS_SS;
        return (M.x86.R_BP + M.x86.R_SI + displacement) & 0xffff;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("bp+di");
        decode_hex2s(displacement);
        OP_DECODE("]");
        DECODE_PRINTF2("%d[bp+di]", displacement);
        M.x86.mode |= SYSMODE_SEG_DS_SS;
        return (M.x86.R_BP + M.x86.R_DI + displacement) & 0xffff;

      case 4:
        SEGPREF_DECODE;
        OP_DECODE("si");
        decode_hex2s(displacement);
        OP_DECODE("]");
        return (M.x86.R_SI + displacement) & 0xffff;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("di");
        decode_hex2s(displacement);
        OP_DECODE("]");
        return (M.x86.R_DI + displacement) & 0xffff;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("bp");
        decode_hex2s(displacement);
        OP_DECODE("]");
        M.x86.mode |= SYSMODE_SEG_DS_SS;
        return (M.x86.R_BP + displacement) & 0xffff;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("bx");
        decode_hex2s(displacement);
        OP_DECODE("]");
        return (M.x86.R_BX + displacement) & 0xffff;
      }
    HALT_SYS();
  }
  HALT_SYS();

  return 0;
}


u32 decode_rm10_address(int rm)
{
  s32 displacement = 0;
  u32 base;
  int sib;

  /* Fetch disp16 if 16-bit addr mode */
  if(!(M.x86.mode & SYSMODE_PREFIX_ADDR)) {
    displacement = (s16) fetch_word_imm();
  }
  else {
    /* Fetch disp32 if no SIB byte */
    if(rm != 4) displacement = (s32) fetch_long_imm();
  }

  if(M.x86.mode & SYSMODE_PREFIX_ADDR) {
    /* 32-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("eax");
        decode_hex8s(displacement);
        OP_DECODE("]");
        return M.x86.R_EAX + displacement;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("ecx");
        decode_hex8s(displacement);
        OP_DECODE("]");
        return M.x86.R_ECX + displacement;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("edx");
        decode_hex8s(displacement);
        OP_DECODE("]");
        return M.x86.R_EDX + displacement;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("ebx");
        decode_hex8s(displacement);
        OP_DECODE("]");
        return M.x86.R_EBX + displacement;

      case 4:
        sib = fetch_byte_imm();
        base = decode_sib_address(sib, 2);
        displacement = (s32) fetch_long_imm();
        decode_hex8s(displacement);
        OP_DECODE("]");
        return base + displacement;
        break;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("ebp");
        decode_hex8s(displacement);
        OP_DECODE("]");
        M.x86.mode |= SYSMODE_SEG_DS_SS;
        return M.x86.R_EBP + displacement;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("esi");
        decode_hex8s(displacement);
        OP_DECODE("]");
        return M.x86.R_ESI + displacement;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("edi");
        decode_hex8s(displacement);
        OP_DECODE("]");
        return M.x86.R_EDI + displacement;
    }
    HALT_SYS();
  }
  else {
    /* 16-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("bx+si");
        decode_hex4s(displacement);
        OP_DECODE("]");
        return (M.x86.R_BX + M.x86.R_SI + displacement) & 0xffff;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("bx+di");
        decode_hex4s(displacement);
        OP_DECODE("]");
        return (M.x86.R_BX + M.x86.R_DI + displacement) & 0xffff;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("bp+si");
        decode_hex4s(displacement);
        OP_DECODE("]");
        M.x86.mode |= SYSMODE_SEG_DS_SS;
        return (M.x86.R_BP + M.x86.R_SI + displacement) & 0xffff;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("bp+di");
        decode_hex4s(displacement);
        OP_DECODE("]");
        M.x86.mode |= SYSMODE_SEG_DS_SS;
        return (M.x86.R_BP + M.x86.R_DI + displacement) & 0xffff;

      case 4:
        SEGPREF_DECODE;
        OP_DECODE("si");
        decode_hex4s(displacement);
        OP_DECODE("]");
        return (M.x86.R_SI + displacement) & 0xffff;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("di");
        decode_hex4s(displacement);
        OP_DECODE("]");
        return (M.x86.R_DI + displacement) & 0xffff;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("bp");
        decode_hex4s(displacement);
        OP_DECODE("]");
        M.x86.mode |= SYSMODE_SEG_DS_SS;
        return (M.x86.R_BP + displacement) & 0xffff;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("bx");
        decode_hex4s(displacement);
        OP_DECODE("]");
        return (M.x86.R_BX + displacement) & 0xffff;
    }
    HALT_SYS();
  }
  HALT_SYS();

  return 0;
}


/*
 *
 * return offset from the SIB Byte
 */
u32 decode_sib_address(int sib, int mod)
{
  u32 base = 0, i = 0, scale = 1;

  /* sib base */
  switch(sib & 0x07) {
    case 0:
      SEGPREF_DECODE;
      OP_DECODE("eax");
      base = M.x86.R_EAX;
      break;

    case 1:
      SEGPREF_DECODE;
      OP_DECODE("ecx");
      base = M.x86.R_ECX;
      break;

    case 2:
      SEGPREF_DECODE;
      OP_DECODE("edx");
      base = M.x86.R_EDX;
      break;

    case 3:
      SEGPREF_DECODE;
      OP_DECODE("ebx");
      base = M.x86.R_EBX;
      break;

    case 4:
      SEGPREF_DECODE;
      OP_DECODE("esp");
      base = M.x86.R_ESP;
      M.x86.mode |= SYSMODE_SEG_DS_SS;
      break;

    case 5:
      SEGPREF_DECODE;
      if(mod == 0) {
        base = fetch_long_imm();
        decode_hex8(base);
      }
      else {
        OP_DECODE("ebp");
        base = M.x86.R_EBP;
        M.x86.mode |= SYSMODE_SEG_DS_SS;
      }
      break;

    case 6:
      SEGPREF_DECODE;
      OP_DECODE("esi");
      base = M.x86.R_ESI;
      break;

    case 7:
      SEGPREF_DECODE;
      OP_DECODE("edi");
      base = M.x86.R_EDI;
      break;
  }

  /* sib index */
  switch((sib >> 3) & 0x07) {
    case 0:
      OP_DECODE("+eax");
      i = M.x86.R_EAX;
      break;

    case 1:
      OP_DECODE("+ecx");
      i = M.x86.R_ECX;
      break;

    case 2:
      OP_DECODE("+edx");
      i = M.x86.R_EDX;
      break;

    case 3:
      OP_DECODE("+ebx");
      i = M.x86.R_EBX;
      break;

    case 4:
      i = 0;
      break;

    case 5:
      OP_DECODE("+ebp");
      i = M.x86.R_EBP;
      break;

    case 6:
      OP_DECODE("+esi");
      i = M.x86.R_ESI;
      break;

    case 7:
      OP_DECODE("+edi");
      i = M.x86.R_EDI;
      break;
  }

  scale = (sib >> 6) & 0x03;

  if(((sib >> 3) & 0x07) != 4) {
    if(scale) {
      OP_DECODE("*");
      *M.x86.disasm_ptr++ = '0' + (1 << scale);
    }
  }

  return base + (i << scale);
}


void X86EMU_reset(X86EMU_sysEnv *emu)
{
  X86EMU_regs *x86 = &emu->x86;

  memset(x86, 0, sizeof *x86);

  x86->R_EFLG = 2;

  x86->R_CS_LIMIT = x86->R_DS_LIMIT = x86->R_SS_LIMIT = x86->R_ES_LIMIT =
  x86->R_FS_LIMIT = x86->R_GS_LIMIT = 0xffff;

  // resp. 0x4093/9b for 4GB
  x86->R_CS_ACC = 0x9b;
  x86->R_SS_ACC = x86->R_DS_ACC = x86->R_ES_ACC = x86->R_FS_ACC = x86->R_GS_ACC = 0x93;

  x86->R_CS = 0xf000;
  x86->R_CS_BASE = 0xf0000;
  x86->R_EIP = 0xfff0;

  x86->R_GDT_LIMIT = 0xffff;
  x86->R_IDT_LIMIT = 0xffff;
}


void X86EMU_trace_code (void)
{
	if (DEBUG_DECODE()) {
		printk("  %04x:%08x ",M.x86.saved_cs, M.x86.saved_eip);
		print_encoded_bytes(M.x86.saved_cs, M.x86.saved_eip);
		print_decoded_instruction();
	}
}

void X86EMU_trace_regs (void)
{
	if (DEBUG_TRACE()) {
		printk("\n");
		x86emu_dump_regs();
	}
}

void x86emu_check_ip_access (void)
{
	if(sys_check_ip) (*sys_check_ip)();
}

void x86emu_check_sp_access (void)
{
}

void x86emu_check_mem_access (u32 dummy)
{
	/*  check bounds, etc */
}

void x86emu_check_data_access (uint dummy1, uint dummy2)
{
	/*  check bounds, etc */
}

void x86emu_decode_printf (char *x)
{
	sprintf(M.x86.decoded_buf+M.x86.enc_str_pos,"%s",x);
	M.x86.enc_str_pos += strlen(x);
}

void x86emu_decode_printf2 (char *x, int y)
{
	char temp[100];
	sprintf(temp,x,y);
	sprintf(M.x86.decoded_buf+M.x86.enc_str_pos,"%s",temp);
	M.x86.enc_str_pos += strlen(temp);
}

void x86emu_end_instr (void)
{
	M.x86.enc_str_pos = 0;
}

void print_encoded_bytes (u16 s, u32 o)
{
  unsigned u;
  char buf1[256];

  for(u = 0, *buf1 = 0; u < M.x86.instr_len; u++) {
    sprintf(buf1 + 2 * u, "%02x", M.x86.instr_buf[u]);
  }
  printk("%-20s  ", buf1);
}

void print_decoded_instruction (void)
{
  if(*M.x86.disasm_buf) {
    printk("%s\n", M.x86.disasm_buf);
  }
  else {
    printk("%s", *M.x86.decoded_buf ? M.x86.decoded_buf : "\n");
  }
}

void x86emu_dump_regs (void)
{
	printk("  eax %08x, ", M.x86.R_EAX );
	printk("ebx %08x, ", M.x86.R_EBX );
	printk("ecx %08x, ", M.x86.R_ECX );
	printk("edx %08x\n", M.x86.R_EDX );
	printk("  esi %08x, ", M.x86.R_ESI );
	printk("edi %08x, ", M.x86.R_EDI );
	printk("ebp %08x, ", M.x86.R_EBP );
	printk("esp %08x\n", M.x86.R_ESP );
	printk("  cs %04x, ", M.x86.R_CS );
	printk("ss %04x, ", M.x86.R_SS );
	printk("ds %04x, ", M.x86.R_DS );
	printk("es %04x, ", M.x86.R_ES );
	printk("fs %04x, ", M.x86.R_FS );
	printk("gs %04x\n", M.x86.R_GS );
	printk("  eip %08x, eflags %08x ", M.x86.R_EIP, M.x86.R_EFLG );
	if (ACCESS_FLAG(F_OF))    printk("of ");
	if (ACCESS_FLAG(F_DF))    printk("df ");
	if (ACCESS_FLAG(F_IF))    printk("if ");
	if (ACCESS_FLAG(F_SF))    printk("sf ");
	if (ACCESS_FLAG(F_ZF))    printk("zf ");
	if (ACCESS_FLAG(F_AF))    printk("af ");
	if (ACCESS_FLAG(F_PF))    printk("pf ");
	if (ACCESS_FLAG(F_CF))    printk("cf ");
	printk("\n");
}


void decode_set_seg_register(sel_t *seg, u16 val)
{
  seg->sel = val;
  seg->base = val << 4;
}

