SUBDIRS=freegcs 

lib_LTLIBRARIES=libSketcher.la Sketcher.la

BUILT_SOURCES=\
		ConstraintPy.cpp \
		SketchPy.cpp \
		SketchObjectPy.cpp \
		SketchObjectSFPy.cpp

libSketcher_la_BUILT=\
		ConstraintPy.h \
		SketchPy.h \
		SketchObjectPy.h \
		SketchObjectSFPy.h

libSketcher_la_SOURCES = \
		AppSketcherPy.cpp \
		Constraint.cpp \
		Constraint.h \
		ConstraintPyImp.cpp \
		PropertyConstraintList.cpp \
		PropertyConstraintList.h \
		SketchObject.cpp \
		SketchObject.h \
		SketchObjectPyImp.cpp \
		SketchObjectSF.cpp \
		SketchObjectSF.h \
		SketchObjectSFPyImp.cpp \
		Sketch.cpp \
		Sketch.h \
		SketchPyImp.cpp \
		PreCompiled.cpp \
		PreCompiled.h


# the library search path.
libSketcher_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Mod/Part/App \
		-L$(OCC_LIB) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libSketcher_la_CPPFLAGS = -DSketcherAppExport=

libSketcher_la_LIBADD = \
		freegcs/libfreegcs.la \
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

%.cpp: %.xml $(top_srcdir)/src/Tools/generateTemplates/templateClassPyExport.py
	$(PYTHON) $(top_srcdir)/src/Tools/generate.py --outputPath $(@D) $<

#--------------------------------------------------------------------------------------
# Loader of libSketcher

Sketcher_la_SOURCES=\
		AppSketcher.cpp

# the library search path.
Sketcher_la_LDFLAGS = $(libSketcher_la_LDFLAGS) -module -avoid-version
Sketcher_la_CPPFLAGS = $(libSketcher_la_CPPFLAGS)

Sketcher_la_LIBADD   = \
		$(libSketcher_la_LIBADD) \
		-lSketcher

Sketcher_la_DEPENDENCIES = libSketcher.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(OCC_INC) -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) \
        -I$(EIGEN3_INC)


libdir = $(prefix)/Mod/Sketcher
datadir = $(prefix)/Mod/Sketcher

CLEANFILES = $(BUILT_SOURCES) $(libSketcher_la_BUILT)

EXTRA_DIST = \
		$(data_DATA) \
		CMakeLists.txt \
		ConstraintPy.xml \
		SketchPy.xml \
		SketchObjectPy.xml \
		SketchObjectSFPy.xml

