import FreeCAD,FreeCADGui
FreeCADGui.activateWorkbench("DrawingWorkbench")
FreeCAD.open(FreeCAD.getResourceDir()+"data/examples/DrawingExample.FCStd")
FreeCADGui.SendMsgToActiveView("ViewFit")
FreeCADGui.activeDocument().activeView().viewAxometric()
