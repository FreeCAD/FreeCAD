# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

if (SMESH_FOUND)
    add_definitions(-DHAVE_SMESH)
    if(SMESH_VERSION_MAJOR LESS_EQUAL 9 AND SMESH_VERSION_MINOR LESS 10 )
      add_definitions(-DHAVE_MEFISTO)
    endif()
endif(SMESH_FOUND)

if(BUILD_FEM_NETGEN)
    add_definitions(-DHAVE_NETGEN)
endif(BUILD_FEM_NETGEN)

set(MeshPartGui_LIBS
    MeshPart
    PartGui
    MeshGui
    FreeCADGui
)

set(MeshPartGui_UIC_SRCS
    CrossSections.ui
    TaskCurveOnMesh.ui
    Tessellation.ui
)

set (MeshPart_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/MeshPart_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${MeshPart_TR_QRC} ${QM_SRCS})
qt_add_resources(MeshPartGui_QRC_SRCS Resources/MeshPart.qrc ${MeshPart_TR_QRC})

SET(MeshPartGui_SRCS
    ${MeshPartGui_QRC_SRCS}
    ${MeshPartGui_UIC_HDRS}
    AppMeshPartGui.cpp
    Command.cpp
    CrossSections.ui
    CrossSections.cpp
    CrossSections.h
    CurveOnMesh.cpp
    CurveOnMesh.h
    Resources/MeshPart.qrc
    PreCompiled.h
    TaskCurveOnMesh.ui
    TaskCurveOnMesh.cpp
    TaskCurveOnMesh.h
    Tessellation.ui
    Tessellation.cpp
    Tessellation.h
    Workbench.cpp
    Workbench.h
)

set(MeshPartGui_Scripts
    ../InitGui.py
)

set(FLATMESH_PY_SRCS
    MeshFlatteningCommand.py
)

add_library(MeshPartGui SHARED ${MeshPartGui_SRCS} ${MeshPartGui_Scripts} ${FLATMESH_PY_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(MeshPartGui PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    MeshPartGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_directories(MeshPartGui PUBLIC ${SMESH_LIB_PATH})
target_link_libraries(MeshPartGui ${MeshPartGui_LIBS})

if (MSVC)
    target_compile_options(MeshPartGui PRIVATE /wd4275)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(MeshPartGui)
endif()


SET_BIN_DIR(MeshPartGui MeshPartGui /Mod/MeshPart)
SET_PYTHON_PREFIX_SUFFIX(MeshPartGui)

fc_target_copy_resource_flat(MeshPartGui
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/MeshPart
    ${MeshPartGui_Scripts}
)

fc_copy_sources(MeshPartGui
   "${CMAKE_BINARY_DIR}/Mod/MeshPart"
    ${FLATMESH_PY_SRCS}
)

INSTALL(TARGETS MeshPartGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${FLATMESH_PY_SRCS} DESTINATION Mod/MeshPart)
