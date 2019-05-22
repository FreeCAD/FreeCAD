if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

generate_from_xml(ViewProviderPartExtPy)

set(PartGui_LIBS
    Part
    FreeCADGui
)

if(MSVC)
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/3rdParty/OpenGL/api
    )
endif(MSVC)

if (BUILD_QT5)
    include_directories(
        ${Qt5Concurrent_INCLUDE_DIRS}
    )
    list(APPEND PartGui_LIBS
        ${Qt5Concurrent_LIBRARIES}
    )
endif()

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
    DlgProjectionOnSurface.h
    PropertyEnumAttacherItem.h
    TaskFaceColors.h
    TaskShapeBuilder.h
    TaskLoft.h
    TaskOffset.h
    TaskSweep.h
    TaskThickness.h
    TaskDimension.h
    TaskCheckGeometry.h
    TaskAttacher.h
)
fc_wrap_cpp(PartGui_MOC_SRCS ${PartGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${PartGui_MOC_SRCS})

if (BUILD_QT5)
    qt5_add_resources(PartGui_QRC_SRCS Resources/Part.qrc)
else()
    qt4_add_resources(PartGui_QRC_SRCS Resources/Part.qrc)
endif()

set(PartGui_UIC_SRCS
    CrossSections.ui
    Mirroring.ui
    DlgBooleanOperation.ui
    DlgExtrusion.ui
    DlgFilletEdges.ui
    DlgImportExportIges.ui
    DlgImportExportStep.ui
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
    DlgProjectionOnSurface.ui
    TaskFaceColors.ui
    TaskShapeBuilder.ui
    TaskLoft.ui
    TaskOffset.ui
    TaskSweep.ui
    TaskAttacher.ui
)

if (BUILD_QT5)
    qt5_wrap_ui(PartGui_UIC_HDRS ${PartGui_UIC_SRCS})
else()
    qt4_wrap_ui(PartGui_UIC_HDRS ${PartGui_UIC_SRCS})
endif()

SET(PartGui_SRCS
    ${PartGui_QRC_SRCS}
    ${PartGui_UIC_HDRS}
    AppPartGui.cpp
    AttacherTexts.h
    AttacherTexts.cpp
    BoxSelection.cpp
    BoxSelection.h
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
    DlgImportExportIges.ui
    DlgImportExportStep.ui
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
    DlgProjectionOnSurface.cpp
    DlgProjectionOnSurface.h
    DlgProjectionOnSurface.ui
    Resources/Part.qrc
    PreCompiled.cpp
    PreCompiled.h
    PropertyEnumAttacherItem.cpp
    PropertyEnumAttacherItem.h
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
    TaskAttacher.h 
    TaskAttacher.cpp 
    ViewProviderPartExtPy.xml
    ViewProviderPartExtPyImp.cpp
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${PartGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(PartGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

SET(PartGuiIcon_SVG
    Resources/icons/PartWorkbench.svg
)

add_library(PartGui SHARED ${PartGui_SRCS} ${PartGuiIcon_SVG})
target_link_libraries(PartGui ${PartGui_LIBS})


SET_BIN_DIR(PartGui PartGui /Mod/Part)
SET_PYTHON_PREFIX_SUFFIX(PartGui)

fc_copy_sources(PartGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Part" ${PartGuiIcon_SVG})

INSTALL(TARGETS PartGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${PartGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Part/Resources/icons")
