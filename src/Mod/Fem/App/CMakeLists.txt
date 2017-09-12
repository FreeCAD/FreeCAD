if(MSVC)
    add_definitions(-DFCAppFem -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

if(BUILD_FEM_NETGEN)
    add_definitions(-DFCWithNetgen ${NETGEN_DEFINITIONS})
endif(BUILD_FEM_NETGEN)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
    ${SMESH_INCLUDE_DIR}
    ${NETGEN_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIRS}
)

link_directories(${OCC_LIBRARY_DIR})

set(Fem_LIBS
    Part
    FreeCADApp
    StdMeshers
    SMESH
    SMDS
    SMESHDS
)

if(BUILD_FEM_NETGEN)
    list(APPEND Fem_LIBS
        NETGENPlugin
    )
endif(BUILD_FEM_NETGEN)

generate_from_xml(FemMeshPy)
generate_from_xml(FemPostPipelinePy)

SET(Python_SRCS
    FemMeshPy.xml
    FemMeshPyImp.cpp
    HypothesisPy.cpp
    HypothesisPy.h
)
if(BUILD_FEM_VTK)
SET(Python_SRCS
    ${Python_SRCS}
    FemPostPipelinePy.xml
    FemPostPipelinePyImp.cpp
)
endif(BUILD_FEM_VTK)
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
    convert2TetGen.py
    importCcxDatResults.py
    importCcxFrdResults.py
    readFenicsXML.py
    writeFenicsXML.py
    readFenicsXDMF.py
    writeFenicsXDMF.py
    importFenicsMesh.py
    importInpMesh.py
    importToolsFem.py
    importVTKResults.py
    importZ88Mesh.py
    importZ88O2Results.py
    Init.py
    InitGui.py
    FemGmshTools.py
    FemInputWriter.py
    FemMesh2Mesh.py
    FemMeshTools.py
    FemResultTools.py
    FemToolsCcx.py
    ObjectsFem.py
    FemUtils.py
    FemConstraint.py
    TestFem.py
)
#SOURCE_GROUP("Scripts" FILES ${FemScripts_SRCS})

SET(FemObjectsScripts_SRCS
    PyObjects/__init__.py
    PyObjects/_FemConstraintSelfWeight.py
    PyObjects/_FemConstraintBodyHeatSource.py
    PyObjects/_FemConstraintFlowVelocity.py
    PyObjects/_FemElementFluid1D.py
    PyObjects/_FemElementGeometry1D.py
    PyObjects/_FemElementGeometry2D.py
    PyObjects/_FemMaterialMechanicalNonlinear.py
    PyObjects/_FemMeshBoundaryLayer.py
    PyObjects/_FemMeshGmsh.py
    PyObjects/_FemMeshGroup.py
    PyObjects/_FemMeshResult.py
    PyObjects/_FemMeshRegion.py
    PyObjects/_FemResultMechanical.py
    PyObjects/_FemSolverCalculix.py
    PyObjects/_FemMaterial.py
)

SET(FemSolver_SRCS
    femsolver/__init__.py
    femsolver/solverbase.py
    femsolver/report.py
    femsolver/reportdialog.py
    femsolver/settings.py
    femsolver/task.py
    femsolver/run.py
    femsolver/signal.py
)

SET(FemCalculix_SRCS
    femsolver/calculix/__init__.py
    femsolver/calculix/solver.py
    femsolver/calculix/tasks.py
    femsolver/calculix/writer.py
)

SET(FemZ88_SRCS
    femsolver/z88/__init__.py
    femsolver/z88/solver.py
    femsolver/z88/tasks.py
    femsolver/z88/writer.py
)

