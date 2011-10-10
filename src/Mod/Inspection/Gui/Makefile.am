
lib_LTLIBRARIES=libInspectionGui.la InspectionGui.la

BUILT_SOURCES=\
		ui_VisualInspection.h \
		moc_VisualInspection.cpp \
		qrc_Inspection.cpp 

libInspectionGui_la_SOURCES=\
		AppInspectionGuiPy.cpp \
		Command.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		ViewProviderInspection.cpp \
		ViewProviderInspection.h \
		VisualInspection.cpp \
		VisualInspection.h \
		Workbench.cpp \
		Workbench.h

includedir = @includedir@/Mod/Inspection/Gui

# the library search path.
libInspectionGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App \
		-L$(top_builddir)/src/Mod/Mesh/App \
		-L$(top_builddir)/src/Mod/Points/App \
		$(sim_ac_coin_ldflags) $(sim_ac_coin_libs) \
		$(sim_ac_soqt_ldflags) $(sim_ac_soqt_libs) \
		$(QT_LIBS) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

libInspectionGui_la_CPPFLAGS = -DInspectionAppExport= -DInspectionGuiExport=

libInspectionGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lMesh \
		-lPoints \
		-lInspection

#--------------------------------------------------------------------------------------
# Loader of libInspectionGui

InspectionGui_la_SOURCES=\
		AppInspectionGui.cpp

# the library search path.
InspectionGui_la_LDFLAGS = $(libInspectionGui_la_LDFLAGS) -module -avoid-version
InspectionGui_la_CPPFLAGS = $(libInspectionGui_la_CPPFLAGS)

InspectionGui_la_LIBADD   = \
		$(libInspectionGui_la_LIBADD) \
		-lInspectionGui

InspectionGui_la_DEPENDENCIES = libInspectionGui.la

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

libdir = $(prefix)/Mod/Inspection

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt \
		VisualInspection.ui \
		Resources/Inspection.qrc

