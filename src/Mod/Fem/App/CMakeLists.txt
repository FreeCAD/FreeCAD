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
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
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

#this FemScripts_SRCS is not used, but can copy py into build folder? so test FreeCAD without make install
SET(FemScripts_SRCS
    convert2TetGen.py
    ccxFrdReader.py
    ccxInpWriter.py
    TestFem.py
    FemShellThickness.py
    FemShellThickness.ui
    FemBeamSection.py
    FemBeamSection.ui
    FemTools.py
    FemExample.py
    CaeAnalysis.py
    CaeSolver.py
    _SolverControlTaskPanel.ui
    CaeTools.py
    FoamCfdSolver.py
    ccxFemSolver.py
    #FoamCaseWriter.py
    CfdExample.py
    _AnalysisControlTaskPanel.py
    _AnalysisControlTaskPanel.ui
    MechanicalAnalysis.ui
    MechanicalAnalysis.py
    MechanicalMaterial.ui
    MechanicalMaterial.py
    ShowDisplacement.ui
    FemCommands.py
    _ResultControlTaskPanel.py
    _JobControlTaskPanel.py
    _AnalysisControlTaskPanel.py
    _ViewProviderFemAnalysis.py
    _FemAnalysis.py
    _CommandMechanicalShowResult.py
    _CommandQuickAnalysis.py
    _CommandPurgeFemResults.py
    _CommandMechanicalJobControl.py
    _CommandFemFromShape.py
    _CommandNewMechanicalAnalysis.py
)
#SOURCE_GROUP("Scripts" FILES ${FemScripts_SRCS})


SET(FemTests_SRCS
    test_files/mesh_points.csv
    test_files/mesh_volumes.csv
    test_files/static_analysis.inp
    test_files/frequency_analysis.inp
    test_files/cube_frequency.inp
    test_files/cube_frequency.dat
    test_files/cube_frequency.frd
    test_files/cube_frequency_expected_values
    test_files/cube_static.inp
    test_files/cube_static.dat
    test_files/cube_static.frd
    test_files/cube_static_expected_values
)

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
    FemSolverObject.cpp
    FemSolverObject.h
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
    FemConstraintPressure.cpp
    FemConstraintPressure.h
    FemConstraintGear.cpp
    FemConstraintGear.h
    FemConstraintPulley.cpp
    FemConstraintPulley.h
	)
SOURCE_GROUP("Constraints" FILES ${FemConstraints_SRCS})

SET(FemResult_SRCS
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

add_library(Fem SHARED ${Fem_SRCS})
target_link_libraries(Fem ${Fem_LIBS})


fc_target_copy_resource(Fem 
    ${CMAKE_SOURCE_DIR}/src/Mod/Fem
    ${CMAKE_BINARY_DIR}/Mod/Fem
	Init.py 
    ${FemScripts_SRCS}
    ${FemTests_SRCS}
    )

SET_BIN_DIR(Fem Fem /Mod/Fem)
SET_PYTHON_PREFIX_SUFFIX(Fem)

INSTALL(TARGETS Fem DESTINATION ${CMAKE_INSTALL_LIBDIR})
