if(MSVC)
    add_definitions(-DFCAppMeshPart -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
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
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${SMESH_INCLUDE_DIR}
    ${VTK_INCLUDE_DIRS}
)


link_directories(${OCC_LIBRARY_DIR})


if(BUILD_FEM_NETGEN)
    set(MeshPart_LIBS
        Part
        Mesh
        StdMeshers
        NETGENPlugin
        SMESH
        SMDS
    )
elseif(BUILD_SMESH)
    set(MeshPart_LIBS
        Part
        Mesh
        StdMeshers
        SMESH
        SMDS
    )
else(BUILD_FEM_NETGEN)
    set(MeshPart_LIBS
        Part
        Mesh
    )
endif(BUILD_FEM_NETGEN)


SET(MeshPart_SRCS
    AppMeshPart.cpp
    AppMeshPartPy.cpp
    CurveProjector.cpp
    CurveProjector.h
    MeshAlgos.cpp
    MeshAlgos.h
    Mesher.cpp
    Mesher.h
    PreCompiled.cpp
    PreCompiled.h
)

add_library(MeshPart SHARED ${MeshPart_SRCS})
target_link_libraries(MeshPart ${MeshPart_LIBS})


fc_target_copy_resource(MeshPart 
    ${CMAKE_SOURCE_DIR}/src/Mod/MeshPart
    ${CMAKE_BINARY_DIR}/Mod/MeshPart
    Init.py)

SET_BIN_DIR(MeshPart MeshPart /Mod/MeshPart)
SET_PYTHON_PREFIX_SUFFIX(MeshPart)

INSTALL(TARGETS MeshPart DESTINATION ${CMAKE_INSTALL_LIBDIR})
