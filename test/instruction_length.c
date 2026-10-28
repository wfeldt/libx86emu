/* SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026, Stefan Reinauer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/* Run: cc -Iinclude test/instruction_length.c api.c mem.c decode.c ops.c
 *         ops2.c prim_ops.c -o /tmp/instruction_length && /tmp/instruction_length
 */
#include <x86emu.h>
#include <stdio.h>
#include <string.h>

static unsigned char code[64];
static unsigned fetched, stop_at;

static unsigned memio(x86emu_t *emu, u32 addr, u32 *value, unsigned type)
{
  unsigned i, width = 1U << (type & 255);
  *value = 0;
  if((type & ~255U) != X86EMU_MEMIO_X) return 0;
  if(addr < 0x1000 || addr + width > 0x1000 + sizeof code) {
    x86emu_stop(emu);
    return 1;
  }
  for(i = 0; i < width; i++) *value |= (u32) code[addr - 0x1000 + i] << (8 * i);
  fetched += width;
  if(stop_at && fetched >= stop_at) x86emu_stop(emu);
  return 0;
}

static int interrupt(x86emu_t *emu, u8 number, unsigned type)
{
  (void) number;
  (void) type;
  x86emu_stop(emu);
  return 1;
}

static unsigned run(unsigned prefixes, unsigned immediate, unsigned stop, unsigned expected)
{
  x86emu_t *emu = x86emu_new(0, 0);
  unsigned failed;
  if(!emu) return 1;
  memset(code, 0, sizeof code);
  memset(code, 0xf3, prefixes);
  if(immediate == 4) code[0] = 0x66;
  code[prefixes] = !immediate ? 0xf4 : immediate == 1 ? 0xb0 : 0xb8;
  x86emu_set_seg_register(emu, emu->x86.seg + R_CS_INDEX, 0);
  emu->x86.R_EIP = 0x1000;
  emu->max_instr = 1;
  x86emu_set_memio_handler(emu, memio);
  x86emu_set_intr_handler(emu, interrupt);
  fetched = 0;
  stop_at = stop;
  x86emu_run(emu, X86EMU_RUN_MAX_INSTR);
  failed = fetched != expected;
  printf("prefixes=%u immediate=%u callback-stop=%u fetched=%u expected=%u %s\n",
    prefixes, immediate, stop, fetched, expected, failed ? "FAIL" : "PASS");
  x86emu_done(emu);
  return failed;
}

int main(void)
{
  unsigned failed = 0;
  failed += run(14, 0, 0, 15); /* longest valid prefix/opcode sequence */
  failed += run(32, 0, 0, 15); /* excessive prefixes */
  failed += run(32, 0, 2, 2);  /* callback stop on a prefix */
  failed += run(13, 1, 0, 15); /* immediate byte exactly fits */
  failed += run(14, 1, 0, 15); /* immediate byte exceeds the limit */
  failed += run(12, 2, 0, 15); /* immediate word exactly fits */
  failed += run(13, 2, 0, 14); /* reject the whole word fetch */
  failed += run(10, 4, 0, 15); /* includes one operand-size prefix */
  failed += run(11, 4, 0, 12); /* reject the whole dword fetch */
  printf("Instruction length: 9 cases, %u failures\n", failed);
  return !!failed;
}
