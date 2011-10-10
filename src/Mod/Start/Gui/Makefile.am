SUBDIRS=Resources

lib_LTLIBRARIES=libStartGui.la StartGui.la

libStartGui_la_SOURCES=\
		AppStartGuiPy.cpp \
		Command.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		Workbench.cpp \
		Workbench.h

# the library search path.
libStartGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App $(QT_LIBS) $(GL_LIBS) \
		$(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libStartGui_la_CPPFLAGS = -DStartAppExport= -DStartGuiExport=

libStartGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lStart

#--------------------------------------------------------------------------------------
# Loader of libStartGui

StartGui_la_SOURCES=\
		AppStartGui.cpp

# the library search path.
StartGui_la_LDFLAGS = $(libStartGui_la_LDFLAGS) -module -avoid-version
StartGui_la_CPPFLAGS = $(libStartGui_la_CPPFLAGS)

StartGui_la_LIBADD   = \
		$(libStartGui_la_LIBADD) \
		Resources/libResources.la \
		-lStartGui

StartGui_la_DEPENDENCIES = libStartGui.la

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

libdir = $(prefix)/Mod/Start

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt

