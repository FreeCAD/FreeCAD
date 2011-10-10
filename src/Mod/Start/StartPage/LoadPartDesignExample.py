import FreeCAD,FreeCADGui
FreeCAD.open(FreeCAD.getResourceDir()+"data/examples/PartDesignExample.FCStd")
FreeCADGui.SendMsgToActiveView("ViewFit")
FreeCADGui.activeDocument().activeView().viewAxometric()
