
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

        SelectionObserverFem.py
        TestFem.py

        ccxDatReader.py
        ccxFrdReader.py
        ccxInpWriter.py
        FemTools.py

        FemCommands.py
        _CommandFemFromShape.py
        _CommandPurgeFemResults.py
        _CommandQuickAnalysis.py

        _CommandMechanicalShowResult.py
        _TaskPanelResultControl.py

        TaskPanelShowDisplacement.ui

        _FemAnalysis.py
        _ViewProviderFemAnalysis.py

        FemAnalysis.py
        _CommandNewMechanicalAnalysis.py

        _CommandSolverJobControl.py
        _TaskPanelFemSolverCalculix.py
        TaskPanelFemSolverCalculix.ui

        FemBeamSection.py
        _FemBeamSection.py
        _ViewProviderFemBeamSection.py
        _CommandFemBeamSection.py
        _TaskPanelFemBeamSection.py
        TaskPanelFemBeamSection.ui

        FemShellThickness.py
        _FemShellThickness.py
        _ViewProviderFemShellThickness.py
        _CommandFemShellThickness.py
        _TaskPanelFemShellThickness.py
        TaskPanelFemShellThickness.ui

        MechanicalMaterial.py
        _MechanicalMaterial.py
        _ViewProviderMechanicalMaterial.py
        _CommandMechanicalMaterial.py
        _TaskPanelMechanicalMaterial.py
        TaskPanelMechanicalMaterial.ui

        FemSolverCalculix.py
        _FemSolverCalculix.py
        _ViewProviderFemSolverCalculix.py
        _CommandFemSolverCalculix.py

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
