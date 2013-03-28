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
    ${SMESH_INCLUDE_DIR} 
)

link_directories(${OCC_LIBRARY_DIR})

set(Fem_LIBS
    Part
	Mesh
    FreeCADApp
    ${SMESH_LIBRARIES} 
    ${SMESH_DEBUG_LIBRARIES}
)

generate_from_xml(FemMeshPy)


SET(Python_SRCS
    FemMeshPy.xml
    FemMeshPyImp.cpp
    HypothesisPy.cpp
    HypothesisPy.h
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(Mod_SRCS
    AppFem.cpp
    AppFemPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${Mod_SRCS})


SET(FemBase_SRCS
    FemMeshObject.cpp
    FemMeshObject.h
    FemMeshShapeObject.cpp
    FemMeshShapeObject.h
    FemAnalysis.cpp
    FemAnalysis.h
    FemMesh.cpp
    FemMesh.h
    FemMeshProperty.cpp
    FemMeshProperty.h
	)
SOURCE_GROUP("Base types" FILES ${FemBase_SRCS})
	
	
SET(FemSet_SRCS
    FemSetObject.cpp
    FemSetObject.h
    FemSetNodesObject.cpp
    FemSetNodesObject.h
    FemSetElementsObject.cpp
    FemSetElementsObject.h
    FemSetFacesObject.cpp
    FemSetFacesObject.h
    FemSetGeometryObject.cpp
    FemSetGeometryObject.h
	)
SOURCE_GROUP("Set objects" FILES ${FemSet_SRCS})
	
SET(FemConstraints_SRCS
    FemConstraint.cpp
    FemConstraint.h
    FemConstraintBearing.h
    FemConstraintBearing.cpp
    FemConstraintFixed.cpp
    FemConstraintFixed.h
    FemConstraintForce.cpp
    FemConstraintForce.h
    FemConstraintGear.cpp
    FemConstraintGear.h
    FemConstraintPulley.cpp
    FemConstraintPulley.h
	)
SOURCE_GROUP("Constraints" FILES ${FemConstraints_SRCS})

SET(Fem_SRCS
	${FemBase_SRCS}
	${FemSet_SRCS}
	${FemConstraints_SRCS}
    ${Mod_SRCS}
    ${Python_SRCS}
)



add_library(Fem SHARED ${Fem_SRCS})
target_link_libraries(Fem ${Fem_LIBS})


fc_target_copy_resource(Fem 
    ${CMAKE_SOURCE_DIR}/src/Mod/Fem
    ${CMAKE_BINARY_DIR}/Mod/Fem
    Init.py 
    convert2TetGen.py
    FemLib.py)

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
    set_target_properties(Fem PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS Fem DESTINATION lib)
