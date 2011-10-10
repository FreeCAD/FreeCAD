SUBDIRS=Resources

lib_LTLIBRARIES=libWebGui.la WebGui.la

BUILT_SOURCES=\
		moc_BrowserView.cpp

libWebGui_la_SOURCES=\
		AppWebGuiPy.cpp \
		Command.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		BrowserView.cpp \
		BrowserView.h \
		Workbench.cpp \
		Workbench.h

# the library search path.
libWebGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui $(QT_LIBS) $(GL_LIBS) \
		$(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libWebGui_la_CPPFLAGS = -DWebAppExport= -DWebGuiExport=

libWebGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui 

#--------------------------------------------------------------------------------------
# Loader of libWebGui

WebGui_la_SOURCES=\
		AppWebGui.cpp

# the library search path.
WebGui_la_LDFLAGS = $(libWebGui_la_LDFLAGS) -module -avoid-version
WebGui_la_CPPFLAGS = $(libWebGui_la_CPPFLAGS)

WebGui_la_LIBADD   = \
		$(libWebGui_la_LIBADD) \
		Resources/libResources.la \
		-lWebGui

WebGui_la_DEPENDENCIES = libWebGui.la

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
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(QT_CXXFLAGS) $(all_includes)

libdir = $(prefix)/Mod/Web

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt

