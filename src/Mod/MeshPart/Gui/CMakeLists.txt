if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

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

if(BUILD_QT5)
    qt5_add_resources(MeshPartGui_QRC_SRCS Resources/MeshPart.qrc)
    qt5_wrap_ui(MeshPartGui_UIC_HDRS ${MeshPartGui_UIC_SRCS})
else()
    qt4_add_resources(MeshPartGui_QRC_SRCS Resources/MeshPart.qrc)
    qt4_wrap_ui(MeshPartGui_UIC_HDRS ${MeshPartGui_UIC_SRCS})
endif()

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

add_library(MeshPartGui SHARED ${MeshPartGui_SRCS} ${MeshPartGui_Scripts} ${FLATMESH_PY_SRCS})
target_link_libraries(MeshPartGui ${MeshPartGui_LIBS})

# external SMESH doesn't support C++17 yet
if(FREECAD_USE_EXTERNAL_SMESH)
    set_target_properties(MeshPartGui PROPERTIES CXX_STANDARD_REQUIRED ON)
    set_target_properties(MeshPartGui PROPERTIES CXX_STANDARD 14)
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
