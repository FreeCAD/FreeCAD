
lib_LTLIBRARIES=libDrawing.la Drawing.la

libDrawing_la_SOURCES=\
		AppDrawingPy.cpp \
		DrawingExport.cpp \
		DrawingExport.h \
		FeaturePage.cpp \
		FeaturePage.h \
		FeatureProjection.cpp \
		FeatureProjection.h \
		FeatureView.cpp \
		FeatureView.h \
		FeatureViewPart.cpp \
		FeatureViewPart.h \
		FeatureViewAnnotation.cpp \
		FeatureViewAnnotation.h \
		FeatureClip.cpp \
		FeatureClip.h \
		PageGroup.cpp \
		PageGroup.h \
		ProjectionAlgos.cpp \
		ProjectionAlgos.h \
		PreCompiled.cpp \
		PreCompiled.h


# the library search path.
libDrawing_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Mod/Part/App \
		-L$(OCC_LIB) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libDrawing_la_CPPFLAGS = -DDrawingExport=

libDrawing_la_LIBADD   = \
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
		-lTKHLR \
		-lTKMesh

#--------------------------------------------------------------------------------------
# Loader of libDrawing

Drawing_la_SOURCES=\
		AppDrawing.cpp

# the library search path.
Drawing_la_LDFLAGS = $(libDrawing_la_LDFLAGS) -module -avoid-version
Drawing_la_CPPFLAGS = $(libDrawing_la_CPPFLAGS)

Drawing_la_LIBADD   = \
		$(libDrawing_la_LIBADD) \
		-lDrawing

Drawing_la_DEPENDENCIES = libDrawing.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(OCC_INC) $(all_includes)


libdir = $(prefix)/Mod/Drawing

EXTRA_DIST = \
		CMakeLists.txt
