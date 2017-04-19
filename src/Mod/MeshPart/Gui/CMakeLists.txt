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
    ${XercesC_INCLUDE_DIRS}
)
link_directories(${OCC_LIBRARY_DIR})

set(MeshPartGui_LIBS
    MeshPart
    PartGui
    MeshGui
    FreeCADGui
)

set(MeshPartGui_MOC_HDRS
    CurveOnMesh.h
    TaskCurveOnMesh.h
    Tessellation.h
)
fc_wrap_cpp(MeshPartGui_MOC_SRCS ${MeshPartGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${MeshPartGui_MOC_SRCS})

set(MeshPartGui_UIC_SRCS
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

add_library(MeshPartGui SHARED ${MeshPartGui_SRCS})
target_link_libraries(MeshPartGui ${MeshPartGui_LIBS})


fc_target_copy_resource(MeshPartGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/MeshPart
    ${CMAKE_BINARY_DIR}/Mod/MeshPart
    InitGui.py)

SET_BIN_DIR(MeshPartGui MeshPartGui /Mod/MeshPart)
SET_PYTHON_PREFIX_SUFFIX(MeshPartGui)

INSTALL(TARGETS MeshPartGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
