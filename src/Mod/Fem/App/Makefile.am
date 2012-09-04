
lib_LTLIBRARIES=libFem.la Fem.la

BUILT_SOURCES=\
		FemMeshPy.cpp 

libFem_la_BUILT=\
		FemMeshPy.h

libFem_la_SOURCES = \
		AppFemPy.cpp \
		FemMesh.cpp \
		FemMesh.h \
		FemMeshPyImp.cpp \
		FemMeshObject.cpp \
		FemMeshObject.h \
		FemMeshProperty.cpp \
		FemMeshProperty.h \
		HypothesisPy.cpp \
		HypothesisPy.h \
		PreCompiled.cpp \
		PreCompiled.h

%.cpp: %.xml $(top_srcdir)/src/Tools/generateTemplates/templateClassPyExport.py
	$(PYTHON) $(top_srcdir)/src/Tools/generate.py --outputPath $(@D) $<

# the library search path.
libFem_la_LDFLAGS = -L../../../Base -L../../../App -L$(OCC_LIB) \
		-L$(top_builddir)/src/Mod/Mesh/App -L$(top_builddir)/src/Mod/Part/App \
		-L$(top_builddir)/src/3rdParty/salomesmesh $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libFem_la_CPPFLAGS = -DFemAppExport=

libFem_la_LIBADD   = \
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
		-lTKGeomBase \
		-lSMESH \
		-lSMESHDS \
		-lSMDS \
		-lStdMeshers

#--------------------------------------------------------------------------------------
# Loader of libFem

Fem_la_SOURCES=\
		AppFem.cpp

# the library search path.
Fem_la_LDFLAGS = $(libFem_la_LDFLAGS) -module -avoid-version
Fem_la_CPPFLAGS = $(libFem_la_CPPFLAGS)

Fem_la_LIBADD   = \
		$(libFem_la_LIBADD) \
		-lFem

Fem_la_DEPENDENCIES = libFem.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(top_srcdir)/src/3rdParty/salomesmesh/inc \
		$(all_includes) -I$(OCC_INC)


libdir = $(prefix)/Mod/Fem

CLEANFILES = $(BUILT_SOURCES) $(libFem_la_BUILT)

EXTRA_DIST = \
		FemMeshPy.xml \
		CMakeLists.txt
