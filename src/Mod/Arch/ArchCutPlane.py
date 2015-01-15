#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014                                                    *
#*   Jonathan Wiedemann <wood.galaxy@gmail.com>                            *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import FreeCAD,ArchCommands
from FreeCAD import Vector
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from DraftTools import translate
else:
    def translate(ctxt,txt):
        return txt

__title__="FreeCAD CutPlane"
__author__ = "Jonathan Wiedemann"
__url__ = "http://www.freecadweb.org"

def cutComponentwithPlane(archObject, cutPlane, sideFace):
    """cut object from a plan define by a face, Behind = 0 , front = 1"""
    cutVolume = ArchCommands.getCutVolume(cutPlane, archObject.Object.Shape)
    if sideFace == 0:
        cutVolume = cutVolume[2]
    else:
        cutVolume = cutVolume[1]
    if cutVolume:
        obj = FreeCAD.ActiveDocument.addObject("Part::Feature", str(translate("Arch","CutVolume")))
        obj.Shape = cutVolume
        obj.ViewObject.ShapeColor = (1.00,0.00,0.00)
        obj.ViewObject.Transparency = 75
        if "Additions" in archObject.Object.PropertiesList:
            return ArchCommands.removeComponents(obj,archObject.Object)
        else:
            cutObj = FreeCAD.ActiveDocument.addObject("Part::Cut", str(translate("Arch","CutPlane")))
            cutObj.Base = archObject.Object
            cutObj.Tool = obj
            return cutObj

class _CommandCutPlane:
    "the Arch CutPlane command definition"
    def GetResources(self):
       return {'Pixmap'  : 'Arch_CutPlane',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Arch_CutPlane","Cut object"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Arch_CutPlane","Cut the object with plane")}

    def IsActive(self):
        return len(FreeCADGui.Selection.getSelection()) > 1

    def Activated(self):
        panel=_CutPlaneTaskPanel()
        FreeCADGui.Control.showDialog(panel)

class _CutPlaneTaskPanel:
    def __init__(self):
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
        QtCore.QObject.connect(self.combobox,QtCore.SIGNAL("currentIndexChanged(int)"),self.previewCutVolume)
        self.previewObj = FreeCAD.ActiveDocument.addObject("Part::Feature", str(translate("Arch", "PreviewCutVolume")))
        self.retranslateUi(self.form)
        self.previewCutVolume(self.combobox.currentIndex())

    def accept(self):
        FreeCAD.ActiveDocument.removeObject(self.previewObj.Name)
        val = self.combobox.currentIndex()
        FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Cutting")))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("Arch.cutComponentwithPlane(FreeCADGui.Selection.getSelectionEx()[0],FreeCADGui.Selection.getSelectionEx()[1].SubObjects[0],"+ str(val) +")")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
        return True

    def reject(self):
        FreeCAD.ActiveDocument.removeObject(self.previewObj.Name)
        FreeCAD.Console.PrintMessage("Cancel Cut Plane\n")
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok|QtGui.QDialogButtonBox.Cancel)

    def previewCutVolume(self, i):
        cutVolume = ArchCommands.getCutVolume(FreeCADGui.Selection.getSelectionEx()[1].SubObjects[0], FreeCADGui.Selection.getSelectionEx()[0].Object.Shape)
        FreeCAD.ActiveDocument.removeObject(self.previewObj.Name)
        self.previewObj = FreeCAD.ActiveDocument.addObject("Part::Feature", "PreviewCutVolume")
        self.previewObj.ViewObject.ShapeColor = (1.00,0.00,0.00)
        self.previewObj.ViewObject.Transparency = 75
        if i == 1:
            cutVolume = cutVolume[1]
        else:
            cutVolume = cutVolume[2]
        if cutVolume:
            self.previewObj.Shape = cutVolume

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Cut Plane", None, QtGui.QApplication.UnicodeUTF8))
        self.title.setText(QtGui.QApplication.translate("Arch", "Cut Plane options", None, QtGui.QApplication.UnicodeUTF8))
        self.infoText.setText(QtGui.QApplication.translate("Arch", "Wich side to cut", None, QtGui.QApplication.UnicodeUTF8))
        self.combobox.addItems([QtGui.QApplication.translate("Arch", "Behind", None, QtGui.QApplication.UnicodeUTF8),
                                    QtGui.QApplication.translate("Arch", "Front", None, QtGui.QApplication.UnicodeUTF8)])

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_CutPlane',_CommandCutPlane())
