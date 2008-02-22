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


#include "include/x86emu_int.h"

#define LOG_STR(a) memcpy(*p, a, sizeof a - 1), *p += sizeof a - 1
#define LOG_SPACE (M.log.ptr - M.log.buf + 512 < M.log.size)

/*----------------------------- Implementation ----------------------------*/

x86emu_t x86emu;

static void handle_interrupt(void);
static void generate_int(u8 nr, unsigned type, unsigned errcode);
static void log_regs(void);
static void log_code(void);
void check_data_access(sel_t *seg, u32 ofs, u32 size);


/****************************************************************************
REMARKS:
Main execution loop for the emulator. We return from here when the system
halts, which is normally caused by a stack fault when we return from the
original real mode call.
****************************************************************************/
void x86emu_exec(void)
{
  u8 op1;

  static unsigned char is_prefix[0x100] = {
    [0x26] = 1, [0x2e] = 1, [0x36] = 1, [0x3e] = 1,
    [0x64 ... 0x67] = 1,
    [0xf0] = 1, [0xf2 ... 0xf3] = 1
  };

  for(;;) {
    *(M.x86.disasm_ptr = M.x86.disasm_buf) = 0;

    M.x86.instr_len = 0;

    M.x86.mode = 0;

    if(ACC_D(M.x86.R_CS_ACC)) {
      M.x86.mode |= _MODE_DATA32 | _MODE_ADDR32 | _MODE_CODE32;
    }
    if(ACC_D(M.x86.R_SS_ACC)) {
      M.x86.mode |= _MODE_STACK32;
    }

    M.x86.default_seg = NULL;

    /* save EIP and CS values */
    M.x86.saved_cs = M.x86.R_CS;
    M.x86.saved_eip = M.x86.R_EIP;

    log_regs();

    if(M.code_check && (*M.code_check)()) break;

    memcpy(M.x86.decode_seg, "[", 1);

    /* handle prefixes here */
    while(is_prefix[op1 = fetch_byte()]) {
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
          M.x86.mode |= _MODE_REPNE;
          break;
        case 0xf3:
          OP_DECODE("repe ");
          M.x86.mode |= _MODE_REPE;
          break;
      }
    }
    (*x86emu_optab[op1])(op1);

    *M.x86.disasm_ptr = 0;

    handle_interrupt();

    log_code();

    M.x86.tsc++;	// time stamp counter

    if(MODE_HALTED) break;
  }
}

/****************************************************************************
REMARKS:
Halts the system by setting the halted system flag.
****************************************************************************/
void x86emu_stop(void)
{
  M.x86.mode |= _MODE_HALTED;
}

/****************************************************************************
REMARKS:
Handles any pending asychronous interrupts.
****************************************************************************/
void handle_interrupt()
{
  char **p = &M.log.ptr;

  if(M.x86.intr_type) {
    if(!M.log.intr || !p || !LOG_SPACE) return;

    if((M.x86.intr_type & 0xff) == INTR_TYPE_FAULT) {
      LOG_STR("* fault ");
    }
    else {
      LOG_STR("* int ");
    }
    decode_hex2(p, M.x86.intr_nr & 0xff);
    LOG_STR("\n");
    **p = 0;

    generate_int(M.x86.intr_nr, M.x86.intr_type, M.x86.intr_errcode);
  }

  M.x86.intr_type = 0;
}

