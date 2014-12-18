if(MSVC)
    add_definitions(-DFCAppFem -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

if(BUILD_FEM_NETGEN)
    add_definitions(-DFCWithNetgen)
endif(BUILD_FEM_NETGEN)


include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/3rdParty/ANN/include
    ${Boost_INCLUDE_DIRS}
    ${QT_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${SMESH_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})


if(BUILD_FEM_NETGEN)
    set(Fem_LIBS
        Part
        FreeCADApp
        StdMeshers
        NETGENPlugin
        SMESH
        SMDS
        SMESHDS
    )
else(BUILD_FEM_NETGEN)
    set(Fem_LIBS
        Part
        FreeCADApp
        StdMeshers
        SMESH
        SMDS
        SMESHDS
    )
endif(BUILD_FEM_NETGEN)

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

SET(FemScripts_SRCS
    convert2TetGen.py
    FemLib.py
    CalculixLib.py
    MechanicalAnalysis.ui
    MechanicalAnalysis.py
    MechanicalMaterial.ui
    MechanicalMaterial.py
    ShowDisplacement.ui
)
#SOURCE_GROUP("Scripts" FILES ${FemScripts_SRCS})

SET(FemBase_SRCS
    FemMeshObject.cpp
    FemMeshObject.h
    FemMeshShapeObject.cpp
    FemMeshShapeObject.h
    FemMeshShapeNetgenObject.cpp
    FemMeshShapeNetgenObject.h
    FemAnalysis.cpp
    FemAnalysis.h
    FemMesh.cpp
    FemMesh.h
    FemResultObject.cpp
    FemResultObject.h
    FemConstraint.cpp
    FemConstraint.h
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

SET(FemResult_SRCS
    FemResultValue.cpp
    FemResultValue.h
    FemResultVector.cpp
    FemResultVector.h
	)
SOURCE_GROUP("ResultObjects" FILES ${FemResult_SRCS})

SET(Fem_SRCS
	${FemBase_SRCS}
	${FemSet_SRCS}
	${FemConstraints_SRCS}
	${FemResult_SRCS}
    ${Mod_SRCS}
    ${Python_SRCS}
)

FILE( GLOB Driver_Resources Driver/*.py )


add_library(Fem SHARED ${Fem_SRCS})
target_link_libraries(Fem ${Fem_LIBS})


fc_target_copy_resource(Fem 
    ${CMAKE_SOURCE_DIR}/src/Mod/Fem
    ${CMAKE_BINARY_DIR}/Mod/Fem
	${Driver_Resources} 
	Init.py 
    ${FemScripts_SRCS}
    )

SET_BIN_DIR(Fem Fem /Mod/Fem)
SET_PYTHON_PREFIX_SUFFIX(Fem)

INSTALL(TARGETS Fem DESTINATION ${CMAKE_INSTALL_LIBDIR})
