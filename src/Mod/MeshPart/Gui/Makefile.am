SUBDIRS=Resources

lib_LTLIBRARIES=libMeshPartGui.la MeshPartGui.la

BUILT_SOURCES=\
		moc_Tessellation.cpp \
		ui_Tessellation.h

libMeshPartGui_la_UI=\
		Tessellation.ui

libMeshPartGui_la_SOURCES=\
		AppMeshPartGuiPy.cpp \
		Command.cpp \
		Tessellation.cpp \
		Tessellation.h \
		PreCompiled.cpp \
		PreCompiled.h \
		Workbench.cpp \
		Workbench.h

# the library search path.
libMeshPartGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App \
		-L../../Mesh/App -L../../Part/App \
		-L$(OCC_LIB) $(QT_LIBS) $(GL_LIBS) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

libMeshPartGui_la_CPPFLAGS = -DMeshPartAppExport= -DMeshPartGuiExport=

libMeshPartGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lMesh \
		-lPart \
		-lMeshPart \
		-lTKernel \
		-lTKMath \
		-lTKXSBase \
		-lTKBRep \
		-lTKTopAlgo \
		-lTKGeomAlgo \
		-lTKGeomBase

#--------------------------------------------------------------------------------------
# Loader of libMeshPartGui

MeshPartGui_la_SOURCES=\
		AppMeshPartGui.cpp

# the library search path.
MeshPartGui_la_LDFLAGS = $(libMeshPartGui_la_LDFLAGS) -module -avoid-version
MeshPartGui_la_CPPFLAGS = $(libMeshPartGui_la_CPPFLAGS)

MeshPartGui_la_LIBADD   = \
		$(libMeshPartGui_la_LIBADD) \
		Resources/libResources.la \
		-lMeshPartGui

MeshPartGui_la_DEPENDENCIES = libMeshPartGui.la

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
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) \
		-I$(OCC_INC) $(QT_CXXFLAGS)


libdir = $(prefix)/Mod/MeshPart

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		$(libMeshPartGui_la_UI) \
		CMakeLists.txt

