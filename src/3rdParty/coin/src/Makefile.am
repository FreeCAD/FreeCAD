# **************************************************************************

CLEANFILES = \
	coin@COIN_MAJOR_VERSION@@SUFFIX@.pdb \
	coin@COIN_MAJOR_VERSION@@SUFFIX@.dll \
	coin@COIN_MAJOR_VERSION@@SUFFIX@.exp \
	vc*.pdb


# **************************************************************************
# Extensions which might be present.

noinst_HEADERS = \
        unconfig.h \
        tidbitsp.h \
        coindefs.h \
        SbBasicP.h

EXTRADIST = \
        update-config.sh \
        Make-Common.tpl

ExtraIncludeDirs = -I$(top_srcdir)/include/Inventor/annex -I$(top_builddir)

if BUILD_WITH_SUPERGLU
SUPERGLUDIR = extensions/superglu
SUPERGLULIBADD = $(SUPERGLUDIR)/lib${SUPERGLUPREFIX}GLU.la
SUPERGLULSTADD = $(SUPERGLUDIR)/${SUPERGLUPREFIX}GLU.lst
else
SUPERGLUDIR =
SUPERGLULIBADD =
SUPERGLULSTADD =
endif

# **************************************************************************

if MAC_FRAMEWORK
SUBDIRS =
else
SUBDIRS = \
	extensions xml actions base bundles caches details draggers elements \
	engines errors events fields fonts glue io manips misc rendering lists nodekits \
	navigation nodes shapenodes projectors sensors upgraders \
	3ds collision hardcopy shadows geo threads shaders profiler \
	vrml97 foreignfiles scxml soscxml doc .
endif

DEFAULT_INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_builddir)/src -I$(top_srcdir)/src $(ExtraIncludeDirs)

# **************************************************************************

if BUILD_WITH_MSVC
lib_LIBRARIES = coin@COIN_MAJOR_VERSION@@SUFFIX@.lib
else
lib_LTLIBRARIES = libCoin@SUFFIX@.la
endif

