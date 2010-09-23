ARCH	:= $(shell uname -m)
ifneq ($(filter i386 i486 i586 i686, $(ARCH)),)
ARCH	:= i386
endif

GIT2LOG = ./git2log --update

CC	= gcc
CFLAGS	= -g -O2 -fPIC -fomit-frame-pointer -Wall
ifneq ($(filter x86_64, $(ARCH)),)
LIBDIR	= /usr/lib64
else
LIBDIR	= /usr/lib
endif
LIBX86	= libx86emu

define VERSION
$(shell $(GIT2LOG) --version VERSION ; cat VERSION)
endef

define MAJOR_VERSION
$(shell $(GIT2LOG) --version VERSION ; cut -d . -f 1 VERSION)
endef

define MINOR_VERSION
$(shell $(GIT2LOG) --version VERSION ; cut -d . -f 2 VERSION)
endef

CFILES	= $(wildcard *.c)
OBJS	= $(CFILES:.c=.o)

LIB_NAME	= $(LIBX86).so.$(VERSION)
LIB_SONAME	= $(LIBX86).so.$(MAJOR_VERSION)

.PHONY: all shared install test clean

%.o: %.c
	$(CC) -c $(CFLAGS) $<

all: changelog shared

changelog: .git/HEAD .git/refs/heads .git/refs/tags
	$(GIT2LOG) --changelog changelog

shared: $(LIB_NAME)

install: shared
	install -D $(LIB_NAME) $(DESTDIR)$(LIBDIR)/$(LIB_NAME)
	ln -snf $(LIB_NAME) $(DESTDIR)$(LIBDIR)/$(LIB_SONAME)
	ln -snf $(LIB_SONAME) $(DESTDIR)$(LIBDIR)/$(LIBX86).so
	install -m 644 -D include/x86emu.h $(DESTDIR)/usr/include/x86emu.h

$(LIB_NAME): .depend $(OBJS)
	$(CC) -shared -Wl,-soname,$(LIB_SONAME) $(OBJS) -o $(LIB_NAME)

test:
	make -C test

clean:
	make -C test clean
	rm -f *.o *~ include/*~ *.so.* .depend

ifneq "$(MAKECMDGOALS)" "clean"
.depend: $(CFILES)
	@$(CC) -MG -MM $(CFLAGS) $(CFILES) >$@
-include .depend
endif

