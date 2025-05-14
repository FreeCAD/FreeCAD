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

__title__="FreeCAD CutPlane"
__author__ = "Jonathan Wiedemann"
__url__ = "https://www.freecad.org"

## @package ArchCutPlane
#  \ingroup ARCH
#  \brief The Cut plane object and tools
#
#  This module handles the Cut Plane object

import FreeCAD
import ArchCommands
import Draft
import Part

if FreeCAD.GuiUp:
    from PySide import QtCore, QtGui
    import FreeCADGui
    from draftutils.translate import translate
else:
    # \cond
    def translate(ctxt, txt):
        return txt
    # \endcond


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
