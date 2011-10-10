
lib_LTLIBRARIES=libImport.la

libImport_la_SOURCES=\
		AppImport.cpp \
		AppImportPy.cpp \
		FeatureImportIges.cpp \
		FeatureImportIges.h \
		FeatureImportStep.cpp \
		FeatureImportStep.h \
		PreCompiled.cpp \
		PreCompiled.h


# the library search path.
libImport_la_LDFLAGS = -L../../../Base -L../../../App -L../../Part/App $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libImport_la_CPPFLAGS = -DAppPartExport=

libImport_la_LIBADD   = \
		-lTKIGES \
		-lTKSTEP \
		-lFreeCADBase \
		-lFreeCADApp \
		-lPart

# set the include path found by configure
AM_CXXFLAGS = $(all_includes) -I../../../

libdir = $(prefix)/Mod/Import

# We need this softlink for Python to load
install-data-local:
		cd $(DESTDIR)$(libdir) && \
		rm -f Import.so && \
		$(LN_S) libImport.so Import.so

EXTRA_DIST = \
		AppImport.dsp \
		AppImport.vcproj \
		Libs.cpp
