
lib_LTLIBRARIES=libPartGui.la PartGui.la

BUILT_SOURCES=\
		ui_CrossSections.h \
		ui_DlgBooleanOperation.h \
		ui_DlgExtrusion.h \
		ui_DlgFilletEdges.h \
		ui_DlgRevolution.h \
		ui_DlgPartBox.h \
		ui_DlgPartCylinder.h \
		ui_DlgPartImportIges.h \
		ui_DlgPartImportStep.h \
		ui_DlgPrimitives.h \
		ui_Location.h \
		ui_DlgSettings3DViewPart.h \
		ui_DlgSettingsGeneral.h \
		ui_Mirroring.h \
		ui_TaskFaceColors.h \
		ui_TaskShapeBuilder.h \
		ui_TaskLoft.h \
		ui_TaskOffset.h \
		ui_TaskSweep.h \
		moc_CrossSections.cpp \
		moc_DlgBooleanOperation.cpp \
		moc_DlgExtrusion.cpp \
		moc_DlgFilletEdges.cpp \
		moc_DlgRevolution.cpp \
		moc_DlgPartBoxImp.cpp \
		moc_DlgPartCylinderImp.cpp \
		moc_DlgPartImportIgesImp.cpp \
		moc_DlgPartImportStepImp.cpp \
		moc_DlgPrimitives.cpp \
		moc_DlgSettings3DViewPartImp.cpp \
		moc_DlgSettingsGeneral.cpp \
		moc_Mirroring.cpp \
		moc_TaskCheckGeometry.cpp \
		moc_TaskFaceColors.cpp \
		moc_TaskShapeBuilder.cpp \
		moc_TaskLoft.cpp \
		moc_TaskOffset.cpp \
		moc_TaskSweep.cpp \
		moc_TaskThickness.cpp \
		qrc_Part.cpp 

libPartGui_la_SOURCES=\
		Command.cpp \
		CommandSimple.cpp \
		CommandParametric.cpp \
		CrossSections.cpp \
		CrossSections.h \
		DlgBooleanOperation.cpp \
		DlgBooleanOperation.h \
		DlgExtrusion.cpp \
		DlgExtrusion.h \
		DlgFilletEdges.cpp \
		DlgFilletEdges.h \
		DlgRevolution.cpp \
		DlgRevolution.h \
		DlgPartBoxImp.cpp \
		DlgPartBoxImp.h \
		DlgPartCylinderImp.cpp \
		DlgPartCylinderImp.h \
		DlgPartImportIgesImp.cpp \
		DlgPartImportIgesImp.h \
		DlgPartImportStepImp.cpp \
		DlgPartImportStepImp.h \
		DlgPrimitives.cpp \
		DlgPrimitives.h \
		DlgSettings3DViewPartImp.cpp \
		DlgSettings3DViewPartImp.h \
		DlgSettingsGeneral.cpp \
		DlgSettingsGeneral.h \
		Mirroring.cpp \
		Mirroring.h \
		TaskCheckGeometry.cpp \
		TaskCheckGeometry.h \
		TaskFaceColors.cpp \
		TaskFaceColors.h \
		TaskShapeBuilder.cpp \
		TaskShapeBuilder.h \
		TaskLoft.cpp \
		TaskLoft.h \
		TaskOffset.cpp \
		TaskOffset.h \
		TaskSweep.cpp \
		TaskSweep.h \
		TaskThickness.cpp \
		TaskThickness.h \
		PreCompiled.cpp \
		PreCompiled.h \
		SoBrepShape.cpp \
		SoFCShapeObject.cpp \
		ViewProvider.cpp \
		ViewProviderExt.cpp \
		ViewProviderReference.cpp \
		ViewProviderBox.cpp \
		ViewProviderCurveNet.cpp \
		ViewProviderImport.cpp \
		ViewProviderExtrusion.cpp \
		ViewProviderMirror.cpp \
		ViewProvider2DObject.cpp \
		ViewProviderPython.cpp \
		ViewProviderBoolean.cpp \
		Workbench.cpp

include_HEADERS=\
		SoBrepShape.h \
		SoFCShapeObject.h \
		ViewProvider.h \
		ViewProviderExt.h \
		ViewProviderReference.h \
		ViewProviderBox.h \
		ViewProviderCurveNet.h \
		ViewProviderImport.h \
		ViewProviderExtrusion.h \
		ViewProviderMirror.h \
		ViewProvider2DObject.h \
		ViewProviderPython.h \
		ViewProviderBoolean.h \
		Workbench.h

# the library search path.
libPartGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App -L$(OCC_LIB) \
		$(sim_ac_coin_ldflags) $(sim_ac_coin_libs) $(sim_ac_soqt_ldflags) $(sim_ac_soqt_libs) \
		$(QT_LIBS) $(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libPartGui_la_CPPFLAGS = -DPartExport= -DPartGuiExport=

libPartGui_la_LIBADD   = \
		@BOOST_SIGNALS_LIB@ \
		@BOOST_SYSTEM_LIB@ \
		-lxerces-c \
		-l@PYTHON_LIB@ \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lTKernel \
		-lTKG2d \
		-lTKG3d \
		-lTKMath \
		-lTKSTEP \
		-lTKIGES \
		-lTKShHealing \
		-lTKXSBase \
		-lTKBool \
		-lTKBO \
		-lTKBRep \
		-lTKTopAlgo \
		-lTKGeomAlgo \
		-lTKGeomBase \
		-lTKPrim \
		-lTKMesh \
		-lPart 

#--------------------------------------------------------------------------------------
# Loader of libPartGui

PartGui_la_SOURCES=\
		AppPartGui.cpp

# the library search path.
PartGui_la_LDFLAGS = $(libPartGui_la_LDFLAGS) -module -avoid-version
PartGui_la_CPPFLAGS = $(libPartGui_la_CPPFLAGS)

PartGui_la_LIBADD   = \
		$(libPartGui_la_LIBADD) \
		-lPartGui

PartGui_la_DEPENDENCIES = libPartGui.la

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
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) $(QT_CXXFLAGS) -I$(OCC_INC) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir)


