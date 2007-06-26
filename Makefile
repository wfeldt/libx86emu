OBJECTS = 
CFLAGS = -O2 -Wall -DDEBUG -g
LIBDIR = /usr/lib
CC     = gcc

ifeq ($(BACKEND), x86emu)
	OBJECTS += x86emu/decode.o x86emu/debug.o x86emu/fpu.o \
	x86emu/ops.o x86emu/ops2.o x86emu/prim_ops.o x86emu/sys.o
else
	OBJECTS += lrmi.o
endif

ifeq ($(LIBRARY), shared)
	CFLAGS += -fPIC
endif

all:
	# $(MAKE) BACKEND=x86emu LIBRARY=shared shared
	# $(MAKE) objclean
	$(MAKE) BACKEND=x86emu LIBRARY=static static

default:
	$(MAKE) LIBRARY=static static
	$(MAKE) objclean
	$(MAKE) LIBRARY=shared shared

static: $(OBJECTS)
	$(AR) cru libx86.a $(OBJECTS)

shared: $(OBJECTS)
	$(CC) $(CFLAGS) -o libx86.so.1 -shared -Wl,-soname,libx86.so.1 $(OBJECTS)

objclean:
	$(MAKE) -C x86emu clean
	rm -f *.o *~

clean: objclean
	rm -f *.so.1 *.a

install: libx86.a
	# install -D libx86.so.1 $(DESTDIR)$(LIBDIR)/libx86.so.1
	install -D libx86.a $(DESTDIR)$(LIBDIR)/libx86.a
	ln -sf $(LIBDIR)/libx86.so.1 $(DESTDIR)$(LIBDIR)/libx86.so
	#install -D lrmi.h $(DESTDIR)/usr/include/libx86.h
	install -D x86emu/include/x86emu.h $(DESTDIR)/usr/include/libx86emu.h
