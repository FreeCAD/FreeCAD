add_library(MeshPart SHARED)

if (SMESH_FOUND)
    add_definitions(-DHAVE_SMESH)
    if(SMESH_VERSION_MAJOR LESS_EQUAL 9 AND SMESH_VERSION_MINOR LESS 10 )
      add_definitions(-DHAVE_MEFISTO)
    endif()
endif(SMESH_FOUND)

if(BUILD_FEM_NETGEN)
    add_definitions(-DHAVE_NETGEN)
endif(BUILD_FEM_NETGEN)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_include_directories(
    MeshPart
    SYSTEM
    PUBLIC
    ${SMESH_INCLUDE_DIR}
    ${VTK_INCLUDE_DIRS}
)

target_include_directories(
    MeshPart
    SYSTEM
    PRIVATE
    ${pybind11_INCLUDE_DIR}
)

target_link_directories(MeshPart PUBLIC ${SMESH_LIB_PATH})

set(MeshPart_LIBS
    Part
    Mesh
)

if (FREECAD_USE_EXTERNAL_SMESH)
   list(APPEND MeshPart_LIBS ${EXTERNAL_SMESH_LIBS})
else()
   if (BUILD_FEM_NETGEN)
       list(APPEND MeshPart_LIBS StdMeshers NETGENPlugin SMESH SMDS)
   elseif (BUILD_SMESH)
       list(APPEND MeshPart_LIBS StdMeshers SMESH SMDS)
   endif()
endif()


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

set(MeshPart_Scripts
    ../Init.py
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${MeshPart_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(MeshPart PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

# Suppress -Wundefined-var-template
if (MINGW AND CMAKE_COMPILER_IS_CLANGXX)
    unset(_flag_found CACHE)
    check_cxx_compiler_flag("-Wno-undefined-var-template" _flag_found)
    if (_flag_found)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-var-template")
    endif()
endif()

target_sources(MeshPart PRIVATE ${MeshPart_SRCS} ${MeshPart_Scripts})
target_link_libraries(MeshPart ${MeshPart_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(MeshPart)
endif()


fc_target_copy_resource_flat(MeshPart
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/MeshPart
    ${MeshPart_Scripts}
)

SET_BIN_DIR(MeshPart MeshPart /Mod/MeshPart)
SET_PYTHON_PREFIX_SUFFIX(MeshPart)

INSTALL(TARGETS MeshPart DESTINATION ${CMAKE_INSTALL_LIBDIR})


################################ flat mesh ###############################
if (BUILD_FLAT_MESH)
    SET(FLATMESH_SRCS
        MeshFlattening.cpp
        MeshFlattening.h
        MeshFlatteningNurbs.h
        MeshFlatteningNurbs.cpp
        MeshFlatteningLscmRelax.h
        MeshFlatteningLscmRelax.cpp
        MeshFlatteningPy.cpp
    )


    add_library(flatmesh SHARED ${FLATMESH_SRCS})
    SET_PYTHON_PREFIX_SUFFIX(flatmesh)
    target_link_libraries(flatmesh ${Python3_LIBRARIES} ${MeshPart_LIBS})

    SET_BIN_DIR(flatmesh flatmesh /Mod/MeshPart)
    install(TARGETS flatmesh DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(BUILD_FLAT_MESH)
############################################################################
