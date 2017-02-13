
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

INSTALL(
    FILES
        # changes on the file list here needs to be made in App/CMakeLists.txt as well
        Init.py
        InitGui.py

        convert2TetGen.py

        FemSelectionObserver.py
        FemMeshTools.py
        FemTools.py
        FemInputWriter.py
        TestFem.py

        FemCommands.py
        _CommandClearMesh.py
        _CommandMeshNetgenFromShape.py
        _CommandPrintMeshInfo.py
        _CommandPurgeResults.py
        _CommandRunSolver.py
        _CommandControlSolver.py

        _CommandShowResult.py
        _TaskPanelShowResult.py
        TaskPanelShowResult.ui

        FemAnalysis.py
        _CommandAnalysis.py

        FemMeshGmsh.py
        _FemMeshGmsh.py
        _ViewProviderFemMeshGmsh.py
        _CommandMeshGmshFromShape.py
        _TaskPanelFemMeshGmsh.py
        TaskPanelFemMeshGmsh.ui

        FemGmshTools.py

        FemMesh2Mesh.py
        _CommandFEMMesh2Mesh.py

        FemMeshGroup.py
        _FemMeshGroup.py
        _ViewProviderFemMeshGroup.py
        _CommandMeshGroup.py
        _TaskPanelFemMeshGroup.py
        TaskPanelFemMeshGroup.ui

        FemMeshRegion.py
        _FemMeshRegion.py
        _ViewProviderFemMeshRegion.py
        _CommandMeshRegion.py
        _TaskPanelFemMeshRegion.py
        TaskPanelFemMeshRegion.ui

        FemBeamSection.py
        _FemBeamSection.py
        _ViewProviderFemBeamSection.py
        _CommandBeamSection.py
        _TaskPanelFemBeamSection.py
        TaskPanelFemBeamSection.ui

        FemShellThickness.py
        _FemShellThickness.py
        _ViewProviderFemShellThickness.py
        _CommandShellThickness.py
        _TaskPanelFemShellThickness.py
        TaskPanelFemShellThickness.ui

        FemConstraintSelfWeight.py
        _FemConstraintSelfWeight.py
        _ViewProviderFemConstraintSelfWeight.py
        _CommandConstraintSelfWeight.py

        FemMaterial.py
        _FemMaterial.py
        _ViewProviderFemMaterial.py
        _CommandMaterialSolid.py
        _CommandMaterialFluid.py
        _TaskPanelFemMaterial.py
        TaskPanelFemMaterial.ui

        FemMaterialMechanicalNonlinear.py
        _FemMaterialMechanicalNonlinear.py
        _ViewProviderFemMaterialMechanicalNonlinear.py
        _CommandMaterialMechanicalNonlinear.py

        # solver CalculiX ccx
        importInpMesh.py
        ccxDatReader.py
        ccxFrdReader.py
        FemInputWriterCcx.py
        FemToolsCcx.py
        FemSolverCalculix.py
        _FemSolverCalculix.py
        _ViewProviderFemSolverCalculix.py
        _CommandSolverCalculix.py
        _TaskPanelFemSolverCalculix.py
        TaskPanelFemSolverCalculix.ui

        # solver Z88
        importZ88Mesh.py
        importZ88Results.py
        FemInputWriterZ88.py
        FemToolsZ88.py
        FemSolverZ88.py
        _FemSolverZ88.py
        _ViewProviderFemSolverZ88.py
        _CommandSolverZ88.py

        # additional imports and exports
        importVTKResults.py

    DESTINATION
        Mod/Fem
)

INSTALL(
    FILES
        # changes on the file list here needs to be made in App/CMakeLists.txt as well
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
        test_files/ccx/spine_points.csv
        test_files/ccx/spine_volumes.csv
        test_files/ccx/spine_thermomech.inp
        test_files/ccx/spine_thermomech.dat
        test_files/ccx/spine_thermomech.frd
        test_files/ccx/spine_thermomech_expected_values
        test_files/ccx/spine_thermomech.fcstd
    DESTINATION
        Mod/Fem/test_files/ccx
)
