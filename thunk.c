#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/io.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHMERRORPTR (pointer)(-1)

#define _INT10_PRIVATE
#include "x86emu/include/xf86int10.h"
#include "x86emu/include/x86emu.h"
#include "x86emu/include/xf86x86emu.h"
#include "lrmi.h"
#include "x86-common.h"

#define DEBUG
#define ALLOC_ENTRIES(x) (V_RAM - 1)
#define TRUE 1
#define FALSE 0

#define __BUILDIO(bwl,bw,type) \
static inline void out##bwl##_local(unsigned long port, unsigned type value) {        __asm__ __volatile__("out" #bwl " %" #bw "0, %w1" : : "a"(value), "Nd"(port)); \
}\
static inline unsigned type in##bwl##_local(unsigned long port) { \
        unsigned type value; \
        __asm__ __volatile__("in" #bwl " %w1, %" #bw "0" : "=a"(value) : "Nd"(port)); \
        return value; \
}\

__BUILDIO(b,b,char)
__BUILDIO(w,w,short)
__BUILDIO(l,,int)


char *mmap_addr = SHMERRORPTR;
struct LRMI_regs *regs;
static void *stack;


void
printk(const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);

	fprintf(stderr, fmt, argptr);
	va_end(argptr);
}

u8 read_b(int addr) {
	return *((char *)mmap_addr + addr);
}

CARD8
x_inb(CARD16 port)
{
	CARD8 val;
	val = inb_local(port);
	return val;
}

CARD16
x_inw(CARD16 port)
{
	CARD16 val;
	val = inw_local(port);
	return val;
}

CARD32
x_inl(CARD16 port)
{
	CARD32 val;
	val = inl_local(port);
	return val;
}

void
x_outb(CARD16 port, CARD8 val)
{
        outb_local(port, val);
}

void
x_outw(CARD16 port, CARD16 val)
{
        outw_local(port, val);
}

void x_outl(CARD16 port, CARD32 val)
{
	outl_local(port, val);
}

void pushw(u16 val)
{
	X86_ESP -= 2;
	MEM_WW(((u32) X86_SS << 4) + X86_SP, val);
}

static void x86emu_do_int(int num)
{
	u32 eflags;
	
	/*	fprintf(stderr, "Calling INT 0x%X (%04X:%04X)\n", num,
			(read_b((num << 2) + 3) << 8) + read_b((num << 2) + 2),
			(read_b((num << 2) + 1) << 8) + read_b((num << 2)));
	
	fprintf(stderr, " EAX is %X\n", (int) X86_EAX);
	*/

	eflags = X86_EFLAGS;
	eflags = eflags | X86_IF_MASK;
	pushw(eflags);
	pushw(X86_CS);
	pushw(X86_IP);
	X86_EFLAGS = X86_EFLAGS & ~(X86_VIF_MASK | X86_TF_MASK);
	X86_CS = (read_b((num << 2) + 3) << 8) + read_b((num << 2) + 2);
	X86_IP = (read_b((num << 2) + 1) << 8) + read_b((num << 2));

	/*	fprintf(stderr, "Leaving interrupt call.\n"); */
}

int LRMI_init() {
	int i;
	X86EMU_intrFuncs intFuncs[256];

	if (!LRMI_common_init())
		return 0;

	mmap_addr = 0;
	
	X86EMU_pioFuncs pioFuncs = {
		(&x_inb),
		(&x_inw),
		(&x_inl),
		(&x_outb),
		(&x_outw),
		(&x_outl)
	};
	
	X86EMU_setupPioFuncs(&pioFuncs);

	for (i=0;i<256;i++)
		intFuncs[i] = x86emu_do_int;
	X86EMU_setupIntrFuncs(intFuncs);
	
	X86_EFLAGS = X86_IF_MASK | X86_IOPL_MASK;

	/*
	 * Allocate a 64k stack.
	 */
	stack = LRMI_alloc_real(64 * 1024);
	X86_SS = (unsigned int) stack >> 4;
	X86_ESP = 0xFFF9;
	memset (stack, 0, 64*1024);

	*((char *)0) = 0x4f; /* Make sure that we end up jumping back to a
				halt instruction */

	M.mem_base = 0;
	M.mem_size = 1024*1024;

	return 1;
}

int real_call(struct LRMI_regs *registers) {
        regs = registers;

        X86_EAX = registers->eax;
        X86_EBX = registers->ebx;
        X86_ECX = registers->ecx;
        X86_EDX = registers->edx;
        X86_ESI = registers->esi;
        X86_EDI = registers->edi;
        X86_EBP = registers->ebp;
        X86_EIP = registers->ip;
        X86_ES = registers->es;
        X86_FS = registers->fs;
        X86_GS = registers->gs;
        X86_CS = registers->cs;

        if (registers->ss != 0) {
                X86_SS = registers->ss;
        } else {
	        X86_SS = (unsigned int) stack >> 4;
	}

	if (registers->ds != 0) { 
		X86_DS = registers->ds;
	}

	if (registers->sp != 0) {
		X86_ESP = registers->sp;
	} else {
	  	X86_ESP = 0xFFF9;
	}
	  
	M.x86.debug |= DEBUG_DECODE_F;

	memset (stack, 0, 64*1024);

	X86EMU_exec();

	registers->eax = X86_EAX;
	registers->ebx = X86_EBX;
	registers->ecx = X86_ECX;
	registers->edx = X86_EDX;
	registers->esi = X86_ESI;
	registers->edi = X86_EDI;
	registers->ebp = X86_EBP;
	registers->es = X86_ES;

	return 1;
}

int LRMI_int(int num, struct LRMI_regs *registers) {
	u32 eflags;
	eflags = X86_EFLAGS;
	eflags = eflags | X86_IF_MASK;
	X86_EFLAGS = X86_EFLAGS  & ~(X86_VIF_MASK | X86_TF_MASK | X86_IF_MASK | X86_NT_MASK);

	registers->cs = (read_b((num << 2) + 3) << 8) + read_b((num << 2) + 2);
	registers->ip = (read_b((num << 2) + 1) << 8) + read_b((num << 2));
	regs = registers;
	return real_call(registers);
}

int LRMI_call(struct LRMI_regs *registers) {
	return real_call(registers);
}

size_t
LRMI_base_addr(void)
{
	return (size_t)mmap_addr;
}
