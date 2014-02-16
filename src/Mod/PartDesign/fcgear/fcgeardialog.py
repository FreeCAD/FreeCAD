# (c) 2014 David Douard <david.douard@gmail.com>
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

from PyQt4 import QtGui as qt
import fcgear
import FreeCAD, FreeCADGui

class GearCreationFrame(qt.QFrame):
    def __init__(self, parent=None):
        super(GearCreationFrame, self).__init__(parent)
        self.Z = qt.QSpinBox(value=26)
        self.m = qt.QDoubleSpinBox(value=2.5)
        self.angle = qt.QDoubleSpinBox(value=20)
        self.split = qt.QComboBox()
        self.split.addItems(['2x3', '1x4'])
        l = qt.QFormLayout(self)
        l.setFieldGrowthPolicy(l.ExpandingFieldsGrow)
        l.addRow('Number of teeth:', self.Z)
        l.addRow('Modules (mm):', self.m)
        l.addRow('Pressure angle:', self.angle)
        l.addRow('Number of curves:', self.split)

class GearDialog(qt.QDialog):
    def __init__(self, parent=None):
        super(GearDialog, self).__init__(parent)
        self.gc = GearCreationFrame()

        btns = qt.QDialogButtonBox.Ok | qt.QDialogButtonBox.Cancel
        buttonBox = qt.QDialogButtonBox(btns,
                                        accepted=self.accept,
                                        rejected=self.reject)
        l = qt.QVBoxLayout(self)
        l.addWidget(self.gc)
        l.addWidget(buttonBox)
        self.setWindowTitle('Gear cration dialog')

    def accept(self):
        if FreeCAD.ActiveDocument is None:
            FreeCAD.newDocument("Gear")

        gear = fcgear.makeGear(self.gc.m.value(),
                               self.gc.Z.value(),
                               self.gc.angle.value(),
                               not self.gc.split.currentIndex())
        FreeCADGui.SendMsgToActiveView("ViewFit")
        return super(GearDialog, self).accept()


if __name__ == '__main__':
    a = qt.QApplication([])
    w = GearDialog()
    w.show()
    a.exec_()
