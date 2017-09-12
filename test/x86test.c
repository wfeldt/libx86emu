/****************************************************************************
*
* Realmode X86 Emulator Library
*
* Copyright (c) 2007-2017 SUSE LINUX GmbH; Author: Steffen Winterfeldt
*
*  ========================================================================
*
*  Permission to use, copy, modify, distribute, and sell this software and
*  its documentation for any purpose is hereby granted without fee,
*  provided that the above copyright notice appear in all copies and that
*  both that copyright notice and this permission notice appear in
*  supporting documentation, and that the name of the authors not be used
*  in advertising or publicity pertaining to distribution of the software
*  without specific, written prior permission.  The authors makes no
*  representations about the suitability of this software for any purpose.
*  It is provided "as is" without express or implied warranty.
*
*  THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
*  EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
*  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
*  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
*  PERFORMANCE OF THIS SOFTWARE.
*
*  ========================================================================
*
* Description:
*   Program to run test suite.
*
****************************************************************************/


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <getopt.h>
#include <inttypes.h>
#include <x86emu.h>
#include <sys/io.h>

typedef struct {
  x86emu_t *emu;
} vm_t;


void lprintf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void flush_log(x86emu_t *emu, char *buf, unsigned size);

void help(void);
int do_int(x86emu_t *emu, u8 num, unsigned type);
char *skip_spaces(char *s);
vm_t *vm_new(void);
void vm_free(vm_t *vm);
int vm_init(vm_t *vm, char *file);
void vm_run(vm_t *vm);
void vm_dump(vm_t *vm, char *file);

char *build_file_name(char *file, char *suffix);
int result_cmp(char *file);
int run_test(char *file);


struct option options[] = {
  { "help",       0, NULL, 'h'  },
  { "verbose",    0, NULL, 'v'  },
  { "show",       1, NULL, 1001 },
  { "max",        1, NULL, 1003 },
  { "stderr",     0, NULL, 1004 },
  { }
};

struct {
  unsigned verbose;
  unsigned inst_max;

  unsigned trace_flags;
  unsigned dump_flags;

  struct {
    unsigned stderr:1;
  } show;

  FILE *log_file;
} opt;


int main(int argc, char **argv)
{
  int i, err = 0;
  char *s, *t;
  unsigned u, tbits, dbits;

  opterr = 0;

  while((i = getopt_long(argc, argv, "hv", options, NULL)) != -1) {
    switch(i) {
      case 'v':
        opt.verbose++;
        break;

      case 1001:
        for(s = optarg; (t = strsep(&s, ",")); ) {
          u = 1;
          tbits = dbits = 0;
          while(*t == '+' || *t == '-') u = *t++ == '+' ? 1 : 0;
               if(!strcmp(t, "trace")) tbits = X86EMU_TRACE_DEFAULT;
          else if(!strcmp(t, "code"))  tbits = X86EMU_TRACE_CODE;
          else if(!strcmp(t, "regs"))  tbits = X86EMU_TRACE_REGS;
          else if(!strcmp(t, "data"))  tbits = X86EMU_TRACE_DATA;
          else if(!strcmp(t, "acc"))   tbits = X86EMU_TRACE_ACC;
          else if(!strcmp(t, "io"))    tbits = X86EMU_TRACE_IO;
          else if(!strcmp(t, "ints"))  tbits = X86EMU_TRACE_INTS;
          else if(!strcmp(t, "time"))  tbits = X86EMU_TRACE_TIME;
          else if(!strcmp(t, "attr"))  dbits = X86EMU_DUMP_ATTR;
          else if(!strcmp(t, "ascii")) dbits = X86EMU_DUMP_ASCII;
          else err = 5;
          if(err) {
            fprintf(stderr, "error: invalid flag '%s'\n", t);
            return 1;
          }
          else {
            if(tbits) {
              if(u) {
                opt.trace_flags |= tbits;
              }
              else {
                opt.trace_flags &= ~tbits;
              }
            }
            if(dbits) {
              if(u) {
                opt.dump_flags |= dbits;
              }
              else {
                opt.dump_flags &= ~dbits;
              }
            }
          }
        }
        break;

      case 1003:
        opt.inst_max = strtoul(optarg, NULL, 0);
        break;

      case 1004:
        opt.show.stderr = 1;
        break;

      default:
        help();
        return i == 'h' ? 0 : 1;
    }
  }

  argc -= optind;
  argv += optind;

  if(!argc) {
    help();

    return 1;
  }


  do {
    err |= run_test(argv[0]);
  }
  while(argv++, --argc);

  return err;
}


void lprintf(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  if(opt.log_file) vfprintf(opt.log_file, format, args);
  va_end(args);
}


