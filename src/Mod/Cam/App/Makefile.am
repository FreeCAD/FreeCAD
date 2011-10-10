
lib_LTLIBRARIES=libCam.la Cam.la


libCam_la_SOURCES=\
		AppCamPy.cpp \
		Approx.cpp \
		Approx.h \
		best_fit.cpp \
		best_fit.h \
		BRepAdaptor_CompCurve2.cxx \
		BRepAdaptor_CompCurve2.h \
		ConvertDyna.cpp \
		ConvertDyna.h \
		cutting_tools.cpp \
		cutting_tools.h \
		edgesort.cpp \
		edgesort.h \
		path_simulate.cpp \
		path_simulate.h \
		PreCompiled.cpp \
		PreCompiled.h \
		routine.cpp \
		routine.h \
		stuff.h \
		SpringbackCorrection.cpp \
		SpringbackCorrection.h \
		UniGridApprox.cpp \
		UniGridApprox.h \
		WireExplorer.cxx \
		WireExplorer.h

# the library search path.
libCam_la_LDFLAGS = -L../../../3rdParty/OCCAdaptMesh -L../../../Base -L../../../App \
		-L../../../Mod/Part/App -L../../../Mod/Mesh/App -L/usr/X11R6/lib -L$(OCC_LIB) \
		$(GTS_LIBS) $(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libCam_la_CPPFLAGS = -DAppCamExport=

libCam_la_LIBADD   = \
		-lxerces-c \
		-lboost_filesystem \
		-l@PYTHON_LIB@ \
		-lOCCAdaptMesh \
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
		-lTKMesh \
		-lblas \
		-lumfpack \
		-lamd

%.cpp: %.xml $(top_srcdir)/src/Tools/generateTemplates/templateClassPyExport.py
	$(PYTHON) $(top_srcdir)/src/Tools/generate.py --outputPath $(@D) $<

#--------------------------------------------------------------------------------------
# Loader of libCam

Cam_la_SOURCES=\
		AppCam.cpp

# the library search path.
Cam_la_LDFLAGS = $(libCam_la_LDFLAGS) -module -avoid-version
Cam_la_CPPFLAGS = $(libCam_la_CPPFLAGS)

Cam_la_LIBADD   = \
		$(libCam_la_LIBADD) \
		-lCam

Cam_la_DEPENDENCIES = libCam.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) -I$(OCC_INC) $(GTS_CFLAGS) \
		  -I$(top_srcdir)/src/3rdParty -I$(top_srcdir)/src/3rdParty/OCCAdaptMesh/Include


libdir = $(prefix)/Mod/Cam

CLEANFILES = $(BUILT_SOURCES) $(libCam_la_BUILT)

EXTRA_DIST = \
		CMakeLists.txt
