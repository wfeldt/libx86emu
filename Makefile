ARCH	:= $(shell uname -m)
ifneq ($(filter i386 i486 i586 i686, $(ARCH)),)
ARCH	:= i386
endif

ifdef LIBX86EMU_VERSION
VERSION := $(shell echo ${LIBX86EMU_VERSION} > VERSION; cat VERSION)
else
GIT2LOG := $(shell if [ -x ./git2log ] ; then echo ./git2log --update ; else echo true ; fi)
VERSION := $(shell $(GIT2LOG) --version VERSION ; cat VERSION)
endif
GITDEPS := $(shell [ -d .git ] && echo .git/HEAD .git/refs/heads .git/refs/tags)
BRANCH  := $(shell [ -d .git ] && git branch | perl -ne 'print $$_ if s/^\*\s*//')
PREFIX  := libx86emu-$(VERSION)

MAJOR_VERSION := $(shell cut -d . -f 1 VERSION)

CC	= gcc
CFLAGS	= -g -O2 -fPIC -fvisibility=hidden -fomit-frame-pointer -Wall
LDFLAGS =

LIBDIR = /usr/lib$(shell ldd /bin/sh | grep -q /lib64/ && echo 64)
LIBX86	= libx86emu

CFILES	= $(wildcard *.c)
OBJS	= $(CFILES:.c=.o)

LIB_NAME	= $(LIBX86).so.$(VERSION)
LIB_SONAME	= $(LIBX86).so.$(MAJOR_VERSION)

.PHONY: all shared install uninstall test demo clean

%.o: %.c
	$(CC) -c $(CFLAGS) $<

all: changelog shared

ifdef LIBX86EMU_VERSION
changelog: ;
else
changelog: $(GITDEPS)
	$(GIT2LOG) --changelog changelog
endif

shared: $(LIB_NAME)

install: shared
	install -D $(LIB_NAME) $(DESTDIR)$(LIBDIR)/$(LIB_NAME)
	ln -snf $(LIB_NAME) $(DESTDIR)$(LIBDIR)/$(LIB_SONAME)
	ln -snf $(LIB_SONAME) $(DESTDIR)$(LIBDIR)/$(LIBX86).so
	install -m 644 -D include/x86emu.h $(DESTDIR)/usr/include/x86emu.h

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_NAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_SONAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIBX86).so
	rm -f $(DESTDIR)/usr/include/x86emu.h

$(LIB_NAME): .depend $(OBJS)
	$(CC) -shared -Wl,-soname,$(LIB_SONAME) $(OBJS) -o $(LIB_NAME) $(LDFLAGS)
	@ln -snf $(LIB_NAME) $(LIB_SONAME)
	@ln -snf $(LIB_SONAME) $(LIBX86).so

test:
	make -C test

demo:
	make -C demo

archive: changelog
	@if [ ! -d .git ] ; then echo no git repo ; false ; fi
	mkdir -p package
	git archive --prefix=$(PREFIX)/ $(BRANCH) > package/$(PREFIX).tar
	tar -r -f package/$(PREFIX).tar --mode=0664 --owner=root --group=root --mtime="`git show -s --format=%ci`" --transform='s:^:$(PREFIX)/:' VERSION changelog
	xz -f package/$(PREFIX).tar

clean:
	make -C test clean
	make -C demo clean
	rm -f *.o *~ include/*~ *.so.* *.so .depend
	rm -rf package

ifneq "$(MAKECMDGOALS)" "clean"
.depend: $(CFILES)
	@$(CC) -MG -MM $(CFLAGS) $(CFILES) >$@
-include .depend
endif

