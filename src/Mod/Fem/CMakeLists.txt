
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


# Python non Gui packages and modules
SET(FemScripts_SRCS
    Init.py
    InitGui.py
    ObjectsFem.py
)

SET(FemCommands_SRCS
    femcommands/__init__.py
    femcommands/commands.py
    femcommands/manager.py
)

SET(FemExamples_SRCS
    femexamples/__init__.py
    femexamples/ccx_cantilever_std.py
    femexamples/manager.py
)

SET(FemExampleMeshes_SRCS
    femexamples/meshes/__init__.py
    femexamples/meshes/mesh_canticcx_tetra10.py
)

SET(FemInOut_SRCS
    feminout/__init__.py
    feminout/convert2TetGen.py
    feminout/importCcxDatResults.py
    feminout/importCcxFrdResults.py
    feminout/importFenicsMesh.py
    feminout/importInpMesh.py
    feminout/importToolsFem.py
    feminout/importVTKResults.py
    feminout/importZ88Mesh.py
    feminout/importZ88O2Results.py
    feminout/readFenicsXDMF.py
    feminout/readFenicsXML.py
    feminout/writeFenicsXDMF.py
    feminout/writeFenicsXML.py
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
    femsolver/writerbase.py
)

SET(FemSolverCalculix_SRCS
    femsolver/calculix/__init__.py
    femsolver/calculix/solver.py
    femsolver/calculix/tasks.py
    femsolver/calculix/writer.py
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

SET(FemSolverFenics_SRCS
    femsolver/fenics/__init__.py
    femsolver/fenics/fenics_tools.py
)

SET(FemSolverZ88_SRCS
    femsolver/z88/__init__.py
    femsolver/z88/solver.py
    femsolver/z88/tasks.py
    femsolver/z88/writer.py
)

SET(FemTests_SRCS
)

SET(FemTestsFiles_SRCS
)

SET(FemTestsCcx_SRCS
)

SET(FemTestsElmer_SRCS
)

SET(FemTestsMesh_SRCS
)

SET(FemTools_SRCS
)

SET(FemObjectsScripts_SRCS
    femobjects/__init__.py
    femobjects/_FemConstraintBodyHeatSource.py
    femobjects/_FemConstraintElectrostaticPotential.py
    femobjects/_FemConstraintFlowVelocity.py
    femobjects/_FemConstraintInitialFlowVelocity.py
    femobjects/_FemConstraintSelfWeight.py
    femobjects/_FemElementFluid1D.py
    femobjects/_FemElementGeometry1D.py
    femobjects/_FemElementGeometry2D.py
    femobjects/_FemElementRotation1D.py
    femobjects/_FemMaterial.py
    femobjects/_FemMaterialMechanicalNonlinear.py
    femobjects/_FemMeshBoundaryLayer.py
    femobjects/_FemMeshGmsh.py
    femobjects/_FemMeshGroup.py
    femobjects/_FemMeshRegion.py
    femobjects/_FemMeshResult.py
    femobjects/_FemResultMechanical.py
    femobjects/_FemSolverCalculix.py
    femobjects/FemConstraint.py
)

SET(FemAllScripts
    ${FemScripts_SRCS}
    ${FemCommands_SRCS}
    ${FemExamples_SRCS}
    ${FemExampleMeshes_SRCS}
    ${FemInOut_SRCS}
    ${FemMesh_SRCS}
    ${FemResult_SRCS}
    ${FemSolver_SRCS}
    ${FemSolverCalculix_SRCS}
    ${FemSolverElmer_SRCS}
    ${FemSolverElmerEquations_SRCS}
    ${FemSolverFenics_SRCS}
    ${FemSolverZ88_SRCS}
    ${FemTests_SRCS}
    ${FemTestsFiles_SRCS}
    ${FemTestsCcx_SRCS}
    ${FemTestsElmer_SRCS}
    ${FemTestsMesh_SRCS}
    ${FemTools_SRCS}
    ${FemObjectsScripts_SRCS}
)


ADD_CUSTOM_TARGET(FemScriptsTarget ALL
    SOURCES ${FemAllScripts}
)

fc_copy_sources(FemScriptsTarget "${CMAKE_BINARY_DIR}/Mod/Fem" ${FemAllScripts})


# install Python packages (for make install)
INSTALL(FILES ${FemScripts_SRCS} DESTINATION Mod/Fem)
INSTALL(FILES ${FemCommands_SRCS} DESTINATION Mod/Fem/femcommands)
INSTALL(FILES ${FemExamples_SRCS} DESTINATION Mod/Fem/femexamples)
INSTALL(FILES ${FemExampleMeshes_SRCS} DESTINATION Mod/Fem/femexamples/meshes)
INSTALL(FILES ${FemInOut_SRCS} DESTINATION Mod/Fem/feminout)
INSTALL(FILES ${FemMesh_SRCS} DESTINATION Mod/Fem/femmesh)
INSTALL(FILES ${FemResult_SRCS} DESTINATION Mod/Fem/femresult)
INSTALL(FILES ${FemSolver_SRCS} DESTINATION Mod/Fem/femsolver)
INSTALL(FILES ${FemSolverCalculix_SRCS} DESTINATION Mod/Fem/femsolver/calculix)
INSTALL(FILES ${FemSolverElmer_SRCS} DESTINATION Mod/Fem/femsolver/elmer)
INSTALL(FILES ${FemSolverElmerEquations_SRCS} DESTINATION Mod/Fem/femsolver/elmer/equations)
INSTALL(FILES ${FemSolverFenics_SRCS} DESTINATION Mod/Fem/femsolver/fenics)
INSTALL(FILES ${FemSolverZ88_SRCS} DESTINATION Mod/Fem/femsolver/z88)
INSTALL(FILES ${FemTests_SRCS} DESTINATION Mod/Fem/femtest)
INSTALL(FILES ${FemTestsFiles_SRCS} DESTINATION Mod/Fem/femtest/testfiles)
INSTALL(FILES ${FemTestsCcx_SRCS} DESTINATION Mod/Fem/femtest/testfiles/ccx)
INSTALL(FILES ${FemTestsElmer_SRCS} DESTINATION Mod/Fem/femtest/testfiles/elmer)
INSTALL(FILES ${FemTestsMesh_SRCS} DESTINATION Mod/Fem/femtest/testfiles/mesh)
INSTALL(FILES ${FemTools_SRCS} DESTINATION Mod/Fem/femtools)
INSTALL(FILES ${FemObjectsScripts_SRCS} DESTINATION Mod/Fem/femobjects)



# Python Gui packages and modules
SET(FemGuiScripts_SRCS
    femguiobjects/__init__.py
    femguiobjects/_TaskPanelFemSolverControl.py
    femguiobjects/_ViewProviderFemConstraintBodyHeatSource.py
    femguiobjects/_ViewProviderFemConstraintElectrostaticPotential.py
    femguiobjects/_ViewProviderFemConstraintFlowVelocity.py
    femguiobjects/_ViewProviderFemConstraintInitialFlowVelocity.py
    femguiobjects/_ViewProviderFemConstraintSelfWeight.py
    femguiobjects/_ViewProviderFemElementFluid1D.py
    femguiobjects/_ViewProviderFemElementGeometry1D.py
    femguiobjects/_ViewProviderFemElementGeometry2D.py
    femguiobjects/_ViewProviderFemElementRotation1D.py
    femguiobjects/_ViewProviderFemMaterial.py
    femguiobjects/_ViewProviderFemMaterialMechanicalNonlinear.py
    femguiobjects/_ViewProviderFemMeshBoundaryLayer.py
    femguiobjects/_ViewProviderFemMeshGmsh.py
    femguiobjects/_ViewProviderFemMeshGroup.py
    femguiobjects/_ViewProviderFemMeshRegion.py
    femguiobjects/_ViewProviderFemMeshResult.py
    femguiobjects/_ViewProviderFemResultMechanical.py
    femguiobjects/_ViewProviderFemSolverCalculix.py
    femguiobjects/FemSelectionWidgets.py
    femguiobjects/ViewProviderFemConstraint.py
)


if(BUILD_GUI)
    ADD_CUSTOM_TARGET(FemGuiScriptsTarget ALL
        SOURCES ${FemGuiScripts_SRCS}
    )
    fc_copy_sources(FemGuiScriptsTarget "${CMAKE_BINARY_DIR}/Mod/Fem" ${FemGuiScripts_SRCS})

    # install Python packages (for make install)
    INSTALL(FILES ${FemGuiScripts_SRCS} DESTINATION Mod/Fem/femguiobjects)
endif(BUILD_GUI)
