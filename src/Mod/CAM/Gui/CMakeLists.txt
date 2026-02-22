# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(PathGui_LIBS
    Path
    PartGui
    FreeCADGui
)

set (Path_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Path_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Path_TR_QRC} ${QM_SRCS})
qt_add_resources(PathResource_SRCS Resources/Path.qrc ${Path_TR_QRC})

SOURCE_GROUP("Resources" FILES ${PathResource_SRCS})

set(PathGui_UIC_SRCS
    DlgSettingsPathColor.ui
    TaskDlgPathCompound.ui
    DlgProcessorChooser.ui
)

SET(PathGui_SRCS_Module
    Command.cpp
    AppPathGui.cpp
    AppPathGuiPy.cpp
    Resources/Path.qrc
    PreCompiled.h
    DlgSettingsPathColor.ui
    DlgSettingsPathColor.cpp
    DlgSettingsPathColor.h
    TaskDlgPathCompound.ui
    TaskDlgPathCompound.cpp
    TaskDlgPathCompound.h
    DlgProcessorChooser.ui
    DlgProcessorChooser.cpp
    DlgProcessorChooser.h
)

SOURCE_GROUP("Module" FILES ${PathGui_SRCS_Module})

SET(PathGui_SRCS_ViewProvider
    ViewProviderPath.cpp
    ViewProviderPath.h
    ViewProviderPathCompound.cpp
    ViewProviderPathCompound.h
    ViewProviderPathShape.cpp
    ViewProviderPathShape.h
    ViewProviderArea.cpp
    ViewProviderArea.h
)

SOURCE_GROUP("ViewProvider" FILES ${PathGui_SRCS_ViewProvider})

SET(PathGui_SRCS
    ${PathResource_SRCS}
    ${PathGui_UIC_HDRS}
    ${PathGui_SRCS_Module}
    ${PathGui_SRCS_ViewProvider}
)

SET(PathGuiIcon_SVG
    Resources/icons/CAMWorkbench.svg
)

add_library(PathGui SHARED ${PathGui_SRCS} ${PathGuiIcon_SVG})

if(FREECAD_USE_PCH)
    target_precompile_headers(PathGui PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    PathGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(PathGui ${PathGui_LIBS})

if (MSVC)
    target_compile_options(PathGui PRIVATE /wd4273)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(PathGui)
endif()

SET_BIN_DIR(PathGui PathGui /Mod/CAM)
SET_PYTHON_PREFIX_SUFFIX(PathGui)

fc_copy_sources(PathGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/CAM" ${PathGuiIcon_SVG})

INSTALL(TARGETS PathGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${PathGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/CAM/Resources/icons")
