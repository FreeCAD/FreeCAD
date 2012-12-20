SUBDIRS=Resources

lib_LTLIBRARIES=libPartDesignGui.la PartDesignGui.la

BUILT_SOURCES=\
		moc_FeaturePickDialog.cpp \
		moc_TaskPadParameters.cpp \
		moc_TaskPocketParameters.cpp \
		moc_TaskChamferParameters.cpp \
		moc_TaskFilletParameters.cpp \
		moc_TaskDraftParameters.cpp \
		moc_TaskGrooveParameters.cpp \
		moc_TaskHoleParameters.cpp \
		moc_TaskLinearPatternParameters.cpp \
		moc_TaskMirroredParameters.cpp \
		moc_TaskMultiTransformParameters.cpp \
		moc_TaskPolarPatternParameters.cpp \
		moc_TaskRevolutionParameters.cpp \
		moc_TaskScaledParameters.cpp \
		moc_TaskTransformedMessages.cpp \
		moc_TaskTransformedParameters.cpp \
		ui_FeaturePickDialog.h \
		ui_TaskGrooveParameters.h \
		ui_TaskPadParameters.h \
		ui_TaskPocketParameters.h \
		ui_TaskChamferParameters.h \
		ui_TaskFilletParameters.h \
		ui_TaskDraftParameters.h \
		ui_TaskHoleParameters.h \
		ui_TaskLinearPatternParameters.h \
		ui_TaskMirroredParameters.h \
		ui_TaskMultiTransformParameters.h \
		ui_TaskPolarPatternParameters.h \
		ui_TaskRevolutionParameters.h \
		ui_TaskScaledParameters.h \
		ui_TaskTransformedMessages.h

libPartDesignGui_la_UI=\
		FeaturePickDialog.ui \
		TaskGrooveParameters.ui \
		TaskPadParameters.ui \
		TaskPocketParameters.ui \
		TaskChamferParameters.ui \
		TaskDraftParameters.ui \
		TaskFilletParameters.ui \
		TaskFilletParameters.ui \
		TaskHoleParameters.ui \
		TaskLinearPatternParameters.ui \
		TaskMirroredParameters.ui \
		TaskMultiTransformParameters.ui \
		TaskPolarPatternParameters.ui \
		TaskRevolutionParameters.ui \
		TaskScaledParameters.ui \
		TaskTransformedMessages.ui

libPartDesignGui_la_SOURCES=\
		AppPartDesignGuiPy.cpp \
		Command.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		FeaturePickDialog.cpp \
		FeaturePickDialog.h \
		ReferenceSelection.cpp \
		ReferenceSelection.h \
		TaskGrooveParameters.cpp \
		TaskGrooveParameters.h \
		TaskPadParameters.cpp \
		TaskPadParameters.h \
		TaskPocketParameters.cpp \
		TaskPocketParameters.h \
		TaskChamferParameters.cpp \
		TaskChamferParameters.h \
		TaskFilletParameters.cpp \
		TaskFilletParameters.h \
		TaskDraftParameters.cpp \
		TaskDraftParameters.h \
		TaskLinearPatternParameters.cpp \
		TaskLinearPatternParameters.h \
		TaskMirroredParameters.cpp \
		TaskMirroredParameters.h \
		TaskMultiTransformParameters.cpp \
		TaskMultiTransformParameters.h \
		TaskPolarPatternParameters.cpp \
		TaskPolarPatternParameters.h \
		TaskRevolutionParameters.cpp \
		TaskRevolutionParameters.h \
		TaskHoleParameters.cpp \
		TaskHoleParameters.h \
		TaskScaledParameters.cpp \
		TaskScaledParameters.h \
		TaskTransformedParameters.cpp \
		TaskTransformedParameters.h \
		TaskTransformedMessages.cpp \
		TaskTransformedMessages.h \
		ViewProvider.cpp \
		ViewProvider.h \
		ViewProviderHole.cpp \
		ViewProviderHole.h \
		ViewProviderLinearPattern.cpp \
		ViewProviderLinearPattern.h \
		ViewProviderMirrored.cpp \
		ViewProviderMirrored.h \
		ViewProviderMultiTransform.cpp \
		ViewProviderMultiTransform.h \
		ViewProviderPad.cpp \
		ViewProviderPad.h \
		ViewProviderPocket.cpp \
		ViewProviderPocket.h \
		ViewProviderPolarPattern.cpp \
		ViewProviderPolarPattern.h \
		ViewProviderChamfer.cpp \
		ViewProviderChamfer.h \
		ViewProviderFillet.cpp \
		ViewProviderFillet.h \
		ViewProviderDraft.cpp \
		ViewProviderDraft.h \
		ViewProviderGroove.cpp \
		ViewProviderGroove.h \
		ViewProviderRevolution.cpp \
		ViewProviderRevolution.h \
		ViewProviderScaled.cpp \
		ViewProviderScaled.h \
		ViewProviderTransformed.cpp \
		ViewProviderTransformed.h \
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
		-lTKG2d \
		-lTKG3d \
		-lTKGeomAlgo \
		-lTKGeomBase \
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

