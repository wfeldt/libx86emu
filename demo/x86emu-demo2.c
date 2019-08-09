/*
 * This is a simple program demonstrating the libx86emu usage.
 *
 * It lets you load an ELF which has been compiled for 32bit protected mode. The
 * emulation trace is logged to the console to show what it is doing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <x86emu.h>
#include <libelf.h>
#include <string.h>

void help(void);
void flush_log(x86emu_t *emu, char *buf, unsigned size);
x86emu_t *emu_new(void);
int emu_init(x86emu_t *emu, char *file);
void emu_run(char *file);


struct option options[] = {
  { "help",       0, NULL, 'h'  },
  { "load",       1, NULL, 'l'  },
  { "max",        1, NULL, 'm'  },
  { }
};


struct {
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

  opt.max_instructions = 10000000;

  opterr = 0;

  while((i = getopt_long(argc, argv, "hm:l:", options, NULL)) != -1) {
    switch(i) {
      case 'm':
        opt.max_instructions = strtoul(optarg, NULL, 0);
        break;

      case 'l':
        opt.load = strtoul(optarg, NULL, 0);
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
    "Usage: x86emu-demo2 [OPTIONS] <file>\n"
    "\n"
    "Load an ELF <file> and run x86 emulation.\n"
   "\n"
    "Options:\n"
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
  x86emu_t *emu = x86emu_new(X86EMU_PERM_R | X86EMU_PERM_W | X86EMU_PERM_X | X86EMU_PERM_VALID, 0);

  /* log buf size of 1000000 is purely arbitrary */
  x86emu_set_log(emu, 10000000, flush_log);

  emu->log.trace = X86EMU_TRACE_DEFAULT;

  return emu;
}


/*
 * Setup registers and memory.
 */
int emu_init(x86emu_t *emu, char *file)
{
  FILE *f;
  unsigned char *elf_start;
  unsigned addr;
  unsigned long size;
  int i;
  Elf32_Ehdr      *hdr     = NULL;
  Elf32_Phdr      *phdr    = NULL;

  // FIXME: Support relocations
  addr = opt.load;

  if(!(f = fopen(file, "r"))) return 0;

  fseek(f, 0, SEEK_END); // seek to end of file
  size = ftell(f); // get current file pointer
  fseek(f, 0, SEEK_SET); // seek back to beginning of file
  elf_start = malloc(size);
  if (!elf_start) { fclose(f); return 0; }

  if (fread (elf_start,1,size,f) != size) { fclose(f); return 0; }
  fclose(f);

  hdr = (Elf32_Ehdr *) elf_start;
  if (hdr->e_machine != EM_386) {
    fprintf(stderr, "ELF has wrong machine\n");
    return 0;
  }
  phdr = (Elf32_Phdr *)(elf_start + hdr->e_phoff);

  for(i=0; i < hdr->e_phnum; ++i) {
    unsigned long j;
    if(phdr[i].p_type != PT_LOAD) {
      continue;
    }
    if(phdr[i].p_filesz > phdr[i].p_memsz) {
      return 0;
    }
    if(!phdr[i].p_filesz) {
      continue;
    }

    // p_filesz can be smaller than p_memsz,
    // the difference is zeroe'd out.
    for (j = 0; j < phdr[i].p_filesz; j++)
      x86emu_write_byte(emu, phdr[i].p_vaddr + j + addr, elf_start[j + phdr[i].p_offset]);
    for (; j < phdr[i].p_memsz; j++)
      x86emu_write_byte(emu, phdr[i].p_vaddr + j + addr, 0);
  }

  emu->x86.R_CS = 0x10;
  emu->x86.R_CS_BASE = 0;
  emu->x86.R_EIP = hdr->e_entry;
  emu->x86.R_EFLG = 2;

  /* Start in protected mode */
  emu->x86.R_CS_LIMIT = 0xffffffff;
  emu->x86.R_DS_LIMIT = 0xffffffff;
  emu->x86.R_SS_LIMIT = 0xffffffff;
  emu->x86.R_ES_LIMIT = 0xffffffff;
  emu->x86.R_FS_LIMIT = 0xffffffff;
  emu->x86.R_GS_LIMIT = 0xffffffff;

  // resp. 0x4493/9b for protected mode
  emu->x86.R_CS_ACC = 0x449b;
  emu->x86.R_SS_ACC = 0x4493;
  emu->x86.R_DS_ACC = 0x4493;
  emu->x86.R_ES_ACC = 0x4493;
  emu->x86.R_FS_ACC = 0x4493;
  emu->x86.R_GS_ACC = 0x4493;

  emu->x86.R_GDT_LIMIT = 0xffffffff;
  emu->x86.R_IDT_LIMIT = 0xffffffff;
  emu->x86.R_CR0 = 1;

  free(elf_start);

  return 1;
}


/*
 * Run emulation.
 */
void emu_run(char *file)
{
  x86emu_t *emu = emu_new();
  unsigned flags = X86EMU_RUN_MAX_INSTR;
  int ok = 0;

  if(!file) return;

  ok = emu_init(emu, file);

  if(ok) {
    printf("***  running %s  ***\n\n", file);

    x86emu_dump(emu, X86EMU_DUMP_MEM | X86EMU_DUMP_ASCII);

    x86emu_reset_access_stats(emu);

    emu->max_instr = 10000;

    x86emu_run(emu, flags);

    x86emu_dump(emu, X86EMU_DUMP_DEFAULT | X86EMU_DUMP_ACC_MEM);

    x86emu_clear_log(emu, 1);
  }

  x86emu_done(emu);
}

