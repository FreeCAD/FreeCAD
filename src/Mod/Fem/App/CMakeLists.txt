if(MSVC)
    add_definitions(-DFCAppFem -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

if(BUILD_FEM_NETGEN)
    add_definitions(-DFCWithNetgen ${NETGEN_DEFINITIONS})
endif(BUILD_FEM_NETGEN)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

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
    ${VTK_INCLUDE_DIR}
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
    FemTools.cpp
    FemTools.h
    PreCompiled.cpp
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${Mod_SRCS})

SET(FemScripts_SRCS
    _CommandAnalysis.py
    _CommandBeamSection.py
    _CommandControlSolver.py
    _CommandMechanicalMaterial.py
    _CommandShowResult.py
    _CommandMeshFromShape.py
    _CommandPurgeResults.py
    _CommandRunSolver.py
    _CommandShellThickness.py
    _CommandSolverCalculix.py
    _CommandSolverZ88.py
    _FemAnalysis.py
    _FemBeamSection.py
    _FemShellThickness.py
    _FemSolverCalculix.py
    _FemSolverZ88.py
    _MechanicalMaterial.py
    _TaskPanelFemBeamSection.py
    _TaskPanelFemShellThickness.py
    _TaskPanelFemSolverCalculix.py
    _TaskPanelMechanicalMaterial.py
    _TaskPanelShowResult.py
    _ViewProviderFemAnalysis.py
    _ViewProviderFemBeamSection.py
    _ViewProviderFemShellThickness.py
    _ViewProviderFemSolverCalculix.py
    _ViewProviderFemSolverZ88.py
    _ViewProviderMechanicalMaterial.py
    ccxDatReader.py
    ccxFrdReader.py
    convert2TetGen.py
    importZ88Mesh.py
    Init.py
    InitGui.py
    FemAnalysis.py
    FemCommands.py
    FemBeamSection.py
    FemInputWriter.py
    FemInputWriterCcx.py
    FemInputWriterZ88.py
    FemMeshTools.py
    FemShellThickness.py
    FemSolverCalculix.py
    FemSolverZ88.py
    FemTools.py
    FemToolsCcx.py
    FemToolsZ88.py
    MechanicalMaterial.py
    FemSelectionObserver.py
    TestFem.py
    z88DispReader.py

    TaskPanelFemBeamSection.ui
    TaskPanelFemShellThickness.ui
    TaskPanelFemSolverCalculix.ui
    TaskPanelMechanicalMaterial.ui
    TaskPanelShowResult.ui
)
#SOURCE_GROUP("Scripts" FILES ${FemScripts_SRCS})

SET(FemTests_SRCS
    test_files/ccx/mesh_points.csv
    test_files/ccx/mesh_volumes.csv
    test_files/ccx/cube_frequency.inp
    test_files/ccx/cube_frequency.dat
    test_files/ccx/cube_frequency.frd
    test_files/ccx/cube_frequency_expected_values
    test_files/ccx/cube_static.inp
    test_files/ccx/cube_static.dat
    test_files/ccx/cube_static.frd
    test_files/ccx/cube_static_expected_values
    test_files/ccx/cube.fcstd
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
    FemConstraintDisplacement.h
    FemConstraintDisplacement.cpp
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
target_link_libraries(Fem ${Fem_LIBS} ${VTK_LIBRARIES})


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