includedir = @includedir@/Mod/Part/Gui
libdir = $(prefix)/Mod/Part

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		Resources/icons/Part_Cylinder.svg \
		Resources/icons/Part_Box.svg \
		Resources/icons/Part_Torus.svg \
		Resources/icons/Part_Cone.svg \
		Resources/icons/Part_Sphere.svg \
		Resources/icons/Part_Booleans.svg \
		Resources/icons/Part_Chamfer.svg \
		Resources/icons/Part_CheckGeometry.svg \
		Resources/icons/Part_Cut.svg \
		Resources/icons/Part_Common.svg \
		Resources/icons/Part_CreatePrimitives.png \
		Resources/icons/Part_CreatePrimitives.svg \
		Resources/icons/Part_Fuse.svg \
		Resources/icons/Part_CrossSections.svg \
		Resources/icons/Part_Section.svg \
		Resources/icons/Part_Extrude.svg \
		Resources/icons/Part_Fillet.svg \
		Resources/icons/Part_Revolve.svg \
		Resources/icons/Part_Import.svg \
		Resources/icons/Part_Loft.svg \
		Resources/icons/Part_Mirror.svg \
		Resources/icons/Part_MirrorPNG.png \
		Resources/icons/Part_Offset.svg \
		Resources/icons/Part_RuledSurface.svg \
		Resources/icons/Part_Shapebuilder.png \
		Resources/icons/Part_Shapebuilder.svg \
		Resources/icons/Part_ShapeInfo.svg \
		Resources/icons/Part_Sweep.svg \
		Resources/icons/Part_Thickness.svg \
		Resources/icons/Tree_Part.svg \
		Resources/icons/preferences-part_design.svg \
		Resources/icons/PartFeature.svg \
		Resources/icons/PartFeature.xpm \
		Resources/icons/PartFeatureImport.xpm \
		CrossSections.ui \
		DlgBooleanOperation.ui \
		DlgExtrusion.ui \
		DlgFilletEdges.ui \
		DlgRevolution.ui \
		DlgPartBox.ui \
		DlgPartCylinder.ui \
		DlgPartImportIges.ui \
		DlgPartImportStep.ui \
		DlgPrimitives.ui \
		Location.ui \
		DlgSettings3DViewPart.ui \
		DlgSettingsGeneral.ui \
		Mirroring.ui \
		TaskFaceColors.ui \
		TaskShapeBuilder.ui \
		TaskLoft.ui \
		TaskOffset.ui \
		TaskSweep.ui \
		Resources/Part.qrc \
		Resources/translations/Part_af.qm \
		Resources/translations/Part_af.ts \
		Resources/translations/Part_de.qm \
		Resources/translations/Part_de.ts \
		Resources/translations/Part_es-ES.qm \
		Resources/translations/Part_es-ES.ts \
		Resources/translations/Part_fi.qm \
		Resources/translations/Part_fi.ts \
		Resources/translations/Part_fr.qm \
		Resources/translations/Part_fr.ts \
		Resources/translations/Part_hr.qm \
		Resources/translations/Part_hr.ts \
		Resources/translations/Part_it.qm \
		Resources/translations/Part_it.ts \
		Resources/translations/Part_nl.qm \
		Resources/translations/Part_nl.ts \
		Resources/translations/Part_no.qm \
		Resources/translations/Part_no.ts \
		Resources/translations/Part_pl.qm \
		Resources/translations/Part_pl.ts \
		Resources/translations/Part_pt-BR.qm \
		Resources/translations/Part_pt-BR.ts \
		Resources/translations/Part_ru.qm \
		Resources/translations/Part_ru.ts \
		Resources/translations/Part_sv-SE.qm \
		Resources/translations/Part_sv-SE.ts \
		Resources/translations/Part_uk.qm \
		Resources/translations/Part_uk.ts \
		Resources/translations/Part_zh-CN.qm \
		Resources/translations/Part_zh-CN.ts \
		Resources/translations/Part_zh-TW.qm \
		Resources/translations/Part_zh-TW.ts \
		Resources/translations/Part_ro.qm \
		Resources/translations/Part_ro.ts \
		Resources/translations/Part_ja.qm \
		Resources/translations/Part_ja.ts \
		Resources/translations/Part_hu.qm \
		Resources/translations/Part_hu.ts \
		Resources/translations/Part_cs.qm \
		Resources/translations/Part_cs.ts \
		Resources/translations/Part_sk.qm \
		Resources/translations/Part_sk.ts \
		Resources/translations/Part_tr.qm \
		Resources/translations/Part_tr.ts \
		CMakeLists.txt
