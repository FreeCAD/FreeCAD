SUBDIRS=Resources

lib_LTLIBRARIES=libReverseEngineeringGui.la ReverseEngineeringGui.la

BUILT_SOURCES=

libReverseEngineeringGui_la_SOURCES=\
		AppReverseEngineeringGuiPy.cpp \
		Command.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		Workbench.cpp \
		Workbench.h

# the library search path.
libReverseEngineeringGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui \
		-L../../../Mod/Mesh/App -L../App -L$(OCC_LIB) $(QT_LIBS) $(GL_LIBS) \
		$(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

libReverseEngineeringGui_la_CPPFLAGS = -DReenExport= -DReenGuiExport=

libReverseEngineeringGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lMesh \
		-lReverseEngineering

#--------------------------------------------------------------------------------------
# Loader of libReverseEngineeringGui

ReverseEngineeringGui_la_SOURCES=\
		AppReverseEngineeringGui.cpp

# the library search path.
ReverseEngineeringGui_la_LDFLAGS = $(libReverseEngineeringGui_la_LDFLAGS) -module -avoid-version
ReverseEngineeringGui_la_CPPFLAGS = $(libReverseEngineeringGui_la_CPPFLAGS)

ReverseEngineeringGui_la_LIBADD   = \
		$(libReverseEngineeringGui_la_LIBADD) \
		Resources/libResources.la \
		-lReverseEngineeringGui

ReverseEngineeringGui_la_DEPENDENCIES = libReverseEngineeringGui.la

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
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(OCC_INC) $(QT_CXXFLAGS) $(all_includes)


libdir = $(prefix)/Mod/ReverseEngineering

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt

