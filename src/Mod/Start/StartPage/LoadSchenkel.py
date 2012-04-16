import FreeCAD,FreeCADGui,Part
Part.open(FreeCAD.getResourceDir()+"examples/Schenkel.stp")
FreeCADGui.activeDocument().sendMsgToViews("ViewFit")
FreeCADGui.activeDocument().sendMsgToViews("ViewAxo")
