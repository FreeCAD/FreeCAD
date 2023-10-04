if (BUILD_SMESH)
    add_definitions(-DHAVE_SMESH -DHAVE_MEFISTO)
endif(BUILD_SMESH)

if(BUILD_FEM_NETGEN)
    add_definitions(-DHAVE_NETGEN)
endif(BUILD_FEM_NETGEN)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
)

link_directories(${OCC_LIBRARY_DIR})
link_directories(${SMESH_LIB_PATH})

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
    PreCompiled.cpp
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

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${MeshPartGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(MeshPartGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(MeshPartGui SHARED ${MeshPartGui_SRCS} ${MeshPartGui_Scripts} ${FLATMESH_PY_SRCS})
target_link_libraries(MeshPartGui ${MeshPartGui_LIBS})


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
