
lib_LTLIBRARIES=libDrawing.la Drawing.la

libTechDraw_la_SOURCES=\
		AppDrawingPy.cpp \
		TechDrawExport.cpp \
		TechDrawExport.h \
		DrawPage.cpp \
		DrawPage.h \
		DrawProjection.cpp \
		DrawProjection.h \
		DrawView.cpp \
		DrawView.h \
		DrawViewPart.cpp \
		DrawViewPart.h \
		DrawViewAnnotation.cpp \
		DrawViewAnnotation.h \
		DrawViewClip.cpp \
		DrawViewClip.h \
		PageGroup.cpp \
		PageGroup.h \
		ProjectionAlgos.cpp \
		ProjectionAlgos.h \
		PreCompiled.cpp \
		PreCompiled.h


# the library search path.
libTechDraw_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Mod/Part/App \
		-L$(OCC_LIB) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libTechDraw_la_CPPFLAGS = -DTechDrawExport=

libTechDraw_la_LIBADD   = \
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

TechDraw_la_SOURCES=\
		AppDrawing.cpp

# the library search path.
TechDraw_la_LDFLAGS = $(libTechDraw_la_LDFLAGS) -module -avoid-version
TechDraw_la_CPPFLAGS = $(libTechDraw_la_CPPFLAGS)

TechDraw_la_LIBADD   = \
		$(libTechDraw_la_LIBADD) \
		-lDrawing

TechDraw_la_DEPENDENCIES = libDrawing.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(OCC_INC) $(all_includes)


libdir = $(prefix)/Mod/TechDraw

EXTRA_DIST = \
		CMakeLists.txt
