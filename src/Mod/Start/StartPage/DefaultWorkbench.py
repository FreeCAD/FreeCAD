import FreeCAD, FreeCADGui
workbench = FreeCAD.ConfigGet("DefaultWorkbench")
if not workbench: workbench = "CompleteWorkbench"
FreeCADGui.activateWorkbench(workbench)
App.newDocument()
