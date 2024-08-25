include_directories (
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS})
link_directories (${OCC_LIBRARY_DIR})

set (ReenGui_LIBS ReverseEngineering MeshGui)

set (ReverseEngineering_TR_QRC
     ${CMAKE_CURRENT_BINARY_DIR}/Resources/ReverseEngineering_translation.qrc)
qt_find_and_add_translation (QM_SRCS "Resources/translations/*_*.ts"
                             ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file (${ReverseEngineering_TR_QRC} ${QM_SRCS})
qt_add_resources (ReenGui_QRC_SRCS Resources/ReverseEngineering.qrc ${ReverseEngineering_TR_QRC})

set (Dialogs_UIC_SRCS FitBSplineCurve.ui FitBSplineSurface.ui Poisson.ui Segmentation.ui
                      SegmentationManual.ui)

set (
    Dialogs_SRCS
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
    SegmentationManual.h)
source_group ("Dialogs" FILES ${Dialogs_SRCS})

set (
    ReenGui_SRCS
    ${ReenGui_QRC_SRCS}
    ${Dialogs_SRCS}
    AppReverseEngineeringGui.cpp
    Command.cpp
    Resources/ReverseEngineering.qrc
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h)

set (ReverseEngineeringGuiIcon_SVG Resources/icons/ReverseEngineeringWorkbench.svg)

if (FREECAD_USE_PCH)
    add_definitions (-D_PreComp_)
    get_msvc_precompiled_source ("PreCompiled.cpp" PCH_SRCS ${ReenGui_SRCS})
    add_msvc_precompiled_header (ReverseEngineeringGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif (FREECAD_USE_PCH)

add_library (ReverseEngineeringGui SHARED ${ReenGui_SRCS} ${ReverseEngineeringGuiIcon_SVG})
target_link_libraries (ReverseEngineeringGui ${ReenGui_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error (ReverseEngineeringGui)
endif ()

set_bin_dir (ReverseEngineeringGui ReverseEngineeringGui /Mod/ReverseEngineering)
set_python_prefix_suffix (ReverseEngineeringGui)

fc_copy_sources (
    ReverseEngineeringGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/ReverseEngineering"
    ${ReverseEngineeringGuiIcon_SVG})

install (TARGETS ReverseEngineeringGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
install (FILES ${ReverseEngineeringGuiIcon_SVG}
         DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/ReverseEngineering/Resources/icons")
