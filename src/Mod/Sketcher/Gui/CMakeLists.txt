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
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(SketcherGui_LIBS
    Sketcher
    PartGui
    FreeCADGui
)

if(BUILD_QT5)
    qt5_add_resources(SketcherGui_SRCS Resources/Sketcher.qrc)
else()
    qt4_add_resources(SketcherGui_SRCS Resources/Sketcher.qrc)
endif()

set(SketcherGui_UIC_SRCS
    TaskSketcherConstraints.ui
    TaskSketcherElements.ui
    TaskSketcherGeneral.ui
    SketcherToolDefaultWidget.ui
    TaskSketcherMessages.ui
    TaskSketcherSolverAdvanced.ui
    TaskSketcherValidation.ui
    InsertDatum.ui
    SketchOrientationDialog.ui
    SketchMirrorDialog.ui
    SketcherSettings.ui
    SketcherSettingsColors.ui
    SketcherSettingsDisplay.ui
    SketchRectangularArrayDialog.ui
    SketcherRegularPolygonDialog.ui
    ConstraintMultiFilterDialog.ui
    ConstraintSettingsDialog.ui
)

if(BUILD_QT5)
    qt5_wrap_ui(SketcherGui_UIC_HDRS ${SketcherGui_UIC_SRCS})
else()
    qt4_wrap_ui(SketcherGui_UIC_HDRS ${SketcherGui_UIC_SRCS})
endif()

SET(SketcherGui_SRCS
    ${SketcherGui_SRCS}
    ${SketcherGui_UIC_HDRS}
    AppSketcherGui.cpp
    GeometryCreationMode.h
    Command.cpp
    AutoConstraint.h
    Utils.h
    Utils.cpp
    CircleEllipseConstructionMethod.h
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
    DrawSketchHandlerPoint.h
    DrawSketchHandlerFillet.h
    DrawSketchHandlerTrimming.h
    DrawSketchHandlerExtend.h
    DrawSketchHandlerSplitting.h
    DrawSketchHandlerInsert.h
    DrawSketchHandlerExternal.h
    DrawSketchHandlerCarbonCopy.h
    DrawSketchHandlerSlot.h
    DrawSketchHandlerArcSlot.h
    DrawSketchHandlerRectangularArray.h
    DrawSketchHandlerTranslate.h
    CommandCreateGeo.cpp
    CommandConstraints.h
    CommandConstraints.cpp
    CommandSketcherTools.cpp
    CommandSketcherBSpline.cpp
    CommandSketcherVirtualSpace.cpp
    CommandAlterGeometry.cpp
    Resources/Sketcher.qrc
    PreCompiled.cpp
    PreCompiled.h
    SoZoomTranslation.cpp
    SoZoomTranslation.h
    SoDatumLabel.cpp
    SoDatumLabel.h
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
    TaskSketcherGeneral.ui
    TaskSketcherGeneral.cpp
    TaskSketcherGeneral.h
    SketcherToolDefaultWidget.ui
    SketcherToolDefaultWidget.cpp
    SketcherToolDefaultWidget.h
    TaskSketcherTool.cpp
    TaskSketcherTool.h
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
    DrawSketchDefaultHandler.h
    DrawSketchDefaultWidgetHandler.h
    Workbench.cpp
    Workbench.h
    EditDatumDialog.cpp
    EditDatumDialog.h
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
    ConstraintMultiFilterDialog.h
    ConstraintMultiFilterDialog.cpp
    ConstraintSettingsDialog.h
    ConstraintSettingsDialog.cpp
    TaskDlgEditSketch.cpp
    TaskDlgEditSketch.h
    ViewProviderPython.cpp
    ViewProviderPython.h
    ViewProviderSketchGeometryExtension.h
    ViewProviderSketchGeometryExtension.cpp
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
