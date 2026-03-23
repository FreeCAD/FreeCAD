## Process this file with automake to generate Makefile.in.

SUBDIRS = Inventor

EXTRA_DIST = \
	SoWinEnterScope.h \
	SoWinLeaveScope.h \
	SoDebug.h

if BUILD_LIBRARY
if ! MAC_FRAMEWORK
thisincdir = $(includedir)
thisinc_HEADERS = \
	SoWinEnterScope.h \
	SoWinLeaveScope.h \
	SoDebug.h

install-thisincHEADERS: $(thisinc_HEADERS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(thisincdir)
	@list='$(thisinc_HEADERS)'; for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  f="`echo $$p | sed -e 's|^.*/||'`"; \
	  src="$$d$$p"; dst="$(DESTDIR)$(thisincdir)/$$f"; \
	  if cmp -s "$$src" "$$dst"; then :; else \
	    echo " $(INSTALL_HEADER) $$src $$dst"; \
	    $(INSTALL_HEADER) "$$src" "$$dst"; \
	  fi \
	done

endif
endif

