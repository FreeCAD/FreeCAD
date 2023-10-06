include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(SketcherGui_LIBS
    Sketcher
    PartGui
    FreeCADGui
)

set (Sketcher_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Sketcher_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Sketcher_TR_QRC} ${QM_SRCS})
qt_add_resources(SketcherGui_SRCS Resources/Sketcher.qrc ${Sketcher_TR_QRC})

generate_from_xml(ViewProviderSketchGeometryExtensionPy)

set(SketcherGui_UIC_SRCS
    TaskSketcherConstraints.ui
    TaskSketcherElements.ui
    TaskSketcherMessages.ui
    TaskSketcherSolverAdvanced.ui
    TaskSketcherValidation.ui
    InsertDatum.ui
    SketchOrientationDialog.ui
    SketchMirrorDialog.ui
    SketcherSettings.ui
    SketcherSettingsGrid.ui
    SketcherSettingsColors.ui
    SketcherSettingsDisplay.ui
    SketchRectangularArrayDialog.ui
    SketcherRegularPolygonDialog.ui
)

SET(SketcherGui_SRCS
    ${SketcherGui_SRCS}
    ${SketcherGui_UIC_HDRS}
    AppSketcherGui.cpp
    GeometryCreationMode.h
    Command.cpp
    AutoConstraint.h
    Utils.h
    Utils.cpp
    DrawSketchHandlerLine.h
    DrawSketchHandlerRectangle.h
    DrawSketchHandlerPolygon.h
    DrawSketchHandlerLineSet.h
    DrawSketchHandlerCircle.h
    DrawSketchHandlerEllipse.h
    DrawSketchHandlerArc.h
    DrawSketchHandlerArcOfEllipse.h
    DrawSketchHandlerArcOfHyperbola.h
    DrawSketchHandlerArcOfParabola.h
    DrawSketchHandlerBSpline.h
    DrawSketchHandlerBSplineByInterpolation.h
    DrawSketchHandlerPoint.h
    DrawSketchHandlerFillet.h
    DrawSketchHandlerTrimming.h
    DrawSketchHandlerExtend.h
    DrawSketchHandlerSplitting.h
    DrawSketchHandlerExternal.h
    DrawSketchHandlerCarbonCopy.h
    DrawSketchHandlerSlot.h
    CommandCreateGeo.cpp
    CommandConstraints.h
    CommandConstraints.cpp
    CommandSketcherTools.cpp
    CommandSketcherBSpline.cpp
    CommandSketcherOverlay.cpp
    CommandSketcherVirtualSpace.cpp
    CommandAlterGeometry.cpp
    Resources/Sketcher.qrc
    PreCompiled.cpp
    PreCompiled.h
    SoZoomTranslation.cpp
    SoZoomTranslation.h
    PropertyConstraintListItem.h
    PropertyConstraintListItem.cpp
    TaskSketcherConstraints.ui
    TaskSketcherConstraints.cpp
    ConstraintFilters.h
    TaskSketcherConstraints.h
    TaskSketcherElements.ui
    TaskSketcherElements.cpp
    TaskSketcherElements.h
    TaskSketcherCreateCommands.cpp
    TaskSketcherCreateCommands.h
    TaskSketcherMessages.ui
    TaskSketcherMessages.cpp
    TaskSketcherMessages.h
    TaskSketcherSolverAdvanced.ui
    TaskSketcherSolverAdvanced.h
    TaskSketcherSolverAdvanced.cpp
    TaskSketcherValidation.ui
    TaskSketcherValidation.cpp
    TaskSketcherValidation.h
    ShortcutListener.cpp
    ShortcutListener.h
    EditModeInformationOverlayCoinConverter.cpp
    EditModeInformationOverlayCoinConverter.h
    EditModeGeometryCoinConverter.cpp
    EditModeGeometryCoinConverter.h
    EditModeCoinManagerParameters.h
    EditModeCoinManagerParameters.cpp
    EditModeCoinManager.cpp
    EditModeCoinManager.h
    EditModeGeometryCoinManager.cpp
    EditModeGeometryCoinManager.h
    EditModeConstraintCoinManager.cpp
    EditModeConstraintCoinManager.h
    ViewProviderSketchCoinAttorney.cpp
    ViewProviderSketchCoinAttorney.h
    ViewProviderSketch.cpp
    ViewProviderSketch.h
    DrawSketchHandler.cpp
    DrawSketchHandler.h
    Workbench.cpp
    Workbench.h
    EditDatumDialog.cpp
    EditDatumDialog.h
    PropertyVisualLayerList.cpp
    PropertyVisualLayerList.h
    SketchOrientationDialog.cpp
    SketchOrientationDialog.h
    SketchMirrorDialog.cpp
    SketchMirrorDialog.h
    SketcherSettings.cpp
    SketcherSettings.h
    SketchRectangularArrayDialog.h
    SketchRectangularArrayDialog.cpp
    SketcherRegularPolygonDialog.h
    SketcherRegularPolygonDialog.cpp
    SnapManager.cpp
    SnapManager.h
    TaskDlgEditSketch.cpp
    TaskDlgEditSketch.h
    ViewProviderPython.cpp
    ViewProviderPython.h
    ViewProviderSketchGeometryExtension.h
    ViewProviderSketchGeometryExtension.cpp
    VisualLayer.cpp
    VisualLayer.h
)

SET(Python_SRCS
    ViewProviderSketchGeometryExtensionPy.xml
    ViewProviderSketchGeometryExtensionPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(SketcherGui_SRCS
    ${SketcherGui_SRCS}
    ${Python_SRCS}
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${SketcherGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(SketcherGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)


SET(SketcherGuiIcon_SVG
    Resources/icons/SketcherWorkbench.svg
)

add_library(SketcherGui SHARED ${SketcherGui_SRCS} ${SketcherGuiIcon_SVG})
target_link_libraries(SketcherGui ${SketcherGui_LIBS})


SET_BIN_DIR(SketcherGui SketcherGui /Mod/Sketcher)
SET_PYTHON_PREFIX_SUFFIX(SketcherGui)

fc_copy_sources(SketcherGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Sketcher" ${SketcherGuiIcon_SVG})

INSTALL(TARGETS SketcherGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${SketcherGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Sketcher/Resources/icons")
