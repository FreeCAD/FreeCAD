#This file can be pasted into FemWorkbench's python console for testing, Material and Mesh need redo in GUI to work!
execfile('/usr/share/freecad/Mod/Start/StartPage/PartDesign.py')
App.setActiveDocument("Unnamed")
App.ActiveDocument=App.getDocument("Unnamed")
Gui.ActiveDocument=Gui.getDocument("Unnamed")
App.ActiveDocument.addObject("Part::Box","Box")
App.ActiveDocument.ActiveObject.Label = "Cube"
App.ActiveDocument.recompute()
Gui.SendMsgToActiveView("ViewFit")
Gui.activateWorkbench("FemWorkbench")
import FemGui
App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject', 'Box_Mesh')
App.activeDocument().ActiveObject.Shape = App.activeDocument().Box
Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)
Gui.activeDocument().resetEdit()
#change should be made here, once FemCommands and CaeAnalysis are split
#import CaeAnalysis
#FemGui.setActiveAnalysis(MechanicalAnalysis.makeCaeAnalysis('MechanicalAnalysis'))
import MechanicalAnalysis
FemGui.setActiveAnalysis(MechanicalAnalysis.makeMechanicalAnalysis('MechanicalAnalysis'))
App.activeDocument().ActiveObject.Member = App.activeDocument().ActiveObject.Member + [App.activeDocument().Box_Mesh]
import MechanicalMaterial
MechanicalMaterial.makeMechanicalMaterial('MechanicalMaterial')
App.activeDocument().MechanicalAnalysis.Member = App.activeDocument().MechanicalAnalysis.Member + [App.ActiveDocument.ActiveObject]
Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name,0)
Gui.getDocument("Unnamed").getObject("Box_Mesh").Visibility=False
Gui.getDocument("Unnamed").getObject("Box").Visibility=True
App.activeDocument().addObject("Fem::ConstraintFixed","FemConstraintFixed")
App.activeDocument().MechanicalAnalysis.Member = App.activeDocument().MechanicalAnalysis.Member + [App.activeDocument().FemConstraintFixed]
App.ActiveDocument.recompute()
Gui.activeDocument().setEdit('FemConstraintFixed')
App.ActiveDocument.FemConstraintFixed.References = [(App.ActiveDocument.Box,"Face6")]
App.ActiveDocument.recompute()
Gui.activeDocument().resetEdit()
App.activeDocument().addObject("Fem::ConstraintForce","FemConstraintForce")
App.activeDocument().FemConstraintForce.Force = 0.0
App.activeDocument().MechanicalAnalysis.Member = App.activeDocument().MechanicalAnalysis.Member + [App.activeDocument().FemConstraintForce]
App.ActiveDocument.recompute()
Gui.activeDocument().setEdit('FemConstraintForce')
App.ActiveDocument.FemConstraintForce.Force = 0.000000
App.ActiveDocument.FemConstraintForce.Direction = None
App.ActiveDocument.FemConstraintForce.Reversed = False
App.ActiveDocument.FemConstraintForce.References = [(App.ActiveDocument.Box,"Face5")]
App.ActiveDocument.recompute()
App.ActiveDocument.FemConstraintForce.Force = 5.000000
App.ActiveDocument.FemConstraintForce.Direction = None
App.ActiveDocument.FemConstraintForce.Reversed = False
App.ActiveDocument.FemConstraintForce.References = [(App.ActiveDocument.Box,"Face4")]
App.ActiveDocument.recompute()
Gui.activeDocument().resetEdit()

#
#need to select material, and refine mesh before solve, or you will got ERROR!!!
#
