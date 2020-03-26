from PyQt4 import QtGui as qt
import fcsprocket
import FreeCAD, FreeCADGui

class SprocketCreationFrame(qt.QFrame):
    def __init__(self, parent=None):
        super(SprocketCreationFrame, self).__init__(parent)
        self.P = qt.QSpinBox(value=0.375)
        self.N = qt.QDoubleSpinBox(value=45)
        self.Dr = qt.QDoubleSpinBox(value=0.20)

        l = qt.QFormLayout(self)
        l.setFieldGrowthPolicy(l.ExpandingFieldsGrow)
        l.addRow('Number of teeth:', self.N)
        l.addRow('Chain Pitch (in):', self.P)
        l.addRow('Roller Diameter (in):', self.Dr)
        

class SprocketDialog(qt.QDialog):
    def __init__(self, parent=None):
        super(SprocketDialog, self).__init__(parent)
        self.gc = SprocketCreationFrame()

        btns = qt.QDialogButtonBox.Ok | qt.QDialogButtonBox.Cancel
        buttonBox = qt.QDialogButtonBox(btns,
                                        accepted=self.accept,
                                        rejected=self.reject)
        l = qt.QVBoxLayout(self)
        l.addWidget(self.gc)
        l.addWidget(buttonBox)
        self.setWindowTitle('Sprocket creation dialog')

    def accept(self):
        if FreeCAD.ActiveDocument is None:
            FreeCAD.newDocument("Sprocket")

        gear = fcgear.makeSprocket(self.gc.m.value(),
                               self.gc.Z.value(),
                               self.gc.angle.value(),
                               not self.gc.split.currentIndex())
        FreeCADGui.SendMsgToActiveView("ViewFit")
        return super(SprocketDialog, self).accept()


if __name__ == '__main__':
    a = qt.QApplication([])
    w = SprocketDialog()
    w.show()
    a.exec_()
