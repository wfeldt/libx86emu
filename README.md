# x86 emulation library

libx86emu is a dependency for [hwinfo](https://github.com/openSUSE/hwinfo).

## API functions

### x86_new

Create new emulation object

    x86emu_t *x86emu_new(unsigned def_mem_perm, unsigned def_io_perm);

def_mem_perm are the default permissions for memory accesses, def_io_perm for io. See
x86emu_set_perm(), x86emu_set_io_perm().

Free object later with x86emu_done().

### x86emu_done

Delete emulation object

    x86emu_t *x86emu_done(x86emu_t *emu);

Frees all memory; returns NULL;

### x86emu_clone

Clone emulation object

    x86emu_t *x86emu_clone(x86emu_t *emu);

Creates a copy of emu. Free the copy later with x86emu_done().

### x86emu_reset

Reset cpu state

    void x86emu_reset(x86emu_t *emu);

Does a normal cpu reset (clear registers, set cs:eip).

### x86emu_run

Start emulation

    unsigned x86emu_run(x86emu_t *emu, unsigned flags);

Flags:

    X86EMU_RUN_TIMEOUT
    X86EMU_RUN_MAX_INSTR
    X86EMU_RUN_NO_EXEC
    X86EMU_RUN_NO_CODE
    X86EMU_RUN_LOOP

* `X86EMU_RUN_TIMEOUT`: set `emu->timeout` to max. seconds to run.
* `X86EMU_RUN_MAX_INSTR`: set `emu->max_instr` to max. instructions to emulate.

Return value indicates why `x86emu_run()` stopped (see flags).

### x86emu_stop

Stop emulation

    void x86emu_stop(x86emu_t *emu);

Use this function in callbacks (e.g. interrupt handler) to tell the emulator
to stop. The emulator returns from `x86emu_run()` when the current instruction
has been finished.

### x86emu_set_log

Set log buffer

    void x86emu_set_log(x86emu_t *emu, char *buffer, unsigned buffer_size, x86emu_flush_func_t flush);
    typedef void (* x86emu_flush_func_t)(x86emu_t *emu, char *buf, unsigned size);

If the log buffer is full, `flush()` is called (if not NULL). The buffer is freed in `x86emu_done()`.


### x86emu_log

Write to log

    void x86emu_log(x86emu_t *emu, const char *format, ...) __attribute__ ((format (printf, 1, 2)));


### x86emu_t

Clear log

  void x86emu_clear_log(x86emu_t *emu, int flush);

Clear log buffer. If flush != 0, write current log via flush() function (see x86emu_set_log()).

### x86emu_dump

Dump emulator state

  void x86emu_dump(x86emu_t *emu, int flags);

Flags:

    X86EMU_DUMP_REGS
    X86EMU_DUMP_MEM
    X86EMU_DUMP_ACC_MEM
    X86EMU_DUMP_INV_MEM
    X86EMU_DUMP_ATTR
    X86EMU_DUMP_ASCII
    X86EMU_DUMP_IO
    X86EMU_DUMP_INTS
    X86EMU_DUMP_TIME

Writes emulator state to log.

### x86emu_set_perm

Memory permissions

    void x86emu_set_perm(x86emu_t *emu, unsigned start, unsigned end, unsigned perm);

`perm` is a bitmask of:

    X86EMU_PERM_R
    X86EMU_PERM_W
    X86EMU_PERM_X
    X86EMU_PERM_VALID
    X86EMU_ACC_R
    X86EMU_ACC_W
    X86EMU_ACC_X
    X86EMU_ACC_INVALID

* `X86EMU_PERM_{R,W,X}`: memory is readable, writable, executable
* `X86EMU_PERM_VALID`: memory has been initialized (say, been written to)
* `X86EMU_ACC_{R,W,X}`: memory has been read, written, executed
* `X86EMU_ACC_INVALID`: there was an invalid access (e.g. tried to read but not readable)

### x86emu_set_page

Direct memory access

    void x86emu_set_page(x86emu_t *emu, unsigned offset, void *address);

Map memory area of `X86EMU_PAGE_SIZE` size at address into emulator at offset. offset
must be `X86EMU_PAGE_SIZE` aligned, address needs not.

Memory permissions still apply (via `x86emu_set_perm()`).

If address is NULL, switch back to emulated memory.

### x86emu_set_io_perm

io permissions

    void x86emu_set_io_perm(x86emu_t *emu, unsigned start, unsigned end, unsigned perm);

`perm`: see `x86emu_set_perm()`.

### x86emu_reset_access_stats

Reset memory access statistics

    void x86emu_reset_access_stats(x86emu_t *emu);

Resets the `X86EMU_ACC_*` bits for the whole memory (see `x86emu_set_perm()`).

### x86emu_code_handler

Execution hook

    x86emu_code_handler_t x86emu_set_code_handler(x86emu_t *emu, x86emu_code_handler_t handler);
    typedef int (* x86emu_code_handler_t)(x86emu_t *emu);

If defined, the function is called before a new instruction is decoded and
emulated. If logging is enabled the current cpu state has already been
logged. If the function returns a value != 0, the emulation is stopped.

### x86emu_intr_handler_t

Set interrupt handler

    x86emu_intr_handler_t x86emu_set_intr_handler(x86emu_t *emu, x86emu_intr_handler_t handler);
    typedef int (* x86emu_intr_handler_t)(x86emu_t *emu, u8 num, unsigned type);

type:

    INTR_TYPE_SOFT
    INTR_TYPE_FAULT

and bitmask of:

    INTR_MODE_RESTART
    INTR_MODE_ERRCODE

If defined, the interrupt handler is called at the start of the interrupt
handling procedure. The handler should return 1 to indicate the interrupt
handling is complete and the emulator can skip its own interrupt processing
or 0 to indicate the emulator should continue with normal interrupt processing.

### x86emu_memio_handler_t

Set alternative callback function that handles memory and io accesses

    x86emu_memio_handler_t x86emu_set_memio_handler(x86emu_t *emu, x86emu_memio_handler_t handler);

    typedef unsigned (* x86emu_memio_handler_t)(x86emu_t *emu, u32 addr, u32 *val, unsigned type);

type: one of

    X86EMU_MEMIO_8
    X86EMU_MEMIO_16
    X86EMU_MEMIO_32
    X86EMU_MEMIO_8_NOPERM

and one of:

    X86EMU_MEMIO_R
    X86EMU_MEMIO_W
    X86EMU_MEMIO_X
    X86EMU_MEMIO_I
    X86EMU_MEMIO_O

Returns old function.

### x86emu_intr_raise

Raise an interrupt

    void x86emu_intr_raise(x86emu_t *emu, u8 intr_nr, unsigned type, unsigned err);

The interrupt is handled before the next instruction. For type see
`x86emu_set_intr_func()`; if `INTR_MODE_ERRCODE` is set, err is the error
code pushed to the stack.

### memory access functions

    unsigned x86emu_read_byte(x86emu_t *emu, unsigned addr);
    unsigned x86emu_read_byte_noperm(x86emu_t *emu, unsigned addr);
    unsigned x86emu_read_word(x86emu_t *emu, unsigned addr); 
    unsigned x86emu_read_dword(x86emu_t *emu, unsigned addr);
    void x86emu_write_byte(x86emu_t *emu, unsigned addr, unsigned val); 
    void x86emu_write_byte_noperm(x86emu_t *emu, unsigned addr, unsigned val); 
    void x86emu_write_word(x86emu_t *emu, unsigned addr, unsigned val); 
    void x86emu_write_dword(x86emu_t *emu, unsigned addr, unsigned val);

Convenience functions to access emulator memory. Memory access restrictions
(see `x86emu_set_perm()`) apply except for `x86emu_*_noperm()` which do not
check permissions.

### x86emu_set_seg_register

Set segment register

    void x86emu_set_seg_register(x86emu_t *emu, sel_t *seg, u16 val);
    R_CS_SEL, R_DS_SEL, R_ES_SEL, R_FS_SEL, R_GS_SEL, R_SS_SEL

Example:

  x86emu_set_seg_register(emu, emu->x86.R_CS_SEL, 0x7c0);


## Debug instruction

If the `X86EMU_TRACE_DEBUG` flags is set, the emulator interprets a special debug instruction:

    db 0x67, 0xeb, LEN, DATA...

which is basically a short jump with address size prefix (an instruction normally not
used). LEN is the size of DATA.

DATA can be:

    db 0x01
    db STRING

Print STRING to log. STRING is not 0-zerminated.

    db 0x02
    dd flags

Set trace flags.

    db 0x03
    dd flags

Clear trace flags.

    db 0x04
    dd flags

Dump emulator state. For flags, see `x86emu_dump()`.

    db 0x05

Reset memory access stats. See `x86emu_reset_access_stats()`.

## openSUSE Development

The package is automatically submitted from the `master` branch to
[system:install:head](https://build.opensuse.org/package/show/system:install:head/libx86emu)
OBS project. From that place it is forwarded to
[openSUSE Factory](https://build.opensuse.org/project/show/openSUSE:Factory).

You can find more information about this workflow in the [linuxrc-devtools
documentation](https://github.com/openSUSE/linuxrc-devtools#opensuse-development).
