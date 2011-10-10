
lib_LTLIBRARIES=lib_TEMPLATE_.la _TEMPLATE_.la

lib_TEMPLATE__la_SOURCES=\
		App_TEMPLATE_Py.cpp \
		PreCompiled.cpp \
		PreCompiled.h

includedir = @includedir@/Mod/_TEMPLATE_/App

# the library search path.
lib_TEMPLATE__la_LDFLAGS = -L../../../Base -L../../../App $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
lib_TEMPLATE__la_CPPFLAGS = -D_TEMPLATE_AppExport=

lib_TEMPLATE__la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp

#--------------------------------------------------------------------------------------
# Loader of lib_TEMPLATE_

_TEMPLATE__la_SOURCES=\
		App_TEMPLATE_.cpp

# the library search path.
_TEMPLATE__la_LDFLAGS = $(lib_TEMPLATE__la_LDFLAGS) -module -avoid-version
_TEMPLATE__la_CPPFLAGS = $(lib_TEMPLATE__la_CPPFLAGS)

_TEMPLATE__la_LIBADD   = \
		$(lib_TEMPLATE__la_LIBADD) \
		-l_TEMPLATE_

_TEMPLATE__la_DEPENDENCIES = lib_TEMPLATE_.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes)

libdir = $(prefix)/Mod/_TEMPLATE_

EXTRA_DIST = \
		CMakeLists.txt
