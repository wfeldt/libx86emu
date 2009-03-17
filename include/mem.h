unsigned vm_memio(x86emu_t *emu, u32 addr, u32 *val, unsigned type);
x86emu_mem_t *emu_mem_new(unsigned perm);
x86emu_mem_t *emu_mem_free(x86emu_mem_t *mem);
x86emu_mem_t *emu_mem_clone(x86emu_mem_t *mem);
void *mem_dup(const void *src, size_t n);

