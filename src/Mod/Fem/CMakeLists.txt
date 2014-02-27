
add_subdirectory(App)
if(FREECAD_BUILD_GUI)
    add_subdirectory(Gui)
endif(FREECAD_BUILD_GUI)


INSTALL(
    FILES
        Init.py
        InitGui.py
        convert2TetGen.py
        CalculixLib.py
        FemExample.py
        FemLib.py
        MechanicalAnalysis.py
        MechanicalMaterial.py
        MechanicalMaterial.ui
        MechanicalAnalysis.ui
        ShowDisplacement.ui
    DESTINATION
        Mod/Fem
)

