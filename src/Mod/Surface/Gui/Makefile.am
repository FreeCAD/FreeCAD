
lib_LTLIBRARIES=libSurfaceGui.la SurfaceGui.la

BUILT_SOURCES=

libSurfaceGui_la_SOURCES=\
		AppSurfaceGuiPy.cpp \
		Command.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		Workbench.cpp \
		Workbench.h

includedir = @includedir@/Mod/Surface/Gui

# the library search path.
libSurfaceGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App \
		$(QT_LIBS) $(all_libraries) \
		$(sim_ac_coin_ldflags) $(sim_ac_coin_libs) \
		$(sim_ac_soqt_ldflags) $(sim_ac_soqt_libs) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

libSurfaceGui_la_CPPFLAGS = -DSurfaceAppExport= -DSurfaceGuiExport=

libSurfaceGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lSurface

#--------------------------------------------------------------------------------------
# Loader of libSurfaceGui

SurfaceGui_la_SOURCES=\
		AppSurfaceGui.cpp

# the library search path.
SurfaceGui_la_LDFLAGS = $(libSurfaceGui_la_LDFLAGS) -module -avoid-version
SurfaceGui_la_CPPFLAGS = $(libSurfaceGui_la_CPPFLAGS)

SurfaceGui_la_LIBADD   = \
		$(libSurfaceGui_la_LIBADD) \
		-lSurfaceGui

SurfaceGui_la_DEPENDENCIES = libSurfaceGui.la

#--------------------------------------------------------------------------------------

# rule for Qt MetaObject Compiler:
moc_%.cpp: %.h
	$(QT_MOC) $< -o $(@F)

# rule for Qt MetaObject Compiler:
%.moc: %.h
	$(QT_MOC) $< -o $(@F)

# rules for Qt User Interface Compiler:
ui_%.h: %.ui
	$(QT_UIC) $< -o $(@F)

# rules for Qt Resource Compiler:
qrc_%.cpp: Resources/%.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) $(QT_CXXFLAGS) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir)

libdir = $(prefix)/Mod/Surface

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt \
		Resources/Surface.qrc