SET(FemGuiScripts_SRCS
    PyGui/FemCommands.py
    PyGui/FemSelectionObserver.py
    PyGui/FemSelectionWidgets.py
    PyGui/__init__.py
    PyGui/_CommandFemAnalysis.py
    PyGui/_CommandFemConstraintSelfWeight.py
    PyGui/_CommandFemConstraintBodyHeatSource.py
    PyGui/_CommandFemConstraintFlowVelocity.py
    PyGui/_CommandFemElementFluid1D.py
    PyGui/_CommandFemElementGeometry1D.py
    PyGui/_CommandFemElementGeometry2D.py
    PyGui/_CommandFemMaterialMechanicalNonlinear.py
    PyGui/_CommandFemMaterialSolid.py
    PyGui/_CommandFemMaterialFluid.py
    PyGui/_CommandFemMesh2Mesh.py
    PyGui/_CommandFemMeshGmshFromShape.py
    PyGui/_CommandFemMeshNetgenFromShape.py
    PyGui/_CommandFemMeshBoundaryLayer.py
    PyGui/_CommandFemMeshClear.py
    PyGui/_CommandFemMeshGroup.py
    PyGui/_CommandFemMeshPrintInfo.py
    PyGui/_CommandFemMeshRegion.py
    PyGui/_CommandFemResultShow.py
    PyGui/_CommandFemResultsPurge.py
    PyGui/_CommandFemSolverCalculix.py
    PyGui/_CommandFemSolverControl.py
    PyGui/_CommandFemSolverRun.py
    PyGui/_CommandFemSolverZ88.py
    PyGui/_TaskPanelFemElementFluid1D.py
    PyGui/_TaskPanelFemElementGeometry1D.py
    PyGui/_TaskPanelFemElementGeometry2D.py
    PyGui/_TaskPanelFemMaterial.py
    PyGui/_TaskPanelFemMeshBoundaryLayer.py
    PyGui/_TaskPanelFemMeshGmsh.py
    PyGui/_TaskPanelFemMeshGroup.py
    PyGui/_TaskPanelFemMeshRegion.py
    PyGui/_TaskPanelFemResultShow.py
    PyGui/_TaskPanelFemSolverCalculix.py
    PyGui/_TaskPanelFemSolverControl.py
    PyGui/_ViewProviderFemConstraintSelfWeight.py
    PyGui/_ViewProviderFemConstraintBodyHeatSource.py
    PyGui/_ViewProviderFemConstraintFlowVelocity.py
    PyGui/_ViewProviderFemElementFluid1D.py
    PyGui/_ViewProviderFemElementGeometry1D.py
    PyGui/_ViewProviderFemElementGeometry2D.py
    PyGui/_ViewProviderFemMaterial.py
    PyGui/_ViewProviderFemMaterialMechanicalNonlinear.py
    PyGui/_ViewProviderFemMeshBoundaryLayer.py
    PyGui/_ViewProviderFemMeshGmsh.py
    PyGui/_ViewProviderFemMeshGroup.py
    PyGui/_ViewProviderFemMeshRegion.py
    PyGui/_ViewProviderFemMeshResult.py
    PyGui/_ViewProviderFemResultMechanical.py
    PyGui/_ViewProviderFemSolverCalculix.py
    PyGui/TaskPanelFemElementFluid1D.ui
    PyGui/TaskPanelFemElementGeometry1D.ui
    PyGui/TaskPanelFemElementGeometry2D.ui
    PyGui/TaskPanelFemMaterial.ui
    PyGui/TaskPanelFemMeshBoundaryLayer.ui
    PyGui/TaskPanelFemMeshGmsh.ui
    PyGui/TaskPanelFemMeshGroup.ui
    PyGui/TaskPanelFemMeshGroupXDMFExport.ui
    PyGui/TaskPanelFemMeshRegion.ui
    PyGui/TaskPanelFemResultShow.ui
    PyGui/TaskPanelFemSolverCalculix.ui
    PyGui/TaskPanelFemFlowVelocity.ui
)

SET(FemTests_SRCS
    test_files/__init__.py
    test_files/ccx/__init__.py
    test_files/ccx/cube_mesh.py
    test_files/ccx/cube_frequency.inp
    test_files/ccx/cube_frequency.dat
    test_files/ccx/cube_frequency.frd
    test_files/ccx/cube_frequency_expected_values
    test_files/ccx/cube_static.inp
    test_files/ccx/cube_static.dat
    test_files/ccx/cube_static.frd
    test_files/ccx/cube_static_expected_values
    test_files/ccx/cube.fcstd
    test_files/ccx/spine_mesh.py
    test_files/ccx/spine_thermomech.inp
    test_files/ccx/spine_thermomech.dat
    test_files/ccx/spine_thermomech.frd
    test_files/ccx/spine_thermomech_expected_values
    test_files/ccx/spine_thermomech.fcstd
    test_files/ccx/Flow1D_mesh.py
    test_files/ccx/Flow1D_thermomech.inp
    test_files/ccx/Flow1D_thermomech.dat
    test_files/ccx/Flow1D_thermomech.frd
    test_files/ccx/Flow1D_thermomech_expected_values
    test_files/ccx/Flow1D_thermomech_inout_nodes.txt
    test_files/ccx/Flow1D_thermomech.fcstd
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
    FemConstraintFluidBoundary.cpp
    FemConstraintFluidBoundary.h
    FemConstraintPressure.cpp
    FemConstraintPressure.h
    FemConstraintGear.cpp
    FemConstraintGear.h
    FemConstraintPulley.cpp
    FemConstraintPulley.h
    FemConstraintDisplacement.h
    FemConstraintDisplacement.cpp
    FemConstraintTemperature.h
    FemConstraintTemperature.cpp
    FemConstraintHeatflux.h
    FemConstraintHeatflux.cpp
    FemConstraintInitialTemperature.h
    FemConstraintInitialTemperature.cpp
    FemConstraintPlaneRotation.cpp
    FemConstraintPlaneRotation.h
    FemConstraintContact.cpp
    FemConstraintContact.h
    FemConstraintTransform.cpp
    FemConstraintTransform.h
    )
SOURCE_GROUP("Constraints" FILES ${FemConstraints_SRCS})

if(BUILD_FEM_VTK)
    SET(FemPost_SRCS
        PropertyPostDataObject.h
        PropertyPostDataObject.cpp
        FemPostObject.h
        FemPostObject.cpp
        FemPostPipeline.h
        FemPostPipeline.cpp
        FemPostFilter.h
        FemPostFilter.cpp
        FemPostFunction.h
        FemPostFunction.cpp
        FemVTKTools.h
        FemVTKTools.cpp
    )
    SOURCE_GROUP("PostObjects" FILES ${FemPost_SRCS})
endif(BUILD_FEM_VTK)


SET(Fem_SRCS
    ${FemBase_SRCS}
    ${FemSet_SRCS}
    ${FemConstraints_SRCS}
    ${FemResult_SRCS}
    ${FemPost_SRCS}
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
    ${FemObjectsScripts_SRCS}
    ${FemGuiScripts_SRCS}
    ${FemTests_SRCS}
    ${FemSolver_SRCS}
    ${FemCalculix_SRCS}
    ${FemZ88_SRCS}
)

SET_BIN_DIR(Fem Fem /Mod/Fem)
SET_PYTHON_PREFIX_SUFFIX(Fem)

INSTALL(TARGETS Fem DESTINATION ${CMAKE_INSTALL_LIBDIR})
