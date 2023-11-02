#*****************************************************************************
#*   Copyright (c) 2014 Jonathan Wiedemann <wood.galaxy@gmail.com> (cutplan) *
#*   Copyright (c) 2019 Jerome Laverroux <jerome.laverroux@free.fr> (cutline)*
#*   Copyright (c) 2023 FreeCAD Project Association                          *
#*                                                                           *
#*   This program is free software; you can redistribute it and/or modify    *
#*   it under the terms of the GNU Lesser General Public License (LGPL)      *
#*   as published by the Free Software Foundation; either version 2 of       *
#*   the License, or (at your option) any later version.                     *
#*   for detail see the LICENCE text file.                                   *
#*                                                                           *
#*   This program is distributed in the hope that it will be useful,         *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
#*   GNU Library General Public License for more details.                    *
#*                                                                           *
#*   You should have received a copy of the GNU Library General Public       *
#*   License along with this program; if not, write to the Free Software     *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307    *
#*   USA                                                                     *
#*                                                                           *
#*****************************************************************************

import FreeCAD
import Part
import Draft
import ArchCommands
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui
    from draftutils.translate import translate
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    # \endcond

__title__="FreeCAD CutPlane"
__author__ = "Jonathan Wiedemann"
__url__ = "https://www.freecad.org"

## @package ArchCutPlane
#  \ingroup ARCH
#  \brief The Cut plane object and tools
#
#  This module handles the Cut Plane object

def getPlanWithLine(line):
    """Function to make a plane along Normal plan"""
    import WorkingPlane
    w = WorkingPlane.get_working_plane().axis
    return line.extrude(w)

def cutComponentwithPlane(baseObj, cutterShp=None, side=0):
    """cut an object with a plane defined by a face.

    Parameters
    ----------
    baseObj: Part::FeaturePython object or selection set (a list of Gui::SelectionObject objects)
        Object to be cut or a selection set: `FreeCADGui.Selection.getSelectionEx("", 0)`.
        If a selection set is provided it should contain baseObj and cutterShp, in that order.

    cutterShp: Part.Shape, optional
        Defaults to `None` in which case cutterShp should be in the baseObj selection set.
        Either a face or an edge. An edge is extruded along the Draft working plane normal.
        The shape should be in the global coordinate system.

    side: 0 or 1, optional
        Defaults to 0.
        Behind = 0, front = 1.
    """
    if isinstance(baseObj, list) \
            and len(baseObj) >= 1 \
            and baseObj[0].isDerivedFrom("Gui::SelectionObject"):
        objs = []
        needSubEle = False
        for sel in baseObj:
            for sub in sel.SubElementNames if sel.SubElementNames else [""]:
                objs.append(Part.getShape(sel.Object, sub, needSubElement=needSubEle, retType=1))
                needSubEle = True
        baseShp, _, baseObj = objs[0]
        cutterShp, _, _ = objs[1]
        baseParent = baseObj.getParentGeoFeatureGroup()
    else:
        baseShp = baseObj.Shape
        baseParent = baseObj.getParentGeoFeatureGroup()
        if baseParent is not None:
            baseShp = baseShp.transformGeometry(baseParent.getGlobalPlacement().toMatrix())

    if cutterShp.ShapeType != "Face":
        cutterShp = getPlanWithLine(cutterShp)

    cutVolume = ArchCommands.getCutVolume(cutterShp, baseShp)
    cutVolume = cutVolume[2] if side == 0 else cutVolume[1]
    if cutVolume:
        obj = FreeCAD.ActiveDocument.addObject("Part::Feature","CutVolume")
        if baseParent is not None:
            cutVolume.Placement = baseParent.getGlobalPlacement().inverse()
        obj.Shape = Part.Compound([cutVolume])
        if baseParent is not None:
            baseParent.addObject(obj)
        if "Additions" in baseObj.PropertiesList:
            ArchCommands.removeComponents(obj, baseObj) # Also changes the obj colors.
        else:
            Draft.format_object(obj, baseObj)
            cutObj = FreeCAD.ActiveDocument.addObject("Part::Cut","CutPlane")
            if baseParent is not None:
                baseParent.addObject(cutObj)
            cutObj.Base = baseObj
            cutObj.Tool = obj

