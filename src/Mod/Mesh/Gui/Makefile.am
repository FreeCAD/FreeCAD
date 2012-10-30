
lib_LTLIBRARIES=libMeshGui.la MeshGui.la

BUILT_SOURCES=\
		ui_DlgEvaluateMesh.h \
		ui_DlgRegularSolid.h \
		ui_DlgSettingsMeshView.h \
		ui_DlgSmoothing.h \
		ui_RemoveComponents.h \
		ui_Segmentation.h \
		moc_DlgEvaluateMeshImp.cpp \
		moc_DlgRegularSolidImp.cpp \
		moc_DlgSettingsMeshView.cpp \
		moc_DlgSmoothing.cpp \
		moc_MeshEditor.cpp \
		moc_PropertyEditorMesh.cpp \
		moc_RemoveComponents.cpp \
		qrc_Mesh.cpp 

libMeshGui_la_SOURCES=\
		Command.cpp \
		DlgEvaluateMeshImp.cpp \
		DlgEvaluateMeshImp.h \
		DlgRegularSolidImp.cpp \
		DlgRegularSolidImp.h \
		DlgSettingsMeshView.cpp \
		DlgSettingsMeshView.h \
		DlgSmoothing.cpp \
		DlgSmoothing.h \
		Doxygen.cpp \
		MeshEditor.cpp \
		MeshEditor.h \
		PreCompiled.cpp \
		PreCompiled.h \
		PropertyEditorMesh.cpp \
		RemoveComponents.cpp \
		RemoveComponents.h \
		Segmentation.cpp \
		SoFCIndexedFaceSet.cpp \
		SoFCMeshObject.cpp \
		ViewProvider.cpp \
		ViewProviderPython.cpp \
		ViewProviderMeshFaceSet.cpp \
		ViewProviderCurvature.cpp \
		ViewProviderDefects.cpp \
		ViewProviderTransform.cpp \
		ViewProviderTransformDemolding.cpp \
		Workbench.cpp

include_HEADERS=\
		PropertyEditorMesh.h \
		Segmentation.h \
		SoFCIndexedFaceSet.h \
		SoFCMeshObject.h \
		ViewProvider.h \
		ViewProviderPython.h \
		ViewProviderMeshFaceSet.h \
		ViewProviderCurvature.h \
		ViewProviderDefects.h \
		ViewProviderTransform.h \
		ViewProviderTransformDemolding.h \
		Workbench.h

# the library search path.
libMeshGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App $(QT_LIBS) \
		$(sim_ac_coin_ldflags) $(sim_ac_coin_libs) $(sim_ac_soqt_ldflags) $(sim_ac_soqt_libs) \
		$(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libMeshGui_la_CPPFLAGS = -DMeshExport= -DMeshGuiExport=

libMeshGui_la_LIBADD   = \
		@BOOST_SIGNALS_LIB@ @BOOST_SYSTEM_LIB@ \
		@GL_LIBS@ @ZIPIOS_LIB@ \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lMesh

#--------------------------------------------------------------------------------------
# Loader of libMeshGui

MeshGui_la_SOURCES=\
		AppMeshGui.cpp

# the library search path.
MeshGui_la_LDFLAGS = $(libMeshGui_la_LDFLAGS) -module -avoid-version
MeshGui_la_CPPFLAGS = $(libMeshGui_la_CPPFLAGS)

MeshGui_la_LIBADD   = \
		$(libMeshGui_la_LIBADD) \
		-l@PYTHON_LIB@ \
		-lMeshGui

MeshGui_la_DEPENDENCIES = libMeshGui.la

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
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) $(QT_CXXFLAGS) $(GTS_CFLAGS) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir)


includedir = @includedir@/Mod/Mesh/Gui
libdir = $(prefix)/Mod/Mesh

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		Resources/icons/mesh_cut.svg \
		Resources/icons/mesh_boundary.svg \
		Resources/icons/Tree_Mesh.svg \
		CMakeLists.txt \
		DlgEvaluateMesh.ui \
		DlgRegularSolid.ui \
		DlgSettingsMeshView.ui \
		DlgSmoothing.ui \
		RemoveComponents.ui \
		Segmentation.ui \
		Resources/Mesh.qrc \
		Resources/translations/Mesh_af.qm \
		Resources/translations/Mesh_af.ts \
		Resources/translations/Mesh_de.qm \
		Resources/translations/Mesh_de.ts \
		Resources/translations/Mesh_es-ES.qm \
		Resources/translations/Mesh_es-ES.ts \
		Resources/translations/Mesh_fi.qm \
		Resources/translations/Mesh_fi.ts \
		Resources/translations/Mesh_fr.qm \
		Resources/translations/Mesh_fr.ts \
		Resources/translations/Mesh_hr.qm \
		Resources/translations/Mesh_hr.ts \
		Resources/translations/Mesh_it.qm \
		Resources/translations/Mesh_it.ts \
		Resources/translations/Mesh_nl.qm \
		Resources/translations/Mesh_nl.ts \
		Resources/translations/Mesh_no.qm \
		Resources/translations/Mesh_no.ts \
		Resources/translations/Mesh_pl.qm \
		Resources/translations/Mesh_pl.ts \
		Resources/translations/Mesh_pt-BR.qm \
		Resources/translations/Mesh_pt-BR.ts \
		Resources/translations/Mesh_ru.qm \
		Resources/translations/Mesh_ru.ts \
		Resources/translations/Mesh_sv-SE.qm \
		Resources/translations/Mesh_sv-SE.ts \
		Resources/translations/Mesh_uk.qm \
		Resources/translations/Mesh_uk.ts \
		Resources/translations/Mesh_zh-CN.qm \
		Resources/translations/Mesh_zh-CN.ts \
		Resources/translations/Mesh_zh-TW.qm \
		Resources/translations/Mesh_zh-TW.ts \
		Resources/translations/Mesh_ja.qm \
		Resources/translations/Mesh_ja.ts \
		Resources/translations/Mesh_ro.qm \
		Resources/translations/Mesh_ro.ts \
		Resources/translations/Mesh_tr.qm \
		Resources/translations/Mesh_tr.ts \
		Resources/translations/Mesh_hu.qm \
		Resources/translations/Mesh_hu.ts \
		Resources/translations/Mesh_sk.qm \
		Resources/translations/Mesh_sk.ts \
		Resources/translations/Mesh_cs.qm \
		Resources/translations/Mesh_cs.ts \
		images.h
