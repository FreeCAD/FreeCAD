
lib_LTLIBRARIES=libJtReader.la JtReader.la

BUILT_SOURCES=\
		FreeCADpov.h

libJtReader_la_SOURCES=\
		PreCompiled.cpp \
		PreCompiled.h \


FreeCADpov.h: FreeCADpov
	$(PYTHON) $(top_srcdir)/src/Tools/PythonToCPP.py $< $@


# the library search path.
libJtReader_la_LDFLAGS = -L../../../Base -L../../../App -L../../Part/App -L/usr/X11R6/lib -L$(OCC_LIB) \
		$(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libJtReader_la_CPPFLAGS = -DAppPartExport= -DAppJtReaderExport= -DFeatureRayExportPov=

libJtReader_la_LIBADD   = \
		-lxerces-c \
		-l@PYTHON_LIB@ \
		-lFreeCADBase \
		-lFreeCADApp 

#--------------------------------------------------------------------------------------
# Loader of libJtReader

JtReader_la_SOURCES=\
		AppJtReader.cpp

# the library search path.
JtReader_la_LDFLAGS = $(libJtReader_la_LDFLAGS) -module -avoid-version
JtReader_la_CPPFLAGS = $(libJtReader_la_CPPFLAGS)

JtReader_la_LIBADD   = \
		$(libJtReader_la_LIBADD) \
		-lJtReader

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes)


libdir = $(prefix)/Mod/JtReader

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt 