void flush_log(x86emu_t *emu, char *buf, unsigned size)
{
  if(!buf || !size || !opt.log_file) return;

  fwrite(buf, size, 1, opt.log_file);
}


void help()
{
  fprintf(stderr,
    "libx86 Test\nusage: x86test test_file\n"
  );
}


int do_int(x86emu_t *emu, u8 num, unsigned type)
{
  if((type & 0xff) == INTR_TYPE_FAULT) {
    x86emu_stop(emu);
  }

  return 0;
}


char *skip_spaces(char *s)
{
  while(isspace(*s)) s++;

  return s;
}


vm_t *vm_new()
{
  vm_t *vm;

  vm = calloc(1, sizeof *vm);

  vm->emu = x86emu_new(X86EMU_PERM_R | X86EMU_PERM_W | X86EMU_PERM_X, 0);
  vm->emu->private = vm;

  x86emu_set_log(vm->emu, 1000000, flush_log);
  x86emu_set_intr_handler(vm->emu, do_int);

  vm->emu->log.trace = opt.trace_flags;

  return vm;
}


void vm_free(vm_t *vm)
{
  x86emu_done(vm->emu);
  free(vm);
}


int vm_init(vm_t *vm, char *file)
{
  FILE *f;
  char buf[1024], *s, *s1, *s2;
  unsigned u, addr, line = 0;

  if(!vm || !file) return 1;

  if(!(f = fopen(file, "r"))) return 0;

  lprintf("- - - - - - - -  initial vm state [%s]  - - - - - - - -\n", file);

  while(fgets(buf, sizeof buf, f)) {
    lprintf("%s", buf);
    line++;
    if((s = strchr(buf, ';'))) *s = 0;
    s = skip_spaces(buf);
    if(*s == 0) continue;
    addr = strtoul(s, &s1, 16);
    if(s1 - s == 8 && *s1 == ':') {
      s = s1 + 1;
      while(
        isspace(s[0]) && isspace(s[1]) &&
        ((isxdigit(s[2]) && isxdigit(s[3])) || (isspace(s[2]) && isspace(s[3]))) &&
        !isxdigit(s[4])
      ) {
        if(isxdigit(s[2])) x86emu_write_byte(vm->emu, addr, strtoul(s, NULL, 16));

        addr++;
        s += 4;
      }
    }
    else {
      while((s1 = strchr(s, '='))) {
        u = strtoul(s1 + 1, &s2, 16);
        if(*s2 && !isspace(*s2)) {
          lprintf("%s:%u: invalid line:\n%s", file, line, buf);
          return 0;
        }
        s1++;
             if(!memcmp(s, "eax=", s1 - s)) vm->emu->x86.R_EAX = u;
        else if(!memcmp(s, "ebx=", s1 - s)) vm->emu->x86.R_EBX = u;
        else if(!memcmp(s, "ecx=", s1 - s)) vm->emu->x86.R_ECX = u;
        else if(!memcmp(s, "edx=", s1 - s)) vm->emu->x86.R_EDX = u;
        else if(!memcmp(s, "esi=", s1 - s)) vm->emu->x86.R_ESI = u;
        else if(!memcmp(s, "edi=", s1 - s)) vm->emu->x86.R_EDI = u;
        else if(!memcmp(s, "ebp=", s1 - s)) vm->emu->x86.R_EBP = u;
        else if(!memcmp(s, "esp=", s1 - s)) vm->emu->x86.R_ESP = u;
        else if(!memcmp(s, "eip=", s1 - s)) vm->emu->x86.R_EIP = u;
        else if(!memcmp(s, "eflags=", s1 - s)) vm->emu->x86.R_EFLG = u;
        else if(!memcmp(s, "cs=", s1 - s)) {
          vm->emu->x86.R_CS = u;
          vm->emu->x86.R_CS_BASE = vm->emu->x86.R_CS << 4;
        }
        else if(!memcmp(s, "ds=", s1 - s)) {
          vm->emu->x86.R_DS = u;
          vm->emu->x86.R_DS_BASE = vm->emu->x86.R_DS << 4;
        }
        else if(!memcmp(s, "es=", s1 - s)) {
          vm->emu->x86.R_ES = u;
          vm->emu->x86.R_ES_BASE = vm->emu->x86.R_ES << 4;
        }
        else if(!memcmp(s, "fs=", s1 - s)) {
          vm->emu->x86.R_FS = u;
          vm->emu->x86.R_FS_BASE = vm->emu->x86.R_FS << 4;
        }
        else if(!memcmp(s, "gs=", s1 - s)) {
          vm->emu->x86.R_GS = u;
          vm->emu->x86.R_GS_BASE = vm->emu->x86.R_GS << 4;
        }
        else if(!memcmp(s, "ss=", s1 - s)) {
          vm->emu->x86.R_SS = u;
          vm->emu->x86.R_SS_BASE = vm->emu->x86.R_SS << 4;
        }
        else if(!memcmp(s, "cs.base=", s1 - s)) vm->emu->x86.R_CS_BASE = u;
        else if(!memcmp(s, "ds.base=", s1 - s)) vm->emu->x86.R_DS_BASE = u;
        else if(!memcmp(s, "es.base=", s1 - s)) vm->emu->x86.R_ES_BASE = u;
        else if(!memcmp(s, "fs.base=", s1 - s)) vm->emu->x86.R_FS_BASE = u;
        else if(!memcmp(s, "gs.base=", s1 - s)) vm->emu->x86.R_GS_BASE = u;
        else if(!memcmp(s, "ss.base=", s1 - s)) vm->emu->x86.R_SS_BASE = u;
        else if(!memcmp(s, "cs.limit=", s1 - s)) vm->emu->x86.R_CS_LIMIT = u;
        else if(!memcmp(s, "ds.limit=", s1 - s)) vm->emu->x86.R_DS_LIMIT = u;
        else if(!memcmp(s, "es.limit=", s1 - s)) vm->emu->x86.R_ES_LIMIT = u;
        else if(!memcmp(s, "fs.limit=", s1 - s)) vm->emu->x86.R_FS_LIMIT = u;
        else if(!memcmp(s, "gs.limit=", s1 - s)) vm->emu->x86.R_GS_LIMIT = u;
        else if(!memcmp(s, "ss.limit=", s1 - s)) vm->emu->x86.R_SS_LIMIT = u;
        else if(!memcmp(s, "cs.acc=", s1 - s)) vm->emu->x86.R_CS_ACC = u;
        else if(!memcmp(s, "ds.acc=", s1 - s)) vm->emu->x86.R_DS_ACC = u;
        else if(!memcmp(s, "es.acc=", s1 - s)) vm->emu->x86.R_ES_ACC = u;
        else if(!memcmp(s, "fs.acc=", s1 - s)) vm->emu->x86.R_FS_ACC = u;
        else if(!memcmp(s, "gs.acc=", s1 - s)) vm->emu->x86.R_GS_ACC = u;
        else if(!memcmp(s, "ss.acc=", s1 - s)) vm->emu->x86.R_SS_ACC = u;
        else if(!memcmp(s, "tr=", s1 - s)) vm->emu->x86.R_TR = u;
        else if(!memcmp(s, "tr.base=", s1 - s)) vm->emu->x86.R_TR_BASE = u;
        else if(!memcmp(s, "tr.limit=", s1 - s)) vm->emu->x86.R_TR_LIMIT = u;
        else if(!memcmp(s, "tr.acc=", s1 - s)) vm->emu->x86.R_TR_ACC = u;
        else if(!memcmp(s, "ldt=", s1 - s)) vm->emu->x86.R_LDT = u;
        else if(!memcmp(s, "ldt.base=", s1 - s)) vm->emu->x86.R_LDT_BASE = u;
        else if(!memcmp(s, "ldt.limit=", s1 - s)) vm->emu->x86.R_LDT_LIMIT = u;
        else if(!memcmp(s, "ldt.acc=", s1 - s)) vm->emu->x86.R_LDT_ACC = u;
        else if(!memcmp(s, "gdt.base=", s1 - s)) vm->emu->x86.R_GDT_BASE = u;
        else if(!memcmp(s, "gdt.limit=", s1 - s)) vm->emu->x86.R_GDT_LIMIT = u;
        else if(!memcmp(s, "idt.base=", s1 - s)) vm->emu->x86.R_IDT_BASE = u;
        else if(!memcmp(s, "idt.limit=", s1 - s)) vm->emu->x86.R_IDT_LIMIT = u;
        else if(!memcmp(s, "cr0=", s1 - s)) vm->emu->x86.R_CR0 = u;
        else if(!memcmp(s, "cr1=", s1 - s)) vm->emu->x86.R_CR1 = u;
        else if(!memcmp(s, "cr2=", s1 - s)) vm->emu->x86.R_CR2 = u;
        else if(!memcmp(s, "cr3=", s1 - s)) vm->emu->x86.R_CR3 = u;
        else if(!memcmp(s, "cr4=", s1 - s)) vm->emu->x86.R_CR4 = u;
        else if(!memcmp(s, "dr0=", s1 - s)) vm->emu->x86.R_DR0 = u;
        else if(!memcmp(s, "dr1=", s1 - s)) vm->emu->x86.R_DR1 = u;
        else if(!memcmp(s, "dr2=", s1 - s)) vm->emu->x86.R_DR2 = u;
        else if(!memcmp(s, "dr3=", s1 - s)) vm->emu->x86.R_DR3 = u;
        else if(!memcmp(s, "dr6=", s1 - s)) vm->emu->x86.R_DR6 = u;
        else if(!memcmp(s, "dr7=", s1 - s)) vm->emu->x86.R_DR7 = u;
        else break;

        s = skip_spaces(s2);
      }
      s = skip_spaces(s);
      if(*s) {
        lprintf("%s:%u: invalid line:\n%s", file, line, buf);
        return 0;
      }
    }
  }

  lprintf("- - - - - - - - - - - - - - - -\n");

  fclose(f);

  return 1;
}


