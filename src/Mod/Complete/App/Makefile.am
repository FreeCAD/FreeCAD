lib_LTLIBRARIES=libComplete.la Complete.la

libComplete_la_SOURCES=\
		AppCompletePy.cpp \
		CompleteConfiguration.h \
		PreCompiled.cpp \
		PreCompiled.h


# the library search path.
libComplete_la_LDFLAGS = -L../../../Base -L../../../App $(all_libraries) -version-info \
		@LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libComplete_la_CPPFLAGS = -DCompleteExport=

libComplete_la_LIBADD   = \
		@BOOST_REGEX_LIB@ @BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp

#--------------------------------------------------------------------------------------
# Loader of libComplete

Complete_la_SOURCES=\
		AppComplete.cpp

# the library search path.
Complete_la_LDFLAGS = $(libComplete_la_LDFLAGS) -module -avoid-version
Complete_la_CPPFLAGS = $(libComplete_la_CPPFLAGS)

Complete_la_LIBADD   = \
		$(libComplete_la_LIBADD) \
		-lComplete

Complete_la_DEPENDENCIES = libComplete.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes)

libdir = $(prefix)/Mod/Complete

EXTRA_DIST = \
		CMakeLists.txt
