unsigned vm_memio(x86emu_t *emu, u32 addr, u32 *val, unsigned type);
x86emu_mem_t *emu_mem_new(unsigned perm);
x86emu_mem_t *emu_mem_free(x86emu_mem_t *mem);
