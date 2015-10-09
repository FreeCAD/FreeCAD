import FreeCAD

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui


class _CommandMechanicalShowResult:
    "the Fem JobControl command definition"
    def GetResources(self):
        return {'Pixmap': 'fem-result',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Result", "Show result"),
                'Accel': "S, R",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Result", "Show result information of an analysis")}

    def Activated(self):
        self.result_object = get_results_object(FreeCADGui.Selection.getSelection())

        if not self.result_object:
            QtGui.QMessageBox.critical(None, "Missing prerequisite", "No result found in active Analysis")
            return

        import _ResultControlTaskPanel
        taskd = _ResultControlTaskPanel._ResultControlTaskPanel()
        FreeCADGui.Control.showDialog(taskd)

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and results_present()


#Code duplidation - to be removed after migration to FemTools
def results_present():
    import FemGui
    results = False
    analysis_members = FemGui.getActiveAnalysis().Member
    for o in analysis_members:
        if o.isDerivedFrom('Fem::FemResultObject'):
            results = True
    return results


#Code duplidation - to be removed after migration to FemTools
def get_results_object(sel):
    import FemGui
    if (len(sel) == 1):
        if sel[0].isDerivedFrom("Fem::FemResultObject"):
            return sel[0]

    for i in FemGui.getActiveAnalysis().Member:
        if(i.isDerivedFrom("Fem::FemResultObject")):
            return i
    return None

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_ShowResult', _CommandMechanicalShowResult())
