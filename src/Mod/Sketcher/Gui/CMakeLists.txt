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
    ${XercesC_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(SketcherGui_LIBS
    Sketcher
    PartGui
    FreeCADGui
)

set(SketcherGui_MOC_HDRS
    TaskSketcherConstrains.h
    TaskSketcherElements.h
    TaskSketcherCreateCommands.h
    TaskSketcherGeneral.h
    TaskSketcherMessages.h
    TaskSketcherSolverAdvanced.h
    TaskSketcherValidation.h
    TaskDlgEditSketch.h
    SketchOrientationDialog.h
    SketchMirrorDialog.h
    SketcherSettings.h
    SketchRectangularArrayDialog.h
    PropertyConstraintListItem.h
)
fc_wrap_cpp(SketcherGui_MOC_SRCS ${SketcherGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${SketcherGui_MOC_SRCS})

if(BUILD_QT5)
    qt5_add_resources(SketcherGui_SRCS Resources/Sketcher.qrc)
else()
    qt4_add_resources(SketcherGui_SRCS Resources/Sketcher.qrc)
endif()

set(SketcherGui_UIC_SRCS
    TaskSketcherConstrains.ui
    TaskSketcherElements.ui
    TaskSketcherGeneral.ui
    TaskSketcherMessages.ui
    TaskSketcherSolverAdvanced.ui
    TaskSketcherValidation.ui
    InsertDatum.ui
    SketchOrientationDialog.ui
    SketchMirrorDialog.ui
    SketcherSettings.ui
    SketchRectangularArrayDialog.ui
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
    CommandCreateGeo.cpp
    CommandConstraints.h
    CommandConstraints.cpp
    CommandSketcherTools.cpp
    CommandSketcherBSpline.cpp
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
    TaskSketcherConstrains.ui
    TaskSketcherConstrains.cpp
    TaskSketcherConstrains.h
    TaskSketcherElements.ui
    TaskSketcherElements.cpp
    TaskSketcherElements.h
    TaskSketcherCreateCommands.cpp
    TaskSketcherCreateCommands.h
    TaskSketcherGeneral.ui
    TaskSketcherGeneral.cpp
    TaskSketcherGeneral.h
    TaskSketcherMessages.ui
    TaskSketcherMessages.cpp
    TaskSketcherMessages.h
    TaskSketcherSolverAdvanced.ui
    TaskSketcherSolverAdvanced.h
    TaskSketcherSolverAdvanced.cpp
    TaskSketcherValidation.ui
    TaskSketcherValidation.cpp
    TaskSketcherValidation.h
    ViewProviderSketch.cpp
    ViewProviderSketch.h
    DrawSketchHandler.cpp
    DrawSketchHandler.h
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
    TaskDlgEditSketch.cpp
    TaskDlgEditSketch.h
    ViewProviderPython.cpp
    ViewProviderPython.h
)

SET(SketcherGui_Scripts
    InitGui.py
    TestSketcherGui.py
)

add_library(SketcherGui SHARED ${SketcherGui_SRCS})
target_link_libraries(SketcherGui ${SketcherGui_LIBS})


fc_target_copy_resource(SketcherGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Sketcher
    ${CMAKE_BINARY_DIR}/Mod/Sketcher
    ${SketcherGui_Scripts})

SET_BIN_DIR(SketcherGui SketcherGui /Mod/Sketcher)
SET_PYTHON_PREFIX_SUFFIX(SketcherGui)

INSTALL(TARGETS SketcherGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

SET(SketcherGuiIcon_SVG
    Resources/icons/SketcherWorkbench.svg
)

fc_copy_sources(SketcherGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Sketcher" ${SketcherGuiIcon_SVG})

INSTALL(FILES ${SketcherGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Sketcher/Resources/icons")
