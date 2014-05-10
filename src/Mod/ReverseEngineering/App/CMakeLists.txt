if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(Reen_LIBS
    Part
    Mesh
    FreeCADApp
)

SET(Reen_SRCS
    AppReverseEngineering.cpp
    AppReverseEngineeringPy.cpp
    ApproxSurface.cpp
    ApproxSurface.h
    PreCompiled.cpp
    PreCompiled.h
)

add_library(ReverseEngineering SHARED ${Reen_SRCS})
target_link_libraries(ReverseEngineering ${Reen_LIBS})


fc_target_copy_resource(ReverseEngineering 
    ${CMAKE_SOURCE_DIR}/src/Mod/ReverseEngineering
    ${CMAKE_BINARY_DIR}/Mod/ReverseEngineering
    Init.py)

SET_BIN_DIR(ReverseEngineering ReverseEngineering /Mod/ReverseEngineering)
SET_PYTHON_PREFIX_SUFFIX(ReverseEngineering)

INSTALL(TARGETS ReverseEngineering DESTINATION ${CMAKE_INSTALL_LIBDIR})
