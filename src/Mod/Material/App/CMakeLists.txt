if(MSVC)
    add_definitions(-DFCAppMaterial -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

add_definitions(-DYAML_CPP_STATIC_DEFINE)

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
link_directories(${YAML_CPP_LIBRARY_DIR})

set(Material_LIBS
    ${Boost_LIBRARIES}
    FreeCADApp
)

include_directories(
    ${QtConcurrent_INCLUDE_DIRS}
)
list(APPEND Material_LIBS
    ${QtConcurrent_LIBRARIES}
    ${YAML_CPP_LIBRARIES}
)

generate_from_xml(MaterialManagerPy)
generate_from_xml(MaterialPy)
generate_from_xml(ModelManagerPy)
generate_from_xml(ModelPropertyPy)
generate_from_xml(ModelPy)

SET(Python_SRCS
    Exceptions.h
    MaterialManagerPy.xml
    MaterialManagerPyImpl.cpp
    MaterialPy.xml
    MaterialPyImpl.cpp
    ModelManagerPy.xml
    ModelManagerPyImpl.cpp
    ModelPropertyPy.xml
    ModelPropertyPyImpl.cpp
    ModelPy.xml
    ModelPyImpl.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(Material_SRCS
    ${Python_SRCS}
    AppMaterial.cpp
    FolderTree.h
    MaterialConfigLoader.cpp
    MaterialConfigLoader.h
    MaterialLibrary.cpp
    MaterialLibrary.h
    MaterialLoader.cpp
    MaterialLoader.h
    MaterialManager.cpp
    MaterialManager.h
    Materials.cpp
    Materials.h
    MaterialValue.cpp
    MaterialValue.h
    Model.cpp
    Model.h
    ModelLibrary.cpp
    ModelLibrary.h
    ModelLoader.cpp
    ModelLoader.h
    ModelManager.cpp
    ModelManager.h
    ModelUuids.h
    PreCompiled.cpp
    PreCompiled.h
    trim.h
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
