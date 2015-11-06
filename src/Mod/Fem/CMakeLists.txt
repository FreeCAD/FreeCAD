
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
        ccxDatReader.py
        ccxFrdReader.py
        ccxInpWriter.py
        FemTools.py
        TestFem.py
        FemExample.py
        MechanicalAnalysis.py
        MechanicalAnalysis.ui
        ShowDisplacement.ui
        FemCommands.py
        _TaskPanelResultControl.py
        _TaskPanelJobControl.py
        _ViewProviderFemAnalysis.py
        _FemAnalysis.py
        _CommandMechanicalShowResult.py
        _CommandQuickAnalysis.py
        _CommandPurgeFemResults.py
        _CommandMechanicalJobControl.py
        _CommandFemFromShape.py
        _CommandNewMechanicalAnalysis.py

        _FemBeamSection.py
        _ViewProviderFemBeamSection.py
        FemBeamSection.py
        _CommandFemBeamSection.py
        _TaskPanelFemBeamSection.py
        FemBeamSection.ui

        _FemShellThickness.py
        _ViewProviderFemShellThickness.py
        FemShellThickness.py
        _CommandFemShellThickness.py
        _TaskPanelFemShellThickness.py
        FemShellThickness.ui

        _MechanicalMaterial.py
        _ViewProviderMechanicalMaterial.py
        MechanicalMaterial.py
        _CommandMechanicalMaterial.py
        _TaskPanelMechanicalMaterial.py
        MechanicalMaterial.ui

    DESTINATION
        Mod/Fem
)

INSTALL(
    FILES
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
    DESTINATION
        Mod/Fem/test_files
)
