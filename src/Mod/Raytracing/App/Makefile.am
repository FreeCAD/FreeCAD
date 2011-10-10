
lib_LTLIBRARIES=libRaytracing.la Raytracing.la

BUILT_SOURCES=\
		FreeCADpov.h

libRaytracing_la_SOURCES=\
		AppRaytracingPy.cpp \
		PovTools.cpp \
		PovTools.h \
		PreCompiled.cpp \
		PreCompiled.h \
		RayFeature.cpp \
		RayFeature.h \
		RayProject.cpp \
		RayProject.h \
		RaySegment.cpp \
		RaySegment.h


FreeCADpov.h: FreeCADpov
	$(PYTHON) $(top_srcdir)/src/Tools/PythonToCPP.py $< $@


# the library search path.
libRaytracing_la_LDFLAGS = -L../../../Base -L../../../App -L../../Part/App -L/usr/X11R6/lib \
		-L$(OCC_LIB) $(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libRaytracing_la_CPPFLAGS = -DAppPartExport= -DAppRaytracingExport= -DFeatureRayExportPov=

libRaytracing_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lTKernel \
		-lTKG2d \
		-lTKG3d \
		-lTKMath \
		-lTKBool \
		-lTKBRep \
		-lTKGeomAlgo \
		-lTKGeomBase \
		-lTKMesh \
		-lPart

#--------------------------------------------------------------------------------------
# Loader of libRaytracing

Raytracing_la_SOURCES=\
		AppRaytracing.cpp

# the library search path.
Raytracing_la_LDFLAGS = $(libRaytracing_la_LDFLAGS) -module -avoid-version
Raytracing_la_CPPFLAGS = $(libRaytracing_la_CPPFLAGS)

Raytracing_la_LIBADD   = \
		$(libRaytracing_la_LIBADD) \
		-lRaytracing

Raytracing_la_DEPENDENCIES = libRaytracing.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) -I$(OCC_INC)


libdir = $(prefix)/Mod/Raytracing

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		resources/FCAnimation.inc \
		resources/FCAnimation.ini \
		resources/FCAnimation.pov \
		resources/FCSimple.pov \
		CMakeLists.txt \
		FreeCADpov
