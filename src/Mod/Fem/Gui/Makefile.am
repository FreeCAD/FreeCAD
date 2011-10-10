SUBDIRS=Resources

lib_LTLIBRARIES=libFemGui.la FemGui.la

libFemGui_la_SOURCES=\
		AppFemGuiPy.cpp \
		Command.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		ViewProviderFemMesh.cpp \
		ViewProviderFemMesh.h \
		Workbench.cpp \
		Workbench.h

# the library search path.
libFemGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App \
		-L$(top_builddir)/src/3rdParty/salomesmesh \
		$(sim_ac_coin_ldflags) $(sim_ac_coin_libs) \
		$(sim_ac_soqt_ldflags) $(sim_ac_soqt_libs) \
		$(QT_LIBS) $(GL_LIBS) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libFemGui_la_CPPFLAGS = -DFemAppExport= -DFemGuiExport=

libFemGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lSMESH \
		-lSMDS \
		-lFem

#--------------------------------------------------------------------------------------
# Loader of libFemGui

FemGui_la_SOURCES=\
		AppFemGui.cpp

# the library search path.
FemGui_la_LDFLAGS = $(libFemGui_la_LDFLAGS) -module -avoid-version
FemGui_la_CPPFLAGS = $(libFemGui_la_CPPFLAGS)

FemGui_la_LIBADD   = \
		$(libFemGui_la_LIBADD) \
		Resources/libResources.la \
		-lFemGui

FemGui_la_DEPENDENCIES = libFemGui.la

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
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(QT_CXXFLAGS) $(all_includes) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir) \
		-I$(OCC_INC) -I$(top_srcdir)/src/3rdParty/salomesmesh/inc


libdir = $(prefix)/Mod/Fem

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt

