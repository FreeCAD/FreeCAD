if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_definitions (-fext-numeric-literals) # fix for gcc and qt5
endif ()

include_directories (
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS})
link_directories (${OCC_LIBRARY_DIR})

set (Path_LIBS Part area-native FreeCADApp)

generate_from_xml (CommandPy)
generate_from_xml (PathPy)
generate_from_xml (FeaturePathCompoundPy)
generate_from_xml (AreaPy)
generate_from_xml (FeatureAreaPy)
generate_from_xml (VoronoiPy)
generate_from_xml (VoronoiCellPy)
generate_from_xml (VoronoiEdgePy)
generate_from_xml (VoronoiVertexPy)

set (
    Python_SRCS
    CommandPy.xml
    CommandPyImp.cpp
    PathPy.xml
    PathPyImp.cpp
    FeaturePathCompoundPy.xml
    FeaturePathCompoundPyImp.cpp
    AreaPy.xml
    AreaPyImp.cpp
    FeatureAreaPy.xml
    FeatureAreaPyImp.cpp
    VoronoiPy.xml
    VoronoiPyImp.cpp
    VoronoiCellPy.xml
    VoronoiCellPyImp.cpp
    VoronoiEdgePy.xml
    VoronoiEdgePyImp.cpp
    VoronoiVertexPy.xml
    VoronoiVertexPyImp.cpp)

set (Mod_SRCS AppPath.cpp AppPathPy.cpp PreCompiled.cpp PreCompiled.h)

set (
    Path_SRCS
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
    ${Python_SRCS})

source_group ("Python" FILES ${Python_SRCS})
source_group ("Module" FILES ${Mod_SRCS})

# if (WIN32) uncomment to use KDL FILE( GLOB KDL_SRCS
# ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/[^.]*.cpp ) FILE( GLOB KDL_HPPS
# ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/[^.]*.hpp
# ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/[^.]*.inl)
#
# FILE( GLOB UTIL_SRCS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.cpp
# ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.cxx) FILE( GLOB UTIL_HPPS
# ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.h
# ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.hpp)
#
# SET(Path_SRCS ${Path_SRCS} ${KDL_SRCS} ${KDL_HPPS} ${UTIL_SRCS} ${UTIL_HPPS} )
#
# SOURCE_GROUP("KDL" FILES ${KDL_SRCS} ${KDL_HPPS} ${UTIL_SRCS} ${UTIL_HPPS} ) endif(WIN32)

add_library (Path SHARED ${Path_SRCS})
target_link_libraries (Path ${Path_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error (Path)
endif ()

# Boost >= 1.75.0
if (NOT ${Boost_VERSION} LESS 107500)
    set_target_properties (Path PROPERTIES CXX_STANDARD_REQUIRED ON)
    set_target_properties (Path PROPERTIES CXX_STANDARD 14)

    # Suppress -Wc++17-extensions when using  OCCT 7.5 or newer
    if (MINGW AND CMAKE_COMPILER_IS_CLANGXX)
        unset (_flag_found CACHE)
        check_cxx_compiler_flag ("-Wno-c++17-extensions" _flag_found)
        if (_flag_found)
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++17-extensions")
        endif ()
    endif ()
endif ()

if (FREECAD_USE_PCH)
    add_definitions (-D_PreComp_)
    get_msvc_precompiled_source ("PreCompiled.cpp" Path_CPP_SRCS ${Path_SRCS})
    add_msvc_precompiled_header (Path PreCompiled.h PreCompiled.cpp Path_CPP_SRCS)
endif (FREECAD_USE_PCH)

set_bin_dir (Path PathApp /Mod/CAM)
set_python_prefix_suffix (Path)

install (TARGETS Path DESTINATION ${CMAKE_INSTALL_LIBDIR})
