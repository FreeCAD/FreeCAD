# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

if(MSVC)
    add_definitions(-DFCAppMaterial -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

add_definitions(-DYAML_CPP_STATIC_DEFINE)

if(BUILD_MATERIAL_EXTERNAL)
    add_definitions(-DBUILD_MATERIAL_EXTERNAL)
endif(BUILD_MATERIAL_EXTERNAL)

set(Materials_LIBS
    FreeCADApp
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

generate_from_py(Array2D)
generate_from_py(Array3D)
generate_from_py(MaterialFilter)
generate_from_py(MaterialFilterOptions)
generate_from_py(MaterialLibrary)
generate_from_py(MaterialManager)
generate_from_py(Material)
generate_from_py(ModelManager)
generate_from_py(ModelProperty)
generate_from_py(MaterialProperty)
generate_from_py(Model)
generate_from_py(UUIDs)

SET(MaterialsAPI_Files
    MaterialAPI/__init__.py
    MaterialAPI/MaterialManagerExternal.py
)

SET(MaterialsAPI_Files
    MaterialAPI/__init__.py
    MaterialAPI/MaterialManagerExternal.py
)

SET(Python_SRCS
    Exceptions.cpp
    Array2D.pyi
    Array2DPyImp.cpp
    Array3D.pyi
    Array3DPyImp.cpp
    MaterialFilterOptions.pyi
    MaterialFilterOptionsPyImp.cpp
    MaterialFilter.pyi
    MaterialFilterPyImp.cpp
    MaterialLibrary.pyi
    MaterialLibraryPyImp.cpp
    MaterialManager.pyi
    MaterialManagerPyImp.cpp
    MaterialProperty.pyi
    MaterialPropertyPyImp.cpp
    Material.pyi
    MaterialPyImp.cpp
    ModelManager.pyi
    ModelManagerPyImp.cpp
    ModelProperty.pyi
    ModelPropertyPyImp.cpp
    Model.pyi
    ModelPyImp.cpp
    UUIDs.pyi
    UUIDsPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(Materials_SRCS
    ${Python_SRCS}
    AppMaterial.cpp
    FolderTree.h
    Library.cpp
    Library.h
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
    MaterialManagerLocal.cpp
    MaterialManagerLocal.h
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
    ModelManagerLocal.cpp
    ModelManagerLocal.h
    ModelUuids.cpp
    ModelUuids.h
    PreCompiled.h
    PropertyMaterial.cpp
    PropertyMaterial.h
    PyVariants.cpp
    PyVariants.h
    trim.h
)

if(BUILD_MATERIAL_EXTERNAL)
    list(APPEND Materials_SRCS
        ExternalManager.cpp
        ExternalManager.h
        MaterialManagerExternal.cpp
        MaterialManagerExternal.h
        ModelManagerExternal.cpp
        ModelManagerExternal.h
    )
endif(BUILD_MATERIAL_EXTERNAL)

add_library(Materials SHARED ${Materials_SRCS})

if(FREECAD_USE_PCH)
    target_precompile_headers(Materials PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    Materials
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    Materials
    SYSTEM
    PUBLIC
    ${YAML_CPP_INCLUDE_DIR}
    ${QtConcurrent_INCLUDE_DIRS}
)

if(BUILD_MATERIAL_EXTERNAL)
    target_include_directories(Materials PUBLIC ${CMAKE_SOURCE_DIR}/src/3rdParty/lru-cache/include)
endif(BUILD_MATERIAL_EXTERNAL)

target_link_directories(Materials PUBLIC ${YAML_CPP_LIBRARY_DIR})
target_link_libraries(Materials ${Materials_LIBS})

if (MSVC)
    target_compile_options(Materials PRIVATE /wd4251 /wd4273)
endif()

if(FREECAD_WARN_ERROR)
    target_compile_warn_error(Materials)
endif()

SET_BIN_DIR(Materials Materials /Mod/Material)
SET_PYTHON_PREFIX_SUFFIX(Materials)

INSTALL(TARGETS Materials DESTINATION ${CMAKE_INSTALL_LIBDIR})

ADD_CUSTOM_TARGET(MaterialsAPILib ALL
    SOURCES ${MaterialsAPI_Files} ${Material_QRC_SRCS}
)

fc_target_copy_resource(MaterialsAPILib
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Material
    ${MaterialsAPI_Files})

INSTALL(FILES ${MaterialsAPI_Files} DESTINATION Mod/Material/MaterialAPI)
