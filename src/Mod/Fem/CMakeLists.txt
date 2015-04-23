
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)


INSTALL(
    FILES
        Init.py
        InitGui.py
        convert2TetGen.py
        CalculixLib.py
        ccxInpWriter.py
        FemExample.py
        MechanicalAnalysis.py
        MechanicalMaterial.py
        MechanicalMaterial.ui
        MechanicalAnalysis.ui
        ShowDisplacement.ui
    DESTINATION
        Mod/Fem
)

