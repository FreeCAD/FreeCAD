
lib_LTLIBRARIES=libSandboxGui.la SandboxGui.la

libSandboxGui_la_SOURCES=\
		Command.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		Workbench.cpp \
		Workbench.h

# the library search path.
libSandboxGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../../Mesh/App -L../../Part/App -L../App \
		$(sim_ac_coin_ldflags) $(sim_ac_coin_libs) $(sim_ac_soqt_ldflags) $(sim_ac_soqt_libs) \
		$(QT_LIBS) $(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

libSandboxGui_la_CPPFLAGS = $(sim_ac_coin_cppflags) -DSandboxAppExport= -DSandboxGuiExport=

libSandboxGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lMesh \
		-lPart \
		-lSandbox

#--------------------------------------------------------------------------------------
# Loader of libSandboxGui

SandboxGui_la_SOURCES=\
		AppSandboxGui.cpp

# the library search path.
SandboxGui_la_LDFLAGS = $(libSandboxGui_la_LDFLAGS) -module -avoid-version
SandboxGui_la_CPPFLAGS = $(libSandboxGui_la_CPPFLAGS)

SandboxGui_la_LIBADD   = \
		$(libSandboxGui_la_LIBADD) \
		-lSandboxGui

SandboxGui_la_DEPENDENCIES = libSandboxGui.la

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
qrc_%.cpp: %.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) $(QT_CXXFLAGS) -I$(OCC_INC)

libdir = $(prefix)/Mod/Sandbox

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt
