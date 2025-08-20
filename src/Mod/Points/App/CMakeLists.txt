add_library(Points SHARED)

if(WIN32)
    add_definitions(-DFCAppPoints)
endif(WIN32)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    Points
    SYSTEM
    PUBLIC
    ${EIGEN3_INCLUDE_DIR}
)

if (NOT FREECAD_USE_EXTERNAL_E57FORMAT)
    target_include_directories(
        Points
        SYSTEM
        PUBLIC
        ${CMAKE_BINARY_DIR}/src/3rdParty/libE57Format
        ${CMAKE_SOURCE_DIR}/src/3rdParty/libE57Format/include
    )
endif()

set(Points_LIBS
    FreeCADApp
)

include_directories(
    SYSTEM
    ${QtConcurrent_INCLUDE_DIRS}
)
list(APPEND Points_LIBS
    ${QtConcurrent_LIBRARIES}
)

generate_from_xml(PointsPy)
generate_from_py_(PointsPy)

SET(Points_SRCS
    AppPoints.cpp
    AppPointsPy.cpp
    Points.cpp
    Points.h
    PointsPy.xml
    PointsPyImp.cpp
    PointsAlgos.cpp
    PointsAlgos.h
    PointsFeature.cpp
    PointsFeature.h
    PointsGrid.cpp
    PointsGrid.h
    PreCompiled.cpp
    PreCompiled.h
    Properties.cpp
    Properties.h
    PropertyPointKernel.cpp
    PropertyPointKernel.h
    Structured.cpp
    Structured.h
    Tools.h
)

set(Points_Scripts
    ../Init.py
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${Points_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Points PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

target_sources(Points PRIVATE ${Points_SRCS} ${Points_Scripts})

if (NOT FREECAD_USE_EXTERNAL_E57FORMAT)
    link_directories(${CMAKE_BINARY_DIR}/src/3rdParty/libE57Format)
endif()

target_link_libraries(Points E57Format ${Points_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Points)
endif()


fc_target_copy_resource_flat(Points
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Points
    ${Points_Scripts})

SET_BIN_DIR(Points Points /Mod/Points)
SET_PYTHON_PREFIX_SUFFIX(Points)

INSTALL(TARGETS Points DESTINATION ${CMAKE_INSTALL_LIBDIR})
