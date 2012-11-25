import FreeCAD,FreeCADGui
FreeCADGui.activateWorkbench("DrawingWorkbench")
FreeCAD.open(FreeCAD.getResourceDir()+"examples/DrawingExample.FCStd")
FreeCADGui.activeDocument().sendMsgToViews("ViewFit")
FreeCADGui.activeDocument().sendMsgToViews("ViewAxo")
