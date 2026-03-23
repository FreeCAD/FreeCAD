###########################################################################
##

if BUILD_LIBRARY
SRCDIRS = data src
else
SRCDIRS =
endif

if BUILD_HTMLPAGES
HTMLDIRS = html
else
HTMLDIRS =
endif

if MAC_FRAMEWORK
MANDIR =
else
MANDIR = man
endif

SUBDIRS = . bin include $(SRCDIRS) $(MANDIR) $(HTMLDIRS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= Coin.pc

BUILT_SOURCES = \
	include/Inventor/C/basic.h \
	include/Inventor/system/gl-headers.h \
	include/Inventor/system/inttypes.h \
	coin-@CONFIG@.cfg \
	Info.plist \
	version.plist

CLEANFILES = \
	coin@COIN_MAJOR_VERSION@.dsp \
	coin@COIN_MAJOR_VERSION@.dsw \
	coin@COIN_MAJOR_VERSION@.ncb \
	coin@COIN_MAJOR_VERSION@.opt \
	coin@COIN_MAJOR_VERSION@.plg

DISTCLEANFILES = \
	coin-@CONFIG@.cfg \
	Coin.pc \
	Info.plist \
	version.plist \
	docs/coin.doxygen \
	docs/coin.doxygen.bak

if BUILD_LIBRARY
m4datadir = $(datadir)/aclocal
dist_m4data_DATA = cfg/coin.m4

cfgdatadir = $(datadir)/Coin/conf
cfgdata_DATA = coin-@CONFIG@.cfg

if MAC_FRAMEWORK
frameworkdatadir = $(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Resources
frameworkdata_DATA = Info.plist version.plist
endif
endif

EXTRA_DIST = \
	FAQ \
	FAQ.legal \
	INSTALL.GNU \
	README.UNIX \
	README.MACOSX \
	README.WIN32 \
	RELNOTES \
	coin.cfg.in \
	Coin.pc.in \
	Info.plist.in \
	version.plist.in \
	data/shaders/glsl2h.sh.in \
	docs/ChangeLog.1999 \
	docs/ChangeLog.2000 \
	docs/ChangeLog.2001 \
	docs/ChangeLog.2002 \
	docs/ChangeLog.2003 \
	docs/ChangeLog.2004 \
	docs/ChangeLog.2005 \
	docs/ChangeLog.2006 \
	docs/ChangeLog.2007 \
	docs/ChangeLog.v3.0.0 \
	docs/ChangeLog.v3.1.0 \
	docs/ChangeLog.v3.1.1 \
	docs/ChangeLog.v3.1.2 \
	docs/ChangeLog.v4.0.0 \
	docs/HACKING \
	docs/announcement-2_0_0.txt \
	docs/announcement-2_0_1.txt \
	docs/announcement-2_0_2.txt \
	docs/announcement-2_1_0.txt \
	docs/announcement-2_2_0.txt \
	docs/announcement-2_2_1.txt \
	docs/announcement-2_3_0.txt \
	docs/announcement-2_4_0.txt \
	docs/announcement-2_4_1.txt \
	docs/announcement-2_4_2.txt \
	docs/announcement-2_4_3.txt \
	docs/announcement-2_4_4.txt \
	docs/announcement-2_4_5.txt \
	docs/announcement-2_4_6.txt \
	docs/announcement-2_5_0.txt \
	docs/announcement-3_0_0.txt \
	docs/announcement-3_1_0.txt \
	docs/announcement-3_1_1.txt \
	docs/announcement-3_1_2.txt \
	docs/autoexp.dat \
	docs/coin.doxygen.in \
	docs/oiki-launch.txt \
	docs/releases.dox \
	cfg/errors.txt \
	cfg/gendsp.pl.in \
	cfg/ltconfig \
	cfg/wrapmsvc.exe \
	cfg/csubst.exe \
	cfg/doxy4win.pl \
	docs/coin.doxygen.in \
	docs/oiki-launch.txt \
	docs/doxygen/header.html \
	docs/doxygen/footer.html \
	docs/doxygen/stylesheet.css \
	docs/doxygen/Coin_logo.png \
	packaging/debian/changelog.in \
	packaging/debian/control.in \
	packaging/debian/copyright.in \
	packaging/debian/rules.in \
	packaging/debian/libcoin40.files.in \
	packaging/debian/libcoin40.README.Debian.in \
	packaging/debian/libcoin40-dev.files.in \
	packaging/debian/libcoin40-dev.README.Debian.in \
	packaging/debian/libcoin40-doc.files.in \
	packaging/debian/libcoin40-doc.README.Debian.in \
	packaging/debian/libcoin40-runtime.files.in \
	packaging/debian/libcoin40-runtime.README.Debian.in \
	packaging/macosx/create-dmg \
	packaging/macosx/coin.info.template.in \
	packaging/macosx/checklist.txt \
	packaging/macosx/Coin_Description.plist.in \
	packaging/macosx/Coin_Info.plist.in \
	packaging/macosx/Coin_Welcome.rtf \
	packaging/macosx/CoinTools_Description.plist.in \
	packaging/macosx/CoinTools_Info.plist.in \
	packaging/macosx/CoinTools_Welcome.rtf \
	packaging/macosx/inst-dmg-files/.DS_Store \
	packaging/macosx/inst-dmg-files/.VolumeIcon.icns \
	packaging/macosx/inst-dmg-files/dmg_bg.tiff \
	packaging/macosx/inst-dmg-files/Desktop_DB \
	packaging/macosx/inst-dmg-files/Desktop_DF \
	packaging/macosx/License.rtf \
	packaging/macosx/makecoinpkg.sh.in \
	packaging/macosx/makecointoolspkg.sh.in \
	packaging/macosx/makedmg.sh.in \
	packaging/macosx/Makefile.in \
	packaging/macosx/makeinstdmg.sh.in \
	packaging/macosx/makenoinstdmg.sh.in \
	packaging/macosx/noinst-dmg-files/.DS_Store \
	packaging/macosx/noinst-dmg-files/.VolumeIcon.icns \
	packaging/macosx/noinst-dmg-files/dmg_bg.tiff \
	packaging/macosx/noinst-dmg-files/Desktop_DB \
	packaging/macosx/noinst-dmg-files/Desktop_DF \
	packaging/macosx/noinst-README-gcc3.txt.in \
	packaging/macosx/noinst-README-gcc4.txt.in \
	packaging/macosx/inst-README-gcc3.txt.in \
	packaging/macosx/inst-README-gcc4.txt.in \
	packaging/macosx/background.tiff \
	packaging/macosx/support/AdiumApplescriptRunner \
	packaging/macosx/support/template.applescript \
	packaging/redhat/coin.spec.in \
	examples/bindings/glutiv.cpp \
	build/misc/generate.sh \
	build/misc/build-docs.bat \
	build/misc/config-wrapper.h \
	build/misc/create-directories.bat \
	build/misc/install-data.bat \
	build/misc/install-headers.bat \
	build/misc/install-sdk.bat \
	build/misc/uninstall-headers.bat \
	build/misc/uninstall-sdk.bat \
	build/msvc6/data/draggerDefaults/centerballDragger.h \
	build/msvc6/data/draggerDefaults/directionalLightDragger.h \
	build/msvc6/data/draggerDefaults/dragPointDragger.h \
	build/msvc6/data/draggerDefaults/handleBoxDragger.h \
	build/msvc6/data/draggerDefaults/jackDragger.h \
	build/msvc6/data/draggerDefaults/pointLightDragger.h \
	build/msvc6/data/draggerDefaults/rotateCylindricalDragger.h \
	build/msvc6/data/draggerDefaults/rotateDiscDragger.h \
	build/msvc6/data/draggerDefaults/rotateSphericalDragger.h \
	build/msvc6/data/draggerDefaults/scale1Dragger.h \
	build/msvc6/data/draggerDefaults/scale2Dragger.h \
	build/msvc6/data/draggerDefaults/scale2UniformDragger.h \
	build/msvc6/data/draggerDefaults/scaleUniformDragger.h \
	build/msvc6/data/draggerDefaults/spotLightDragger.h \
	build/msvc6/data/draggerDefaults/tabBoxDragger.h \
	build/msvc6/data/draggerDefaults/tabPlaneDragger.h \
	build/msvc6/data/draggerDefaults/trackballDragger.h \
	build/msvc6/data/draggerDefaults/transformBoxDragger.h \
	build/msvc6/data/draggerDefaults/transformerDragger.h \
	build/msvc6/data/draggerDefaults/translate1Dragger.h \
	build/msvc6/data/draggerDefaults/translate2Dragger.h \
	build/msvc6/data/shaders/lights/DirectionalLight.h \
	build/msvc6/data/shaders/lights/PointLight.h \
	build/msvc6/data/shaders/lights/SpotLight.h \
	build/msvc6/data/shaders/lights/DirSpotLight.h \
	build/msvc6/data/shaders/vsm/VsmLookup.h \
	build/msvc6/docs/coin4.doxygen \
	build/msvc6/coin4.dsp \
	build/msvc6/coin4.dsw \
	build/msvc6/coin4_install.dsp \
	build/msvc6/coin4_uninstall.dsp \
	build/msvc6/coin4_docs.dsp \
	build/msvc6/include/Inventor/C/basic.h \
	build/msvc6/include/Inventor/system/gl-headers.h \
	build/msvc6/include/Inventor/system/inttypes.h \
	build/msvc6/src/config.h \
	build/msvc6/src/config-debug.h \
	build/msvc6/src/config-release.h \
	build/msvc6/src/setup.h \
	build/msvc7/data/draggerDefaults/centerballDragger.h \
	build/msvc7/data/draggerDefaults/directionalLightDragger.h \
	build/msvc7/data/draggerDefaults/dragPointDragger.h \
	build/msvc7/data/draggerDefaults/handleBoxDragger.h \
	build/msvc7/data/draggerDefaults/jackDragger.h \
	build/msvc7/data/draggerDefaults/pointLightDragger.h \
	build/msvc7/data/draggerDefaults/rotateCylindricalDragger.h \
	build/msvc7/data/draggerDefaults/rotateDiscDragger.h \
	build/msvc7/data/draggerDefaults/rotateSphericalDragger.h \
	build/msvc7/data/draggerDefaults/scale1Dragger.h \
	build/msvc7/data/draggerDefaults/scale2Dragger.h \
	build/msvc7/data/draggerDefaults/scale2UniformDragger.h \
	build/msvc7/data/draggerDefaults/scaleUniformDragger.h \
	build/msvc7/data/draggerDefaults/spotLightDragger.h \
	build/msvc7/data/draggerDefaults/tabBoxDragger.h \
	build/msvc7/data/draggerDefaults/tabPlaneDragger.h \
	build/msvc7/data/draggerDefaults/trackballDragger.h \
	build/msvc7/data/draggerDefaults/transformBoxDragger.h \
	build/msvc7/data/draggerDefaults/transformerDragger.h \
	build/msvc7/data/draggerDefaults/translate1Dragger.h \
	build/msvc7/data/draggerDefaults/translate2Dragger.h \
	build/msvc7/data/shaders/lights/DirectionalLight.h \
	build/msvc7/data/shaders/lights/PointLight.h \
	build/msvc7/data/shaders/lights/SpotLight.h \
	build/msvc7/data/shaders/lights/DirSpotLight.h \
	build/msvc7/data/shaders/vsm/VsmLookup.h \
	build/msvc7/docs/coin4.doxygen \
	build/msvc7/coin4.sln \
	build/msvc7/coin4.vcproj \
	build/msvc7/coin4_install.vcproj \
	build/msvc7/coin4_uninstall.vcproj \
	build/msvc7/coin4_docs.vcproj \
	build/msvc7/include/Inventor/C/basic.h \
	build/msvc7/include/Inventor/system/gl-headers.h \
	build/msvc7/include/Inventor/system/inttypes.h \
	build/msvc7/src/config.h \
	build/msvc7/src/config-debug.h \
	build/msvc7/src/config-release.h \
	build/msvc7/src/setup.h \
	build/msvc8/data/draggerDefaults/centerballDragger.h \
	build/msvc8/data/draggerDefaults/directionalLightDragger.h \
	build/msvc8/data/draggerDefaults/dragPointDragger.h \
	build/msvc8/data/draggerDefaults/handleBoxDragger.h \
	build/msvc8/data/draggerDefaults/jackDragger.h \
	build/msvc8/data/draggerDefaults/pointLightDragger.h \
	build/msvc8/data/draggerDefaults/rotateCylindricalDragger.h \
	build/msvc8/data/draggerDefaults/rotateDiscDragger.h \
	build/msvc8/data/draggerDefaults/rotateSphericalDragger.h \
	build/msvc8/data/draggerDefaults/scale1Dragger.h \
	build/msvc8/data/draggerDefaults/scale2Dragger.h \
	build/msvc8/data/draggerDefaults/scale2UniformDragger.h \
	build/msvc8/data/draggerDefaults/scaleUniformDragger.h \
	build/msvc8/data/draggerDefaults/spotLightDragger.h \
	build/msvc8/data/draggerDefaults/tabBoxDragger.h \
	build/msvc8/data/draggerDefaults/tabPlaneDragger.h \
	build/msvc8/data/draggerDefaults/trackballDragger.h \
	build/msvc8/data/draggerDefaults/transformBoxDragger.h \
	build/msvc8/data/draggerDefaults/transformerDragger.h \
	build/msvc8/data/draggerDefaults/translate1Dragger.h \
	build/msvc8/data/draggerDefaults/translate2Dragger.h \
	build/msvc8/data/shaders/lights/DirectionalLight.h \
	build/msvc8/data/shaders/lights/PointLight.h \
	build/msvc8/data/shaders/lights/SpotLight.h \
	build/msvc8/data/shaders/lights/DirSpotLight.h \
	build/msvc8/data/shaders/vsm/VsmLookup.h \
	build/msvc8/docs/coin4.doxygen \
	build/msvc8/coin4.sln \
	build/msvc8/coin4.vcproj \
	build/msvc8/coin4_install.vcproj \
	build/msvc8/coin4_uninstall.vcproj \
	build/msvc8/coin4_docs.vcproj \
	build/msvc8/include/Inventor/C/basic.h \
	build/msvc8/include/Inventor/system/gl-headers.h \
	build/msvc8/include/Inventor/system/inttypes.h \
	build/msvc8/src/config.h \
	build/msvc8/src/config-debug.h \
	build/msvc8/src/config-release.h \
	build/msvc8/src/setup.h \
	build/msvc9/data/draggerDefaults/centerballDragger.h \
	build/msvc9/data/draggerDefaults/directionalLightDragger.h \
	build/msvc9/data/draggerDefaults/dragPointDragger.h \
	build/msvc9/data/draggerDefaults/handleBoxDragger.h \
	build/msvc9/data/draggerDefaults/jackDragger.h \
	build/msvc9/data/draggerDefaults/pointLightDragger.h \
	build/msvc9/data/draggerDefaults/rotateCylindricalDragger.h \
	build/msvc9/data/draggerDefaults/rotateDiscDragger.h \
	build/msvc9/data/draggerDefaults/rotateSphericalDragger.h \
	build/msvc9/data/draggerDefaults/scale1Dragger.h \
	build/msvc9/data/draggerDefaults/scale2Dragger.h \
	build/msvc9/data/draggerDefaults/scale2UniformDragger.h \
	build/msvc9/data/draggerDefaults/scaleUniformDragger.h \
	build/msvc9/data/draggerDefaults/spotLightDragger.h \
	build/msvc9/data/draggerDefaults/tabBoxDragger.h \
	build/msvc9/data/draggerDefaults/tabPlaneDragger.h \
	build/msvc9/data/draggerDefaults/trackballDragger.h \
	build/msvc9/data/draggerDefaults/transformBoxDragger.h \
	build/msvc9/data/draggerDefaults/transformerDragger.h \
	build/msvc9/data/draggerDefaults/translate1Dragger.h \
	build/msvc9/data/draggerDefaults/translate2Dragger.h \
	build/msvc9/data/shaders/lights/DirectionalLight.h \
	build/msvc9/data/shaders/lights/PointLight.h \
	build/msvc9/data/shaders/lights/SpotLight.h \
	build/msvc9/data/shaders/lights/DirSpotLight.h \
	build/msvc9/data/shaders/vsm/VsmLookup.h \
	build/msvc9/docs/coin4.doxygen \
	build/msvc9/coin4.sln \
	build/msvc9/coin4.vcproj \
	build/msvc9/coin4_install.vcproj \
	build/msvc9/coin4_uninstall.vcproj \
	build/msvc9/coin4_docs.vcproj \
	build/msvc9/include/Inventor/C/basic.h \
	build/msvc9/include/Inventor/system/gl-headers.h \
	build/msvc9/include/Inventor/system/inttypes.h \
	build/msvc9/src/config.h \
	build/msvc9/src/config-debug.h \
	build/msvc9/src/config-release.h \
	build/msvc9/src/setup.h \
	testsuite/Makefile.in \
	testsuite/StandardTests.cpp \
	testsuite/TestSuiteInit.cpp \
	testsuite/TestSuiteUtils.h \
	testsuite/TestSuiteUtils.cpp \
	testsuite/TestSuiteMisc.h \
	testsuite/TestSuiteMisc.cpp \
	testsuite/makeextract.sh \
	testsuite/makemakefile.sh \
	testsuite/README

# FIXME: These files should really be added instead of the ones
#        containing '_' instead of ' '. If this is fixed, also
#        update packaging/macosx/make*dmg.sh.in (kintel 20040119)
#	packaging/macosx/inst-dmg-files/Desktop DB \
#	packaging/macosx/inst-dmg-files/Desktop DF \
#	packaging/macosx/noinst-dmg-files/Desktop DB \
#	packaging/macosx/noinst-dmg-files/Desktop DF \
#

coin-@CONFIG@.cfg: config.status $(srcdir)/coin.cfg.in
	@$(SHELL) config.status --file=$@:coin.cfg.in

Coin.pc: config.status $(srcdir)/Coin.pc.in
	@$(SHELL) config.status --file=$@:Coin.pc.in

Info.plist: config.status $(srcdir)/Info.plist.in
	@$(SHELL) config.status --file=$@:Info.plist.in

version.plist: config.status $(srcdir)/version.plist.in
	@$(SHELL) config.status --file=$@:version.plist.in

# sources needed to generate docs must be created by this rule
built-sources: $(BUILT_SOURCES)

include/Inventor/C/basic.h: $(srcdir)/include/Inventor/C/basic.h.in config.status
	@./config.status --header=$@

include/Inventor/system/gl-headers.h: $(srcdir)/include/Inventor/system/gl-headers.h.in config.status
	@./config.status --file=include/Inventor/system/gl-headers.h.new:include/Inventor/system/gl-headers.h.in; \
	if cmp -s include/Inventor/system/gl-headers.h include/Inventor/system/gl-headers.h.new; then \
	  echo "include/Inventor/system/gl-headers.h is unchanged"; \
	  rm -f include/Inventor/system/gl-headers.h.new; \
	else \
	  mv include/Inventor/system/gl-headers.h.new include/Inventor/system/gl-headers.h; \
	fi

include/Inventor/system/inttypes.h: $(srcdir)/include/Inventor/system/inttypes.h.in config.status
	@./config.status --header=$@

docs/coin.doxygen: $(srcdir)/docs/coin.doxygen.in config.status
	@if test -d docs; then :; else mkdir docs; fi
	@./config.status --file=$@:$@.in
	@$(srcdir)/cfg/doxy4win.pl docs/coin.doxygen

doxygen-doc: built-sources
	if test x"@COIN_DOC_HTML@" = x"YES"; then \
	  mkdir -p "@coin_html_dir@"; \
	else :; fi
	if test x"@COIN_DOC_MAN@" = x"YES"; then \
	  mkdir -p "@coin_man_dir@"; \
	else :; fi
	@sim_ac_doxygen_exe@ $(top_builddir)/docs/coin.doxygen

doxygen-docs: built-sources
	if test x"@COIN_DOC_HTML@" = x"YES"; then \
	  mkdir -p "@coin_html_dir@"; \
	else :; fi
	if test x"@COIN_DOC_MAN@" = x"YES"; then \
	  mkdir -p "@coin_man_dir@"; \
	else :; fi
	@sim_ac_doxygen_exe@ $(top_builddir)/docs/coin.doxygen

if BUILD_LIBRARY
if MAC_FRAMEWORK
install-exec-local:
install-data-local:
	@$(NORMAL_INSTALL)
	@echo "Setting up Inventor.framework structure (Mac OS X)"
	@if test -d "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)"; then :; else \
	  echo " $(mkinstalldirs) $(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)"; \
	  $(mkinstalldirs) "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)"; \
	  echo " $(mkinstalldirs) $(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Resources"; \
	  $(mkinstalldirs) "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Resources"; \
	  echo " $(mkinstalldirs) $(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Resources/include"; \
	  $(mkinstalldirs) "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Resources/include"; \
	  echo " $(mkinstalldirs) $(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Headers"; \
	  $(mkinstalldirs) "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Headers"; \
	fi
	@if test -e "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/Current"; then \
	  echo " rm -f $(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/Current"; \
	  rm -f "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/Current"; \
	fi; \
	echo " ln -s $(MAC_FRAMEWORK_VERSION) $(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/Current"; \
	ln -s $(MAC_FRAMEWORK_VERSION) "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/Current";
	@for symlink in Headers Resources Libraries; do \
	  if test -L "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/$$symlink"; then \
	    echo " rm -f $(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/$$symlink"; \
	    rm -f "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/$$symlink"; \
	  fi; \
	  echo " ln -s Versions/Current/$$symlink $(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/$$symlink"; \
	  ln -s Versions/Current/$$symlink "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/$$symlink" || exit 1; \
	done
  # Additional symlink to allow non-framework (-I) includes.
	@echo "cd $(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Resources/include && rm -f $(MAC_FRAMEWORK_NAME) && ln -s ../../Headers $(MAC_FRAMEWORK_NAME) && cd -"; \
	cd "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Resources/include" && rm -f $(MAC_FRAMEWORK_NAME) && ln -s ../../Headers $(MAC_FRAMEWORK_NAME) && cd -;
  # Link to SoQt.framework to allow <Inventor/Qt/xxx.h> includes. 
	cd "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Headers" && rm -f Qt && ln -s ../../../../SoQt.framework/Headers Qt && cd -;
else
install-data-local:
install-exec-local: $(srcdir)/cfg/wrapmsvc.exe
	@$(NORMAL_INSTALL)
	@if @BUILD_WITH_MSVC@; then \
	  echo " $(mkinstalldirs) $(DESTDIR)$(bindir)"; \
	  $(mkinstalldirs) $(DESTDIR)$(bindir); \
	  p=wrapmsvc.exe; \
	  echo " $(INSTALL_PROGRAM) $(srcdir)/cfg/$$p $(DESTDIR)$(bindir)/$$p"; \
	  $(INSTALL_PROGRAM) $(srcdir)/cfg/$$p $(DESTDIR)$(bindir)/$$p; \
	else :; fi
endif
endif

install-symlinks: install
	@(cd src; $(MAKE) install-symlinks;) 

svn-update:
	@cd $(top_srcdir) && svn update

testsuite-run:
	@$(MAKE) -C testsuite

# **************************************************************************
# misc rules for automatic debian packaging.  main: `debian-packages'

if DEB_PACKAGEABLE

coin-$(VERSION):
	if test -d coin-$(VERSION); then \
	  rm -rf coin-$(VERSION); \
	else :; fi;
	$(MAKE) distdir;
	mv Coin-$(VERSION) coin-$(VERSION)

coin_$(VERSION).orig.tar.gz:
	$(MAKE) dist
	mv Coin-$(VERSION).tar.gz coin_$(VERSION).orig.tar.gz

debian-files: $(DEBIAN_DIST_DIR)
	test -d $(DEBIAN_DIST_DIR)/debian || mkdir $(DEBIAN_DIST_DIR)/debian;
	for f in packaging/debian/*.in; do \
	  file=`echo $$f | sed 's/.*\///g; s/\.in$$//g'`; \
	  ./config.status --file=$(DEBIAN_DIST_DIR)/debian/$$file:$$f; \
	done;
	for f in rules preinst prerm postinst postrm; do \
	  if test -f $(DEBIAN_DIST_DIR)/debian/$$f; then \
	    echo " chmod +x $(DEBIAN_DIST_DIR)/debian/$$f"; \
	    chmod +x $(DEBIAN_DIST_DIR)/debian/$$f; \
	  fi; \
	done;

debian-control:
	distdir=coin-$(VERSION); \
	$(MAKE) DEBIAN_DIST_DIR=$$distdir debian-files;

debian-setup: coin-$(VERSION) debian-control

debian-packages: debian-setup coin_$(VERSION).orig.tar.gz
	@( cd coin-$(VERSION); dpkg-buildpackage )

endif

# **************************************************************************
# misc rules for automatic Microsoft Windows packaging.
# main: `windows-packages'

if WIN_PACKAGEABLE

windows-files:
	@if test -d windows; then :; else mkdir windows; fi
	@for file in $(top_srcdir)/packaging/windows/*.in; do \
	  basename=`echo $$file | sed 's/^.*\///g; s/\.in$$//'`; \
	  ./config.status --file=windows/$$basename:$$file; \
	done

windows-packages: windows-files
	@( cd windows; $(MAKE) VERSION=$(VERSION) )

endif

# **************************************************************************
# misc rules for automatic Mac OS X packaging.
# main: `macosx-packages'

if MACOSX_PACKAGEABLE

macosx-files:
	@if test -d macosx; then :; else mkdir macosx; fi
	@for file in $(top_srcdir)/packaging/macosx/*.in; do \
	  basename=`basename $$file .in`; \
          if ! test -f macosx/$$basename; then \
	    ./config.status --file=macosx/$$basename:$$file; \
          fi \
	done

macosx-packages: macosx-files
macosx-packages:
	@( cd macosx; $(MAKE) VERSION=$(VERSION) )

endif

# EOF ######################################################################
