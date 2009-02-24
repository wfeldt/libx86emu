#include "include/x86emu_int.h"

x86emu_mem_t *x86emu_mem_new(void)
{
  x86emu_mem_t *mem;

  mem = calloc(1, sizeof *mem);
  mem->def_attr = MEM2_DEF_ATTR;

  return mem;
}


x86emu_mem_t *x86emu_mem_free(x86emu_mem_t *mem)
{
  if(mem) {


    free(mem);
  }

  return NULL;
}


mem2_page_t *vm_get_page(x86emu_mem_t *mem, unsigned addr)
{
  mem2_pdir_t *pdir;
  mem2_ptable_t *ptable;
  mem2_page_t page;
  unsigned pdir_idx = addr >> (32 - MEM2_PDIR_BITS);
  unsigned ptable_idx = (addr >> MEM2_PAGE_BITS) & ((1 << MEM2_PTABLE_BITS) - 1);
  unsigned u;

  pdir = mem->pdir;
  if(!pdir) {
    mem->pdir = pdir = calloc(1, sizeof *pdir);
    // fprintf(stderr, "pdir = %p (%d)\n", pdir, sizeof *pdir);
  }

  ptable = (*pdir)[pdir_idx];
  if(!ptable) {
    ptable = (*pdir)[pdir_idx] = calloc(1, sizeof *ptable);
    // fprintf(stderr, "ptable = %p\n", ptable);
    for(u = 0; u < (1 << MEM2_PTABLE_BITS); u++) {
      (*ptable)[u].def_attr = mem->def_attr;
      // fprintf(stderr, "ptable[%u] = %p\n", u, &((*ptable)[u].def_attr));
    }
    // fprintf(stderr, "pdir[%d] = %p (%d)\n", pdir_idx, ptable, sizeof *ptable);
  }

  page = (*ptable)[ptable_idx];
  if(!page.attr) {
    page.attr = calloc(1, 2 * MEM2_PAGE_SIZE);
    page.data = page.attr + MEM2_PAGE_SIZE;
    // fprintf(stderr, "page = %p, page.def_attr = %p\n", page, &page.def_attr);
    memset(page.attr, page.def_attr, MEM2_PAGE_SIZE);
    (*ptable)[ptable_idx] = page;
    // fprintf(stderr, "page.attr[%d] = %p\n", ptable_idx, page.attr);
  }

  return (*ptable) + ptable_idx;
}


unsigned vm_read_byte(x86emu_mem_t *mem, unsigned addr)
{
  mem2_page_t *page;
  unsigned page_idx = addr & ((1 << MEM2_PAGE_BITS) - 1);
  unsigned char *attr;

  page = vm_get_page(mem, addr);
  attr = page->attr + page_idx;

  if(*attr & MEM2_R) {
    *attr |= MEM2_WAS_R;
    if(!(*attr & MEM2_WAS_W)) {
      *attr |= MEM2_INV_R;
      mem->invalid_read = 1;
    }
    return page->data[page_idx];
  }

  mem->invalid_read = 1;

  return 0xff;
}


unsigned vm_read_byte_noerr(x86emu_mem_t *mem, unsigned addr)
{
  mem2_page_t *page;
  unsigned page_idx = addr & ((1 << MEM2_PAGE_BITS) - 1);
  unsigned char *attr;

  page = vm_get_page(mem, addr);
  attr = page->attr + page_idx;

  return (*attr & MEM2_R) ? page->data[page_idx] : 0xff;
}


void vm_write_byte(x86emu_mem_t *mem, unsigned addr, unsigned val)
{
  mem2_page_t *page;
  unsigned page_idx = addr & ((1 << MEM2_PAGE_BITS) - 1);
  unsigned char *attr;

  page = vm_get_page(mem, addr);
  attr = page->attr + page_idx;

  if(*attr & MEM2_W) *attr |= MEM2_WAS_W;

  page->data[page_idx] = val;
}


unsigned vm_read_word(x86emu_mem_t *mem, unsigned addr)
{
  return vm_read_byte(mem, addr) + (vm_read_byte(mem, addr + 1) << 8);
}


unsigned vm_read_dword(x86emu_mem_t *mem, unsigned addr)
{
  return vm_read_word(mem, addr) + (vm_read_word(mem, addr + 2) << 16);
}


uint64_t vm_read_qword(x86emu_mem_t *mem, unsigned addr)
{
  return vm_read_dword(mem, addr) + ((uint64_t) vm_read_dword(mem, addr + 4) << 32);

}


unsigned vm_read_segofs16(x86emu_mem_t *mem, unsigned addr)
{
  return vm_read_word(mem, addr) + (vm_read_word(mem, addr + 2) << 4);
}


void vm_write_word(x86emu_mem_t *mem, unsigned addr, unsigned val)
{
  vm_write_byte(mem, addr, val);
  vm_write_byte(mem, addr + 1, val >> 8);
}


void vm_write_dword(x86emu_mem_t *mem, unsigned addr, unsigned val)
{
  vm_write_word(mem, addr, val);
  vm_write_word(mem, addr + 2, val >> 16);
}


void vm_write_qword(x86emu_mem_t *mem, unsigned addr, uint64_t val)
{
  vm_write_dword(mem, addr, val);
  vm_write_dword(mem, addr + 4, val >> 32);
}


