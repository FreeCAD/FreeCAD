if(MSVC)
    add_definitions(-DFCAppMaterial -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${YAML_CPP_INCLUDE_DIR}
)
#link_directories(${YAML_CPP_LIBRARIES})

set(Material_LIBS
    FreeCADApp
)

include_directories(
    ${QtConcurrent_INCLUDE_DIRS}
)
list(APPEND Material_LIBS
    ${QtConcurrent_LIBRARIES}
    ${YAML_CPP_LIBRARIES}
)

SET(Material_SRCS
    AppMaterial.cpp
    Materials.cpp
    Materials.h
    Model.cpp
    Model.h
    PreCompiled.cpp
    PreCompiled.h
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${Material_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Material PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(Material SHARED ${Material_SRCS})
target_link_libraries(Material ${Material_LIBS})

SET_BIN_DIR(Material Material /Mod/Material)
SET_PYTHON_PREFIX_SUFFIX(Material)

INSTALL(TARGETS Material DESTINATION ${CMAKE_INSTALL_LIBDIR})
