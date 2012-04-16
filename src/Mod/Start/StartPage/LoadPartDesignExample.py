import FreeCAD,FreeCADGui
FreeCAD.open(FreeCAD.getResourceDir()+"examples/PartDesignExample.FCStd")
FreeCADGui.activeDocument().sendMsgToViews("ViewFit")
FreeCADGui.activeDocument().sendMsgToViews("ViewAxo")
