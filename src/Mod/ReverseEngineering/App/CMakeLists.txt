# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

if (PCL_SURFACE_FOUND AND PCL_FEATURES_FOUND)
    add_definitions(-DHAVE_PCL_SURFACE)

    if(EXISTS "${PCL_INCLUDE_DIRS}/pcl/surface/on_nurbs/fitting_surface_tdm.h")
        add_definitions(-DHAVE_PCL_OPENNURBS)
    endif()
endif ()

if (PCL_FILTERS_FOUND AND PCL_FEATURES_FOUND)
    add_definitions(-DHAVE_PCL_FILTERS)
endif ()

if (PCL_SEGMENTATION_FOUND AND PCL_FEATURES_FOUND)
    add_definitions(-DHAVE_PCL_SEGMENTATION)
endif ()

if (PCL_SAMPLE_CONSENSUS_FOUND)
    add_definitions(-DHAVE_PCL_SAMPLE_CONSENSUS)
endif ()

set(Reen_LIBS
    Part
    Mesh
    Points
    FreeCADApp
    ${PCL_COMMON_LIBRARIES}
    ${PCL_KDTREE_LIBRARIES}
    ${PCL_FEATURES_LIBRARIES}
    ${PCL_FILTERS_LIBRARIES}
    ${PCL_SEARCH_LIBRARIES}
    ${PCL_SURFACE_LIBRARIES}
    ${PCL_SEGMENTATION_LIBRARIES}
    ${PCL_SAMPLE_CONSENSUS_LIBRARIES}
    ${QT_QTCORE_LIBRARY}
)

SET(Reen_SRCS
    AppReverseEngineering.cpp
    ApproxSurface.cpp
    ApproxSurface.h
    BSplineFitting.cpp
    BSplineFitting.h
    RegionGrowing.cpp
    RegionGrowing.h
    SampleConsensus.cpp
    SampleConsensus.h
    Segmentation.cpp
    Segmentation.h
    SurfaceTriangulation.cpp
    SurfaceTriangulation.h
    PreCompiled.h
)

add_library(ReverseEngineering SHARED ${Reen_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(ReverseEngineering PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    ReverseEngineering
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_include_directories(
    ReverseEngineering
    SYSTEM
    PUBLIC
    ${PCL_INCLUDE_DIRS}
    ${FLANN_INCLUDE_DIRS}
)
target_link_libraries(ReverseEngineering ${Reen_LIBS})

if (MSVC)
    target_compile_options(ReverseEngineering PRIVATE /wd4275)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(ReverseEngineering)
endif()

SET_BIN_DIR(ReverseEngineering ReverseEngineering /Mod/ReverseEngineering)
SET_PYTHON_PREFIX_SUFFIX(ReverseEngineering)

INSTALL(TARGETS ReverseEngineering DESTINATION ${CMAKE_INSTALL_LIBDIR})
