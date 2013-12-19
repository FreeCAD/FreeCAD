if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${SOQT_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(PartGui_LIBS
    Part
    FreeCADGui
)

set(PartGui_MOC_HDRS
    CrossSections.h
    Mirroring.h
    DlgBooleanOperation.h
    DlgExtrusion.h
    DlgFilletEdges.h
    DlgPartBoxImp.h
    DlgPartCylinderImp.h
    DlgPartImportIgesImp.h
    DlgPartImportStepImp.h
    DlgPrimitives.h
    DlgRevolution.h
    DlgSettings3DViewPartImp.h
    DlgSettingsGeneral.h
    DlgSettingsObjectColor.h
    TaskFaceColors.h
    TaskShapeBuilder.h
    TaskLoft.h
    TaskOffset.h
    TaskSweep.h
    TaskThickness.h
    TaskDimension.h
    TaskCheckGeometry.h
)
fc_wrap_cpp(PartGui_MOC_SRCS ${PartGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${PartGui_MOC_SRCS})

qt4_add_resources(PartGui_QRC_SRCS Resources/Part.qrc)

set(PartGui_UIC_SRCS
    CrossSections.ui
    Mirroring.ui
    DlgBooleanOperation.ui
    DlgExtrusion.ui
    DlgFilletEdges.ui
    DlgPartBox.ui
    DlgPartCylinder.ui
    DlgPartImportIges.ui
    DlgPartImportStep.ui
    DlgPrimitives.ui
    Location.ui
    DlgRevolution.ui
    DlgSettings3DViewPart.ui
    DlgSettingsGeneral.ui
    DlgSettingsObjectColor.ui
    TaskFaceColors.ui
    TaskShapeBuilder.ui
    TaskLoft.ui
    TaskOffset.ui
    TaskSweep.ui
)
qt4_wrap_ui(PartGui_UIC_HDRS ${PartGui_UIC_SRCS})

SET(PartGui_SRCS
    ${PartGui_QRC_SRCS}
    ${PartGui_UIC_HDRS}
    AppPartGui.cpp
    Command.cpp
    CommandSimple.cpp
    CommandParametric.cpp
    CrossSections.cpp
    CrossSections.h
    CrossSections.ui
    Mirroring.cpp
    Mirroring.h
    Mirroring.ui
    DlgBooleanOperation.cpp
    DlgBooleanOperation.h
    DlgBooleanOperation.ui
    DlgExtrusion.cpp
    DlgExtrusion.h
    DlgExtrusion.ui
    DlgFilletEdges.cpp
    DlgFilletEdges.h
    DlgFilletEdges.ui
    DlgPartBox.ui
    DlgPartBoxImp.cpp
    DlgPartBoxImp.h
    DlgPartCylinderImp.cpp
    DlgPartCylinderImp.h
    DlgPartCylinder.ui
    DlgPartImportIges.ui
    DlgPartImportIgesImp.cpp
    DlgPartImportIgesImp.h
    DlgPartImportStep.ui
    DlgPartImportStepImp.cpp
    DlgPartImportStepImp.h
    DlgPrimitives.cpp
    DlgPrimitives.h
    DlgPrimitives.ui
    Location.ui
    DlgRevolution.cpp
    DlgRevolution.h
    DlgRevolution.ui
    DlgSettings3DViewPart.ui
    DlgSettings3DViewPartImp.cpp
    DlgSettings3DViewPartImp.h
    DlgSettingsGeneral.cpp
    DlgSettingsGeneral.h
    DlgSettingsGeneral.ui
    DlgSettingsObjectColor.cpp
    DlgSettingsObjectColor.h
    DlgSettingsObjectColor.ui
    Resources/Part.qrc
    PreCompiled.cpp
    PreCompiled.h
    SoFCShapeObject.cpp
    SoFCShapeObject.h
    SoBrepEdgeSet.cpp
    SoBrepEdgeSet.h
    SoBrepFaceSet.cpp
    SoBrepFaceSet.h
    SoBrepPointSet.cpp
    SoBrepPointSet.h
    ViewProvider.cpp
    ViewProvider.h
    ViewProviderExt.cpp
    ViewProviderExt.h
    ViewProviderReference.cpp
    ViewProviderReference.h
    ViewProviderBox.cpp
    ViewProviderBox.h
    ViewProviderCompound.cpp
    ViewProviderCompound.h
    ViewProviderCircleParametric.cpp
    ViewProviderCircleParametric.h
    ViewProviderLineParametric.cpp
    ViewProviderLineParametric.h
    ViewProviderPointParametric.cpp
    ViewProviderPointParametric.h
    ViewProviderEllipseParametric.cpp
    ViewProviderEllipseParametric.h
    ViewProviderHelixParametric.cpp
    ViewProviderHelixParametric.h
    ViewProviderPlaneParametric.cpp
    ViewProviderPlaneParametric.h
    ViewProviderSphereParametric.cpp
    ViewProviderSphereParametric.h
    ViewProviderCylinderParametric.cpp
    ViewProviderCylinderParametric.h
    ViewProviderConeParametric.cpp
    ViewProviderConeParametric.h
    ViewProviderPrism.cpp
    ViewProviderPrism.h
    ViewProviderRegularPolygon.cpp
    ViewProviderRegularPolygon.h
    ViewProviderTorusParametric.cpp
    ViewProviderTorusParametric.h
    ViewProviderCurveNet.cpp
    ViewProviderCurveNet.h
    ViewProviderSpline.cpp
    ViewProviderSpline.h
    ViewProviderImport.cpp
    ViewProviderImport.h
    ViewProviderExtrusion.cpp
    ViewProviderExtrusion.h
    ViewProvider2DObject.cpp
    ViewProvider2DObject.h
    ViewProviderMirror.cpp
    ViewProviderMirror.h
    ViewProviderPython.cpp
    ViewProviderPython.h
    ViewProviderBoolean.cpp
    ViewProviderBoolean.h
    ViewProviderRuledSurface.cpp
    ViewProviderRuledSurface.h
    Workbench.cpp
    Workbench.h
    TaskFaceColors.cpp
    TaskFaceColors.h
    TaskFaceColors.ui
    TaskShapeBuilder.cpp
    TaskShapeBuilder.h
    TaskShapeBuilder.ui
    TaskLoft.cpp
    TaskLoft.h
    TaskLoft.ui
    TaskOffset.cpp
    TaskOffset.h
    TaskOffset.ui
    TaskSweep.cpp
    TaskSweep.h
    TaskSweep.ui
    TaskThickness.cpp
    TaskThickness.h
    TaskDimension.cpp
    TaskDimension.h
    TaskCheckGeometry.cpp
    TaskCheckGeometry.h
)

SET(PartGui_Scripts
    InitGui.py
    TestPartGui.py
)


add_library(PartGui SHARED ${PartGui_SRCS})
target_link_libraries(PartGui ${PartGui_LIBS})


fc_target_copy_resource(PartGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Part
    ${CMAKE_BINARY_DIR}/Mod/Part
    ${PartGui_Scripts})

if(MSVC)
    set_target_properties(PartGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(PartGui PROPERTIES DEBUG_OUTPUT_NAME "PartGui_d")
    set_target_properties(PartGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Part)
    set_target_properties(PartGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(PartGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(PartGui PROPERTIES DEBUG_OUTPUT_NAME "PartGui_d")
    set_target_properties(PartGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Part)
    set_target_properties(PartGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(PartGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Part)
    set_target_properties(PartGui PROPERTIES PREFIX "")
    set_target_properties(PartGui PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS PartGui DESTINATION lib)
