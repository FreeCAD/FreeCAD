import FreeCAD,FreeCADGui
FreeCADGui.activateWorkbench("DrawingWorkbench")
FreeCAD.open(FreeCAD.getResourceDir()+"examples/DrawingExample.FCStd")
FreeCADGui.SendMsgToActiveView("ViewFit")
FreeCADGui.activeDocument().activeView().viewAxometric()
