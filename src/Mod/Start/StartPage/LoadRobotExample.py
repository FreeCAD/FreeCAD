import FreeCAD,FreeCADGui
FreeCAD.open(FreeCAD.getResourceDir()+"examples/RobotExample.FCStd")
FreeCADGui.SendMsgToActiveView("ViewFit")
FreeCADGui.activeDocument().activeView().viewAxometric()
