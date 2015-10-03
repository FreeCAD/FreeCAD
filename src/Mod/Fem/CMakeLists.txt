
#add_subdirectory(App)
#if(BUILD_GUI)
#    add_subdirectory(Gui)
#endif(BUILD_GUI)

#update this by ls *.py/ls *.ui
INSTALL(
    FILES
        Init.py
        InitGui.py
        convert2TetGen.py
        ccxFrdReader.py
        ccxInpWriter.py
        CaeAnalysis.py
	CaeSolver.py
	FemCommands.py
	FemResultControlTaskPanel.py
        TestFem.py
        #FemExample.py
        MechanicalMaterial.py
	FemShellThickness.py
	FemBeamSection.py
        MechanicalMaterial.ui
        MechanicalAnalysis.ui
        ShowDisplacement.ui
	FemShellThickness.ui
	FemBeamSection.ui
	Preimport.py
    DESTINATION
        Mod/Cae
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
