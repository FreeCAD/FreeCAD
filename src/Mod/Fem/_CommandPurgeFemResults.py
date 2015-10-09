import FreeCAD
from FemTools import FemTools

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore


class _CommandPurgeFemResults:
    def GetResources(self):
        return {'Pixmap': 'fem-purge-results',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_PurgeResults", "Purge results"),
                'Accel': "S, S",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_PurgeResults", "Purge results from an analysis")}

    def Activated(self):
        fea = FemTools()
        fea.reset_all()

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and results_present()


#Code duplication to be removed after migration to FemTools
def results_present():
    import FemGui
    results = False
    analysis_members = FemGui.getActiveAnalysis().Member
    for o in analysis_members:
        if o.isDerivedFrom('Fem::FemResultObject'):
            results = True
    return results

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_PurgeResults', _CommandPurgeFemResults())
