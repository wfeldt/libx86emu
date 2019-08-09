CC         = gcc
CFLAGS     = -g -Wall -fomit-frame-pointer -O2

.PHONY: clean

x86emu-demo: x86emu-demo.c
	$(CC) $(CFLAGS) $< -lx86emu -o $@

x86emu-demo2: x86emu-demo2.c
	$(CC) $(CFLAGS) $< -lx86emu -o $@

clean:
	rm -f *~ *.o x86emu-demo