class _CommandCutLine:
    "the Arch CutPlane command definition"
    def GetResources(self):
        return {"Pixmap": "Arch_CutLine",
                "MenuText": QtCore.QT_TRANSLATE_NOOP("Arch_CutLine", "Cut with line"),
                "ToolTip": QtCore.QT_TRANSLATE_NOOP("Arch_CutLine", "Cut an object with a line")}

    def IsActive(self):
        return len(FreeCADGui.Selection.getSelection()) > 1

    def Activated(self):
        sels = FreeCADGui.Selection.getSelectionEx()
        if len(sels) != 2:
            FreeCAD.Console.PrintError("You must select exactly two objects, the shape to be cut and a line\n")
            return
        if not sels[1].SubObjects:
            FreeCAD.Console.PrintError("You must select a line from the second object (cut line), not the whole object\n")
            return
        panel=_CutPlaneTaskPanel(linecut=True)
        FreeCADGui.Control.showDialog(panel)

class _CommandCutPlane:
    "the Arch CutPlane command definition"
    def GetResources(self):
       return {"Pixmap": "Arch_CutPlane",
               "MenuText": QtCore.QT_TRANSLATE_NOOP("Arch_CutPlane","Cut with plane"),
               "ToolTip": QtCore.QT_TRANSLATE_NOOP("Arch_CutPlane","Cut an object with a plane")}

    def IsActive(self):
        return len(FreeCADGui.Selection.getSelection()) > 1

    def Activated(self):
        sels = FreeCADGui.Selection.getSelectionEx()
        if len(sels) != 2:
            FreeCAD.Console.PrintError("You must select exactly two objects, the shape to be cut and the cut plane\n")
            return
        if not sels[1].SubObjects:
            FreeCAD.Console.PrintError("You must select a face from the second object (cut plane), not the whole object\n")
            return
        panel=_CutPlaneTaskPanel()
        FreeCADGui.Control.showDialog(panel)

class _CutPlaneTaskPanel:
    def __init__(self,linecut=False):
        sels = FreeCADGui.Selection.getSelectionEx("", 0)
        shapes = []
        needSubEle = False
        for sel in sels:
            for sub in sel.SubElementNames if sel.SubElementNames else [""]:
                shapes.append(Part.getShape(sel.Object, sub, needSubElement=needSubEle, retType=0))
                needSubEle = True
        self.base = shapes[0]
        self.plan = shapes[1]
        if linecut:
            self.plan = getPlanWithLine(self.plan)

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
        QtCore.QObject.connect(self.combobox,QtCore.SIGNAL("currentIndexChanged(int)"),self.previewCutVolume)
        self.retranslateUi(self.form)
        self.previewCutVolume(self.combobox.currentIndex())

    def isAllowedAlterSelection(self):
        return False

    def accept(self):
        FreeCAD.ActiveDocument.removeObject(self.previewObj.Name)
        side = self.combobox.currentIndex()
        sels = FreeCADGui.Selection.getSelectionEx()
        if len(sels) > 1 and sels[1].SubObjects:
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Cutting"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("sels = FreeCADGui.Selection.getSelectionEx('', 0)")
            FreeCADGui.doCommand("Arch.cutComponentwithPlane(sels, side=" + str(side) + ")")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            return True
        FreeCAD.Console.PrintError("Wrong selection\n")
        return True

    def reject(self):
        FreeCAD.ActiveDocument.removeObject(self.previewObj.Name)
        FreeCAD.Console.PrintMessage("Cancel Cut Plane\n")
        return True

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok|QtGui.QDialogButtonBox.Cancel)

    def previewCutVolume(self, i):
        cutVolume = ArchCommands.getCutVolume(self.plan,self.base)
        if i == 1:
            cutVolume = cutVolume[1]
        else:
            cutVolume = cutVolume[2]
        if cutVolume:
            self.previewObj.Shape = cutVolume

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(QtGui.QApplication.translate("Arch", "Cut Plane", None))
        self.title.setText(QtGui.QApplication.translate("Arch", "Cut Plane options", None))
        self.infoText.setText(QtGui.QApplication.translate("Arch", "Which side to cut", None))
        self.combobox.addItems([QtGui.QApplication.translate("Arch", "Behind", None),
                                    QtGui.QApplication.translate("Arch", "Front", None)])

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Arch_CutPlane',_CommandCutPlane())
    FreeCADGui.addCommand('Arch_CutLine', _CommandCutLine())
