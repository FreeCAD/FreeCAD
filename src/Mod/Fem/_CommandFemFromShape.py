import FreeCAD

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore


class _CommandFemFromShape:
    def GetResources(self):
        return {'Pixmap': 'fem-fem-mesh-from-shape',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_CreateFromShape", "Create FEM mesh"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_CreateFromShape", "Create FEM mesh from shape")}

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FEM mesh")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.addModule("MechanicalAnalysis")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Part::Feature")):
                FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject', '" + sel[0].Name + "_Mesh')")
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Shape = App.activeDocument()." + sel[0].Name)
                FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")

        FreeCADGui.Selection.clearSelection()

    def IsActive(self):
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) == 1:
            return sel[0].isDerivedFrom("Part::Feature")
        return False

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_CreateFromShape', _CommandFemFromShape())
