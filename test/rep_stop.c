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
/* Run: cc -Iinclude test/rep_stop.c api.c mem.c decode.c ops.c ops2.c
 *         prim_ops.c -o /tmp/rep_stop && /tmp/rep_stop
 */
#include <x86emu.h>
#include <stdio.h>
#include <string.h>

static unsigned char code[8];
static unsigned opcode, prefix, stop_requested, stop_enabled;
static unsigned reads, writes, inputs, outputs;

static unsigned memio(x86emu_t *emu, u32 addr, u32 *value, unsigned type)
{
  unsigned width = 1U << (type & 255), op = type & ~255U, i;
  *value = 0;
  if(op == X86EMU_MEMIO_X) {
    if(addr < 0x1000 || addr + width > 0x1000 + sizeof code) {
      x86emu_stop(emu);
      return 1;
    }
    for(i = 0; i < width; i++) *value |= (u32) code[addr - 0x1000 + i] << (8 * i);
    return 0;
  }
  if(op == X86EMU_MEMIO_R) {
    reads++;
    if(prefix == 0xf2 && addr >= 0x3000) *value = 1;
  }
  if(op == X86EMU_MEMIO_W) writes++;
  if(op == X86EMU_MEMIO_I) inputs++;
  if(op == X86EMU_MEMIO_O) outputs++;
  if(stop_enabled && !stop_requested &&
     ((opcode == 0xa4 || opcode == 0xaa || opcode == 0x6c) ? op == X86EMU_MEMIO_W :
      opcode == 0x6e ? op == X86EMU_MEMIO_O :
      opcode == 0xa6 ? op == X86EMU_MEMIO_R && reads == 2 : op == X86EMU_MEMIO_R)) {
    stop_requested = 1;
    x86emu_stop(emu);
  }
  return 0;
}

int main(void)
{
  const unsigned opcodes[] = {0xa4, 0xa6, 0xaa, 0xac, 0xae, 0x6c, 0x6e};
  unsigned op, size, addr32, repne, stop, n, count, failed = 0, cases = 0;
  for(op = 0; op < sizeof opcodes / sizeof *opcodes; op++)
  for(size = 0; size < 3; size++)
  for(addr32 = 0; addr32 < 2; addr32++)
  for(repne = 0; repne < 2; repne++)
  for(stop = 0; stop < 2; stop++) {
    x86emu_t *emu = x86emu_new(0, 0);
    if(!emu) return 2;
    n = 0;
    memset(code, 0xf4, sizeof code);
    if(addr32) code[n++] = 0x67;
    if(size == 2) code[n++] = 0x66;
    prefix = repne ? 0xf2 : 0xf3;
    code[n++] = prefix;
    opcode = opcodes[op];
    code[n] = opcode + (size != 0);
    reads = writes = inputs = outputs = stop_requested = 0;
    stop_enabled = stop;
    x86emu_set_seg_register(emu, emu->x86.seg + R_CS_INDEX, 0);
    emu->x86.R_EIP = 0x1000;
    emu->x86.R_ECX = 64;
    emu->x86.R_ESI = 0x2000;
    emu->x86.R_EDI = 0x3000;
    emu->x86.R_EAX = 0;
    x86emu_set_memio_handler(emu, memio);
    x86emu_run(emu, 0);
    count = stop ? 1 : 64;
    if(reads != count * (op == 1 ? 2 : op == 0 || op == 3 || op == 4 || op == 6) ||
       writes != count * (op == 0 || op == 2 || op == 5) ||
       inputs != count * (op == 5) || outputs != count * (op == 6) || stop_requested != stop) {
      printf("FAIL op=%02x size=%u addr32=%u repne=%u stop=%u R=%u W=%u I=%u O=%u\n",
        opcode, size, addr32, repne, stop, reads, writes, inputs, outputs);
      failed++;
    }
    cases++;
    x86emu_done(emu);
  }
  printf("REP stop: %u cases, %u failures\n", cases, failed);
  return !!failed;
}