/****************************************************************************
PARAMETERS:
intrnum - Interrupt number to raise

REMARKS:
Raise the specified interrupt to be handled before the execution of the
next instruction.
****************************************************************************/
void x86emu_intr_raise(u8 intr_nr, unsigned type, unsigned err)
{
  if(!M.x86.intr_type) {
    M.x86.intr_nr = intr_nr;
    M.x86.intr_type = type;
    M.x86.intr_errcode = err;
  }
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

  fetched = fetch_byte();

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
****************************************************************************/
u8 fetch_byte(void)
{
  u8 fetched;

  fetched = (*M.mem.rdb)(M.x86.R_CS_BASE + M.x86.R_EIP);

  if(MODE_CODE32) {
    M.x86.R_EIP++;
  }
  else {
    M.x86.R_IP++;
  }

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
****************************************************************************/
u16 fetch_word(void)
{
  u16 fetched;

  fetched = (*M.mem.rdw)(M.x86.R_CS_BASE + M.x86.R_EIP);

  if(MODE_CODE32) {
    M.x86.R_EIP += 2;
  }
  else {
    M.x86.R_IP += 2;
  }

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
****************************************************************************/
u32 fetch_long(void)
{
  u32 fetched;

  fetched = (*M.mem.rdl)(M.x86.R_CS_BASE + M.x86.R_EIP);

  if(MODE_CODE32) {
    M.x86.R_EIP += 4;
  }
  else {
    M.x86.R_IP += 4;
  }

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
static sel_t *get_data_segment(void)
{
  sel_t *seg;

  if(!(seg = M.x86.default_seg)) {
    seg = M.x86.seg + (M.x86.mode & _MODE_SEG_DS_SS ? R_SS_INDEX : R_DS_INDEX);
  }

  return seg;
}

/****************************************************************************
PARAMETERS:
offset	- Offset to load data from

RETURNS:
Byte value read from the absolute memory location.
****************************************************************************/
u8 fetch_data_byte(u32 ofs)
{
  return fetch_data_byte_abs(get_data_segment(), ofs);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to load data from

RETURNS:
Word value read from the absolute memory location.
****************************************************************************/
u16 fetch_data_word(u32 ofs)
{
  return fetch_data_word_abs(get_data_segment(), ofs);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to load data from

RETURNS:
Long value read from the absolute memory location.
****************************************************************************/
u32 fetch_data_long(u32 ofs)
{
  return fetch_data_long_abs(get_data_segment(), ofs);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to load data from
offset	- Offset to load data from

RETURNS:
Byte value read from the absolute memory location.
****************************************************************************/
u8 fetch_data_byte_abs(sel_t *seg, u32 ofs)
{
  u8 val;
  u32 addr;
  char **p = &M.log.ptr;

  check_data_access(seg, ofs, 1);

  val = (*M.mem.rdb)(addr = seg->base + ofs);

  if(!M.log.data || !p || !LOG_SPACE) return val;

  LOG_STR("* r [");
  decode_hex8(p, addr);
  LOG_STR("] = ");
  decode_hex2(p, val);

  LOG_STR("\n");
  **p = 0;

  return val;
}

/****************************************************************************
PARAMETERS:
segment	- Segment to load data from
offset	- Offset to load data from

RETURNS:
Word value read from the absolute memory location.
****************************************************************************/
u16 fetch_data_word_abs(sel_t *seg, u32 ofs)
{
  u16 val;
  u32 addr;
  char **p = &M.log.ptr;

  check_data_access(seg, ofs, 2);

  val = (*M.mem.rdw)(addr = seg->base + ofs);

  if(!M.log.data || !p || !LOG_SPACE) return val;

  LOG_STR("* r [");
  decode_hex8(p, addr);
  LOG_STR("] = ");
  decode_hex4(p, val);

  LOG_STR("\n");
  **p = 0;

  return val;
}

/****************************************************************************
PARAMETERS:
segment	- Segment to load data from
offset	- Offset to load data from

RETURNS:
Long value read from the absolute memory location.
****************************************************************************/
u32 fetch_data_long_abs(sel_t *seg, u32 ofs)
{
  u32 val, addr;
  char **p = &M.log.ptr;

  check_data_access(seg, ofs, 4);

  val = (*M.mem.rdl)(addr = seg->base + ofs);

  if(!M.log.data || !p || !LOG_SPACE) return val;

  LOG_STR("* r [");
  decode_hex8(p, addr);
  LOG_STR("] = ");
  decode_hex8(p, val);

  LOG_STR("\n");
  **p = 0;

  return val;
}

/****************************************************************************
PARAMETERS:
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a word value to an segmented memory location. The segment used is
the current 'default' segment, which may have been overridden.
****************************************************************************/
void store_data_byte(u32 ofs, u8 val)
{
  store_data_byte_abs(get_data_segment(), ofs, val);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a word value to an segmented memory location. The segment used is
the current 'default' segment, which may have been overridden.
****************************************************************************/
void store_data_word(u32 ofs, u16 val)
{
  store_data_word_abs(get_data_segment(), ofs, val);
}

/****************************************************************************
PARAMETERS:
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a long value to an segmented memory location. The segment used is
the current 'default' segment, which may have been overridden.
****************************************************************************/
void store_data_long(u32 ofs, u32 val)
{
  store_data_long_abs(get_data_segment(), ofs, val);
}

/****************************************************************************
PARAMETERS:
segment	- Segment to store data at
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a byte value to an absolute memory location.
****************************************************************************/
void store_data_byte_abs(sel_t *seg, u32 ofs, u8 val)
{
  u32 addr;
  char **p = &M.log.ptr;

  check_data_access(seg, ofs, 1);

  (*M.mem.wrb)(addr = seg->base + ofs, val);

  if(!M.log.data || !p || !LOG_SPACE) return;

  LOG_STR("* w [");
  decode_hex8(p, addr);
  LOG_STR("] = ");
  decode_hex2(p, val);

  LOG_STR("\n");
  **p = 0;
}

/****************************************************************************
PARAMETERS:
segment	- Segment to store data at
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a word value to an absolute memory location.
****************************************************************************/
void store_data_word_abs(sel_t *seg, u32 ofs, u16 val)
{
  u32 addr;
  char **p = &M.log.ptr;

  check_data_access(seg, ofs, 2);

  (*M.mem.wrw)(addr = seg->base + ofs, val);

  if(!M.log.data || !p || !LOG_SPACE) return;

  LOG_STR("* w [");
  decode_hex8(p, addr);
  LOG_STR("] = ");
  decode_hex4(p, val);

  LOG_STR("\n");
  **p = 0;
}

/****************************************************************************
PARAMETERS:
segment	- Segment to store data at
offset	- Offset to store data at
val		- Value to store

REMARKS:
Writes a long value to an absolute memory location.
****************************************************************************/
void store_data_long_abs(sel_t *seg, u32 ofs, u32 val)
{
  u32 addr;
  char **p = &M.log.ptr;

  check_data_access(seg, ofs, 4);

  (*M.mem.wrl)(addr = seg->base + ofs, val);

  if(!M.log.data || !p || !LOG_SPACE) return;

  LOG_STR("* w [");
  decode_hex8(p, addr);
  LOG_STR("] = ");
  decode_hex8(p, val);

  LOG_STR("\n");
  **p = 0;
}


u8 fetch_io_byte(u32 port)
{
  char **p = &M.log.ptr;
  u8 val;

  if(!M.io.inb) return 0xff;

  val = (*M.io.inb)(port);

  if(!M.log.io || !p || !LOG_SPACE) return val;

  LOG_STR("* in [");
  decode_hex4(p, port);
  LOG_STR("] = ");
  decode_hex2(p, val);

  LOG_STR("\n");
  **p = 0;

  return val;
}


u16 fetch_io_word(u32 port)
{
  char **p = &M.log.ptr;
  u16 val;

  if(!M.log.io || !M.io.inw) return 0xffff;

  val = (*M.io.inw)(port);

  if(!p || !LOG_SPACE) return val;

  LOG_STR("* in [");
  decode_hex4(p, port);
  LOG_STR("] = ");
  decode_hex4(p, val);

  LOG_STR("\n");
  **p = 0;

  return val;
}


u32 fetch_io_long(u32 port)
{
  char **p = &M.log.ptr;
  u32 val;

  if(!M.io.inl) return 0xffffffff;

  val = (*M.io.inl)(port);

  if(!M.log.io || !p || !LOG_SPACE) return val;

  LOG_STR("* in [");
  decode_hex4(p, port);
  LOG_STR("] = ");
  decode_hex8(p, val);

  LOG_STR("\n");
  **p = 0;

  return val;
}


void store_io_byte(u32 port, u8 val)
{
  char **p = &M.log.ptr;

  if(!M.io.outb) return;

  (*M.io.outb)(port, val);

  if(!M.log.io || !p || !LOG_SPACE) return;

  LOG_STR("* out [");
  decode_hex4(p, port);
  LOG_STR("] = ");
  decode_hex2(p, val);

  LOG_STR("\n");
  **p = 0;
}


void store_io_word(u32 port, u16 val)
{
  char **p = &M.log.ptr;

  if(!M.io.outw) return;

  (*M.io.outw)(port, val);

  if(!M.log.io || !p || !LOG_SPACE) return;

  LOG_STR("* out [");
  decode_hex4(p, port);
  LOG_STR("] = ");
  decode_hex4(p, val);

  LOG_STR("\n");
  **p = 0;
}


void store_io_long(u32 port, u32 val)
{
  char **p = &M.log.ptr;

  if(!M.io.outl) return;

  (*M.io.outl)(port, val);

  if(!M.log.io || !p || !LOG_SPACE) return;

  LOG_STR("* out [");
  decode_hex4(p, port);
  LOG_STR("] = ");
  decode_hex8(p, val);

  LOG_STR("\n");
  **p = 0;
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
u8* decode_rm_byte_register(int reg)
{
  switch(reg) {
    case 0:
      OP_DECODE("al");
      return &M.x86.R_AL;

    case 1:
      OP_DECODE("cl");
      return &M.x86.R_CL;

    case 2:
      OP_DECODE("dl");
      return &M.x86.R_DL;

    case 3:
      OP_DECODE("bl");
      return &M.x86.R_BL;

    case 4:
      OP_DECODE("ah");
      return &M.x86.R_AH;

    case 5:
      OP_DECODE("ch");
      return &M.x86.R_CH;

    case 6:
      OP_DECODE("dh");
      return &M.x86.R_DH;

    case 7:
      OP_DECODE("bh");
      return &M.x86.R_BH;
  }

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
      INTR_RAISE_UD;
      reg = 6;
      break;
  }

  return M.x86.seg + reg;
}


void decode_hex(char **p, u32 ofs)
{
  static const char *h = "0123456789abcdef";

  if(ofs) {
    while(!(ofs & 0xf0000000)) ofs <<= 4;
    for(; ofs & 0xf0000000; ofs <<= 4) {
      *(*p)++ = h[(ofs >> 28) & 0xf];
    }
  }
  else {
    *(*p)++ = '0';
  }
}


void decode_hex1(char **p, u32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = *p;

  (*p)++;

  *s = h[ofs & 0xf];
}


void decode_hex2(char **p, u32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = *p;

  *p += 2;

  s[1] = h[ofs & 0xf];
  ofs >>= 4;
  s[0] = h[ofs & 0xf];
}


void decode_hex4(char **p, u32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = *p;

  *p += 4;

  s[3] = h[ofs & 0xf];
  ofs >>= 4;
  s[2] = h[ofs & 0xf];
  ofs >>= 4;
  s[1] = h[ofs & 0xf];
  ofs >>= 4;
  s[0] = h[ofs & 0xf];
}


void decode_hex8(char **p, u32 ofs)
{
  decode_hex4(p, ofs >> 16);
  decode_hex4(p, ofs & 0xffff);
}


void decode_hex_addr(char **p, u32 ofs)
{
  if(MODE_CODE32) {
    decode_hex4(p, ofs >> 16);
    decode_hex4(p, ofs & 0xffff);
  }
  else {
    decode_hex4(p, ofs & 0xffff);
  }
}


void decode_hex2s(char **p, s32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = *p;

  *p += 3;

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


void decode_hex4s(char **p, s32 ofs)
{
  static const char *h = "0123456789abcdef";
  char *s = *p;

  *p += 5;

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


void decode_hex8s(char **p, s32 ofs)
{
  if(ofs >= 0) {
    *(*p)++ = '+';
  }
  else {
    *(*p)++ = '-';
    ofs = -ofs;
  }

  decode_hex8(p, ofs);
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
		_MODE_SEG_DS_SS will be zero.  After every instruction
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
      INTR_RAISE_UD;
      break;
  }

  return 0;
}


u32 decode_rm00_address(int rm)
{
  u32 offset, base;
  int sib;

  if(MODE_ADDR32) {
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
        sib = fetch_byte();
        base = decode_sib_address(sib, 0);
        OP_DECODE("]");
        return base;

      case 5:
        offset = fetch_long();
        SEGPREF_DECODE;
        DECODE_HEX8(offset);
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
        M.x86.mode |= _MODE_SEG_DS_SS;
        return (M.x86.R_BP + M.x86.R_SI) & 0xffff;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("bp+di]");
        M.x86.mode |= _MODE_SEG_DS_SS;
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
        offset = fetch_word();
        SEGPREF_DECODE;
        DECODE_HEX4(offset);
        OP_DECODE("]");
        return offset;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("bx]");
        return M.x86.R_BX;
      }
  }

  return 0;
}


u32 decode_rm01_address(int rm)
{
  s32 displacement = 0;
  u32 base;
  int sib;

  /* Fetch disp8 if no SIB byte */
  if(!(MODE_ADDR32 && (rm == 4))) {
    displacement = (s8) fetch_byte();
  }

  if(MODE_ADDR32) {
    /* 32-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("eax");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return M.x86.R_EAX + displacement;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("ecx");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return M.x86.R_ECX + displacement;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("edx");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return M.x86.R_EDX + displacement;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("ebx");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return M.x86.R_EBX + displacement;

      case 4:
        sib = fetch_byte();
        base = decode_sib_address(sib, 1);
        displacement = (s8) fetch_byte();
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return base + displacement;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("ebp");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return M.x86.R_EBP + displacement;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("esi");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return M.x86.R_ESI + displacement;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("edi");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return M.x86.R_EDI + displacement;
    }
  }
  else {
    /* 16-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("bx+si");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return (M.x86.R_BX + M.x86.R_SI + displacement) & 0xffff;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("bx+di");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return (M.x86.R_BX + M.x86.R_DI + displacement) & 0xffff;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("bp+si");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        M.x86.mode |= _MODE_SEG_DS_SS;
        return (M.x86.R_BP + M.x86.R_SI + displacement) & 0xffff;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("bp+di");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        M.x86.mode |= _MODE_SEG_DS_SS;
        return (M.x86.R_BP + M.x86.R_DI + displacement) & 0xffff;

      case 4:
        SEGPREF_DECODE;
        OP_DECODE("si");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return (M.x86.R_SI + displacement) & 0xffff;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("di");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return (M.x86.R_DI + displacement) & 0xffff;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("bp");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        M.x86.mode |= _MODE_SEG_DS_SS;
        return (M.x86.R_BP + displacement) & 0xffff;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("bx");
        DECODE_HEX2S(displacement);
        OP_DECODE("]");
        return (M.x86.R_BX + displacement) & 0xffff;
      }
  }

  return 0;
}


u32 decode_rm10_address(int rm)
{
  s32 displacement = 0;
  u32 base;
  int sib;

  /* Fetch disp16 if 16-bit addr mode */
  if(!MODE_ADDR32) {
    displacement = (s16) fetch_word();
  }
  else {
    /* Fetch disp32 if no SIB byte */
    if(rm != 4) displacement = (s32) fetch_long();
  }

  if(MODE_ADDR32) {
    /* 32-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("eax");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return M.x86.R_EAX + displacement;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("ecx");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return M.x86.R_ECX + displacement;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("edx");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return M.x86.R_EDX + displacement;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("ebx");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return M.x86.R_EBX + displacement;

      case 4:
        sib = fetch_byte();
        base = decode_sib_address(sib, 2);
        displacement = (s32) fetch_long();
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return base + displacement;
        break;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("ebp");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        M.x86.mode |= _MODE_SEG_DS_SS;
        return M.x86.R_EBP + displacement;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("esi");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return M.x86.R_ESI + displacement;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("edi");
        DECODE_HEX8S(displacement);
        OP_DECODE("]");
        return M.x86.R_EDI + displacement;
    }
  }
  else {
    /* 16-bit addressing */
    switch(rm) {
      case 0:
        SEGPREF_DECODE;
        OP_DECODE("bx+si");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        return (M.x86.R_BX + M.x86.R_SI + displacement) & 0xffff;

      case 1:
        SEGPREF_DECODE;
        OP_DECODE("bx+di");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        return (M.x86.R_BX + M.x86.R_DI + displacement) & 0xffff;

      case 2:
        SEGPREF_DECODE;
        OP_DECODE("bp+si");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        M.x86.mode |= _MODE_SEG_DS_SS;
        return (M.x86.R_BP + M.x86.R_SI + displacement) & 0xffff;

      case 3:
        SEGPREF_DECODE;
        OP_DECODE("bp+di");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        M.x86.mode |= _MODE_SEG_DS_SS;
        return (M.x86.R_BP + M.x86.R_DI + displacement) & 0xffff;

      case 4:
        SEGPREF_DECODE;
        OP_DECODE("si");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        return (M.x86.R_SI + displacement) & 0xffff;

      case 5:
        SEGPREF_DECODE;
        OP_DECODE("di");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        return (M.x86.R_DI + displacement) & 0xffff;

      case 6:
        SEGPREF_DECODE;
        OP_DECODE("bp");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        M.x86.mode |= _MODE_SEG_DS_SS;
        return (M.x86.R_BP + displacement) & 0xffff;

      case 7:
        SEGPREF_DECODE;
        OP_DECODE("bx");
        DECODE_HEX4S(displacement);
        OP_DECODE("]");
        return (M.x86.R_BX + displacement) & 0xffff;
    }
  }

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
      M.x86.mode |= _MODE_SEG_DS_SS;
      break;

    case 5:
      SEGPREF_DECODE;
      if(mod == 0) {
        base = fetch_long();
        DECODE_HEX8(base);
      }
      else {
        OP_DECODE("ebp");
        base = M.x86.R_EBP;
        M.x86.mode |= _MODE_SEG_DS_SS;
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


void x86emu_reset(x86emu_t *emu)
{
  x86emu_regs_t *x86 = &emu->x86;

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


void log_code()
{
  unsigned u;
  char **p = &M.log.ptr;

  if(!M.log.code || !p || !LOG_SPACE) return;

  decode_hex(p, M.x86.tsc);
  LOG_STR(" ");
  decode_hex4(p, M.x86.saved_cs);
  LOG_STR(":");
  MODE_CODE32 ? decode_hex8(p, M.x86.saved_eip) : decode_hex4(p, M.x86.saved_eip);
  LOG_STR(" ");

  for(u = 0; u < M.x86.instr_len; u++) {
    decode_hex2(p, M.x86.instr_buf[u]);
  }

  while(u++ < 12) {
    LOG_STR("  ");
  }

  LOG_STR(" ");

  u = M.x86.disasm_ptr - M.x86.disasm_buf;
  memcpy(*p, M.x86.disasm_buf, u);
  *p += u;

  LOG_STR("\n");

  **p = 0;
}


void log_regs()
{
  char **p = &M.log.ptr;

  if(!M.log.regs || !p || !LOG_SPACE) return;

  LOG_STR("\neax ");
  decode_hex8(p, M.x86.R_EAX);
  LOG_STR(", ebx ");
  decode_hex8(p, M.x86.R_EBX);
  LOG_STR(", ecx ");
  decode_hex8(p, M.x86.R_ECX);
  LOG_STR(", edx ");
  decode_hex8(p, M.x86.R_EDX);

  LOG_STR("\nesi ");
  decode_hex8(p, M.x86.R_ESI);
  LOG_STR(", edi ");
  decode_hex8(p, M.x86.R_EDI);
  LOG_STR(", ebp ");
  decode_hex8(p, M.x86.R_EBP);
  LOG_STR(", esp ");
  decode_hex8(p, M.x86.R_ESP);

  LOG_STR("\ncs ");
  decode_hex4(p, M.x86.R_CS);
  LOG_STR(", ss ");
  decode_hex4(p, M.x86.R_SS);
  LOG_STR(", ds ");
  decode_hex4(p, M.x86.R_DS);
  LOG_STR(", es ");
  decode_hex4(p, M.x86.R_ES);
  LOG_STR(", fs ");
  decode_hex4(p, M.x86.R_FS);
  LOG_STR(", gs ");
  decode_hex4(p, M.x86.R_GS);

  LOG_STR("\neip ");
  decode_hex8(p, M.x86.R_EIP);
  LOG_STR(", eflags ");
  decode_hex8(p, M.x86.R_EFLG);

  if(ACCESS_FLAG(F_OF)) LOG_STR(" of");
  if(ACCESS_FLAG(F_DF)) LOG_STR(" df");
  if(ACCESS_FLAG(F_IF)) LOG_STR(" if");
  if(ACCESS_FLAG(F_SF)) LOG_STR(" sf");
  if(ACCESS_FLAG(F_ZF)) LOG_STR(" zf");
  if(ACCESS_FLAG(F_AF)) LOG_STR(" af");
  if(ACCESS_FLAG(F_PF)) LOG_STR(" pf");
  if(ACCESS_FLAG(F_CF)) LOG_STR(" cf");

  LOG_STR("\n");

  **p = 0;
}


void check_data_access(sel_t *seg, u32 ofs, u32 size)
{
  char **p = &M.log.ptr;
  static char seg_name[7] = "ecsdfg?";
  unsigned idx = seg - M.x86.seg;

  if(M.log.acc && p && LOG_SPACE) {
    LOG_STR("* acc ");
    switch(size) {
      case 1:
        LOG_STR("byte ");
        break;
      case 2:
        LOG_STR("word ");
        break;
      case 4:
        LOG_STR("dword ");
        break;
    }
    if(idx > 6) idx = 6;
    *(*p)++ = seg_name[idx];
    LOG_STR("s:");
    decode_hex8(p, ofs);
    LOG_STR("\n");

    **p = 0;
  }

  if(ofs + size - 1 > seg->limit) {
    INTR_RAISE_GP(seg->sel);
  }

  return;
}


void decode_set_seg_register(sel_t *seg, u16 val)
{
  int err = 1;
  unsigned ofs, acc;
  u32 dl, dh, base, limit;
  u32 dt_base, dt_limit;

  if(MODE_REAL) {
    seg->sel = val;
    seg->base = val << 4;

    err = 0;
  }
  else {
    ofs = val & ~7;

    if(val & 4) {
      dt_base = M.x86.R_LDT_BASE;
      dt_limit = M.x86.R_LDT_BASE;
    }
    else {
      dt_base = M.x86.R_GDT_BASE;
      dt_limit = M.x86.R_GDT_BASE;
    }

    if(ofs == 0) {
      seg->sel = 0;
      seg->base = 0;
      seg->limit = 0;
      seg->acc = 0;

      err = 0;
    }
    else if(ofs + 7 <= dt_limit) {
      dl = (*M.mem.rdl)(dt_base + ofs);
      dh = (*M.mem.rdl)(dt_base + ofs + 4);

      acc = ((dh >> 8) & 0xff) + ((dh >> 12) & 0xf00);
      base = ((dl >> 16) & 0xffff) + ((dh & 0xff) << 16) + (dh & 0xff000000);
      limit = (dl & 0xffff) + (dh & 0xf0000);
      if(ACC_G(acc)) limit = (limit << 12) + 0xfff;

      if(ACC_P(acc) && ACC_S(acc)) {
        seg->sel = val;
        seg->base = base;
        seg->limit = limit;
        seg->acc = acc;

        err = 0;
      }
    }
  }

  if(err) INTR_RAISE_GP(val);
}


void idt_lookup(u8 nr, u32 *new_cs, u32 *new_eip)
{
  if(MODE_REAL) {
    *new_eip = (*M.mem.rdw)(M.x86.R_IDT_BASE + nr * 4);
    *new_cs = (*M.mem.rdw)(M.x86.R_IDT_BASE + nr * 4 + 2);
  }
  else {
  }
}


void generate_int(u8 nr, unsigned type, unsigned errcode)
{
  u32 cs, eip, new_cs, new_eip;
  int i;

  i = M.intr_table[nr] ? (*M.intr_table[nr])(nr, type) : 0;

  if(!i) {
    if(type & INTR_MODE_RESTART) {
      eip = M.x86.saved_eip;
      cs = M.x86.saved_cs;
    }
    else {
      eip = M.x86.R_EIP;
      cs = M.x86.R_CS;
    }

    new_cs = cs;
    new_eip = eip;

    idt_lookup(nr, &new_cs, &new_eip);

    if(MODE_PROTECTED && MODE_CODE32) {
      push_long(M.x86.R_EFLG);
      push_long(cs);
      push_long(eip);
    }
    else {
      push_word(M.x86.R_FLG);
      push_word(cs);
      push_word(eip);
    }

    if(type & INTR_MODE_ERRCODE) push_long(errcode);

    CLEAR_FLAG(F_IF);
    CLEAR_FLAG(F_TF);

    decode_set_seg_register(M.x86.seg + R_CS_INDEX, new_cs);
    M.x86.R_EIP = new_eip;
  }
}

#undef LOG_STR

