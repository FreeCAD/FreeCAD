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

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${PCL_INCLUDE_DIRS}
    ${FLANN_INCLUDE_DIRS}
)

link_directories(${OCC_LIBRARY_DIR})

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
    PreCompiled.cpp
    PreCompiled.h
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${Reen_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(ReverseEngineering PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(ReverseEngineering SHARED ${Reen_SRCS})
target_link_libraries(ReverseEngineering ${Reen_LIBS})

SET_BIN_DIR(ReverseEngineering ReverseEngineering /Mod/ReverseEngineering)
SET_PYTHON_PREFIX_SUFFIX(ReverseEngineering)

INSTALL(TARGETS ReverseEngineering DESTINATION ${CMAKE_INSTALL_LIBDIR})
