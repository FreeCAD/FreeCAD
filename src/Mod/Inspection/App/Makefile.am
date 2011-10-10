
lib_LTLIBRARIES=libInspection.la Inspection.la

libInspection_la_SOURCES=\
		AppInspectionPy.cpp \
		InspectionFeature.cpp \
		InspectionFeature.h \
		PreCompiled.cpp \
		PreCompiled.h

includedir = @includedir@/Mod/Inspection/App

# the library search path.
libInspection_la_LDFLAGS = -L$(top_builddir)/src/Base -L$(top_builddir)/src/App \
		-L$(top_builddir)/src/Mod/Mesh/App \
		-L$(top_builddir)/src/Mod/Points/App \
		-L$(top_builddir)/src/Mod/Part/App \
		-L$(OCC_LIB) $(QT4_CORE_LIBS) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libInspection_la_CPPFLAGS = -DInspectionAppExport=

libInspection_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lTKernel \
		-lTKFillet \
		-lTKG2d \
		-lTKG3d \
		-lTKMath \
		-lTKMesh \
		-lTKXSBase \
		-lTKBool \
		-lTKBO \
		-lTKBRep \
		-lTKTopAlgo \
		-lTKGeomAlgo \
		-lTKGeomBase \
		-lTKOffset \
		-lTKPrim \
		-lFreeCADBase \
		-lFreeCADApp \
		-lMesh \
		-lPoints \
		-lPart

#--------------------------------------------------------------------------------------
# Loader of libInspection

Inspection_la_SOURCES=\
		AppInspection.cpp

# the library search path.
Inspection_la_LDFLAGS = $(libInspection_la_LDFLAGS) -module -avoid-version
Inspection_la_CPPFLAGS = $(libInspection_la_CPPFLAGS)

Inspection_la_LIBADD   = \
		$(libInspection_la_LIBADD) \
		-lInspection

Inspection_la_DEPENDENCIES = libInspection.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) \
		$(QT4_CORE_CXXFLAGS) -I$(OCC_INC)

libdir = $(prefix)/Mod/Inspection

EXTRA_DIST = \
		CMakeLists.txt
