
if(BUILD_FEM_VTK)
    add_definitions(-DFC_USE_VTK)
endif(BUILD_FEM_VTK)


# suppress warnings from smesh
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overloaded-virtual")
endif()


add_subdirectory(App)


if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)


SET(FemScripts_SRCS
    Init.py
    InitGui.py
    FemConstraint.py
    FemInputWriter.py
    FemToolsCcx.py
    FemUtils.py
    ObjectsFem.py
    TestFem.py
)

SET(FemCommand_SRCS
    femcommand/__init__.py
    femcommand/commands.py
    femcommand/manager.py
)

SET(FemInterface_SRCS
    feminterface/__init__.py
    feminterface/convert2TetGen.py
    feminterface/importCcxDatResults.py
    feminterface/importCcxFrdResults.py
    feminterface/importFenicsMesh.py
    feminterface/importInpMesh.py
    feminterface/importToolsFem.py
    feminterface/importVTKResults.py
    feminterface/importZ88Mesh.py
    feminterface/importZ88O2Results.py
    feminterface/readFenicsXDMF.py
    feminterface/readFenicsXML.py
    feminterface/writeFenicsXDMF.py
    feminterface/writeFenicsXML.py
)

SET(FemMesh_SRCS
    femmesh/__init__.py
    femmesh/femmesh2mesh.py
    femmesh/gmshtools.py
    femmesh/meshtools.py
)

SET(FemResult_SRCS
    femresult/__init__.py
    femresult/resulttools.py
)

SET(FemSolver_SRCS
    femsolver/__init__.py
    femsolver/equationbase.py
    femsolver/report.py
    femsolver/reportdialog.py
    femsolver/run.py
    femsolver/settings.py
    femsolver/signal.py
    femsolver/solverbase.py
    femsolver/task.py
)

SET(FemSolverElmer_SRCS
    femsolver/elmer/__init__.py
    femsolver/elmer/sifio.py
    femsolver/elmer/solver.py
    femsolver/elmer/tasks.py
    femsolver/elmer/writer.py
)

SET(FemSolverElmerEquations_SRCS
    femsolver/elmer/equations/__init__.py
    femsolver/elmer/equations/electrostatic.py
    femsolver/elmer/equations/elasticity.py
    femsolver/elmer/equations/equation.py
    femsolver/elmer/equations/flow.py
    femsolver/elmer/equations/fluxsolver.py
    femsolver/elmer/equations/heat.py
    femsolver/elmer/equations/linear.py
    femsolver/elmer/equations/nonlinear.py
)

SET(FemSolverCalculix_SRCS
    femsolver/calculix/__init__.py
    femsolver/calculix/solver.py
    femsolver/calculix/tasks.py
    femsolver/calculix/writer.py
)

SET(FemSolverZ88_SRCS
    femsolver/z88/__init__.py
    femsolver/z88/solver.py
    femsolver/z88/tasks.py
    femsolver/z88/writer.py
)

SET(FemTests_SRCS
    femtest/__init__.py
    femtest/testfemcommon.py
)

SET(FemTestsFiles_SRCS
    femtest/testfiles/__init__.py
)

SET(FemTestsCcx_SRCS
    femtest/testfiles/ccx/__init__.py
    femtest/testfiles/ccx/cube_mesh.py
    femtest/testfiles/ccx/cube_frequency.inp
    femtest/testfiles/ccx/cube_frequency.dat
    femtest/testfiles/ccx/cube_frequency.frd
    femtest/testfiles/ccx/cube_frequency_expected_values
    femtest/testfiles/ccx/cube_static.inp
    femtest/testfiles/ccx/cube_static.dat
    femtest/testfiles/ccx/cube_static.frd
    femtest/testfiles/ccx/cube_static_expected_values
    femtest/testfiles/ccx/cube.fcstd
    femtest/testfiles/ccx/spine_mesh.py
    femtest/testfiles/ccx/spine_thermomech.inp
    femtest/testfiles/ccx/spine_thermomech.dat
    femtest/testfiles/ccx/spine_thermomech.frd
    femtest/testfiles/ccx/spine_thermomech_expected_values
    femtest/testfiles/ccx/spine_thermomech.fcstd
    femtest/testfiles/ccx/Flow1D_mesh.py
    femtest/testfiles/ccx/Flow1D_thermomech.inp
    femtest/testfiles/ccx/Flow1D_thermomech.dat
    femtest/testfiles/ccx/Flow1D_thermomech.frd
    femtest/testfiles/ccx/Flow1D_thermomech_expected_values
    femtest/testfiles/ccx/Flow1D_thermomech_inout_nodes.txt
    femtest/testfiles/ccx/Flow1D_thermomech.fcstd
)

SET(FemTestsElmer_SRCS
    femtest/testfiles/elmer/__init__.py
    femtest/testfiles/elmer/case.sif
    femtest/testfiles/elmer/group_mesh.geo
    femtest/testfiles/elmer/ELMERSOLVER_STARTINFO
)

