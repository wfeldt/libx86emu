#include "include/x86emu_int.h"


#define LOG_STR(a) memcpy(*p, a, sizeof a - 1), *p += sizeof a - 1
#define LOG_FREE(emu) (emu->log.size + emu->log.buf - emu->log.ptr)

#define LINE_LEN 16


void x86emu_set_memio_func(x86emu_t *emu, x86emu_memio_func_t func)
{
  if(emu) {
    emu->memio = func;
  }
}


void x86emu_set_intr_func(x86emu_t *emu, unsigned num, x86emu_intr_func_t handler)
{
  if(emu && num < sizeof emu->intr_table / sizeof *emu->intr_table) emu->intr_table[num] = handler;
}


void x86emu_set_code_check(x86emu_t *emu, x86emu_code_check_t func)
{
  if(emu) {
    emu->code_check = func;
  }
}


void x86emu_set_io_perm(x86emu_t *emu, unsigned start, unsigned len, unsigned perm)
{
  if(!emu) return;

  for(; len && start < sizeof emu->io.map / sizeof *emu->io.map; start++, len--) {
    emu->io.map[start] = perm;
  }

  for(start = perm = 0; start < sizeof emu->io.map / sizeof *emu->io.map; start++) {
    perm |= emu->io.map[start];
  }

  emu->io.iopl_needed = (perm & (X86EMU_PERM_R | X86EMU_PERM_W)) ? 1 : 0;

#if WITH_IOPL 
  emu->io.iopl_ok = emu->io.iopl_needed && getiopl() != 3 ? 0 : 1;
#else 
  emu->io.iopl_ok = 1;
#endif
}


void x86emu_set_log(x86emu_t *emu, unsigned buffer_size, x86emu_flush_func_t flush)
{
  if(emu) {
    if(emu->log.buf) free(emu->log.buf);
    emu->log.size = buffer_size;
    emu->log.buf = buffer_size ? calloc(1, buffer_size) : NULL;
    emu->log.ptr = emu->log.buf;
    emu->log.flush = flush;
  }
}


unsigned x86emu_clear_log(x86emu_t *emu, int flush)
{
  if(!emu) emu = &x86emu;

  if(flush && emu->log.flush) {
    if(emu->log.ptr && emu->log.ptr != emu->log.buf) {
      emu->log.flush(emu->log.buf, emu->log.ptr - emu->log.buf);
    }
  }
  if((emu->log.ptr = emu->log.buf)) *emu->log.ptr = 0;

  return emu->log.ptr ? LOG_FREE(emu) : 0;
}


void x86emu_log(x86emu_t *emu, const char *format, ...)
{
  va_list args;
  int size;

  if(!emu || !emu->log.ptr) return;

  size = emu->log.size - (emu->log.ptr - emu->log.buf);

  va_start(args, format);
  if(size > 0) {
    size = vsnprintf(emu->log.ptr, size, format, args);
    if(size > 0) {
      emu->log.ptr += size;
    }
    else {
      *emu->log.ptr = 0;
    }
  }
  va_end(args);  
}


x86emu_t *x86emu_new(unsigned def_mem_perm, unsigned def_io_perm)
{
  x86emu_t *emu = calloc(1, sizeof *emu);

  emu->mem = x86emu_mem_new(def_mem_perm);

  if(def_io_perm) x86emu_set_io_perm(emu, 0, 1 << 16, def_io_perm);

  x86emu_set_memio_func(emu, vm_memio);

  x86emu_reset(emu);

  return emu;
}


void x86emu_done(x86emu_t *emu)
{
  if(emu) {
    if(emu->log.buf) free(emu->log.buf);

    free(emu);
  }
}


static void dump_data(unsigned char *data, unsigned char *attr, char *str_data, char *str_attr)
{
  unsigned u;
  char c;
  int ok = 0;
  char *sd = str_data, *sa = str_attr;

  for(u = 0; u < LINE_LEN; u++) {
    *str_data++ = (attr[u] & X86EMU_ACC_INVALID) ? '*' : ' ';
    if((attr[u] & X86EMU_ACC_W)) {
      ok = 1;
      decode_hex2(&str_data, data[u]);

      c = (attr[u] & X86EMU_PERM_R) ? (attr[u] & X86EMU_ACC_R) ? 'R' : 'r' : ' ';
      *str_attr++ = c;
      c = (attr[u] & X86EMU_PERM_W) ? (attr[u] & X86EMU_ACC_W) ? 'W' : 'w' : ' ';
      *str_attr++ = c;
      c = (attr[u] & X86EMU_PERM_X) ? (attr[u] & X86EMU_ACC_X) ? 'X' : 'x' : ' ';
      *str_attr++ = c;
    }
    else {
      *str_data++ = ' ';
      *str_data++ = ' ';

      *str_attr++ = ' ';
      *str_attr++ = ' ';
      *str_attr++ = ' ';
    }
    *str_data++ = ' ';
    *str_attr++ = ' ';
  }

  if(!ok) {
    str_data = sd;
    str_attr = sa;
  }

  *str_data = *str_attr = 0;

  while(str_data > sd && str_data[-1] == ' ') *--str_data = 0;
  while(str_attr > sa && str_attr[-1] == ' ') *--str_attr = 0;
}


