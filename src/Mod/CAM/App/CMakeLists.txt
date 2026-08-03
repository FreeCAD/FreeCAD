# SPDX-License-Identifier: LGPL-2.1-or-later

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_definitions(-fext-numeric-literals) #fix for gcc and qt5
endif()

set(Path_LIBS
    Part
    area-native
    FreeCADApp
)

generate_from_py(Command)
generate_from_py(Path)
generate_from_py(FeaturePathCompound)
generate_from_py(Area)
generate_from_py(FeatureArea)
generate_from_py(Voronoi)
generate_from_py(VoronoiCell)
generate_from_py(VoronoiEdge)
generate_from_py(VoronoiVertex)

SET(Python_SRCS
    Command.pyi
    CommandPyImp.cpp
    Path.pyi
    PathPyImp.cpp
    FeaturePathCompound.pyi
    FeaturePathCompoundPyImp.cpp
    Area.pyi
    AreaPyImp.cpp
    FeatureArea.pyi
    FeatureAreaPyImp.cpp
    Voronoi.pyi
    VoronoiPyImp.cpp
    VoronoiCell.pyi
    VoronoiCellPyImp.cpp
    VoronoiEdge.pyi
    VoronoiEdgePyImp.cpp
    VoronoiVertex.pyi
    VoronoiVertexPyImp.cpp
)

SET(Mod_SRCS
    AppPath.cpp
    AppPathPy.cpp
    PreCompiled.h
)

SET(Path_SRCS
    Command.cpp
    Command.h
    Path.cpp
    Path.h
    PropertyPath.cpp
    PropertyPath.h
    FeaturePath.cpp
    FeaturePath.h
    FeaturePathCompound.cpp
    FeaturePathCompound.h
    FeaturePathShape.cpp
    FeaturePathShape.h
    Area.cpp
    Area.h
    AreaParams.h
    ParamsHelper.h
    FeatureArea.cpp
    FeatureArea.h
    PathSegmentWalker.h
    PathSegmentWalker.cpp
    Voronoi.cpp
    Voronoi.h
    VoronoiCell.cpp
    VoronoiCell.h
    VoronoiEdge.cpp
    VoronoiEdge.h
    VoronoiVertex.cpp
    VoronoiVertex.h
    ${Mod_SRCS}
    ${Python_SRCS}
)

SOURCE_GROUP("Python" FILES ${Python_SRCS})
SOURCE_GROUP("Module" FILES ${Mod_SRCS})

#if (WIN32) uncomment to use KDL
#    FILE( GLOB KDL_SRCS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/[^.]*.cpp )
#    FILE( GLOB KDL_HPPS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/[^.]*.hpp
#                        ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/[^.]*.inl)
#
#    FILE( GLOB UTIL_SRCS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.cpp
#                         ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.cxx)
#    FILE( GLOB UTIL_HPPS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.h
#                         ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.hpp)
#
#    SET(Path_SRCS
#        ${Path_SRCS}
#        ${KDL_SRCS}
#        ${KDL_HPPS}
#        ${UTIL_SRCS}
#        ${UTIL_HPPS}
#    )
#
#    SOURCE_GROUP("KDL" FILES ${KDL_SRCS} ${KDL_HPPS} ${UTIL_SRCS} ${UTIL_HPPS} )
#endif(WIN32)

add_library(Path SHARED ${Path_SRCS})

target_include_directories(
    Path
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(
    Path
    SYSTEM
    PUBLIC
    ${OCC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
)
target_link_libraries(Path ${Path_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Path)
endif()


if(FREECAD_USE_PCH)
    target_precompile_headers(Path PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

SET_BIN_DIR(Path PathApp /Mod/CAM)
SET_PYTHON_PREFIX_SUFFIX(Path)

INSTALL(TARGETS Path DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_library(tsp_solver SHARED tsp_solver_pybind.cpp tsp_solver.cpp)
target_include_directories(tsp_solver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${pybind11_INCLUDE_DIR})
target_link_libraries(tsp_solver PRIVATE pybind11::module Python3::Python)
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(tsp_solver)
endif()

SET_BIN_DIR(tsp_solver tsp_solver /Mod/CAM)
SET_PYTHON_PREFIX_SUFFIX(tsp_solver)

INSTALL(TARGETS tsp_solver DESTINATION ${CMAKE_INSTALL_LIBDIR})
