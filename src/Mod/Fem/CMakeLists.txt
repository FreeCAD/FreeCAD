
if(BUILD_FEM_VTK)
    include(${VTK_USE_FILE})
    add_definitions(-DFC_USE_VTK)
endif(BUILD_FEM_VTK)

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
        _CommandMeshFromShape.py
        _CommandPurgeResults.py
        _CommandRunSolver.py
        _CommandControlSolver.py

        _CommandShowResult.py
        _TaskPanelShowResult.py
        TaskPanelShowResult.ui

        FemAnalysis.py
        _FemAnalysis.py
        _ViewProviderFemAnalysis.py
        _CommandAnalysis.py

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

        MechanicalMaterial.py
        _MechanicalMaterial.py
        _ViewProviderMechanicalMaterial.py
        _CommandMechanicalMaterial.py
        _TaskPanelMechanicalMaterial.py
        TaskPanelMechanicalMaterial.ui

        # solver CalculiX ccx
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
        z88DispReader.py
        FemInputWriterZ88.py
        FemToolsZ88.py
        FemSolverZ88.py
        _FemSolverZ88.py
        _ViewProviderFemSolverZ88.py
        _CommandSolverZ88.py

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
    DESTINATION
        Mod/Fem/test_files/ccx
)