void x86emu_dump(x86emu_t *emu, int flags)
{
  x86emu_mem_t *mem = emu->mem;
  mem2_pdir_t *pdir;
  mem2_ptable_t *ptable;
  mem2_page_t page;
  unsigned pdir_idx, u, u1, u2, addr;
  char str_data[LINE_LEN * 8], str_attr[LINE_LEN * 8], fbuf[64];
  unsigned char def_data[LINE_LEN], def_attr[LINE_LEN];

  if(mem && mem->pdir && (flags & (X86EMU_DUMP_MEM | X86EMU_DUMP_ATTR))) {
    x86emu_log(emu, ";        ");
    for(u1 = 0; u1 < 16; u1++) x86emu_log(emu, "%4x", u1);
    x86emu_log(emu, "\n");

    pdir = mem->pdir;
    for(pdir_idx = 0; pdir_idx < (1 << X86EMU_PDIR_BITS); pdir_idx++) {
      ptable = (*pdir)[pdir_idx];
      if(!ptable) continue;
      for(u1 = 0; u1 < (1 << X86EMU_PTABLE_BITS); u1++) {
        page = (*ptable)[u1];
        if(page.data) {
          for(u2 = 0; u2 < (1 << X86EMU_PAGE_BITS); u2 += LINE_LEN) {
            memcpy(def_data, page.data + u2, LINE_LEN);
            if(page.attr) {
              memcpy(def_attr, page.attr + u2, LINE_LEN);
            }
            else {
              memset(def_attr, page.def_attr, LINE_LEN);
            }
            dump_data(def_data, def_attr, str_data, str_attr);
            if(*str_data) {
              addr = (((pdir_idx << X86EMU_PTABLE_BITS) + u1) << X86EMU_PAGE_BITS) + u2;
              x86emu_log(emu, "%08x: %s\n", addr, str_data);
              if((flags & X86EMU_DUMP_ATTR)) x86emu_log(emu, "          %s\n", str_attr);
            }
          }
        }
      }
    }

    x86emu_log(emu, "\n");
  }

  if((flags & X86EMU_DUMP_IO)) {
    x86emu_log(emu, "; - - io accesses\n");

    for(u = 0; u < sizeof emu->io.map / sizeof *emu->io.map; u++) {
      if(emu->io.map[u] & (X86EMU_ACC_R | X86EMU_ACC_W | X86EMU_ACC_INVALID)) {
        x86emu_log(emu,
          "%04x: %c%c%c in=%08x out=%08x\n",
          u,
          (emu->io.map[u] & X86EMU_ACC_INVALID) ? '*' : ' ',
          (emu->io.map[u] & X86EMU_PERM_R) ? 'r' : ' ',
          (emu->io.map[u] & X86EMU_PERM_W) ? 'w' : ' ',
          emu->io.stats_i[u], emu->io.stats_o[u]
        );
      }
    }

    x86emu_log(emu, "\n");
  }

  if((flags & X86EMU_DUMP_REGS)) {
    x86emu_log(emu, "cr0=%08x cr1=%08x cr2=%08x cr3=%08x cr4=%08x\n",
      emu->x86.R_CR0, emu->x86.R_CR1, emu->x86.R_CR2, emu->x86.R_CR3, emu->x86.R_CR4
    );

    x86emu_log(emu, "dr0=%08x dr1=%08x dr2=%08x dr3=%08x dr6=%08x dr7=%08x\n\n",
      emu->x86.R_DR0, emu->x86.R_DR1, emu->x86.R_DR2, emu->x86.R_DR3,
      emu->x86.R_DR6, emu->x86.R_DR7
    );

    x86emu_log(emu,
      "gdt.base=%08x gdt.limit=%04x\n",
      emu->x86.R_GDT_BASE, emu->x86.R_GDT_LIMIT
    );

    x86emu_log(emu,
      "idt.base=%08x idt.limit=%04x\n",
      emu->x86.R_IDT_BASE, emu->x86.R_IDT_LIMIT
    );

    x86emu_log(emu,
      "tr=%04x tr.base=%08x tr.limit=%08x tr.acc=%04x\n",
      emu->x86.R_TR, emu->x86.R_TR_BASE, emu->x86.R_TR_LIMIT, emu->x86.R_TR_ACC
    );
    x86emu_log(emu,
      "ldt=%04x ldt.base=%08x ldt.limit=%08x ldt.acc=%04x\n\n",
      emu->x86.R_LDT, emu->x86.R_LDT_BASE, emu->x86.R_LDT_LIMIT, emu->x86.R_LDT_ACC
    );

    x86emu_log(emu,
      "cs=%04x cs.base=%08x cs.limit=%08x cs.acc=%04x\n",
      emu->x86.R_CS, emu->x86.R_CS_BASE, emu->x86.R_CS_LIMIT, emu->x86.R_CS_ACC
    );
    x86emu_log(emu,
      "ss=%04x ss.base=%08x ss.limit=%08x ss.acc=%04x\n",
      emu->x86.R_SS, emu->x86.R_SS_BASE, emu->x86.R_SS_LIMIT, emu->x86.R_SS_ACC
    );
    x86emu_log(emu,
      "ds=%04x ds.base=%08x ds.limit=%08x ds.acc=%04x\n",
      emu->x86.R_DS, emu->x86.R_DS_BASE, emu->x86.R_DS_LIMIT, emu->x86.R_DS_ACC
    );
    x86emu_log(emu,
      "es=%04x es.base=%08x es.limit=%08x es.acc=%04x\n",
      emu->x86.R_ES, emu->x86.R_ES_BASE, emu->x86.R_ES_LIMIT, emu->x86.R_ES_ACC
    );
    x86emu_log(emu,
      "fs=%04x fs.base=%08x fs.limit=%08x fs.acc=%04x\n",
      emu->x86.R_FS, emu->x86.R_FS_BASE, emu->x86.R_FS_LIMIT, emu->x86.R_FS_ACC
    );
    x86emu_log(emu,
      "gs=%04x gs.base=%08x gs.limit=%08x gs.acc=%04x\n\n",
      emu->x86.R_GS, emu->x86.R_GS_BASE, emu->x86.R_GS_LIMIT, emu->x86.R_GS_ACC
    );
    x86emu_log(emu, "eax=%08x ebx=%08x ecx=%08x edx=%08x\n",
      emu->x86.R_EAX, emu->x86.R_EBX, emu->x86.R_ECX, emu->x86.R_EDX
    );
    x86emu_log(emu, "esi=%08x edi=%08x ebp=%08x esp=%08x\n",
      emu->x86.R_ESI, emu->x86.R_EDI, emu->x86.R_EBP, emu->x86.R_ESP
    );
    x86emu_log(emu, "eip=%08x eflags=%08x", emu->x86.R_EIP, emu->x86.R_EFLG);

    *fbuf = 0;
    if(emu->x86.R_EFLG & 0x800) strcat(fbuf, " of");
    if(emu->x86.R_EFLG & 0x400) strcat(fbuf, " df");
    if(emu->x86.R_EFLG & 0x200) strcat(fbuf, " if");
    if(emu->x86.R_EFLG & 0x080) strcat(fbuf, " sf");
    if(emu->x86.R_EFLG & 0x040) strcat(fbuf, " zf");
    if(emu->x86.R_EFLG & 0x010) strcat(fbuf, " af");
    if(emu->x86.R_EFLG & 0x004) strcat(fbuf, " pf");
    if(emu->x86.R_EFLG & 0x001) strcat(fbuf, " cf");

    if(*fbuf) x86emu_log(emu, " ;%s", fbuf);

    x86emu_log(emu, "\n\n");
  }

  if((flags & X86EMU_DUMP_INTS)) {
    x86emu_log(emu, "; - - interrupt statistics\n");
    for(u1 = 0; u1 < 0x100; u1++) {
      if(emu->x86.intr_stats[u1]) x86emu_log(emu, "int %02x: %08x\n", u1, emu->x86.intr_stats[u1]);
    }

    x86emu_log(emu, "\n");
  }
}


#undef LINE_LEN