if MAC_FRAMEWORK
# hack to change libdir value when installing Mac OS X compiler framework
lib@nop@dir = $(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/Libraries
endif

# **************************************************************************

if MAC_FRAMEWORK
libCoin@SUFFIX@_la_LIBADD = 
else
if HACKING_DYNAMIC_MODULES
libCoin@SUFFIX@_la_LIBADD = 
else
libCoin@SUFFIX@_la_LIBADD = actions/libactions.la base/libbase.la       \
	bundles/libbundles.la caches/libcaches.la                       \
	elements/libelements.la elements/GL/libglelements.la            \
	engines/libengines.la errors/liberrors.la events/libevents.la   \
	fields/libfields.la fonts/libfonts.la glue/libglue.la           \
	io/libio.la misc/libmisc.la rendering/librendering.la           \
	lists/liblists.la nodekits/libnodekits.la nodes/libnodes.la     \
	shapenodes/libshapenodes.la projectors/libprojectors.la         \
	sensors/libsensors.la details/libdetails.la                     \
	draggers/libdraggers.la manips/libmanips.la                     \
	upgraders/libupgraders.la navigation/libnavigation.la           \
	3ds/libformat3ds.la collision/libcollision.la                   \
	hardcopy/libhardcopy.la threads/libthreads.la                   \
	shaders/libshaders.la shadows/libshadows.la geo/libgeo.la       \
	foreignfiles/libforeignfiles.la xml/libxml.la                   \
	xml/expat/libexpat.la profiler/libprofiler.la                   \
	vrml97/libvrml97.la scxml/libscxml.la soscxml/libsoscxml.la $(SUPERGLULIBADD)
endif
endif

# **************************************************************************
# Source code files for local build & dist.

if MAC_FRAMEWORK
RegularSources = \
	actions/all-actions-cpp.cpp \
	base/all-base-cpp.cpp \
	bundles/all-bundles-cpp.cpp \
	caches/all-caches-cpp.cpp \
	elements/all-elements-cpp.cpp \
	elements/GL/all-glelements-cpp.cpp \
	engines/all-engines-cpp.cpp \
	engines/all-engines-c.c \
	errors/all-errors-cpp.cpp \
	events/all-events-cpp.cpp \
	fields/all-fields-cpp.cpp \
	fonts/all-fonts-cpp.cpp \
	glue/all-glue-cpp.cpp \
	io/all-io-cpp.cpp \
	misc/all-misc-cpp.cpp \
	rendering/all-rendering-cpp.cpp \
	lists/all-lists-cpp.cpp \
	nodekits/all-nodekits-cpp.cpp \
	nodes/all-nodes-cpp.cpp \
	shapenodes/all-shapenodes-cpp.cpp \
	projectors/all-projectors-cpp.cpp \
	sensors/all-sensors-cpp.cpp \
	details/all-details-cpp.cpp \
	draggers/all-draggers-cpp.cpp \
	manips/all-manips-cpp.cpp \
	upgraders/all-upgraders-cpp.cpp \
	navigation/all-navigation-cpp.cpp \
	3ds/all-3ds-cpp.cpp \
	collision/all-collision-cpp.cpp \
	hardcopy/all-hardcopy-cpp.cpp \
	threads/all-threads-cpp.cpp \
	shaders/all-shaders-cpp.cpp \
	shadows/all-shadows-cpp.cpp \
	geo/all-geo-cpp.cpp \
	foreignfiles/all-foreignfiles-cpp.cpp \
	xml/all-xml-cpp.cpp \
	xml/expat/all-expat-c.c \
	profiler/all-profiler-cpp.cpp \
	vrml97/all-vrml97-cpp.cpp \
	scxml/all-scxml-cpp.cpp \
	soscxml/all-soscxml-cpp.cpp \
	tidbits.cpp
else
RegularSources = \
	tidbits.cpp
endif

PrivateHeaders =

libCoin@SUFFIX@_la_SOURCES = $(RegularSources)
coin@COIN_MAJOR_VERSION@@SUFFIX@_lib_SOURCES = $(RegularSources)

EXTRA_libCoin@SUFFIX@_la_SOURCES = $(PrivateHeaders)
EXTRA_coin@COIN_MAJOR_VERSION@@SUFFIX@_lib_SOURCES = $(PrivateHeaders)

# **************************************************************************

coin@COIN_MAJOR_VERSION@@SUFFIX@_lib_LIBADD = actions/actions.lst       \
	base/base.lst bundles/bundles.lst caches/caches.lst             \
	elements/elements.lst elements/GL/glelements.lst                \
	engines/engines.lst errors/errors.lst events/events.lst         \
	fields/fields.lst fonts/fonts.lst glue/glue.lst io/io.lst       \
	misc/misc.lst rendering/rendering.lst lists/lists.lst           \
	nodekits/nodekits.lst nodes/nodes.lst                           \
	shapenodes/shapenodes.lst projectors/projectors.lst             \
	sensors/sensors.lst navigation/navigation.lst                   \
	details/details.lst draggers/draggers.lst manips/manips.lst     \
	upgraders/upgraders.lst 3ds/format3ds.lst                       \
	collision/collision.lst hardcopy/hardcopy.lst                   \
	threads/threads.lst shaders/shaders.lst shadows/shadows.lst     \
	geo/geo.lst foreignfiles/foreignfiles.lst xml/xml.lst           \
	xml/expat/expat.lst profiler/profiler.lst vrml97/vrml97.lst     \
	scxml/scxml.lst soscxml/soscxml.lst $(SUPERGLULSTADD)

# **************************************************************************
# Installation information.

# On Mac OS 10.2 or later, create two-level namespace binaries and use
# prebinding. Also pad the install_name to make it possible to change
# it later (e.g. to include the library in an app bundle).  On Mac OS
# 10.3 or later, use -undefined dynamic_lookup to delay symbol lookup
# to runtime (needed to make enable_hacking work)

if MACOSX_10_2FF
if MACOSX_USE_DYNAMIC_LOOKUP
# Must export deployment target environment, since it is required by
# the linker to allow the -undefined dynamic_lookup flag.
export MACOSX_DEPLOYMENT_TARGET=@MACOSX_DEPLOYMENT_TARGET@
LibtoolLinkArgs = -no-undefined -Wl,-twolevel_namespace -Wl,-undefined,dynamic_lookup -Wl,-headerpad_max_install_names 
else
LibtoolLinkArgs = -no-undefined -Wl,-twolevel_namespace -Wl,-undefined,error -Wl,-prebind -Wl,-seg1addr,0x3f000000 -Wl,-headerpad_max_install_names 
endif
else
LibtoolLinkArgs = -no-undefined
endif

libCoin@SUFFIX@_la_LDFLAGS = \
	$(LibtoolLinkArgs) \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

# **************************************************************************
# Custom rule for linking a Visual C++ (MS Windows) library.

coin@COIN_MAJOR_VERSION@@SUFFIX@.lib: Makefile Coin.RES \
  $(coin@COIN_MAJOR_VERSION@@SUFFIX@_lib_OBJECTS) \
  $(coin@COIN_MAJOR_VERSION@@SUFFIX@_lib_DEPENDENCIES)
	@basename="coin@COIN_MAJOR_VERSION@@SUFFIX@"; \
	rm -f "$$basename.dll" "$$basename.lib"; \
	echo "Linking $$basename.lib..."; \
	echo $(CC) $(LDFLAGS) $(LIBFLAGS) \
	  $(coin@COIN_MAJOR_VERSION@@SUFFIX@_lib_OBJECTS) \
	  $(coin@COIN_MAJOR_VERSION@@SUFFIX@_lib_LIBADD) $(LIBS) Coin.RES; \
	$(CC) $(LDFLAGS) $(LIBFLAGS) \
	  $(coin@COIN_MAJOR_VERSION@@SUFFIX@_lib_OBJECTS) \
	  $(coin@COIN_MAJOR_VERSION@@SUFFIX@_lib_LIBADD) $(LIBS) Coin.RES ;

# **************************************************************************

if MAC_FRAMEWORK
install-data-local: install-exec-am
	cd $(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/Versions/Current && ln -sf Libraries/libCoin.dylib $(MAC_FRAMEWORK_NAME) && cd -; \
	cd $(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework && ln -sf Versions/Current/$(MAC_FRAMEWORK_NAME) $(MAC_FRAMEWORK_NAME) && install_name_tool -id $(MAC_FRAMEWORK_NAME).framework/Versions/$(MAC_FRAMEWORK_VERSION)/$(MAC_FRAMEWORK_NAME) $(MAC_FRAMEWORK_NAME) && cd -;
else
install-data-local:
	@basename="coin@COIN_MAJOR_VERSION@@SUFFIX@"; \
	for ext in lib; do \
	  file="$$basename.$$ext"; \
	  if test -f "$$file"; then \
	    $(mkinstalldirs) "$(DESTDIR)$(libdir)"; \
	    echo " $(INSTALL_DATA) $$file $(DESTDIR)$(libdir)/$$file"; \
	    $(INSTALL_DATA) "$$file" "$(DESTDIR)$(libdir)/$$file" || exit 1; \
	  fi \
	done;
endif

if BUILD_WITH_MSVC
install-exec-local: coin@COIN_MAJOR_VERSION@@SUFFIX@.lib
	@basename="coin@COIN_MAJOR_VERSION@@SUFFIX@"; \
	for ext in dll pdb; do \
	  file="$$basename.$$ext"; \
	  if test -f "$$file"; then \
	    $(mkinstalldirs) "$(DESTDIR)$(bindir)"; \
	    echo " $(INSTALL_PROGRAM) $$file $(DESTDIR)$(bindir)/$$file"; \
	    $(INSTALL_PROGRAM) "$$file" "$(DESTDIR)$(bindir)/$$file" || exit 1; \
	  fi \
	done
else
install-exec-local:
endif

install-symlinks:
	@for dir in \
	    actions base bundles caches details draggers elements engines \
	    errors events fields fonts glue io manips misc lists nodekits \
	    nodes navigation shapenodes projectors sensors upgraders 3ds \
	    collision hardcopy threads shaders shadows geo xml vrml97 \
	    foreignfiles ; do \
              (cd $$dir; $(MAKE) install-symlinks;) \
        done ; 

uninstall-local:
	@basename="coin@COIN_MAJOR_VERSION@@SUFFIX@"; \
	for file in \
	  "$(DESTDIR)$(bindir)/$$basename.dll" \
	  "$(DESTDIR)$(bindir)/$$basename.pdb" \
	  "$(DESTDIR)$(libdir)/$$basename.lib" \
	  "$(DESTDIR)$(MAC_FRAMEWORK_PREFIX)/$(MAC_FRAMEWORK_NAME).framework/$(MAKE_FRAMEWORK_NAME)"; do \
	  if test -f "$$file"; then \
	    echo " rm -f" "$$file"; \
	    rm -f "$$file"; \
	  fi \
	done; \
	baseincdir="$(DESTDIR)$(libCoinincdir)"; \
	for dir in \
	  "$$baseincdir/3ds" \
	  "$$baseincdir/actions" \
	  "$$baseincdir/base" \
	  "$$baseincdir/bundles" \
	  "$$baseincdir/caches" \
	  "$$baseincdir/collision" \
	  "$$baseincdir/details" \
	  "$$baseincdir/draggers" \
	  "$$baseincdir/elements" \
	  "$$baseincdir/engines" \
	  "$$baseincdir/errors" \
	  "$$baseincdir/events" \
	  "$$baseincdir/fields" \
	  "$$baseincdir/fonts" \
	  "$$baseincdir/glue" \
	  "$$baseincdir/hardcopy" \
	  "$$baseincdir/lists" \
	  "$$baseincdir/lock" \
	  "$$baseincdir/manips" \
	  "$$baseincdir/misc" \
	  "$$baseincdir/nodekits" \
	  "$$baseincdir/nodes" \
	  "$$baseincdir/navigation" \
	  "$$baseincdir/projectors" \
	  "$$baseincdir/sensors" \
	  "$$baseincdir/system" \
	  "$$baseincdir/threads" \
	  "$$baseincdir/upgraders" \
	  "$$baseincdir/VRMLnodes" \
	  "$$baseincdir/MPEG" \
	  "$$baseincdir/C/XML" \
	  "$$baseincdir/C/base" \
	  "$$baseincdir/C/errors" \
	  "$$baseincdir/C/glue" \
	  "$$baseincdir/C/threads" \
	  "$$baseincdir/C" \
	  "$$baseincdir"; do \
	  if test -d "$$dir"; then \
	    echo " rmdir $$dir"; \
	    rmdir "$$dir"; \
	  fi \
	done; \
	:


Coin.RES: Coin.rc
	rc Coin.rc
