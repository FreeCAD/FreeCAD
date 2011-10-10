import FreeCAD,FreeCADGui,Part
Part.open(FreeCAD.getResourceDir()+"data/examples/Schenkel.stp")
FreeCADGui.SendMsgToActiveView("ViewFit")
Gui.activeDocument().activeView().viewAxometric()
