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
generate_from_py(Array2D)
generate_from_xml(Array3DPy)
generate_from_py(Array3D)
generate_from_xml(MaterialFilterPy)
generate_from_py(MaterialFilter)
generate_from_xml(MaterialManagerPy)
generate_from_py(MaterialManager)
generate_from_xml(MaterialPy)
generate_from_py(Material)
generate_from_xml(ModelManagerPy)
generate_from_py(ModelManager)
generate_from_xml(ModelPropertyPy)
generate_from_py(ModelProperty)
generate_from_xml(ModelPy)
generate_from_py(Model)
generate_from_xml(UUIDsPy)
generate_from_py(UUIDs)

SET(Materials_XML_SRCS
    Array2DPy.xml
    Array3DPy.xml
    MaterialManagerPy.xml
    MaterialPy.xml
    MaterialFilterPy.xml
    ModelManagerPy.xml
    ModelPropertyPy.xml
    ModelPy.xml
    UUIDsPy.xml
)
SOURCE_GROUP("Python" FILES ${Materials_XML_SRCS})

SET(Materials_Python_SRCS
    Exceptions.h
    Array2DPyImp.cpp
    Array3DPyImp.cpp
    MaterialManagerPyImp.cpp
    MaterialPyImp.cpp
    MaterialFilterPyImp.cpp
    ModelManagerPyImp.cpp
    ModelPropertyPyImp.cpp
    ModelPyImp.cpp
    UUIDsPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Materials_Python_SRCS})

SET(Materials_SRCS
    ${Materials_XML_SRCS}
    ${Materials_Python_SRCS}
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
