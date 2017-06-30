
lib_LTLIBRARIES=libGDMLGui.la GDMLGui.la

BUILT_SOURCES=

libGDMLGui_la_SOURCES=\
		AppGDMLGuiPy.cpp \
		Command.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		Workbench.cpp \
		Workbench.h

includedir = @includedir@/Mod/GDML/Gui

# the library search path.
libGDMLGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App \
		$(QT_LIBS) $(all_libraries) \
		$(sim_ac_coin_ldflags) $(sim_ac_coin_libs) \
		$(sim_ac_soqt_ldflags) $(sim_ac_soqt_libs) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

libGDMLGui_la_CPPFLAGS = -DGDMLAppExport= -DGDMLGuiExport=

libGDMLGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lGDML

#--------------------------------------------------------------------------------------
# Loader of libGDMLGui

GDMLGui_la_SOURCES=\
		AppGDMLGui.cpp

# the library search path.
GDMLGui_la_LDFLAGS = $(libGDMLGui_la_LDFLAGS) -module -avoid-version
GDMLGui_la_CPPFLAGS = $(libGDMLGui_la_CPPFLAGS)

GDMLGui_la_LIBADD   = \
		$(libGDMLGui_la_LIBADD) \
		-lGDMLGui

GDMLGui_la_DEPENDENCIES = libGDMLGui.la

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

libdir = $(prefix)/Mod/GDML

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt \
		Resources/GDML.qrc

