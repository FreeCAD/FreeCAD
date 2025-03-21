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
)

include_directories(
    SYSTEM
    ${YAML_CPP_INCLUDE_DIR}
)
link_directories(${YAML_CPP_LIBRARY_DIR})

set(Materials_LIBS
    FreeCADApp
)

include_directories(
    SYSTEM
    ${QtConcurrent_INCLUDE_DIRS}
)
list(APPEND Materials_LIBS
    ${QtConcurrent_LIBRARIES}
)

if(yaml-cpp_VERSION VERSION_LESS 0.8.0)
    list(APPEND Materials_LIBS
        ${YAML_CPP_LIBRARIES}
    )
else()
    list(APPEND Materials_LIBS
        yaml-cpp::yaml-cpp
    )
endif()

generate_from_xml(Array2DPy)
generate_from_xml(Array3DPy)
generate_from_xml(MaterialFilterPy)
generate_from_xml(MaterialManagerPy)
generate_from_xml(MaterialPy)
generate_from_xml(ModelManagerPy)
generate_from_xml(ModelPropertyPy)
generate_from_xml(ModelPy)
generate_from_xml(UUIDsPy)

SET(Python_SRCS
    Exceptions.h
    Array2DPy.xml
    Array2DPyImp.cpp
    Array3DPy.xml
    Array3DPyImp.cpp
    MaterialManagerPy.xml
    MaterialManagerPyImp.cpp
    MaterialPy.xml
    MaterialPyImp.cpp
    MaterialFilterPy.xml
    MaterialFilterPyImp.cpp
    ModelManagerPy.xml
    ModelManagerPyImp.cpp
    ModelPropertyPy.xml
    ModelPropertyPyImp.cpp
    ModelPy.xml
    ModelPyImp.cpp
    UUIDsPy.xml
    UUIDsPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(Materials_SRCS
    ${Python_SRCS}
    AppMaterial.cpp
    FolderTree.h
    MaterialConfigLoader.cpp
    MaterialConfigLoader.h
    MaterialFilter.cpp
    MaterialFilter.h
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
    ModelUuids.cpp
    ModelUuids.h
    PreCompiled.cpp
    PreCompiled.h
    PropertyMaterial.cpp
    PropertyMaterial.h
    trim.h
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${Materials_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Materials PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(Materials SHARED ${Materials_SRCS})
target_link_libraries(Materials ${Materials_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Materials)
endif()

SET_BIN_DIR(Materials Materials /Mod/Material)
SET_PYTHON_PREFIX_SUFFIX(Materials)

INSTALL(TARGETS Materials DESTINATION ${CMAKE_INSTALL_LIBDIR})
