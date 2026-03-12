# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(ReenGui_LIBS
    ReverseEngineering
    MeshGui
)

set (ReverseEngineering_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/ReverseEngineering_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${ReverseEngineering_TR_QRC} ${QM_SRCS})
qt_add_resources(ReenGui_QRC_SRCS Resources/ReverseEngineering.qrc ${ReverseEngineering_TR_QRC})

set(Dialogs_UIC_SRCS
    FitBSplineCurve.ui
    FitBSplineSurface.ui
    Poisson.ui
    Segmentation.ui
    SegmentationManual.ui
)

SET(Dialogs_SRCS
    ${Dialogs_UIC_HDRS}
    ${Dialogs_UIC_SRCS}
    FitBSplineCurve.cpp
    FitBSplineCurve.h
    FitBSplineSurface.cpp
    FitBSplineSurface.h
    Poisson.cpp
    Poisson.h
    Segmentation.cpp
    Segmentation.h
    SegmentationManual.cpp
    SegmentationManual.h
)
SOURCE_GROUP("Dialogs" FILES ${Dialogs_SRCS})

SET(ReenGui_SRCS
    ${ReenGui_QRC_SRCS}
    ${Dialogs_SRCS}
    AppReverseEngineeringGui.cpp
    Command.cpp
    Resources/ReverseEngineering.qrc
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

SET(ReverseEngineeringGuiIcon_SVG
    Resources/icons/ReverseEngineeringWorkbench.svg
)

add_library(ReverseEngineeringGui SHARED
    ${ReenGui_SRCS}
    ${ReverseEngineeringGuiIcon_SVG}
)

if(FREECAD_USE_PCH)
    target_precompile_headers(ReverseEngineeringGui PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    ReverseEngineeringGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(ReverseEngineeringGui ${ReenGui_LIBS})

if (MSVC)
    target_compile_options(ReverseEngineeringGui PRIVATE /wd4275)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(ReverseEngineeringGui)
endif()

SET_BIN_DIR(ReverseEngineeringGui ReverseEngineeringGui /Mod/ReverseEngineering)
SET_PYTHON_PREFIX_SUFFIX(ReverseEngineeringGui)

fc_copy_sources(ReverseEngineeringGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/ReverseEngineering" ${ReverseEngineeringGuiIcon_SVG})

install(TARGETS ReverseEngineeringGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${ReverseEngineeringGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/ReverseEngineering/Resources/icons")