SET(FemGuiScripts_SRCS
    PyGui/__init__.py
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
    PyGui/_ViewProviderFemConstraintBodyHeatSource.py
    PyGui/_ViewProviderFemConstraintElectrostaticPotential.py
    PyGui/_ViewProviderFemConstraintFlowVelocity.py
    PyGui/_ViewProviderFemConstraintInitialFlowVelocity.py
    PyGui/_ViewProviderFemConstraintSelfWeight.py
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
    PyGui/FemSelectionObserver.py
    PyGui/FemSelectionWidgets.py
    PyGui/TaskPanelFemElectrostaticPotential.ui
    PyGui/TaskPanelFemElementFluid1D.ui
    PyGui/TaskPanelFemElementGeometry1D.ui
    PyGui/TaskPanelFemElementGeometry2D.ui
    PyGui/TaskPanelFemFlowVelocity.ui
    PyGui/TaskPanelFemInitialFlowVelocity.ui
    PyGui/TaskPanelFemMaterial.ui
    PyGui/TaskPanelFemMeshBoundaryLayer.ui
    PyGui/TaskPanelFemMeshGmsh.ui
    PyGui/TaskPanelFemMeshGroup.ui
    PyGui/TaskPanelFemMeshGroupXDMFExport.ui
    PyGui/TaskPanelFemMeshRegion.ui
    PyGui/TaskPanelFemResultShow.ui
    PyGui/TaskPanelFemSolverCalculix.ui
)

SET(FemObjectsScripts_SRCS
    PyObjects/__init__.py
    PyObjects/_FemConstraintBodyHeatSource.py
    PyObjects/_FemConstraintElectrostaticPotential.py
    PyObjects/_FemConstraintFlowVelocity.py
    PyObjects/_FemConstraintInitialFlowVelocity.py
    PyObjects/_FemConstraintSelfWeight.py
    PyObjects/_FemElementFluid1D.py
    PyObjects/_FemElementGeometry1D.py
    PyObjects/_FemElementGeometry2D.py
    PyObjects/_FemMaterial.py
    PyObjects/_FemMaterialMechanicalNonlinear.py
    PyObjects/_FemMeshBoundaryLayer.py
    PyObjects/_FemMeshGmsh.py
    PyObjects/_FemMeshGroup.py
    PyObjects/_FemMeshRegion.py
    PyObjects/_FemMeshResult.py
    PyObjects/_FemResultMechanical.py
    PyObjects/_FemSolverCalculix.py
)

SET(FemAllScripts
    ${FemScripts_SRCS}
    ${FemCommand_SRCS}
    ${FemInterface_SRCS}
    ${FemMesh_SRCS}
    ${FemResult_SRCS}
    ${FemSolver_SRCS}
    ${FemSolverCalculix_SRCS}
    ${FemSolverElmer_SRCS}
    ${FemSolverElmerEquations_SRCS}
    ${FemSolverZ88_SRCS}
    ${FemTests_SRCS}
    ${FemTestsFiles_SRCS}
    ${FemTestsCcx_SRCS}
    ${FemTestsElmer_SRCS}
    ${FemGuiScripts_SRCS}
    ${FemObjectsScripts_SRCS}
)


ADD_CUSTOM_TARGET(FemScriptsTarget ALL
    SOURCES ${FemAllScripts}
)

fc_copy_sources(FemScriptsTarget "${CMAKE_BINARY_DIR}/Mod/Fem" ${FemAllScripts})


# install Python packages (for make install)
INSTALL(FILES ${FemScripts_SRCS} DESTINATION Mod/Fem)
INSTALL(FILES ${FemCommand_SRCS} DESTINATION Mod/Fem/femcommand)
INSTALL(FILES ${FemInterface_SRCS} DESTINATION Mod/Fem/feminterface)
INSTALL(FILES ${FemMesh_SRCS} DESTINATION Mod/Fem/femmesh)
INSTALL(FILES ${FemResult_SRCS} DESTINATION Mod/Fem/femresult)
INSTALL(FILES ${FemSolver_SRCS} DESTINATION Mod/Fem/femsolver)
INSTALL(FILES ${FemSolverCalculix_SRCS} DESTINATION Mod/Fem/femsolver/calculix)
INSTALL(FILES ${FemSolverElmer_SRCS} DESTINATION Mod/Fem/femsolver/elmer)
INSTALL(FILES ${FemSolverElmerEquations_SRCS} DESTINATION Mod/Fem/femsolver/elmer/equations)
INSTALL(FILES ${FemSolverZ88_SRCS} DESTINATION Mod/Fem/femsolver/z88)
INSTALL(FILES ${FemTests_SRCS} DESTINATION Mod/Fem/femtest)
INSTALL(FILES ${FemTestsFiles_SRCS} DESTINATION Mod/Fem/femtest/testfiles)
INSTALL(FILES ${FemTestsCcx_SRCS} DESTINATION Mod/Fem/femtest/testfiles/ccx)
INSTALL(FILES ${FemTestsElmer_SRCS} DESTINATION Mod/Fem/femtest/testfiles/elmer)
INSTALL(FILES ${FemObjectsScripts_SRCS} DESTINATION Mod/Fem/PyObjects)
INSTALL(FILES ${FemGuiScripts_SRCS} DESTINATION Mod/Fem/PyGui)
