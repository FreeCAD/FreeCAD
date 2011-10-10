
lib_LTLIBRARIES=libSandbox.la Sandbox.la

libSandbox_la_SOURCES=\
		DocumentProtector.cpp \
		DocumentProtector.h \
		DocumentProtectorPy.cpp \
		DocumentProtectorPy.h \
		DocumentThread.cpp \
		DocumentThread.h \
		PreCompiled.cpp \
		PreCompiled.h


# the library search path.
libSandbox_la_LDFLAGS = -L../../../Base -L../../../App -L../../Mesh/App \
		$(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libSandbox_la_CPPFLAGS = -DSandboxAppExport=

libSandbox_la_LIBADD   = \
		-l@PYTHON_LIB@ \
		@BOOST_SYSTEM_LIB@ \
		-lxerces-c \
		-lQtCore \
		-lFreeCADBase \
		-lFreeCADApp \
		-lMesh

#--------------------------------------------------------------------------------------
# Loader of libSandbox

Sandbox_la_SOURCES=\
		AppSandbox.cpp

# the library search path.
Sandbox_la_LDFLAGS = $(libSandbox_la_LDFLAGS) -module -avoid-version
Sandbox_la_CPPFLAGS = $(libSandbox_la_CPPFLAGS)

Sandbox_la_LIBADD   = \
		$(libSandbox_la_LIBADD) \
		-lSandbox

Sandbox_la_DEPENDENCIES = libSandbox.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) $(QT_CXXFLAGS)

libdir = $(prefix)/Mod/Sandbox

EXTRA_DIST = \
		CMakeLists.txt
