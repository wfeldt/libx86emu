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
void flush_log(char *buf, unsigned size);

void help(void);
int do_int(u8 num, unsigned type);
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
  { "code",       0, NULL, 1001 },
  { "regs",       0, NULL, 1002 },
  { "max",        1, NULL, 1003 },
  { "stderr",     0, NULL, 1004 },
  { "data",       0, NULL, 1005 },
  { "io",         0, NULL, 1006 },
  { "intr",       0, NULL, 1007 },
  { "acc",        0, NULL, 1008 },
  { "attr",       0, NULL, 1009 },
  { }
};

struct {
  unsigned verbose;
  unsigned inst_max;
  struct {
    unsigned regs:1;
    unsigned code:1;
    unsigned data:1;
    unsigned io:1;
    unsigned intr:1;
    unsigned acc:1;
    unsigned stderr:1;
    unsigned attr:1;
  } show;
} opt;


FILE *log_file = NULL;

int main(int argc, char **argv)
{
  int i, err = 0;

  opt.inst_max = 100000;

  opterr = 0;

  while((i = getopt_long(argc, argv, "hv", options, NULL)) != -1) {
    switch(i) {
      case 'v':
        opt.verbose++;
        break;

      case 1001:
        opt.show.code = 1;
        break;

      case 1002:
        opt.show.regs = 1;
        break;

      case 1003:
        opt.inst_max = strtoul(optarg, NULL, 0);
        break;

      case 1004:
        opt.show.stderr = 1;
        break;

      case 1005:
        opt.show.data = 1;
        break;

      case 1006:
        opt.show.io = 1;
        break;

      case 1007:
        opt.show.intr = 1;
        break;

      case 1008:
        opt.show.acc = 1;
        break;

      case 1009:
        opt.show.attr = 1;
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
  if(log_file) vfprintf(log_file, format, args);
  va_end(args);
}


void flush_log(char *buf, unsigned size)
{
  if(!buf || !size || !log_file) return;

  fwrite(buf, size, 1, log_file);
}


void help()
{
  fprintf(stderr,
    "libx86 Test\nusage: x86test test_file\n"
  );
}


int do_int(u8 num, unsigned type)
{
  if((type & 0xff) == INTR_TYPE_FAULT) {
    x86emu_stop();
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
  unsigned u;

  vm = calloc(1, sizeof *vm);

  vm->emu = x86emu_new(X86EMU_PERM_R | X86EMU_PERM_W | X86EMU_PERM_X, 0);
  vm->emu->private = vm;

  x86emu_set_log(vm->emu, 1000000, flush_log);

  for(u = 0; u < 0x100; u++) x86emu_set_intr_func(vm->emu, u, do_int);

  return vm;
}


void vm_free(vm_t *vm)
{
  free(vm);
}


int vm_init(vm_t *vm, char *file)
{
  x86emu_mem_t *mem = vm->emu->mem;
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
        if(isxdigit(s[2])) vm_write_byte(mem, addr, strtoul(s, NULL, 16));

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
  if(opt.show.regs) vm->emu->log.regs = 1;
  if(opt.show.code) vm->emu->log.code = 1;
  if(opt.show.data) vm->emu->log.data = 1;
  if(opt.show.acc) vm->emu->log.acc = 1;
  if(opt.show.io) vm->emu->log.io = 1;
  if(opt.show.intr) vm->emu->log.intr = 1;

  vm->emu->x86.tsc = 0;
  vm->emu->x86.tsc_max = opt.inst_max;

  // x86emu_set_perm(vm->emu, 0x1004, 1, X86EMU_ACC_W | X86EMU_PERM_R);

  // x86emu_set_io_perm(vm->emu, 0, 0x400, X86EMU_PERM_R | X86EMU_PERM_W);
  // iopl(3);
  x86emu_exec(vm->emu);

  x86emu_clear_log(vm->emu, 1);
}


void vm_dump(vm_t *vm, char *file)
{
  FILE *old_log;

  old_log = log_file;

  if(file) log_file = fopen(file, "w");

  if(log_file) {
    x86emu_dump(vm->emu, X86EMU_DUMP_MEM | X86EMU_DUMP_REGS | (!file && opt.show.attr ? X86EMU_DUMP_ATTR : 0));
    x86emu_clear_log(vm->emu, 1);
  }

  if(file) fclose(log_file);

  log_file = old_log;
}


char *build_file_name(char *file, char *suffix)
{
  int i;
  static char *s = NULL;

  free(s);

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

  return err;
}


int run_test(char *file)
{
  vm_t *vm = vm_new();
  int ok = 0, result = 0;

  if(!file) return 1;

  log_file = opt.show.stderr ? stderr : fopen(build_file_name(file, ".log"), "w");

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

  fclose(log_file);
  log_file = NULL;

  return result == 1 ? 1 : 0;
}


