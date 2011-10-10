
lib_LTLIBRARIES=libImage.la Image.la

libImage_la_SOURCES=\
		ImageBase.cpp \
		ImagePlane.cpp \
		PreCompiled.cpp \
		PreCompiled.h

includedir = @includedir@/Mod/Image/App

include_HEADERS=\
		ImageBase.h \
		ImagePlane.h

# the library search path.
libImage_la_LDFLAGS = -L../../../Base -L../../../App $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libImage_la_CPPFLAGS = -DImageAppExport=

#		$(opencv_LIBS)
libImage_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp

#--------------------------------------------------------------------------------------
# Loader of libImage

Image_la_SOURCES=\
		AppImage.cpp

# the library search path.
Image_la_LDFLAGS = $(libImage_la_LDFLAGS) -module -avoid-version
Image_la_CPPFLAGS = $(libImage_la_CPPFLAGS)

Image_la_LIBADD   = \
		$(libImage_la_LIBADD) \
		-lImage

Image_la_DEPENDENCIES = libImage.la

#--------------------------------------------------------------------------------------

# set the include path found by configure 
# $(opencv_CFLAGS)
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes)

libdir = $(prefix)/Mod/Image

EXTRA_DIST = \
		CMakeLists.txt
