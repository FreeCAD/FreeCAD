if(MSVC)
    add_definitions(-DFCAppFem -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)


include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/3rdParty/ANN/include
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(Fem_LIBS
    Part
	Mesh
    FreeCADApp
)

if(SMESH_FOUND)
    include_directories( ${SMESH_INCLUDE_DIR} )
    list( APPEND Fem_LIBS ${SMESH_LIBRARIES} )
endif(SMESH_FOUND)

generate_from_xml(FemMeshPy)


SET(Python_SRCS
    FemMeshPy.xml
    FemMeshPyImp.cpp
    HypothesisPy.cpp
    HypothesisPy.h
)

SET(Mod_SRCS
    AppFem.cpp
    AppFemPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)

SET(Fem_SRCS
    FemMeshObject.cpp
    FemMeshObject.h
    FemMesh.cpp
    FemMesh.h
    FemMeshProperty.cpp
    FemMeshProperty.h
    ${Mod_SRCS}
    ${Python_SRCS}
)


SOURCE_GROUP("Python" FILES ${Python_SRCS})
SOURCE_GROUP("Module" FILES ${Mod_SRCS})

add_library(Fem SHARED ${Fem_SRCS})
target_link_libraries(Fem ${Fem_LIBS})
fc_copy_script("Mod/Fem" "Fem" Init.py convert2TetGen.py)

if(MSVC)
    set_target_properties(Fem PROPERTIES SUFFIX ".pyd")
    set_target_properties(Fem PROPERTIES DEBUG_OUTPUT_NAME "Fem_d")
    set_target_properties(Fem PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Fem)
    set_target_properties(Fem PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Fem PROPERTIES SUFFIX ".pyd")
    set_target_properties(Fem PROPERTIES DEBUG_OUTPUT_NAME "Fem_d")
    set_target_properties(Fem PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Fem)
    set_target_properties(Fem PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Fem PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Fem)
    set_target_properties(Fem PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS Fem DESTINATION lib)
