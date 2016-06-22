/*
 * This is a simple program demonstrating the libx86emu usage.
 *
 * It lets you load a binary blob at some address and run the emulation.The
 * emulation trace is logged to the console to show what it is doing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <x86emu.h>

void help(void);
void flush_log(x86emu_t *emu, char *buf, unsigned size);
x86emu_t *emu_new(void);
int emu_init(x86emu_t *emu, char *file);
void emu_run(char *file);


struct option options[] = {
  { "help",       0, NULL, 'h'  },
  { "load",       1, NULL, 'l'  },
  { "start",      1, NULL, 's'  },
  { "max",        1, NULL, 'm'  },
  { }
};


struct {
  struct {
    unsigned segment;
    unsigned offset;
  } start;
  unsigned load;
  unsigned max_instructions;
  char *file;
} opt;


/*
 * Parse options, then run emulation.
 */
int main(int argc, char **argv)
{
  int i;
  char *str;

  opt.start.segment = 0;
  opt.start.offset = opt.load = 0x7c00;
  opt.max_instructions = 10000000;

  opterr = 0;

  while((i = getopt_long(argc, argv, "hm:l:s:", options, NULL)) != -1) {
    switch(i) {
      case 'm':
        opt.max_instructions = strtoul(optarg, NULL, 0);
        break;

      case 'l':
        opt.load = strtoul(optarg, NULL, 0);
        break;

      case 's':
        opt.start.offset = strtoul(optarg, &str, 0);
        if(*str == ':') {
          opt.start.segment = opt.start.offset;
          opt.start.offset = strtoul(str + 1, NULL, 0);
        }
        break;

      default:
        help();
        return i == 'h' ? 0 : 1;
    }
  }

  if(argc == optind + 1) {
    opt.file = argv[optind];
  }
  else {
    help();
    return 1;
  }

  emu_run(opt.file);

  return 0;
}


/*
 * Display short usage message.
 */
void help()
{
  printf(
    "Usage: x86emu-demo [OPTIONS] FILE\n"
    "\n"
    "Load FILE and run x86 emulation.\n"
   "\n"
    "Options:\n"
    "  -l, --load ADDRESS\n"
    "      load FILE at ADDRESS into memory (default: 0x7c00).\n"
    "  -s, --start ADDRESS\n"
    "      start emulation at ADDRESS (default 0:0x7c00).\n"
    "      Note: ADDRESS may contain a colon (':') to separate segment and offset values;\n"
    "      if not, segment = 0 is assumed.\n"
    "  -m, --max N\n"
    "      stop after emulating N instructions.\n"
    "  -h, --help\n"
    "      show this text\n"
  );
}


/*
 * Write emulation log to console.
 */
void flush_log(x86emu_t *emu, char *buf, unsigned size)
{
  if(!buf || !size) return;

  fwrite(buf, size, 1, stdout);
}


/*
 * Create new emulation object.
 */
x86emu_t *emu_new()
{
  x86emu_t *emu = x86emu_new(X86EMU_PERM_R | X86EMU_PERM_W | X86EMU_PERM_X, 0);

  /* log buf size of 1000000 is purely arbitrary */
  x86emu_set_log(emu, 1000000, flush_log);

  emu->log.trace = X86EMU_TRACE_DEFAULT;

  return emu;
}


/*
 * Setup registers and memory.
 */
int emu_init(x86emu_t *emu, char *file)
{
  FILE *f;
  unsigned addr;
  int i;

  addr = opt.load;

  x86emu_set_seg_register(emu, emu->x86.R_CS_SEL, opt.start.segment);
  emu->x86.R_EIP = opt.start.offset;

  if(!(f = fopen(file, "r"))) return 0;

  while((i = fgetc(f)) != EOF) {
    x86emu_write_byte(emu, addr++, i);
  }

  fclose(f);

  return 1;
}


/*
 * Run emulation.
 */
void emu_run(char *file)
{
  x86emu_t *emu = emu_new();
  unsigned flags = X86EMU_RUN_MAX_INSTR | X86EMU_RUN_NO_EXEC | X86EMU_RUN_NO_CODE | X86EMU_RUN_LOOP;
  int ok = 0;

  if(!file) return;

  ok = emu_init(emu, file);

  if(ok) {
    printf("***  running %s  ***\n\n", file);

    x86emu_dump(emu, X86EMU_DUMP_MEM | X86EMU_DUMP_ASCII);

    x86emu_reset_access_stats(emu);

    emu->max_instr = opt.max_instructions;
    x86emu_run(emu, flags);

    x86emu_dump(emu, X86EMU_DUMP_DEFAULT | X86EMU_DUMP_ACC_MEM);

    x86emu_clear_log(emu, 1);
  }

  x86emu_done(emu);
}

