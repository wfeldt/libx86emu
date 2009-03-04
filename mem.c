#include "include/x86emu_int.h"
#include <sys/io.h>

x86emu_mem_t *x86emu_mem_new(unsigned perm)
{
  x86emu_mem_t *mem;

  mem = calloc(1, sizeof *mem);
  mem->def_attr = perm;

  return mem;
}


x86emu_mem_t *x86emu_mem_free(x86emu_mem_t *mem)
{
  if(mem) {


    free(mem);
  }

  return NULL;
}


mem2_page_t *vm_get_page(x86emu_mem_t *mem, unsigned addr, int create)
{
  mem2_pdir_t *pdir;
  mem2_ptable_t *ptable;
  mem2_page_t page;
  unsigned pdir_idx = addr >> (32 - X86EMU_PDIR_BITS);
  unsigned ptable_idx = (addr >> X86EMU_PAGE_BITS) & ((1 << X86EMU_PTABLE_BITS) - 1);
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
    for(u = 0; u < (1 << X86EMU_PTABLE_BITS); u++) {
      (*ptable)[u].def_attr = mem->def_attr;
      // fprintf(stderr, "ptable[%u] = %p\n", u, &((*ptable)[u].def_attr));
    }
    // fprintf(stderr, "pdir[%d] = %p (%d)\n", pdir_idx, ptable, sizeof *ptable);
  }

  if(create) {
    page = (*ptable)[ptable_idx];
    if(!page.attr) {
      page.attr = calloc(1, 2 * X86EMU_PAGE_SIZE);
      page.data = page.attr + X86EMU_PAGE_SIZE;
      // fprintf(stderr, "page = %p, page.def_attr = %p\n", page, &page.def_attr);
      memset(page.attr, page.def_attr, X86EMU_PAGE_SIZE);
      (*ptable)[ptable_idx] = page;
      // fprintf(stderr, "page.attr[%d] = %p\n", ptable_idx, page.attr);
    }
  }

  return (*ptable) + ptable_idx;
}


void x86emu_set_perm(x86emu_t *emu, unsigned start, unsigned end, unsigned perm)
{
  x86emu_mem_t *mem;
  mem2_page_t *page;
  unsigned idx;

  if(!emu || !(mem = emu->mem)) return;

  if(start > end) return;

  if((idx = start & (X86EMU_PAGE_SIZE - 1))) {
    page = vm_get_page(mem, start, 1);
    for(; idx < X86EMU_PAGE_SIZE && start <= end; start++) {
      page->attr[idx++] = perm;
    }
    if(!start || start > end) return;
  }

  for(; end - start >= X86EMU_PAGE_SIZE - 1; start += X86EMU_PAGE_SIZE) {
    page = vm_get_page(mem, start, 0);
    page->def_attr = perm;
    if(page->attr) memset(page->attr, page->def_attr, X86EMU_PAGE_SIZE);
    if(!start) return;
  }

  if(start > end) return;

  page = vm_get_page(mem, start, 1);
  end = end - start + 1;
  for(idx = 0; idx < end; idx++) {
    page->attr[idx] = perm;
  }
}


void x86emu_set_page_address(x86emu_t *emu, unsigned page, void *address)
{
  x86emu_mem_t *mem;
  mem2_page_t *p;
  unsigned u;

  if(!emu || !(mem = emu->mem)) return;

  p = vm_get_page(mem, page, 1);

  if(address) {
    p->data = address;

    // tag memory as initialized
    for(u = 0; u < X86EMU_PAGE_SIZE; u++) {
      p->attr[u] |= X86EMU_ACC_W;
    }
  }
  else {
    p->data = p->attr + X86EMU_PAGE_SIZE;
  }
}


unsigned vm_read_byte(x86emu_mem_t *mem, unsigned addr)
{
  mem2_page_t *page;
  unsigned page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  unsigned char *attr;

  page = vm_get_page(mem, addr, 1);
  attr = page->attr + page_idx;

  if(*attr & X86EMU_PERM_R) {
    *attr |= X86EMU_ACC_R;
    if(!(*attr & X86EMU_ACC_W)) {
      *attr |= X86EMU_ACC_INVALID;
      mem->invalid = 1;
    }
    return page->data[page_idx];
  }

  mem->invalid = 1;

  return 0xff;
}


unsigned vm_read_byte_noerr(x86emu_mem_t *mem, unsigned addr)
{
  mem2_page_t *page;
  unsigned page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  unsigned char *attr;

  page = vm_get_page(mem, addr, 1);
  attr = page->attr + page_idx;

  return (*attr & X86EMU_PERM_R) ? page->data[page_idx] : 0xff;
}


void vm_write_byte(x86emu_mem_t *mem, unsigned addr, unsigned val)
{
  mem2_page_t *page;
  unsigned page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  unsigned char *attr;

  page = vm_get_page(mem, addr, 1);
  attr = page->attr + page_idx;

  if(*attr & X86EMU_PERM_W) {
    *attr |= X86EMU_ACC_W;
    page->data[page_idx] = val;
  }
  else {
    *attr |= X86EMU_ACC_INVALID;

    mem->invalid = 1;
  }
}


unsigned vm_x_byte(x86emu_mem_t *mem, unsigned addr)
{
  mem2_page_t *page;
  unsigned page_idx = addr & (X86EMU_PAGE_SIZE - 1);
  unsigned char *attr;

  page = vm_get_page(mem, addr, 1);
  attr = page->attr + page_idx;

  if(*attr & X86EMU_PERM_X) {
    *attr |= X86EMU_ACC_X;
    if(!(*attr & X86EMU_ACC_W)) {
      *attr |= X86EMU_ACC_INVALID;
      mem->invalid = 1;
    }
    return page->data[page_idx];
  }

  mem->invalid = 1;

  return 0xff;
}


