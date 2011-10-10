SUBDIRS=Resources

lib_LTLIBRARIES=libCompleteGui.la CompleteGui.la

libCompleteGui_la_SOURCES=\
		AppCompleteGuiPy.cpp \
		Command.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		Workbench.cpp \
		Workbench.h

# the library search path.
libCompleteGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App $(QT_LIBS) $(GL_LIBS) \
		$(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libCompleteGui_la_CPPFLAGS = -DCompleteAppExport= -DCompleteGuiExport=

libCompleteGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lComplete

#--------------------------------------------------------------------------------------
# Loader of libCompleteGui

CompleteGui_la_SOURCES=\
		AppCompleteGui.cpp

# the library search path.
CompleteGui_la_LDFLAGS = $(libCompleteGui_la_LDFLAGS) -module -avoid-version
CompleteGui_la_CPPFLAGS = $(libCompleteGui_la_CPPFLAGS)

CompleteGui_la_LIBADD   = \
		$(libCompleteGui_la_LIBADD) \
		Resources/libResources.la \
		-lCompleteGui

CompleteGui_la_DEPENDENCIES = libCompleteGui.la

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

libdir = $(prefix)/Mod/Complete

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt

