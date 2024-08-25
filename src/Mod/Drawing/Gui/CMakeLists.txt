include_directories (
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS})
link_directories (${OCC_LIBRARY_DIR})

set (DrawingGui_LIBS Drawing FreeCADGui)

set (DrawingGui_UIC_SRCS TaskOrthoViews.ui)

set (Drawing_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Drawing_translation.qrc)
qt_find_and_add_translation (QM_SRCS "Resources/translations/*_*.ts"
                             ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file (${Drawing_TR_QRC} ${QM_SRCS})
qt_add_resources (DrawingGui_SRCS Resources/Drawing.qrc ${Drawing_TR_QRC})

set (
    DrawingGui_SRCS
    ${DrawingGui_SRCS}
    ${QM_SRCS}
    ${Drawing_TR_QRC}
    AppDrawingGui.cpp
    AppDrawingGuiPy.cpp
    Command.cpp
    Resources/Drawing.qrc
    PreCompiled.cpp
    PreCompiled.h
    TaskDialog.cpp
    TaskDialog.h
    TaskOrthoViews.ui
    TaskOrthoViews.cpp
    TaskOrthoViews.h
    Workbench.cpp
    Workbench.h)
set (DrawingGuiView_SRCS DrawingView.cpp DrawingView.h)
set (DrawingGuiViewProvider_SRCS ViewProviderPage.cpp ViewProviderPage.h ViewProviderView.cpp
                                 ViewProviderView.h)

source_group ("Mod" FILES ${DrawingGui_SRCS})
source_group ("SVG-View" FILES ${DrawingGuiView_SRCS})
source_group ("ViewProvider" FILES ${DrawingGuiViewProvider_SRCS})

set (DrawingGuiTaskDlgs_SRCS TaskOrthoViews.ui)
source_group ("TaskDialogs" FILES ${DrawingGuiTaskDlgs_SRCS})

set (DrawingGuiComplete_SRCS ${DrawingGui_SRCS} ${DrawingGuiView_SRCS}
                             ${DrawingGuiViewProvider_SRCS})

if (FREECAD_USE_PCH)
    add_definitions (-D_PreComp_)
    get_msvc_precompiled_source ("PreCompiled.cpp" PCH_SRCS ${DrawingGuiComplete_SRCS})
    add_msvc_precompiled_header (DrawingGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif (FREECAD_USE_PCH)

set (DrawingGuiIcon_SVG Resources/icons/DrawingWorkbench.svg)

add_library (DrawingGui SHARED ${DrawingGui_SRCS} ${DrawingGuiView_SRCS}
                               ${DrawingGuiViewProvider_SRCS} ${DrawingGuiIcon_SVG})
target_link_libraries (DrawingGui ${DrawingGui_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error (DrawingGui)
endif ()

set_bin_dir (DrawingGui DrawingGui /Mod/Drawing)
set_python_prefix_suffix (DrawingGui)

fc_copy_sources (DrawingGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Drawing"
                 ${DrawingGuiIcon_SVG})

install (TARGETS DrawingGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
install (FILES ${DrawingGuiIcon_SVG}
         DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Drawing/Resources/icons")