void vm_run(vm_t *vm)
{
  unsigned flags;

  // x86emu_set_perm(vm->emu, 0x1004, 0x1004, X86EMU_PERM_VALID | X86EMU_PERM_R);

  // x86emu_set_io_perm(vm->emu, 0, 0x3ff, X86EMU_PERM_R | X86EMU_PERM_W);
  // iopl(3);

  flags = X86EMU_RUN_LOOP | X86EMU_RUN_NO_CODE;

  if(opt.inst_max) {
    vm->emu->max_instr = opt.inst_max;
    flags |= X86EMU_RUN_MAX_INSTR;
  }
  x86emu_run(vm->emu, flags);

  x86emu_clear_log(vm->emu, 1);
}


void vm_dump(vm_t *vm, char *file)
{
  FILE *old_log;

  old_log = opt.log_file;

  if(file) opt.log_file = fopen(file, "w");

  if(opt.log_file) {
    x86emu_dump(vm->emu,
      X86EMU_DUMP_MEM | X86EMU_DUMP_REGS | (!file && opt.dump_flags ? opt.dump_flags : 0)
    );
    x86emu_clear_log(vm->emu, 1);
  }

  if(file) fclose(opt.log_file);

  opt.log_file = old_log;
}


char *build_file_name(char *file, char *suffix)
{
  int i;
  static char *s = NULL;

  free(s);
  s = NULL;

  if(!file || !suffix) return s;

  i = strlen(file);
  // 5 = strlen(".init")
  if(i >= 5 && !strcmp(file + i - 5, ".init")) {
    s = calloc(1, i - 5 + strlen(suffix) + 1);
    memcpy(s, file, i - 5);
    strcat(s, suffix);
  }
  else {
    asprintf(&s, "%s%s", file, suffix);
  }

  return s;
}


