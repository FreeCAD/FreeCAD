
lib_LTLIBRARIES=libReverseEngineering.la ReverseEngineering.la

libReverseEngineering_la_SOURCES=\
		AppReverseEngineeringPy.cpp \
		ApproxSurface.cpp \
		PreCompiled.cpp \
		PreCompiled.h


include_HEADERS=\
		ApproxSurface.h

# the library search path.
libReverseEngineering_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Mod/Part/App \
		-L../../../Mod/Mesh/App -L$(OCC_LIB) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
		
libReverseEngineering_la_CPPFLAGS = -DReenExport=

libReverseEngineering_la_LIBADD   = \
		@BOOST_REGEX_LIB@ @BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lPart \
		-lMesh \
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
# Loader of libReverseEngineering

ReverseEngineering_la_SOURCES=\
		AppReverseEngineering.cpp

# the library search path.
ReverseEngineering_la_LDFLAGS = $(libReverseEngineering_la_LDFLAGS) -module -avoid-version
ReverseEngineering_la_CPPFLAGS = $(libReverseEngineering_la_CPPFLAGS)

ReverseEngineering_la_LIBADD   = \
		$(libReverseEngineering_la_LIBADD) \
		-lReverseEngineering

ReverseEngineering_la_DEPENDENCIES = libReverseEngineering.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(OCC_INC) $(all_includes)


includedir = @includedir@/Mod/ReverseEngineering/App
libdir = $(prefix)/Mod/ReverseEngineering

EXTRA_DIST = \
		CMakeLists.txt
