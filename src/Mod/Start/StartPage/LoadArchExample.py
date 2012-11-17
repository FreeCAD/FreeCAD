import FreeCAD,FreeCADGui
FreeCAD.open(FreeCAD.getResourceDir()+"examples/ArchDetail.FCStd")
FreeCADGui.activeDocument().sendMsgToViews("ViewFit")