unsigned vm_i_byte(unsigned addr)
{
  unsigned char *perm;

  addr &= 0xffff;
  perm = M.io.map + addr;

  if(
    M.io.iopl_ok &&
    (*perm & X86EMU_PERM_R)
  ) {
    *perm |= X86EMU_ACC_R;

    M.io.stats_i[addr]++;

    return inb(addr);
  }
  else {
    *perm |= X86EMU_ACC_INVALID;
  }

  M.mem->invalid = 1;

  return 0xff;
}


unsigned vm_i_word(unsigned addr)
{
  unsigned char *perm;
  unsigned val;

  addr &= 0xffff;
  perm = M.io.map + addr;

  if(
    !M.io.iopl_ok ||
    addr == 0xffff ||
    !(perm[0] & X86EMU_PERM_R) ||
    !(perm[1] & X86EMU_PERM_R)
  ) {
    val = vm_i_byte(addr);
    val += (vm_i_byte(addr + 1) << 8);

    return val;
  }

  perm[0] |= X86EMU_ACC_R;
  perm[1] |= X86EMU_ACC_R;

  M.io.stats_i[addr]++;
  M.io.stats_i[addr + 1]++;

  return inw(addr);
}


unsigned vm_i_dword(unsigned addr)
{
  unsigned char *perm;
  unsigned val;

  addr &= 0xffff;
  perm = M.io.map + addr;

  if(
    !M.io.iopl_ok ||
    addr >= 0xfffd ||
    !(perm[0] & X86EMU_PERM_R) ||
    !(perm[1] & X86EMU_PERM_R) ||
    !(perm[2] & X86EMU_PERM_R) ||
    !(perm[3] & X86EMU_PERM_R)
  ) {
    val = vm_i_byte(addr);
    val += (vm_i_byte(addr + 1) << 8);
    val += (vm_i_byte(addr + 2) << 16);
    val += (vm_i_byte(addr + 3) << 24);

    return val;
  }

  perm[0] |= X86EMU_ACC_R;
  perm[1] |= X86EMU_ACC_R;
  perm[2] |= X86EMU_ACC_R;
  perm[3] |= X86EMU_ACC_R;

  M.io.stats_i[addr]++;
  M.io.stats_i[addr + 1]++;
  M.io.stats_i[addr + 2]++;
  M.io.stats_i[addr + 3]++;

  return inl(addr);
}


void vm_o_byte(unsigned addr, unsigned val)
{
  unsigned char *perm;

  addr &= 0xffff;
  perm = M.io.map + addr;

  if(
    M.io.iopl_ok &&
    (*perm & X86EMU_PERM_W)
  ) {
    *perm |= X86EMU_ACC_W;

    M.io.stats_o[addr]++;

    outb(val, addr);
  }
  else {
    *perm |= X86EMU_ACC_INVALID;

    M.mem->invalid = 1;
  }
}


void vm_o_word(unsigned addr, unsigned val)
{
  unsigned char *perm;

  addr &= 0xffff;
  perm = M.io.map + addr;

  if(
    !M.io.iopl_ok ||
    addr == 0xffff ||
    !(perm[0] & X86EMU_PERM_W) ||
    !(perm[1] & X86EMU_PERM_W)
  ) {
    vm_o_byte(addr, val);
    vm_o_byte(addr + 1, val);

    return;
  }

  perm[0] |= X86EMU_ACC_W;
  perm[1] |= X86EMU_ACC_W;

  M.io.stats_o[addr]++;
  M.io.stats_o[addr + 1]++;

  outw(val, addr);
}


void vm_o_dword(unsigned addr, unsigned val)
{
  unsigned char *perm;

  addr &= 0xffff;
  perm = M.io.map + addr;

  if(
    !M.io.iopl_ok ||
    addr >= 0xfffd ||
    !(perm[0] & X86EMU_PERM_W) ||
    !(perm[1] & X86EMU_PERM_W) ||
    !(perm[2] & X86EMU_PERM_W) ||
    !(perm[3] & X86EMU_PERM_W)
  ) {
    vm_o_byte(addr, val);
    vm_o_byte(addr + 1, val);
    vm_o_byte(addr + 2, val);
    vm_o_byte(addr + 3, val);

    return;
  }

  perm[0] |= X86EMU_ACC_W;
  perm[1] |= X86EMU_ACC_W;
  perm[2] |= X86EMU_ACC_W;
  perm[3] |= X86EMU_ACC_W;

  M.io.stats_o[addr]++;
  M.io.stats_o[addr + 1]++;
  M.io.stats_o[addr + 2]++;
  M.io.stats_o[addr + 3]++;

  outl(val, addr);
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
  x86emu_mem_t *mem = M.mem;
  unsigned bits = type & 0xff;

  type &= ~0xff;

  mem->invalid = 0;

  switch(type) {
    case X86EMU_MEMIO_R:
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
      break;

    case X86EMU_MEMIO_X:
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
      switch(bits) {
        case X86EMU_MEMIO_8:
          vm_o_byte(addr, *val);
          break;
        case X86EMU_MEMIO_16:
          vm_o_word(addr, *val);
          break;
        case X86EMU_MEMIO_32:
          vm_o_dword(addr, *val);
          break;
      }
      break;
  }

  return mem->invalid;
}

