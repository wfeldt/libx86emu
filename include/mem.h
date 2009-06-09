unsigned vm_memio(x86emu_t *emu, u32 addr, u32 *val, unsigned type) L_SYM;
x86emu_mem_t *emu_mem_new(unsigned perm) L_SYM;
x86emu_mem_t *emu_mem_free(x86emu_mem_t *mem) L_SYM;
x86emu_mem_t *emu_mem_clone(x86emu_mem_t *mem) L_SYM;
void *mem_dup(const void *src, size_t n) L_SYM;

