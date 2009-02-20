#include "include/x86emu_int.h"


#define LOG_STR(a) memcpy(*p, a, sizeof a - 1), *p += sizeof a - 1
#define LOG_FREE(emu) (emu->log.size + emu->log.buf - emu->log.ptr)


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


x86emu_t *x86emu_new()
{
  x86emu_t *emu = calloc(1, sizeof *emu);

  emu->mem = x86emu_mem_new();

  x86emu_set_memio_func(emu, vm_memio);

  x86emu_reset(emu);

  return emu;
}


x86emu_t *x86emu_done(x86emu_t *emu)
{
  if(emu) {
    if(emu->log.buf) free(emu->log.buf);

    free(emu);
  }

  return NULL;
}

