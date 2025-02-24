SHELL = /bin/sh
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
ARCHIVE_PREFIX  := libx86emu-$(VERSION)

PREFIX ?= /usr/local
EXEC_PREFIX ?= $(PREFIX)
ifeq "$(ARCH)" "x86_64"
LIBDIR ?= $(EXEC_PREFIX)/lib64
else
LIBDIR ?= $(EXEC_PREFIX)/lib
endif
INCLUDEDIR ?= $(PREFIX)/include
OLDINCLUDEDIR ?= /usr/include

MAJOR_VERSION := $(shell cut -d . -f 1 VERSION)

CC	?= gcc
CFLAGS	?= -g -O2 -Wall -fomit-frame-pointer
ALL_CFLAGS	= -fPIC -fvisibility=hidden $(CFLAGS)

export CC CFLAGS ARCH

LIBX86	= libx86emu

CFILES	= $(wildcard *.c)
OBJS	= $(CFILES:.c=.o)

LIB_NAME	= $(LIBX86).so.$(VERSION)
LIB_SONAME	= $(LIBX86).so.$(MAJOR_VERSION)
