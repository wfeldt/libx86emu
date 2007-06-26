/*
Linux Real Mode Interface - A library of DPMI-like functions for Linux.

Copyright (C) 1998 by Josh Vanderhoof

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL JOSH VANDERHOOF BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "lrmi.h"

#define REAL_MEM_BASE 	((void *)0x01000)
#define REAL_MEM_SIZE 	0xa0000
#define REAL_MEM_BLOCKS 	0x100

struct mem_block {
	unsigned int size : 20;
	unsigned int free : 1;
};

static struct {
	int ready;
	int count;
	struct mem_block blocks[REAL_MEM_BLOCKS];
} mem_info = { 0 };

static int
real_mem_init(void)
{
	void *m;
	int fd_zero;

	if (mem_info.ready)
		return 1;

	fd_zero = open("/dev/zero", O_RDWR);
	if (fd_zero == -1) {
		perror("open /dev/zero");
		return 0;
	}

	m = mmap((void *)REAL_MEM_BASE, REAL_MEM_SIZE,
	 PROT_READ | PROT_WRITE | PROT_EXEC,
	 MAP_FIXED | MAP_SHARED, fd_zero, 0);

	if (m == (void *)-1) {
		perror("mmap /dev/zero");
		close(fd_zero);
		return 0;
	}

	close(fd_zero);

	mem_info.ready = 1;
	mem_info.count = 1;
	mem_info.blocks[0].size = REAL_MEM_SIZE;
	mem_info.blocks[0].free = 1;

	return 1;
}

static void
real_mem_deinit(void)
{
	if (mem_info.ready) {
		munmap((void *)REAL_MEM_BASE, REAL_MEM_SIZE);
		mem_info.ready = 0;
	}
}


static void
insert_block(int i)
{
	memmove(
	 mem_info.blocks + i + 1,
	 mem_info.blocks + i,
	 (mem_info.count - i) * sizeof(struct mem_block));

	mem_info.count++;
}

static void
delete_block(int i)
{
	mem_info.count--;

	memmove(
	 mem_info.blocks + i,
	 mem_info.blocks + i + 1,
	 (mem_info.count - i) * sizeof(struct mem_block));
}

void *
LRMI_alloc_real(int size)
{
	int i;
	char *r = (char *)REAL_MEM_BASE;

	if (!mem_info.ready)
		return NULL;

	if (mem_info.count == REAL_MEM_BLOCKS)
		return NULL;

	size = (size + 15) & ~15;

	for (i = 0; i < mem_info.count; i++) {
		if (mem_info.blocks[i].free && size < mem_info.blocks[i].size) {
			insert_block(i);

			mem_info.blocks[i].size = size;
			mem_info.blocks[i].free = 0;
			mem_info.blocks[i + 1].size -= size;

			return (void *)r;
		}

		r += mem_info.blocks[i].size;
	}

	return NULL;
}


void
LRMI_free_real(void *m)
{
	int i;
	char *r = (char *)REAL_MEM_BASE;

	if (!mem_info.ready)
		return;

	i = 0;
	while (m != (void *)r) {
		r += mem_info.blocks[i].size;
		i++;
		if (i == mem_info.count)
			return;
	}

	mem_info.blocks[i].free = 1;

	if (i + 1 < mem_info.count && mem_info.blocks[i + 1].free) {
		mem_info.blocks[i].size += mem_info.blocks[i + 1].size;
		delete_block(i + 1);
	}

	if (i - 1 >= 0 && mem_info.blocks[i - 1].free) {
		mem_info.blocks[i - 1].size += mem_info.blocks[i].size;
		delete_block(i);
	}
}

#define DEFAULT_STACK_SIZE 	0x1000

static inline void
set_bit(unsigned int bit, void *array)
{
	unsigned char *a = array;

	a[bit / 8] |= (1 << (bit % 8));
}

static inline unsigned int
get_int_seg(int i)
{
	return *(unsigned short *)(i * 4 + 2);
}


static inline unsigned int
get_int_off(int i)
{
	return *(unsigned short *)(i * 4);
}

int LRMI_common_init(void)
{
	void *m;
	int fd_mem;

	if (!real_mem_init())
		return 0;

	/*
	 Map the Interrupt Vectors (0x0 - 0x400) + BIOS data (0x400 - 0x502)
	 and the ROM (0xa0000 - 0x100000)
	*/
	fd_mem = open("/dev/mem", O_RDWR);

	if (fd_mem == -1) {
		real_mem_deinit();
		perror("open /dev/mem");
		return 0;
	}

	m = mmap((void *)0, 0x502,
	 PROT_READ | PROT_WRITE | PROT_EXEC,
	 MAP_FIXED | MAP_SHARED, fd_mem, 0);

	if (m == (void *)-1) {
		close(fd_mem);
		real_mem_deinit();
		perror("mmap /dev/mem");
		return 0;
	}

	m = mmap((void *)0xa0000, 0x100000 - 0xa0000,
	 PROT_READ | PROT_WRITE,
	 MAP_FIXED | MAP_SHARED, fd_mem, 0xa0000);

	if (m == (void *)-1) {
		munmap((void *)0, 0x502);
		close(fd_mem);
		real_mem_deinit();
		perror("mmap /dev/mem");
		return 0;
	}

	close(fd_mem);

	return 1;
}
