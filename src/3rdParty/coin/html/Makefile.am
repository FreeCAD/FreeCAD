# NOTES:
#   This Makefile.am relies on $(htmldir) being AC_SUBST'ed into place...

if BUILD_HTMLPAGES

# **************************************************************************
# build trickery

$(top_builddir)/docs/coin.doxygen: $(top_srcdir)/docs/coin.doxygen.in $(top_builddir)/config.status
	@( cd $(top_builddir); $(MAKE) docs/coin.doxygen )

index.html: $(top_builddir)/docs/coin.doxygen $(DOXYGEN_INPUT)
	@( cd ..; \
	echo " $(sim_ac_doxygen_exe) docs/coin.doxygen"; \
	"$(sim_ac_doxygen_exe)" docs/coin.doxygen )

filelist.txt: index.html
	@if cmp -s $(top_srcdir)/docs/doxygen/Coin_logo.png Coin_logo.png; then :; else \
	  echo  " cp $(top_srcdir)/docs/doxygen/Coin_logo.png ."; \
	  cp $(top_srcdir)/docs/doxygen/Coin_logo.png .; \
	fi
	@ls -1 | egrep '(\.html|\.png|\.gif|\.css|\.tag)$$' >filelist.txt

all-local: filelist.txt

# **************************************************************************
# install trickery
# - uninstall after "make clean" will regenerate docs - ought to be fixed

install-data-local: install-html

uninstall-local: uninstall-html

if MAC_FRAMEWORK
install-html: filelist.txt
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Documentation"
	@echo " ln -sf Versions/Current/Documentation $(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Documentation"; \
	ln -sf Versions/Current/Documentation "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Documentation"
	@destdir="$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Documentation"; \
	list="`cat filelist.txt`"; \
	for file in $$list; do \
	  echo " $(INSTALL_DATA) $$file $(DESTDIR)$$destdir/$$file"; \
	  $(INSTALL_DATA) $$file "$(DESTDIR)$$destdir/$$file"; \
	done

uninstall-html: filelist.txt
	@$(NORMAL_UNINSTALL)
	@destdir="$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Documentation"; \
	list="`cat filelist.txt`"; \
	for file in $$list; do \
	  echo " rm -f $(DESTDIR)$(destdir)/$$file"; \
	  rm -f "$(DESTDIR)$(destdir)/$$file"; \
	done
	@echo " rmdir $(DESTDIR)$(destdir)"; \
	rmdir $(DESTDIR)$(destdir) 2>/dev/null; \
	:
else
install-html: filelist.txt
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(htmldir)
	@list="`cat filelist.txt`"; \
	for file in $$list; do \
	  echo " $(INSTALL_DATA) $$file $(DESTDIR)$(htmldir)/$$file"; \
	  $(INSTALL_DATA) $$file "$(DESTDIR)$(htmldir)/$$file"; \
	done

uninstall-html: filelist.txt
	@$(NORMAL_UNINSTALL)
	@list="`cat filelist.txt`"; \
	for file in $$list; do \
	  echo " rm -f $(DESTDIR)$(htmldir)/$$file"; \
	  rm -f "$(DESTDIR)$(htmldir)/$$file"; \
	done
	@echo " rmdir $(DESTDIR)$(htmldir)"; \
	rmdir $(DESTDIR)$(htmldir) 2>/dev/null; \
	:
endif

endif

# IRIX make barfs if clean-local is moved inside the conditional clause.
# Automake bug?  20011025 larsa
clean-local:
	@if test -f filelist.txt; then \
	  cat filelist.txt | xargs rm -f filelist.txt; \
	fi

