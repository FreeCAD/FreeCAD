# Cae gui init module
# (c) 2015 Juergen Riegel
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one (?) runs when the gui is up

class FemWorkbench ( Workbench ):
    """Cae workbench object"""
    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Fem/Resources/icons/preferences-fem.svg"
        self.__class__.MenuText = "Cae"
        self.__class__.ToolTip = "CAE(FEM+CFD) workbench"
    def Initialize(self):
        # load the c++ module
        import Fem
        import FemGui
        
        import CaeAnalysis
        import FemShellThickness
        import FemBeamSection
        import MechanicalMaterial
        import FemCommands
        
    def GetClassName(self):
        return "FemGui::Workbench"  # <FemToCae> change name later if C++ code got renamed

Gui.addWorkbench(FemWorkbench()) #<FemToCae> change name later if C++ code got renamed







