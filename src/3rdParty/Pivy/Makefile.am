
lib_LTLIBRARIES=_coin.la _soqt.la

#--------------------------------------------------------------------------------------
# Coin

_coin_la_SOURCES=\
		coin_wrap.cpp \
		coin_header_includes.h

# the library search path.
_coin_la_LDFLAGS = $(sim_ac_coin_ldflags) -module -avoid-version
_coin_la_CPPFLAGS = -fno-strict-aliasing

_coin_la_LIBADD   = \
		-l@PYTHON_LIB@ \
		$(sim_ac_coin_libs)

_coindir = $(prefix)/bin/pivy
_coin_DATA = \
		__init__.py \
		coin.py \
		sogui.py

#--------------------------------------------------------------------------------------
# SoQt

_soqt_la_SOURCES=\
		soqt_wrap.cpp

# the library search path.
_soqt_la_LDFLAGS = $(sim_ac_coin_ldflags) $(sim_ac_soqt_ldflags) $(QT_LIBS) -module -avoid-version
_soqt_la_CPPFLAGS = -fno-strict-aliasing

_soqt_la_LIBADD   = \
		-l@PYTHON_LIB@ \
		$(sim_ac_coin_libs) \
		$(sim_ac_soqt_libs)

_soqt.la: $(_soqt_la_OBJECTS) $(_soqt_la_DEPENDENCIES) 
	$(CXXLINK) -rpath $(libdir)/gui $(_soqt_la_LDFLAGS) $(_soqt_la_OBJECTS) $(_soqt_la_LIBADD) $(LIBS)

_soqtdir = $(prefix)/bin/pivy/gui
_soqt_DATA = \
		__init__.py \
		soqt.py

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = $(all_includes) $(QT_CXXFLAGS) -I$(top_srcdir)/src -I$(top_builddir)/src

libdir = $(prefix)/bin/pivy
install-exec-hook:
		$(mkdir_p) $(DESTDIR)$(libdir)/gui
		mv $(DESTDIR)$(libdir)/_soqt.* \
		   $(DESTDIR)$(libdir)/gui/

uninstall-hook:
		rm -r $(DESTDIR)$(libdir)/gui

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		__init__.py \
		coin.py \
		sogui.py \
		soqt.py \
		AUTHORS \
		LICENSE \
		README \
		CMakeLists.txt

