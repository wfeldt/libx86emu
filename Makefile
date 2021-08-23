include config.mk

.PHONY: all shared install uninstall test demo clean distclean

%.o: %.c
	$(CC) -c $(ALL_CFLAGS) $<

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
	install -m 644 -D include/x86emu.h $(DESTDIR)$(INCLUDEDIR)/x86emu.h
	if [ "$(OLDINCLUDEDIR)" -a "$(OLDINCLUDEDIR)" != "$(INCLUDEDIR)" ] ; then\
		install -m 644 -D include/x86emu.h $(DESTDIR)$(OLDINCLUDEDIR)/x86emu.h ;\
	fi

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_NAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_SONAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIBX86).so
	rm -f $(DESTDIR)$(INCLUDEDIR)/x86emu.h
	rm -f $(DESTDIR)$(OLDINCLUDEDIR)/x86emu.h

$(LIB_NAME): .depend $(OBJS)
	$(CC) -shared -Wl,-soname,$(LIB_SONAME) $(ALL_CFLAGS) $(OBJS) -o $(LIB_NAME) $(LDFLAGS)
	@ln -snf $(LIB_NAME) $(LIB_SONAME)
	@ln -snf $(LIB_SONAME) $(LIBX86).so

test:
	$(MAKE) -C test

demo:
	$(MAKE) -C demo

archive: changelog
	@if [ ! -d .git ] ; then echo no git repo ; false ; fi
	mkdir -p package
	git archive --prefix=$(ARCHIVE_PREFIX)/ $(BRANCH) > package/$(ARCHIVE_PREFIX).tar
	tar -r -f package/$(ARCHIVE_PREFIX).tar --mode=0664 --owner=root --group=root --mtime="`git show -s --format=%ci`" --transform='s:^:$(ARCHIVE_PREFIX)/:' VERSION changelog
	xz -f package/$(ARCHIVE_PREFIX).tar

clean:
	$(MAKE) -C test clean
	$(MAKE) -C demo clean
	rm -f *.o *~ include/*~ *.so.* *.so .depend
	rm -rf package

distclean: clean
	rm -f VERSION changelog

ifneq "$(MAKECMDGOALS)" "clean"
.depend: $(CFILES)
	@$(CC) -MG -MM $(ALL_CFLAGS) $(CFILES) >$@
-include .depend
endif
