import FreeCAD

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui
    from PySide import QtCore


class _CommandMechanicalJobControl:
    "the Fem JobControl command definition"
    def GetResources(self):
        return {'Pixmap': 'fem-new-analysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_JobControl", "Start calculation"),
                'Accel': "S, C",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_JobControl", "Dialog to start the calculation of the mechanical anlysis")}

    def Activated(self):
        import _JobControlTaskPanel
        taskd = _JobControlTaskPanel._JobControlTaskPanel(FemGui.getActiveAnalysis())
        #taskd.obj = vobj.Object
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is not None


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_MechanicalJobControl', _CommandMechanicalJobControl())
