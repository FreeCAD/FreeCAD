if(WIN32)
    add_definitions(-DFCAppPoints)
endif(WIN32)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${CMAKE_BINARY_DIR}/src/3rdParty/libE57Format
    ${CMAKE_SOURCE_DIR}/src/3rdParty/libE57Format/include
)

set(Points_LIBS
    FreeCADApp
)

include_directories(
    ${QtConcurrent_INCLUDE_DIRS}
)
list(APPEND Points_LIBS
    ${QtConcurrent_LIBRARIES}
)

generate_from_xml(PointsPy)

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

add_library(Points SHARED ${Points_SRCS} ${Points_Scripts})

link_directories(${CMAKE_BINARY_DIR}/src/3rdParty/libE57Format)

target_link_libraries(Points E57Format ${Points_LIBS})


fc_target_copy_resource_flat(Points
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Points
    ${Points_Scripts})

SET_BIN_DIR(Points Points /Mod/Points)
SET_PYTHON_PREFIX_SUFFIX(Points)

INSTALL(TARGETS Points DESTINATION ${CMAKE_INSTALL_LIBDIR})
