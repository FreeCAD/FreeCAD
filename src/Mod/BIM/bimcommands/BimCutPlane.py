# SPDX-License-Identifier: LGPL-2.1-or-later

# *****************************************************************************
# *                                                                           *
# *   Copyright (c) 2014 Jonathan Wiedemann <wood.galaxy@gmail.com> (cutplan) *
# *   Copyright (c) 2019 Jerome Laverroux <jerome.laverroux@free.fr> (cutline)*
# *   Copyright (c) 2023 FreeCAD Project Association                          *
# *                                                                           *
# *   This file is part of FreeCAD.                                           *
# *                                                                           *
# *   FreeCAD is free software: you can redistribute it and/or modify it      *
# *   under the terms of the GNU Lesser General Public License as             *
# *   published by the Free Software Foundation, either version 2.1 of the    *
# *   License, or (at your option) any later version.                         *
# *                                                                           *
# *   FreeCAD is distributed in the hope that it will be useful, but          *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of              *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU        *
# *   Lesser General Public License for more details.                         *
# *                                                                           *
# *   You should have received a copy of the GNU Lesser General Public        *
# *   License along with FreeCAD. If not, see                                 *
# *   <https://www.gnu.org/licenses/>.                                        *
# *                                                                           *
# *****************************************************************************

"""The Arch CutPlane command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")



class Arch_CutPlane:
    "the Arch CutPlane command definition"

    def GetResources(self):
       return {"Pixmap": "Arch_CutPlane",
               "MenuText": QT_TRANSLATE_NOOP("Arch_CutPlane", "Cut With Plane"),
               "ToolTip": QT_TRANSLATE_NOOP("Arch_CutPlane", "Cut an object with a plane")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v and len(FreeCADGui.Selection.getSelection()) > 1

    def Activated(self):
        import  ArchCutPlane
        baseObj, baseShp, cutterShp = ArchCutPlane._getShapes(FreeCADGui.Selection.getSelectionEx("", 0))
        if baseObj is None:
            FreeCAD.Console.PrintError(
                translate("Arch", "Select two objects, an object to be cut and an object defining a cutting plane, in that order")+"\n")
            return
        if baseShp is None:
            FreeCAD.Console.PrintError(translate("Arch", "The first object does not have a shape")+"\n")
            return
        if cutterShp is None:
            FreeCAD.Console.PrintError(translate("Arch", "The second object does not define a plane")+"\n")
            return
        panel= CutPlaneTaskPanel()
        FreeCADGui.Control.showDialog(panel)

class CutPlaneTaskPanel:
    def __init__(self):
        import ArchCutPlane
        from PySide import QtCore, QtGui
        _, self.base, self.cutter = ArchCutPlane._getShapes(FreeCADGui.Selection.getSelectionEx("", 0))

        self.previewObj = FreeCAD.ActiveDocument.addObject("Part::Feature", "PreviewCutVolume")
        self.previewObj.ViewObject.ShapeColor = (1.00, 0.00, 0.00)
        self.previewObj.ViewObject.Transparency = 75

        self.form = QtGui.QWidget()
        self.form.setObjectName("TaskPanel")
        self.grid = QtGui.QGridLayout(self.form)
        self.grid.setObjectName("grid")
        self.title = QtGui.QLabel(self.form)
        self.grid.addWidget(self.title, 1, 0)
        self.infoText =  QtGui.QLabel(self.form)
        self.grid.addWidget(self.infoText, 2, 0)
        self.combobox = QtGui.QComboBox()
        self.combobox.setCurrentIndex(0)
        self.grid.addWidget(self.combobox, 2, 1)
        QtCore.QObject.connect(self.combobox,QtCore.SIGNAL("currentIndexChanged(int)"), self.previewCutVolume)
        self.retranslateUi(self.form)
        self.previewCutVolume(self.combobox.currentIndex())

    def isAllowedAlterSelection(self):
        return False

    def accept(self):
        FreeCAD.ActiveDocument.removeObject(self.previewObj.Name)
        side = self.combobox.currentIndex()
        FreeCAD.ActiveDocument.openTransaction(translate("Arch", "Cutting"))
        FreeCADGui.addModule("ArchCutPlane")
        FreeCADGui.doCommand("sels = FreeCADGui.Selection.getSelectionEx('', 0)")
        FreeCADGui.doCommand("ArchCutPlane.cutComponentwithPlane(sels, side=" + str(side) + ")")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        return True

    def reject(self):
        FreeCAD.ActiveDocument.removeObject(self.previewObj.Name)
        FreeCAD.Console.PrintMessage("Cancel Cut Plane\n")
        return True

    def getStandardButtons(self):
        from PySide import QtGui
        return QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel

    def previewCutVolume(self, i):
        import Arch
        cutVolume = Arch.getCutVolume(self.cutter, self.base)
        if i == 1:
            cutVolume = cutVolume[1]
        else:
            cutVolume = cutVolume[2]
        if cutVolume:
            self.previewObj.Shape = cutVolume

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(translate("Arch", "Cut Plane"))
        self.title.setText(translate("Arch", "Cut Plane Options"))
        self.infoText.setText(translate("Arch", "Which side to cut"))
        self.combobox.addItems([translate("Arch", "Behind"), translate("Arch", "Front")])


FreeCADGui.addCommand("Arch_CutPlane", Arch_CutPlane())
