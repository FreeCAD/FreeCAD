if (BUILD_SMESH)
    add_definitions(-DHAVE_SMESH -DHAVE_MEFISTO)
endif(BUILD_SMESH)

if(BUILD_FEM_NETGEN)
    add_definitions(-DHAVE_NETGEN)
endif(BUILD_FEM_NETGEN)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${SMESH_INCLUDE_DIR}
    ${VTK_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)


link_directories(${OCC_LIBRARY_DIR})
link_directories(${SMESH_LIB_PATH})

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

add_library(MeshPart SHARED ${MeshPart_SRCS} ${MeshPart_Scripts})
target_link_libraries(MeshPart ${MeshPart_LIBS})


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
    if (FREECAD_USE_PYBIND11)
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
        target_link_libraries(flatmesh ${PYTHON_LIBRARIES} ${MeshPart_LIBS})

        SET_BIN_DIR(flatmesh flatmesh /Mod/MeshPart)
        install(TARGETS flatmesh DESTINATION ${CMAKE_INSTALL_LIBDIR})
    else()
        if(NOT FREECAD_LIBPACK_USE OR FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)
            # boost-python >= 1.67 on some platforms has suffix
            set(BOOST_PY_SUFFIX ${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR})

            find_package( Boost COMPONENTS python${BOOST_PY_SUFFIX} )
            if (NOT Boost_PYTHON${BOOST_PY_SUFFIX}_FOUND)
                # try just the major version
                find_package( Boost COMPONENTS python${PYTHON_VERSION_MAJOR} )
                if (NOT Boost_PYTHON${PYTHON_VERSION_MAJOR}_FOUND)
                    # unversioned
                    find_package( Boost COMPONENTS python REQUIRED)
                endif()
            endif()

            if(Boost_FOUND)
                include_directories(${Boost_INCLUDE_DIRS})
                SET(FLATMESH_SRCS
                    MeshFlattening.cpp
                    MeshFlattening.h
                    MeshFlatteningNurbs.h
                    MeshFlatteningNurbs.cpp
                    MeshFlatteningLscmRelax.h
                    MeshFlatteningLscmRelax.cpp
                    MeshFlatteningBoostPython.cpp
                )


                add_library(flatmesh SHARED ${FLATMESH_SRCS})
                SET_PYTHON_PREFIX_SUFFIX(flatmesh)

                if (BUILD_DYNAMIC_LINK_PYTHON)
                    target_link_libraries(flatmesh ${PYTHON_LIBRARIES})
                endif(BUILD_DYNAMIC_LINK_PYTHON)

                target_link_libraries(flatmesh ${MeshPart_LIBS} ${Boost_LIBRARIES})

                SET_BIN_DIR(flatmesh flatmesh /Mod/MeshPart)
                install(TARGETS flatmesh DESTINATION ${CMAKE_INSTALL_LIBDIR})
            endif()
        endif()
    endif()
endif(BUILD_FLAT_MESH)
############################################################################
