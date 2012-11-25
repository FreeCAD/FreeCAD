import FreeCAD,FreeCADGui
FreeCAD.open(FreeCAD.getResourceDir()+"examples/RobotExample.FCStd")
FreeCADGui.activeDocument().sendMsgToViews("ViewFit")
FreeCADGui.activeDocument().sendMsgToViews("ViewAxo")
