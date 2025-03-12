# -*- coding: utf8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""The BIM Rewire command"""


import FreeCAD
import FreeCADGui


QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Rewire:

    def GetResources(self):
        return {
            "Pixmap": "BIM_Rewire",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Rewire", "Rewire"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Rewire", "Recreates wires from selected objects"
            ),
            "Accel": "R,W",
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        import Part
        import Draft
        import DraftGeomUtils

        objs = FreeCADGui.Selection.getSelection()
        names = []
        edges = []
        for obj in objs:
            if (
                hasattr(obj, "Shape")
                and hasattr(obj.Shape, "Edges")
                and obj.Shape.Edges
            ):
                edges.extend(obj.Shape.Edges)
                names.append(obj.Name)
        wires = DraftGeomUtils.findWires(edges)
        FreeCAD.ActiveDocument.openTransaction("Rewire")
        selectlist = []
        for wire in wires:
            if DraftGeomUtils.hasCurves(wire):
                nobj = FreeCAD.ActiveDocument.addObject("Part::Feature", "Wire")
                nobj.shape = wire
                selectlist.append(nobj)
            else:
                selectlist.append(
                    Draft.makeWire([v.Point for v in wire.OrderedVertexes])
                )
        for name in names:
            FreeCAD.ActiveDocument.removeObject(name)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.Selection.clearSelection()
        for obj in selectlist:
            FreeCADGui.Selection.addSelection(obj)
        FreeCAD.ActiveDocument.recompute()


FreeCADGui.addCommand("BIM_Rewire", BIM_Rewire())
