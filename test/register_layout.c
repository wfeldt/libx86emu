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
/* Run on both little- and big-endian hosts; no guest code is executed. */
#include <x86emu.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
  x86emu_regs_t r;
  unsigned failed = 0;
  x86emu_t *emu;
  memset(&r, 0, sizeof r);
  r.R_EAX = 0x12345678;
  if(r.R_AX != 0x5678 || r.R_AH != 0x56 || r.R_AL != 0x78) failed++;
  r.R_AL = 0xab;
  if(r.R_EAX != 0x123456ab) failed++;
  r.R_AH = 0xcd;
  if(r.R_EAX != 0x1234cdab) failed++;
  r.R_AX = 0xef01;
  if(r.R_EAX != 0x1234ef01) failed++;
  emu = x86emu_new(X86EMU_PERM_RW, 0);
  if(!emu) return 2;
  x86emu_write_dword(emu, 0x1000, 0x12345678);
  if(x86emu_read_byte(emu, 0x1000) != 0x78 ||
     x86emu_read_byte(emu, 0x1001) != 0x56 ||
     x86emu_read_byte(emu, 0x1002) != 0x34 ||
     x86emu_read_byte(emu, 0x1003) != 0x12) failed++;
  x86emu_write_byte(emu, 0x1004, 0x21);
  x86emu_write_byte(emu, 0x1005, 0x43);
  x86emu_write_byte(emu, 0x1006, 0x65);
  x86emu_write_byte(emu, 0x1007, 0x07);
  if(x86emu_read_word(emu, 0x1004) != 0x4321) failed++;
  if(x86emu_read_dword(emu, 0x1004) != 0x07654321) failed++;
  x86emu_write_word(emu, 0x1008, 0x4321);
  if(x86emu_read_byte(emu, 0x1008) != 0x21 ||
     x86emu_read_byte(emu, 0x1009) != 0x43) failed++;
  x86emu_done(emu);
  printf("Register and memory layout: %u failures\n", failed);
  return !!failed;
}