unsigned vm_memio(u32 addr, u32 *val, unsigned type)
{
  x86emu_mem_t *mem = x86emu.mem;
  unsigned err = 0, bits = type & 0xff;

  type &= ~0xff;

  switch(type) {
    case X86EMU_MEMIO_R:
      mem->invalid_read = 0;
      switch(bits) {
        case X86EMU_MEMIO_8:
          *val = vm_read_byte(mem, addr);
          break;
        case X86EMU_MEMIO_16:
          *val = vm_read_word(mem, addr);
          break;
        case X86EMU_MEMIO_32:
          *val = vm_read_dword(mem, addr);
          break;
      }
      err = mem->invalid_read;
      break;

    case X86EMU_MEMIO_W:
      switch(bits) {
        case X86EMU_MEMIO_8:
          vm_write_byte(mem, addr, *val);
          break;
        case X86EMU_MEMIO_16:
          vm_write_word(mem, addr, *val);
          break;
        case X86EMU_MEMIO_32:
          vm_write_dword(mem, addr, *val);
          break;
      }
      err = mem->invalid_write;
      if(mem->invalid_write) x86emu_stop();
      break;

    case X86EMU_MEMIO_X:
      mem->invalid_read = 0;
      switch(bits) {
        case X86EMU_MEMIO_8:
          *val = vm_read_byte(mem, addr);
          break;
        case X86EMU_MEMIO_16:
          *val = vm_read_word(mem, addr);
          break;
        case X86EMU_MEMIO_32:
          *val = vm_read_dword(mem, addr);
          break;
      }
      err = mem->invalid_read;
      break;

    case X86EMU_MEMIO_I:
      switch(bits) {
        case X86EMU_MEMIO_8:
          *val = 0xff;
          break;
        case X86EMU_MEMIO_16:
          *val = 0xffff;
          break;
        case X86EMU_MEMIO_32:
          *val = 0xffffffff;
          break;
      }
      break;

    case X86EMU_MEMIO_O:
      break;
  }

  return err;
}

#define LINE_LEN 16

static void dump_data(unsigned char *data, unsigned char *attr, char *str_data, char *str_attr)
{
  unsigned u;
  char c;
  int ok = 0;
  char *sd = str_data, *sa = str_attr;


  for(u = 0; u < LINE_LEN; u++) {
    *str_data++ = (attr[u] & MEM2_INV_R) ? '*' : ' ';
    if((attr[u] & MEM2_WAS_W)) {
      ok = 1;
      decode_hex2(&str_data, data[u]);

      c = (attr[u] & MEM2_R) ? (attr[u] & MEM2_WAS_R) ? 'R' : 'r' : ' ';
      *str_attr++ = c;
      c = (attr[u] & MEM2_W) ? (attr[u] & MEM2_WAS_W) ? 'w' : 'w' : ' ';
      *str_attr++ = c;
      c = (attr[u] & MEM2_X) ? (attr[u] & MEM2_WAS_X) ? 'X' : 'x' : ' ';
      *str_attr++ = c;
    }
    else {
      *str_data++ = ' ';
      *str_data++ = ' ';

      *str_attr++ = ' ';
      *str_attr++ = ' ';
      *str_attr++ = ' ';
    }
    // *str_data++ = (attr[u] & MEM2_INV_R) ? '*' : ' ';
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


void x86emu_mem_dump(x86emu_t *emu, int flags)
{
  x86emu_mem_t *mem = emu->mem;

  mem2_pdir_t *pdir;
  mem2_ptable_t *ptable;
  mem2_page_t page;
  unsigned pdir_idx, u1, u2, addr;
  char str_data[LINE_LEN * 8], str_attr[LINE_LEN * 8];
  unsigned char def_data[LINE_LEN], def_attr[LINE_LEN];

  if(!mem || !mem->pdir) return;

  // x86emu_log(emu, "; - - - memory dump - - -\n");
  x86emu_log(emu, ";        ");
  for(u1 = 0; u1 < 16; u1++) x86emu_log(emu, "%4x", u1);
  x86emu_log(emu, "\n");

  pdir = mem->pdir;
  for(pdir_idx = 0; pdir_idx < (1 << MEM2_PDIR_BITS); pdir_idx++) {
    ptable = (*pdir)[pdir_idx];
    if(!ptable) continue;
    for(u1 = 0; u1 < (1 << MEM2_PTABLE_BITS); u1++) {
      page = (*ptable)[u1];
      if(page.data) {
        for(u2 = 0; u2 < (1 << MEM2_PAGE_BITS); u2 += LINE_LEN) {
          memcpy(def_data, page.data + u2, LINE_LEN);
          if(page.attr) {
            memcpy(def_attr, page.attr + u2, LINE_LEN);
          }
          else {
            memset(def_attr, page.def_attr, LINE_LEN);
          }
          dump_data(def_data, def_attr, str_data, str_attr);
          if(*str_data) {
            addr = (((pdir_idx << MEM2_PTABLE_BITS) + u1) << MEM2_PAGE_BITS) + u2;
            x86emu_log(emu, "%08x: %s\n", addr, str_data);
            if((flags & 1)) x86emu_log(emu, "          %s\n", str_attr);
          }
        }
      }
    }
  }

  x86emu_log(emu, "\n");
}


