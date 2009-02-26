#include "include/x86emu_int.h"
#include <sys/io.h>

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
      *attr |= MEM2_INVALID;
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

  if(*attr & MEM2_W) {
    *attr |= MEM2_WAS_W;
    page->data[page_idx] = val;
  }
  else {
    *attr |= MEM2_INVALID;
  }
}


unsigned vm_x_byte(x86emu_mem_t *mem, unsigned addr)
{
  mem2_page_t *page;
  unsigned page_idx = addr & ((1 << MEM2_PAGE_BITS) - 1);
  unsigned char *attr;

  page = vm_get_page(mem, addr);
  attr = page->attr + page_idx;

  if(*attr & MEM2_X) {
    *attr |= MEM2_WAS_X;
    if(!(*attr & MEM2_WAS_W)) {
      *attr |= MEM2_INVALID;
      mem->invalid_read = mem->invalid_exec = 1;
    }
    return page->data[page_idx];
  }

  mem->invalid_exec = 1;

  return 0xff;
}


unsigned vm_i_byte(unsigned addr)
{
  unsigned char *perm;

  addr &= 0xffff;
  perm = x86emu.io.map + addr;

  if(*perm & MEM2_R) {
    *perm |= MEM2_WAS_R;

    return inb(addr);
  }
  else {
    *perm |= MEM2_INVALID;
  }

  return 0xff;
}


unsigned vm_i_word(unsigned addr)
{
  unsigned char *perm;

  addr &= 0xffff;

  if(addr == 0xffff) return vm_i_byte(addr) + (vm_i_byte(addr + 1) << 8);

  perm = x86emu.io.map + addr;

  if((perm[0] & MEM2_R) && (perm[1] & MEM2_R)) {
    perm[0] |= MEM2_WAS_R;
    perm[1] |= MEM2_WAS_R;

    return inw(addr);
  }
  else {
    perm[0] |= MEM2_INVALID;
    perm[1] |= MEM2_INVALID;
  }

  return 0xffff;
}


unsigned vm_i_dword(unsigned addr)
{
  unsigned char *perm;

  addr &= 0xffff;

  if(addr >= 0xfffd) return vm_i_word(addr) + (vm_i_word(addr + 2) << 16);

  perm = x86emu.io.map + addr;

  if(
    (perm[0] & MEM2_R) &&
    (perm[1] & MEM2_R) &&
    (perm[2] & MEM2_R) &&
    (perm[3] & MEM2_R)
  ) {
    perm[0] |= MEM2_WAS_R;
    perm[1] |= MEM2_WAS_R;
    perm[2] |= MEM2_WAS_R;
    perm[3] |= MEM2_WAS_R;

    return inl(addr);
  }
  else {
    perm[0] |= MEM2_INVALID;
    perm[1] |= MEM2_INVALID;
    perm[2] |= MEM2_INVALID;
    perm[3] |= MEM2_INVALID;
  }

  return 0xffffffff;
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


unsigned vm_x_word(x86emu_mem_t *mem, unsigned addr)
{
  return vm_x_byte(mem, addr) + (vm_x_byte(mem, addr + 1) << 8);
}


unsigned vm_x_dword(x86emu_mem_t *mem, unsigned addr)
{
  return vm_x_word(mem, addr) + (vm_x_word(mem, addr + 2) << 16);
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
      mem->invalid_exec = 0;
      switch(bits) {
        case X86EMU_MEMIO_8:
          *val = vm_x_byte(mem, addr);
          break;
        case X86EMU_MEMIO_16:
          *val = vm_x_word(mem, addr);
          break;
        case X86EMU_MEMIO_32:
          *val = vm_x_dword(mem, addr);
          break;
      }
      err = mem->invalid_exec;
      break;

    case X86EMU_MEMIO_I:
      switch(bits) {
        case X86EMU_MEMIO_8:
          *val = vm_i_byte(addr);
          break;
        case X86EMU_MEMIO_16:
          *val = vm_i_word(addr);
          break;
        case X86EMU_MEMIO_32:
          *val = vm_i_dword(addr);
          break;
      }
      break;

    case X86EMU_MEMIO_O:
      break;
  }

  return err;
}

