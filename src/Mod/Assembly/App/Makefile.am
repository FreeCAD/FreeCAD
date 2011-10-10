
lib_LTLIBRARIES=libAssembly.la Assembly.la

libAssembly_la_SOURCES=\
                AppAssemblyPy.cpp \
		PreCompiled.cpp \
		PreCompiled.h


# the library search path.
libAssembly_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Mod/Part/App $(all_libraries) -L$(OCC_LIB) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libAssembly_la_CPPFLAGS = -DAssemblyAppExport=

libAssembly_la_LIBADD   = \
		@BOOST_REGEX_LIB@ @BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lPart \
		-lTKernel \
		-lTKG2d \
		-lTKG3d \
		-lTKMath \
		-lTKSTEP \
		-lTKIGES \
		-lTKSTL \
		-lTKShHealing \
		-lTKXSBase \
		-lTKBool \
		-lTKBO \
		-lTKBRep \
		-lTKTopAlgo \
		-lTKGeomAlgo \
		-lTKGeomBase \
		-lTKOffset \
		-lTKPrim \
		-lTKHLR

#--------------------------------------------------------------------------------------
# Loader of libAssembly

Assembly_la_SOURCES=\
		AppAssembly.cpp

# the library search path.
Assembly_la_LDFLAGS = $(libAssembly_la_LDFLAGS) -module -avoid-version
Assembly_la_CPPFLAGS = $(libAssembly_la_CPPFLAGS)

Assembly_la_LIBADD   = \
		$(libAssembly_la_LIBADD) \
		-lAssembly

Assembly_la_DEPENDENCIES = libAssembly.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(OCC_INC) $(all_includes)


libdir = $(prefix)/Mod/Assembly

EXTRA_DIST = \
		CMakeLists.txt
