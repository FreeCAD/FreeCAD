SUBDIRS=Resources

lib_LTLIBRARIES=libPartDesignGui.la PartDesignGui.la

BUILT_SOURCES=\
		moc_TaskChamfer.cpp \
		moc_TaskPadParameters.cpp \
		moc_TaskPatternRectangularParameters.cpp \
		moc_TaskPocketParameters.cpp \
		moc_TaskHoleParameters.cpp \
		moc_TaskRevolutionParameters.cpp \
		ui_TaskChamfer.h \
		ui_TaskPadParameters.h \
		ui_TaskPatternRectangularParameters.h \
		ui_TaskPocketParameters.h \
		ui_TaskHoleParameters.h \
		ui_TaskRevolutionParameters.h 

libPartDesignGui_la_UI=\
		TaskChamfer.ui \
		TaskPadParameters.ui \
		TaskPatternRectangularParameters.ui \
		TaskPocketParameters.ui \
		TaskHoleParameters.ui \
		TaskRevolutionParameters.ui

libPartDesignGui_la_SOURCES=\
		AppPartDesignGuiPy.cpp \
		Command.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		TaskChamfer.cpp \
		TaskChamfer.h \
		TaskPadParameters.cpp \
		TaskPadParameters.h \
		TaskPatternRectangularParameters.cpp \
		TaskPatternRectangularParameters.h \
		TaskPocketParameters.cpp \
		TaskPocketParameters.h \
		TaskRevolutionParameters.cpp \
		TaskRevolutionParameters.h \
		TaskHoleParameters.cpp \
		TaskHoleParameters.h \
		ViewProvider.cpp \
		ViewProvider.h \
		ViewProviderHole.cpp \
		ViewProviderHole.h \
		ViewProviderPad.cpp \
		ViewProviderPad.h \
		ViewProviderPocket.cpp \
		ViewProviderPocket.h \
		ViewProviderRevolution.cpp \
		ViewProviderRevolution.h \
		ViewProviderPatternRectangular.cpp \
		ViewProviderPatternRectangular.h \
		Workbench.cpp \
		Workbench.h

# the library search path.
libPartDesignGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui \
		-L../../Part/App -L../../Part/Gui -L../App \
		-L$(OCC_LIB) $(QT_LIBS) $(GL_LIBS) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

libPartDesignGui_la_CPPFLAGS = -DPartDesignAppExport= -DPartDesignGuiExport=

libPartDesignGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		@BOOST_SIGNALS_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lPart \
		-lPartGui \
		-lTKernel \
		-lTKMath \
		-lTKXSBase \
		-lTKBRep \
		-lTKTopAlgo \
		-lPartDesign

#--------------------------------------------------------------------------------------
# Loader of libPartDesignGui

PartDesignGui_la_SOURCES=\
		AppPartDesignGui.cpp

# the library search path.
PartDesignGui_la_LDFLAGS = $(libPartDesignGui_la_LDFLAGS) -module -avoid-version
PartDesignGui_la_CPPFLAGS = $(libPartDesignGui_la_CPPFLAGS)

PartDesignGui_la_LIBADD   = \
		$(libPartDesignGui_la_LIBADD) \
		Resources/libResources.la \
		-lPartDesignGui

PartDesignGui_la_DEPENDENCIES = libPartDesignGui.la

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
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(srcdir)/.. \
		$(all_includes) $(QT_CXXFLAGS) -I$(OCC_INC) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir)


libdir = $(prefix)/Mod/PartDesign

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		$(libPartDesignGui_la_UI) \
		CMakeLists.txt

