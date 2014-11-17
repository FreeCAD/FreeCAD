
lib_LTLIBRARIES=libSurface.la Surface.la

libSurface_la_SOURCES=\
		AppSurfacePy.cpp \
		PreCompiled.cpp \
		PreCompiled.h

includedir = @includedir@/Mod/Surface/App

# the library search path.
libSurface_la_LDFLAGS = -L../../../Base -L../../../App $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libSurface_la_CPPFLAGS = -DSurfaceAppExport=

libSurface_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp

#--------------------------------------------------------------------------------------
# Loader of libSurface

Surface_la_SOURCES=\
		AppSurface.cpp

# the library search path.
Surface_la_LDFLAGS = $(libSurface_la_LDFLAGS) -module -avoid-version
Surface_la_CPPFLAGS = $(libSurface_la_CPPFLAGS)

Surface_la_LIBADD   = \
		$(libSurface_la_LIBADD) \
		-lSurface

Surface_la_DEPENDENCIES = libSurface.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes)

libdir = $(prefix)/Mod/Surface

EXTRA_DIST = \
		CMakeLists.txt
