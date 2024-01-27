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
    def translate(ctxt, txt):
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

# _getShapes(FreeCADGui.Selection.getSelectionEx("", 0))
def _getShapes(sels):
    """Check and process the user selection.
    Returns a tuple: (baseObj, baseShp, cutterShp).
    baseShp and cutterShp are in the global coordinate system, cutterShp is a planar face.
    If the selection is not valid one or more items in the tuple will be `None`.
    """
    if not sels:
        return None, None, None
    objs = []
    needSubEle = False
    for sel in sels:
        for sub in sel.SubElementNames if sel.SubElementNames else [""]:
            objs.append(Part.getShape(sel.Object, sub, needSubElement=needSubEle, retType=1))
            needSubEle = True
    if len(objs) != 2:
        return None, None, None
    baseShp, _, baseObj = objs[0]
    cutterShp, _, _ = objs[1]
    if baseShp.isNull():
        return baseObj, None, None
    if cutterShp.isNull():
        return baseObj, baseShp, None
    if cutterShp.ShapeType == "Edge":
        if isinstance(cutterShp.Curve, Part.Line):
            cutterShp = _extrudeEdge(cutterShp)
        else:
            try:
                cutterShp = Part.Face(Part.Wire(cutterShp))
            except Part.OCCError:
                pass
    elif cutterShp.ShapeType == "Wire":
        if len(cutterShp.Edges) == 1 and isinstance(cutterShp.Edges[0].Curve, Part.Line):
            cutterShp = _extrudeEdge(cutterShp.Edges[0])
        else:
            try:
                cutterShp = Part.Face(cutterShp)
            except Part.OCCError:
                pass
    if not cutterShp.Faces and cutterShp.Vertexes:
        plane = cutterShp.findPlane()
        if plane is not None:
            # Directly creating a face from the plane results in an almost
            # endless face that ArchCommands.getCutVolume() cannot handle.
            # We therefore create a small triangular face.
            pt_main = cutterShp.Vertexes[0].Point
            mtx = plane.Rotation.toMatrix()
            pt_u = mtx.col(0) + pt_main
            pt_v = mtx.col(1) + pt_main
            cutterShp = Part.Face(Part.makePolygon([pt_main, pt_u, pt_v, pt_main]))
    # _extrudeEdge can create a face with a zero area (if the edge is parallel to the WP normal):
    if not cutterShp.Faces \
            or cutterShp.Faces[0].Area < 1e-6 \
            or cutterShp.findPlane() is None:
        return baseObj, baseShp, None
    return baseObj, baseShp, cutterShp.Faces[0]

def _extrudeEdge(edge):
    """Exrude an edge along the WP normal"""
    import WorkingPlane
    return edge.extrude(WorkingPlane.get_working_plane().axis)

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
        baseObj, baseShp, cutterShp = _getShapes(baseObj)
        baseParent = baseObj.getParentGeoFeatureGroup()
    else:
        baseShp = baseObj.Shape
        baseParent = baseObj.getParentGeoFeatureGroup()
        if baseParent is not None:
            baseShp = baseShp.transformGeometry(baseParent.getGlobalPlacement().toMatrix())

    if cutterShp.ShapeType != "Face":
        cutterShp = _extrudeEdge(cutterShp)

    cutVolume = ArchCommands.getCutVolume(cutterShp, baseShp)
    cutVolume = cutVolume[2] if side == 0 else cutVolume[1]
    if cutVolume:
        obj = FreeCAD.ActiveDocument.addObject("Part::Feature", "CutVolume")
        if baseParent is not None:
            cutVolume.Placement = baseParent.getGlobalPlacement().inverse()
        obj.Shape = Part.Compound([cutVolume])
        if baseParent is not None:
            baseParent.addObject(obj)
        if "Additions" in baseObj.PropertiesList:
            ArchCommands.removeComponents(obj, baseObj) # Also changes the obj colors.
        else:
            Draft.format_object(obj, baseObj)
            cutObj = FreeCAD.ActiveDocument.addObject("Part::Cut", "CutPlane")
            if baseParent is not None:
                baseParent.addObject(cutObj)
            cutObj.Base = baseObj
            cutObj.Tool = obj

class _CommandCutPlane:
    "the Arch CutPlane command definition"
    def GetResources(self):
       return {"Pixmap": "Arch_CutPlane",
               "MenuText": QtCore.QT_TRANSLATE_NOOP("Arch_CutPlane", "Cut with plane"),
               "ToolTip": QtCore.QT_TRANSLATE_NOOP("Arch_CutPlane", "Cut an object with a plane")}

    def IsActive(self):
        return len(FreeCADGui.Selection.getSelection()) > 1

    def Activated(self):
        baseObj, baseShp, cutterShp = _getShapes(FreeCADGui.Selection.getSelectionEx("", 0))
        if baseObj is None:
            FreeCAD.Console.PrintError(
                translate("Arch", "Select two objects, an object to be cut and an object defining a cutting plane, in that order\n")
            )
            return
        if baseShp is None:
            FreeCAD.Console.PrintError(translate("Arch", "The first object does not have a shape\n"))
            return
        if cutterShp is None:
            FreeCAD.Console.PrintError(translate("Arch", "The second object does not define a plane\n"))
            return
        panel=_CutPlaneTaskPanel()
        FreeCADGui.Control.showDialog(panel)

class _CutPlaneTaskPanel:
    def __init__(self):
        _, self.base, self.cutter = _getShapes(FreeCADGui.Selection.getSelectionEx("", 0))

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
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("sels = FreeCADGui.Selection.getSelectionEx('', 0)")
        FreeCADGui.doCommand("Arch.cutComponentwithPlane(sels, side=" + str(side) + ")")
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
        cutVolume = ArchCommands.getCutVolume(self.cutter, self.base)
        if i == 1:
            cutVolume = cutVolume[1]
        else:
            cutVolume = cutVolume[2]
        if cutVolume:
            self.previewObj.Shape = cutVolume

    def retranslateUi(self, TaskPanel):
        TaskPanel.setWindowTitle(translate("Arch", "Cut Plane"))
        self.title.setText(translate("Arch", "Cut Plane options"))
        self.infoText.setText(translate("Arch", "Which side to cut"))
        self.combobox.addItems([translate("Arch", "Behind"), translate("Arch", "Front")])

if FreeCAD.GuiUp:
    FreeCADGui.addCommand("Arch_CutPlane", _CommandCutPlane())

