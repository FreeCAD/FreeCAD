lib_LTLIBRARIES=libStart.la Start.la

libStart_la_SOURCES=\
		AppStartPy.cpp \
		StartConfiguration.h \
		PreCompiled.cpp \
		PreCompiled.h


# the library search path.
libStart_la_LDFLAGS = -L../../../Base -L../../../App $(all_libraries) -version-info \
		@LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libStart_la_CPPFLAGS = -DStartAppExport=

libStart_la_LIBADD   = \
		@BOOST_REGEX_LIB@ @BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp

#--------------------------------------------------------------------------------------
# Loader of libStart

Start_la_SOURCES=\
		AppStart.cpp

# the library search path.
Start_la_LDFLAGS = $(libStart_la_LDFLAGS) -module -avoid-version
Start_la_CPPFLAGS = $(libStart_la_CPPFLAGS)

Start_la_LIBADD   = \
		$(libStart_la_LIBADD) \
		-lStart

Start_la_DEPENDENCIES = libStart.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes)

libdir = $(prefix)/Mod/Start

EXTRA_DIST = \
		CMakeLists.txt
