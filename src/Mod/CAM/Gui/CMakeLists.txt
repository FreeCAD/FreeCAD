include_directories (
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS})

link_directories (${OCC_LIBRARY_DIR})

set (PathGui_LIBS Path PartGui FreeCADGui)

set (Path_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Path_translation.qrc)
qt_find_and_add_translation (QM_SRCS "Resources/translations/*_*.ts"
                             ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file (${Path_TR_QRC} ${QM_SRCS})
qt_add_resources (PathResource_SRCS Resources/Path.qrc ${Path_TR_QRC})

source_group ("Resources" FILES ${PathResource_SRCS})

set (PathGui_UIC_SRCS DlgSettingsPathColor.ui TaskDlgPathCompound.ui DlgProcessorChooser.ui)

set (
    PathGui_SRCS_Module
    Command.cpp
    AppPathGui.cpp
    AppPathGuiPy.cpp
    Resources/Path.qrc
    PreCompiled.cpp
    PreCompiled.h
    DlgSettingsPathColor.ui
    DlgSettingsPathColor.cpp
    DlgSettingsPathColor.h
    TaskDlgPathCompound.ui
    TaskDlgPathCompound.cpp
    TaskDlgPathCompound.h
    DlgProcessorChooser.ui
    DlgProcessorChooser.cpp
    DlgProcessorChooser.h)

source_group ("Module" FILES ${PathGui_SRCS_Module})

set (
    PathGui_SRCS_ViewProvider
    ViewProviderPath.cpp
    ViewProviderPath.h
    ViewProviderPathCompound.cpp
    ViewProviderPathCompound.h
    ViewProviderPathShape.cpp
    ViewProviderPathShape.h
    ViewProviderArea.cpp
    ViewProviderArea.h)

source_group ("ViewProvider" FILES ${PathGui_SRCS_ViewProvider})

set (PathGui_SRCS ${PathResource_SRCS} ${PathGui_UIC_HDRS} ${PathGui_SRCS_Module}
                  ${PathGui_SRCS_ViewProvider})

if (FREECAD_USE_PCH)
    add_definitions (-D_PreComp_)
    get_msvc_precompiled_source ("PreCompiled.cpp" PCH_SRCS ${PathGui_SRCS})
    add_msvc_precompiled_header (PathGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif (FREECAD_USE_PCH)

set (PathGuiIcon_SVG Resources/icons/CAMWorkbench.svg)

add_library (PathGui SHARED ${PathGui_SRCS} ${PathGuiIcon_SVG})
target_link_libraries (PathGui ${PathGui_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error (PathGui)
endif ()

set_bin_dir (PathGui PathGui /Mod/CAM)
set_python_prefix_suffix (PathGui)

fc_copy_sources (PathGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/CAM" ${PathGuiIcon_SVG})

install (TARGETS PathGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
install (FILES ${PathGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/CAM/Resources/icons")
