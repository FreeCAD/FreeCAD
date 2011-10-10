SUBDIRS=Resources

lib_LTLIBRARIES=libAssemblyGui.la AssemblyGui.la

BUILT_SOURCES=\
		moc_AssemblyView.cpp

libAssemblyGui_la_SOURCES=\
		AppAssemblyGuiPy.cpp \
		Command.cpp \
		AssemblyView.cpp \
		AssemblyView.h \
		PreCompiled.cpp \
		PreCompiled.h \
		Workbench.cpp \
		Workbench.h

# the library search path.
libAssemblyGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App $(QT_LIBS) $(GL_LIBS) \
		$(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libAssemblyGui_la_CPPFLAGS = -DAssemblyAppExport= -DAssemblyGuiExport=

libAssemblyGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lAssembly

#--------------------------------------------------------------------------------------
# Loader of libAssemblyGui

AssemblyGui_la_SOURCES=\
		AppAssemblyGui.cpp

# the library search path.
AssemblyGui_la_LDFLAGS = $(libAssemblyGui_la_LDFLAGS) -module -avoid-version
AssemblyGui_la_CPPFLAGS = $(libAssemblyGui_la_CPPFLAGS)

AssemblyGui_la_LIBADD   = \
		$(libAssemblyGui_la_LIBADD) \
		Resources/libResources.la \
		-lAssemblyGui

AssemblyGui_la_DEPENDENCIES = libAssemblyGui.la

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


libdir = $(prefix)/Mod/Assembly

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt

