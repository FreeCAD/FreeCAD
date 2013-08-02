if(MSVC)
    add_definitions(-DFCAppMeshPart -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)


add_definitions(-DHAVE_SMESH)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/3rdParty/salomesmesh/inc
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${SMESH_INCLUDE_DIR}
)


link_directories(${OCC_LIBRARY_DIR})


set(MeshPart_LIBS
    Part
    Mesh
    StdMeshers
    #NETGENPlugin
    SMESH
)


SET(MeshPart_SRCS
    AppMeshPart.cpp
    AppMeshPartPy.cpp
    CurveProjector.cpp
    CurveProjector.h
    MeshAlgos.cpp
    MeshAlgos.h
    Mesher.cpp
    Mesher.h
    PreCompiled.cpp
    PreCompiled.h
)

add_library(MeshPart SHARED ${MeshPart_SRCS})
target_link_libraries(MeshPart ${MeshPart_LIBS})


fc_target_copy_resource(MeshPart 
    ${CMAKE_SOURCE_DIR}/src/Mod/MeshPart
    ${CMAKE_BINARY_DIR}/Mod/MeshPart
    Init.py)

if(MSVC)
    set_target_properties(MeshPart PROPERTIES SUFFIX ".pyd")
    set_target_properties(MeshPart PROPERTIES DEBUG_OUTPUT_NAME "MeshPart_d")
    set_target_properties(MeshPart PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/MeshPart)
    set_target_properties(MeshPart PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(MeshPart PROPERTIES SUFFIX ".pyd")
    set_target_properties(MeshPart PROPERTIES DEBUG_OUTPUT_NAME "MeshPart_d")
    set_target_properties(MeshPart PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/MeshPart)
    set_target_properties(MeshPart PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(MeshPart PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/MeshPart)
    set_target_properties(MeshPart PROPERTIES PREFIX "")
    set_target_properties(MeshPart PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS MeshPart DESTINATION lib)
