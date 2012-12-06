SUBDIRS=Resources

lib_LTLIBRARIES=libDrawingGui.la DrawingGui.la

BUILT_SOURCES=\
		moc_DrawingView.cpp \
		moc_TaskOrthoViews.cpp \
		ui_TaskOrthoViews.h \
		moc_TaskDialog.cpp

libDrawingGui_la_UI=\
        TaskOrthoViews.ui

libDrawingGui_la_SOURCES=\
		AppDrawingGuiPy.cpp \
		Command.cpp \
		DrawingView.cpp \
		DrawingView.h \
		PreCompiled.cpp \
		PreCompiled.h \
		TaskDialog.cpp \
		TaskDialog.h \
		TaskOrthoViews.cpp \
		TaskOrthoViews.h \
		ViewProviderPage.cpp \
		ViewProviderPage.h \
		ViewProviderView.cpp \
		ViewProviderView.h \
		Workbench.cpp \
		Workbench.h

# the library search path.
libDrawingGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../../Part/App -L../App \
		-L$(OCC_LIB) $(QT_LIBS) $(GL_LIBS) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libDrawingGui_la_CPPFLAGS = -DDrawingAppExport= -DDrawingGuiExport=

libDrawingGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lTKernel \
		-lTKMath \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lPart \
		-lDrawing

#--------------------------------------------------------------------------------------
# Loader of libDrawingGui

DrawingGui_la_SOURCES=\
		AppDrawingGui.cpp

# the library search path.
DrawingGui_la_LDFLAGS = $(libDrawingGui_la_LDFLAGS) -module -avoid-version
DrawingGui_la_CPPFLAGS = $(libDrawingGui_la_CPPFLAGS)

DrawingGui_la_LIBADD   = \
		$(libDrawingGui_la_LIBADD) \
		Resources/libResources.la \
		-lDrawingGui

DrawingGui_la_DEPENDENCIES = libDrawingGui.la

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
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) $(QT_CXXFLAGS) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir) -I$(OCC_INC)


libdir = $(prefix)/Mod/Drawing

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		$(libDrawingGui_la_UI) \
		CMakeLists.txt

