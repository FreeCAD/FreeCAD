import FreeCAD
from FemTools import FemTools

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui
    from PySide import QtCore, QtGui


class _CommandFrequencyAnalysis:
    def GetResources(self):
        return {'Pixmap': 'fem-frequency-analysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Frequency_Analysis", "Run frequency analysis with CalculiX ccx"),
                'Accel': "R, F",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Frequency_Analysis", "Write .inp file and run frequency analysis with CalculiX ccx")}

    def Activated(self):
        def load_results(ret_code):
            if ret_code == 0:
                self.fea.load_results()
            else:
                print "CalculiX failed ccx finished with error {}".format(ret_code)

        self.fea = FemTools()
        self.fea.reset_all()
        self.fea.set_analysis_type('frequency')
        message = self.fea.check_prerequisites()
        if message:
            QtGui.QMessageBox.critical(None, "Missing prerequisite", message)
            return
        self.fea.finished.connect(load_results)
        QtCore.QThreadPool.globalInstance().start(self.fea)

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is not None


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_Frequency_Analysis', _CommandFrequencyAnalysis())
