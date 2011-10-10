import FreeCAD,FreeCADGui
FreeCAD.open(FreeCAD.getResourceDir()+"data/examples/RobotExample.FCStd")
FreeCADGui.SendMsgToActiveView("ViewFit")
FreeCADGui.activeDocument().activeView().viewAxometric()
