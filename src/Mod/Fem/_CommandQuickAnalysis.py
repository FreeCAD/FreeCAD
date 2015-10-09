import FreeCAD
from FemTools import FemTools

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui
    from PySide import QtCore, QtGui


class _CommandQuickAnalysis:
    def GetResources(self):
        return {'Pixmap': 'fem-quick-analysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Quick_Analysis", "Run CalculiX ccx"),
                'Accel': "R, C",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Quick_Analysis", "Write .inp file and run CalculiX ccx")}

    def Activated(self):
        def load_results(ret_code):
            if ret_code == 0:
                self.fea.load_results()
                self.show_results_on_mesh()
            else:
                print "CalculiX failed ccx finished with error {}".format(ret_code)

        self.fea = FemTools()
        self.fea.reset_all()
        message = self.fea.check_prerequisites()
        if message:
            QtGui.QMessageBox.critical(None, "Missing prerequisite", message)
            return
        self.fea.finished.connect(load_results)
        QtCore.QThreadPool.globalInstance().start(self.fea)

    def show_results_on_mesh(self):
        #FIXME proprer mesh refreshing as per FreeCAD.FEM_dialog settings required
        # or confirmation that it's safe to call restore_result_dialog
        import _ResultControlTaskPanel
        tp = _ResultControlTaskPanel._ResultControlTaskPanel()
        tp.restore_result_dialog()

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is not None


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_Quick_Analysis', _CommandQuickAnalysis())
