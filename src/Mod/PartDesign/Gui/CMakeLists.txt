include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src/3rdParty/json/single_include/nlohmann/
)
link_directories(${OCC_LIBRARY_DIR})

generate_from_xml(ViewProviderPy)


set(PartDesignGui_LIBS
    PartDesign
    SketcherGui
    PartGui
    SketcherGui
    FreeCADGui
)

set (PartDesign_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/PartDesign_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${PartDesign_TR_QRC} ${QM_SRCS})
qt_add_resources(PartDesignGui_SRCS Resources/PartDesign.qrc ${PartDesign_TR_QRC})

set(PartDesignGui_UIC_SRCS
    TaskFeaturePick.ui
    TaskPadPocketParameters.ui
    TaskChamferParameters.ui
    TaskFilletParameters.ui
    TaskDraftParameters.ui
    TaskThicknessParameters.ui
    TaskBooleanParameters.ui
    TaskHoleParameters.ui
    TaskRevolutionParameters.ui
    TaskTransformedMessages.ui
    TaskMirroredParameters.ui
    TaskLinearPatternParameters.ui
    TaskPolarPatternParameters.ui
    TaskScaledParameters.ui
    TaskMultiTransformParameters.ui
    TaskShapeBinder.ui
    TaskPrimitiveParameters.ui
    TaskPipeParameters.ui
    TaskPipeOrientation.ui
    TaskPipeScaling.ui
    TaskLoftParameters.ui
    DlgReference.ui
    DlgActiveBody.ui
    TaskHelixParameters.ui
)

SET(PartDesignGuiViewProvider_SRCS
    ViewProvider.cpp
    ViewProvider.h
    ViewProviderBody.cpp
    ViewProviderBody.h
    ViewProviderSketchBased.cpp
    ViewProviderSketchBased.h
    ViewProviderPad.cpp
    ViewProviderPad.h
    ViewProviderHole.cpp
    ViewProviderHole.h
    ViewProviderPocket.cpp
    ViewProviderPocket.h
    ViewProviderChamfer.cpp
    ViewProviderChamfer.h
    ViewProviderFillet.cpp
    ViewProviderFillet.h
    ViewProviderDraft.cpp
    ViewProviderDraft.h
    ViewProviderThickness.cpp
    ViewProviderThickness.h
    ViewProviderDressUp.cpp
    ViewProviderDressUp.h
    ViewProviderRevolution.cpp
    ViewProviderRevolution.h
    ViewProviderGroove.cpp
    ViewProviderGroove.h
    ViewProviderTransformed.cpp
    ViewProviderTransformed.h
    ViewProviderMirrored.cpp
    ViewProviderMirrored.h
    ViewProviderLinearPattern.cpp
    ViewProviderLinearPattern.h
    ViewProviderPolarPattern.cpp
    ViewProviderPolarPattern.h
    ViewProviderScaled.cpp
    ViewProviderScaled.h
    ViewProviderMultiTransform.cpp
    ViewProviderMultiTransform.h
    ViewProviderDatum.cpp
    ViewProviderDatum.h
    ViewProviderDatumPoint.cpp
    ViewProviderDatumPoint.h
    ViewProviderDatumLine.cpp
    ViewProviderDatumLine.h
    ViewProviderDatumPlane.cpp
    ViewProviderDatumPlane.h
    ViewProviderDatumCS.cpp
    ViewProviderDatumCS.h
    ViewProviderShapeBinder.h
    ViewProviderShapeBinder.cpp
    ViewProviderBoolean.cpp
    ViewProviderBoolean.h
    ViewProviderAddSub.cpp
    ViewProviderAddSub.h
    ViewProviderPrimitive.h
    ViewProviderPrimitive.cpp
    ViewProviderPipe.h
    ViewProviderPipe.cpp
    ViewProviderLoft.h
    ViewProviderLoft.cpp
    ViewProviderHelix.h
    ViewProviderHelix.cpp
    ViewProviderBase.h
    ViewProviderBase.cpp
)
SOURCE_GROUP("ViewProvider" FILES ${PartDesignGuiViewProvider_SRCS})

