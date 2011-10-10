
lib_LTLIBRARIES=libMeshPart.la MeshPart.la

libMeshPart_la_SOURCES=\
		AppMeshPartPy.cpp \
		CurveProjector.cpp \
		CurveProjector.h \
		MeshAlgos.cpp \
		MeshAlgos.h \
		Mesher.cpp \
		Mesher.h \
		PreCompiled.cpp \
		PreCompiled.h


# the library search path.
libMeshPart_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Mod/Part/App \
		-L../../../Mod/Mesh/App -L$(OCC_LIB) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

libMeshPart_la_CPPFLAGS = -DMeshPartAppExport=

libMeshPart_la_LIBADD   = \
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
		-lTKXSBase \
		-lTKBRep \
		-lTKTopAlgo \
		-lTKGeomAlgo \
		-lTKGeomBase
		
#if HAVE_SALOMESMESH
SMESH_LIBRARY = @top_builddir@/src/3rdParty/salomesmesh
libMeshPart_la_LDFLAGS += -L$(SMESH_LIBRARY)
libMeshPart_la_LIBADD += \
		-lSMESH \
		-lSMDS \
		-lStdMeshers
#endif

#--------------------------------------------------------------------------------------
# Loader of libMeshPart

MeshPart_la_SOURCES=\
		AppMeshPart.cpp

# the library search path.
MeshPart_la_LDFLAGS = $(libMeshPart_la_LDFLAGS) -module -avoid-version
MeshPart_la_CPPFLAGS = $(libMeshPart_la_CPPFLAGS)

MeshPart_la_LIBADD   = \
		$(libMeshPart_la_LIBADD) \
		-lMeshPart

MeshPart_la_DEPENDENCIES = libMeshPart.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) -I$(OCC_INC)

#if HAVE_SALOMESMESH
SMESH_INCLUDE = @top_srcdir@/src/3rdParty/salomesmesh/inc
AM_CXXFLAGS += -I$(SMESH_INCLUDE) -DHAVE_SMESH
#endif


libdir = $(prefix)/Mod/MeshPart

EXTRA_DIST = \
		CMakeLists.txt