int result_cmp(char *file)
{
  FILE *f0, *f1;
  int err = 1;
  unsigned char *buf0, *buf1;
  int i, l0, l1;

  f0 = fopen(build_file_name(file, ".result"), "r");
  f1 = fopen(build_file_name(file, ".done"), "r");

  if(!f1) err = 2;

  if(f0 && f1) {
    l0 = fseek(f0, 0, SEEK_END);
    l1 = fseek(f1, 0, SEEK_END);
    if(!l0 && !l1) {
      l0 = ftell(f0);
      l1 = ftell(f1);
      rewind(f0);
      rewind(f1);
      if(l0 && l0 == l1) {
        i = l0;
        buf0 = malloc(i);
        buf1 = malloc(i);
        l0 = fread(buf0, 1, i, f0);
        l1 = fread(buf1, 1, i, f1);
        if(l0 == i && l1 == i && !memcmp(buf0, buf1, i)) err = 0;
        free(buf1);
        free(buf0);
      }
    }
  }

  if(f1) fclose(f1);
  if(f0) fclose(f0);

  build_file_name(NULL, NULL);

  return err;
}


int run_test(char *file)
{
  vm_t *vm = vm_new();
  int ok = 0, result = 0;

  if(!file) return 1;

  opt.log_file = opt.show.stderr ? stderr : fopen(build_file_name(file, ".log"), "w");

  ok = vm_init(vm, file);

  if(ok) {
    lprintf("%s: starting test\n", file);

    vm_run(vm);

    lprintf("\n- - - - - - - -  final vm state  - - - - - - - -\n");
    vm_dump(vm, NULL);
    lprintf("- - - - - - - - - - - - - - - -\n");

    vm_dump(vm, build_file_name(file, ".result"));
  }

  vm_free(vm);

  if(ok) {
    result = result_cmp(file);
  }
  else {
    result = 1;
  }

  lprintf("%s: %s\n", file, result == 0 ? "ok" : result == 1 ? "failed" : "unchecked");
  fprintf(stderr, "%s  %s\n", result == 0 ? "ok" : result == 1 ? "F " : "- ", file);

  fclose(opt.log_file);
  opt.log_file = NULL;

  build_file_name(NULL, NULL);

  return result == 1 ? 1 : 0;
}