SET(PartDesignGuiTaskDlgs_SRCS
    TaskFeaturePick.ui
    TaskFeaturePick.cpp
    TaskFeaturePick.h
    ReferenceSelection.cpp
    ReferenceSelection.h
    TaskFeatureParameters.cpp
    TaskFeatureParameters.h
    TaskSketchBasedParameters.cpp
    TaskSketchBasedParameters.h
    TaskExtrudeParameters.cpp
    TaskExtrudeParameters.h
    TaskPadPocketParameters.ui
    TaskPadParameters.cpp
    TaskPadParameters.h
    TaskPocketParameters.cpp
    TaskPocketParameters.h
    TaskChamferParameters.ui
    TaskChamferParameters.cpp
    TaskChamferParameters.h
    TaskFilletParameters.ui
    TaskFilletParameters.cpp
    TaskFilletParameters.h
    TaskDraftParameters.ui
    TaskDraftParameters.cpp
    TaskDraftParameters.h
    TaskThicknessParameters.ui
    TaskThicknessParameters.cpp
    TaskThicknessParameters.h
    TaskDressUpParameters.cpp
    TaskDressUpParameters.h
    TaskRevolutionParameters.ui
    TaskRevolutionParameters.cpp
    TaskRevolutionParameters.h
    TaskTransformedMessages.ui
    TaskTransformedMessages.cpp
    TaskTransformedMessages.h
    TaskTransformedParameters.cpp
    TaskTransformedParameters.h
    TaskMirroredParameters.ui
    TaskMirroredParameters.cpp
    TaskMirroredParameters.h
    TaskLinearPatternParameters.ui
    TaskLinearPatternParameters.cpp
    TaskLinearPatternParameters.h
    TaskPolarPatternParameters.ui
    TaskPolarPatternParameters.cpp
    TaskPolarPatternParameters.h
    TaskScaledParameters.ui
    TaskScaledParameters.cpp
    TaskScaledParameters.h
    TaskMultiTransformParameters.ui
    TaskMultiTransformParameters.cpp
    TaskMultiTransformParameters.h
    TaskHoleParameters.ui
    TaskHoleParameters.cpp
    TaskHoleParameters.h
    TaskDatumParameters.cpp
    TaskDatumParameters.h
    TaskShapeBinder.ui
    TaskShapeBinder.cpp
    TaskShapeBinder.h
    TaskBooleanParameters.ui
    TaskBooleanParameters.cpp
    TaskBooleanParameters.h
    TaskPrimitiveParameters.h
    TaskPrimitiveParameters.cpp
    TaskPipeParameters.ui
    TaskPipeOrientation.ui
    TaskPipeScaling.ui
    TaskPipeParameters.h
    TaskPipeParameters.cpp
    TaskLoftParameters.ui
    TaskLoftParameters.h
    TaskLoftParameters.cpp
    TaskHelixParameters.ui
    TaskHelixParameters.h
    TaskHelixParameters.cpp
    DlgActiveBody.h
    DlgActiveBody.cpp
)
SOURCE_GROUP("TaskDialogs" FILES ${PartDesignGuiTaskDlgs_SRCS})

SET(PartDesignGuiModule_SRCS
    AppPartDesignGui.cpp
    Command.cpp
    CommandPrimitive.cpp
    CommandBody.cpp
    EnumFlags.h
    Resources/PartDesign.qrc
    PreCompiled.cpp
    PreCompiled.h
    SketchWorkflow.cpp
    SketchWorkflow.h
    Utils.cpp
    Utils.h
    Workbench.cpp
    Workbench.h
    WorkflowManager.cpp
    WorkflowManager.h
)
SOURCE_GROUP("Module" FILES ${PartDesignGuiModule_SRCS})


SET(Python_SRCS
    ViewProviderPy.xml
    ViewProviderPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})


SET(PartDesignGui_SRCS
    ${PartDesignGui_SRCS}
    ${PartDesignGui_UIC_HDRS}
    ${PartDesignGuiModule_SRCS}
    ${PartDesignGuiTaskDlgs_SRCS}
    ${PartDesignGuiViewProvider_SRCS}
    ${Python_SRCS}
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${PartDesignGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(PartDesignGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)


SET(PartDesignGuiIcon_SVG
    Resources/icons/PartDesignWorkbench.svg
)

add_library(PartDesignGui SHARED ${PartDesignGui_SRCS} ${PartDesignGuiIcon_SVG})
target_link_libraries(PartDesignGui ${PartDesignGui_LIBS})

SET_BIN_DIR(PartDesignGui PartDesignGui /Mod/PartDesign)
SET_PYTHON_PREFIX_SUFFIX(PartDesignGui)

fc_copy_sources(PartDesignGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/PartDesign" ${PartDesignGuiIcon_SVG})

INSTALL(TARGETS PartDesignGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${PartDesignGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/PartDesign/Resources/icons")
