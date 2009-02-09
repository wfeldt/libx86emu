#include "include/x86emu_int.h"


#define LOG_STR(a) memcpy(*p, a, sizeof a - 1), *p += sizeof a - 1
#define LOG_SPACE(a) (M.log.ptr - M.log.buf + a < M.log.size)


void x86emu_set_memio_func(x86emu_t *emu, x86emu_memio_func_t func)
{
  emu->memio = func;
}


void x86emu_set_intr_func(x86emu_t *emu, unsigned num, x86emu_intr_func_t handler)
{
  if(num < sizeof emu->intr_table / sizeof *emu->intr_table) emu->intr_table[num] = handler;
}


void x86emu_set_code_check(x86emu_t *emu, x86emu_code_check_t func)
{
  emu->code_check = func;
}


void x86emu_set_log(x86emu_t *emu, char *buffer, unsigned buffer_size)
{
  emu->log.size = buffer_size;
  emu->log.buf = buffer;
  emu->log.ptr = emu->log.buf;
  *emu->log.buf = 0;
}


void x86emu_clear_log()
{
  M.log.ptr = M.log.buf;
  *M.log.ptr = 0;
  M.log.full = 0;
}


char *x86emu_get_log()
{
  char **p = &M.log.ptr;

  if(p && M.log.full) {
    M.log.full = 0;
    if(LOG_SPACE(32)) {
      LOG_STR("*** LOG FULL ***\n");
    }
  }

  return M.log.buf;
}


void x86emu_log(const char *format, ...)
{
  va_list args;
  int size = M.log.size - (M.log.ptr - M.log.buf);

  va_start(args, format);
  if(size > 0) {
    size = vsnprintf(M.log.ptr, size, format, args);
    if(size > 0) {
      M.log.ptr += size;
    }
    else {
      *M.log.ptr = 0;
    }
  }
  va_end(args);  
}


