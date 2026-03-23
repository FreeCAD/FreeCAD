
$(top_builddir)/docs/coin.doxygen: $(top_srcdir)/docs/coin.doxygen.in $(top_builddir)/config.status
	@( cd $(top_builddir); $(MAKE) docs/coin.doxygen )

SoDB.3: $(top_builddir)/docs/coin.doxygen
	@( cd ../..; \
	echo " $(sim_ac_doxygen_exe) docs/coin.doxygen"; \
	"$(sim_ac_doxygen_exe)" docs/coin.doxygen )

filelist.txt: SoDB.3
	ls *.3 >filelist.txt


if BUILD_MANPAGES

all-local: filelist.txt

install-data-local: install-man

uninstall-local: uninstall-man

install-man: filelist.txt
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man3
	@list="`cat filelist.txt`"; \
	for i in $$list; do \
	  echo " $(INSTALL_DATA) $$i $(DESTDIR)$(mandir)/man3/$$i"; \
	  $(INSTALL_DATA) $$i $(DESTDIR)$(mandir)/man3/$$i; \
	done

uninstall-man: filelist.txt
	@$(NORMAL_UNINSTALL)
	@list="`cat filelist.txt`"; \
	for i in $$list; do \
	  echo " rm -f $(DESTDIR)$(mandir)/man3/$$i"; \
	  rm -f $(DESTDIR)$(mandir)/man3/$$i; \
	done
	@echo " rmdir $(DESTDIR)$(mandir)"; \
	rmdir $(DESTDIR)$(mandir) 2>/dev/null; \
	:

endif

# IRIX make barfs if clean-local is moved inside the conditional clause.
# Automake bug?  20011025 larsa
clean-local:
	rm -f filelist.txt *.3

