
lib_LTLIBRARIES=libPartDesign.la PartDesign.la

libPartDesign_la_SOURCES=\
		AppPartDesignPy.cpp \
		Feature.cpp \
		Feature.h \
		FeatureFace.cpp \
		FeatureFace.h \
		FeatureFillet.cpp \
		FeatureFillet.h \
		FeaturePad.cpp \
		FeaturePad.h \
		FeaturePocket.cpp \
		FeaturePocket.h \
		FeatureChamfer.cpp \
		FeatureChamfer.h \
		FeatureDraft.cpp \
		FeatureDraft.h \
		FeatureDressUp.cpp \
		FeatureDressUp.h \
		FeatureGroove.cpp \
		FeatureGroove.h \
		Body.cpp \
		Body.h \
		FeatureLinearPattern.cpp \
		FeatureLinearPattern.h \
		FeatureMirrored.cpp \
		FeatureMirrored.h \
		FeatureMultiTransform.cpp \
		FeatureMultiTransform.h \
		FeaturePolarPattern.cpp \
		FeaturePolarPattern.h \
		FeatureScaled.cpp \
		FeatureScaled.h \
		FeatureSketchBased.cpp \
		FeatureSketchBased.h \
		FeatureRevolution.cpp \
		FeatureRevolution.h \
		FeatureAdditive.cpp \
		FeatureAdditive.h \
		FeatureSubtractive.cpp \
		FeatureSubtractive.h \
		FeatureHole.cpp \
		FeatureHole.h \
		FeatureTransformed.cpp \
		FeatureTransformed.h \
		PreCompiled.cpp \
		PreCompiled.h


# the library search path.
libPartDesign_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Mod/Part/App \
		-L$(OCC_LIB) $(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libPartDesign_la_CPPFLAGS = -DPartDesignAppExport=

libPartDesign_la_LIBADD   = \
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
		-lTKXSBase \
		-lTKBRep \
		-lTKGeomAlgo \
		-lTKGeomBase \
		-lTKOffset \
		-lTKPrim \
		-lTKFeat \
		-lTKFillet \
		-lTKShHealing \
		-lTKBool \
		-lTKBO \
		-lTKTopAlgo 

#--------------------------------------------------------------------------------------
# Loader of libPartDesign

PartDesign_la_SOURCES=\
		AppPartDesign.cpp

# the library search path.
PartDesign_la_LDFLAGS = $(libPartDesign_la_LDFLAGS) -module -avoid-version
PartDesign_la_CPPFLAGS = $(libPartDesign_la_CPPFLAGS)

PartDesign_la_LIBADD   = \
		$(libPartDesign_la_LIBADD) \
		-lPartDesign

PartDesign_la_DEPENDENCIES = libPartDesign.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(OCC_INC) -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes)


libdir = $(prefix)/Mod/PartDesign

EXTRA_DIST = \
		CMakeLists.txt
