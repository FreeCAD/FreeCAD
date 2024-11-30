# (c) 2020 Adam Spontarelli <adam@vector-space.org>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License (LGPL)
#   as published by the Free Software Foundation; either version 2 of
#   the License, or (at your option) any later version.
#   for detail see the LICENCE text file.
#
#   FCGear is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Library General Public License for more details.
#
#   You should have received a copy of the GNU Library General Public
#   License along with FCGear; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307

from PySide import QtGui as qt
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

        gear_unused = fcgear.makeSprocket(self.gc.m.value(),
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
