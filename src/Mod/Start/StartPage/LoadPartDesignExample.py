import FreeCAD,FreeCADGui
FreeCAD.open(FreeCAD.getResourceDir()+"examples/PartDesignExample.FCStd")
FreeCADGui.SendMsgToActiveView("ViewFit")
FreeCADGui.activeDocument().activeView().viewAxometric()
