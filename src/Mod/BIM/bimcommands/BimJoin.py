# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Yorik van Havre <yorik@uncreated.net>              *
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

"""
BIM join command
This command joins different objects that can be joined, currently only Walls
"""

import math
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class BIM_Join:
    """
    Base class for the different BIM Join commands
    """

    Supported = translate("BIM", "Supported objects: Walls")

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        """Executes the command"""

        # check we have 2 objects of the right type
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) != 2:
            FreeCAD.Console.PrintError(
                translate(
                    "BIM", "The BIM Join command needs exactly 2 objects selected"
                )
                + "\n"
            )
        wires = []
        for i, obj in enumerate(sel):
            wire = None
            if hasattr(obj, "EndingStart") and hasattr(obj, "EndingEnd"):
                wire = self.get_baseline(obj)
            if wire:
                wires.append(wire)
            else:
                FreeCAD.Console.PrintError(
                    translate("BIM", "This object cannot be extended")
                    + ": "
                    + obj.Label
                    + "\n"
                )
                return
        # perform the joining
        w2 = self.get_width(sel[1])
        pla1, pos1, pla2, pos2 = self.get_intersection(wires, w2)
        doc = sel[0].Document
        if not pla1 or not pla2 or not pos1 or not pos2:
            FreeCAD.Console.PrintError(
                translate("BIM", "Unable to find an intersection, aborting.") + "\n"
            )
            return
        # apply the placements
        doc.openTransaction(translate("BIM", "Join objects"))
        if pos1 == "Start":
            sel[0].EndingStart = self.relativize_placement(pla1, sel[0])
        elif pos1 == "End":
            sel[0].EndingEnd = self.relativize_placement(pla1, sel[0])
        if pos2 == "Start":
            sel[1].EndingStart = self.relativize_placement(pla2, sel[1])
        elif pos2 == "End":
            sel[1].EndingEnd = self.relativize_placement(pla2, sel[1])
        doc.commitTransaction()
        doc.recompute()

    def get_width(self, obj):
        """Finds a width for the given object"""

        if hasattr(obj, "Width"):
            if hasattr(obj.Width, "Value"):
                return obj.Width.Value
            return obj.Width
        return None

    def get_baseline(self, obj):
        """Attempts to find a base wire for the given object"""

        import Part

        base = getattr(obj, "Base", None)
        if base:
            shp = getattr(base, "Shape", None)
            if shp:
                if shp.ShapeType in ["Wire", "Edge"]:
                    return shp
        else:
            if hasattr(obj, "Proxy") and hasattr(obj.Proxy, "calc_endpoints"):
                pts = obj.Proxy.calc_endpoints(obj)
                shp = Part.makeLine(pts[0], pts[1])
                return shp
        return None

    def get_intersection(self, wires, w2):
        """
        Gets intersection planes between the given baselines.
        Returns, for each wire, one placement and one position (Start or End, or None):
        pla1, pos1, pla2, res2.
        Since max four intersections are possible, the one retained will be
        the one that is the closest to the center of the bounding box
        of all baselines
        """

        import DraftGeomUtils

        pla1, pla2 = None
        pos1, pos2 = None
        dist = None
        bb = wires[0].BoundBox
        bb.add(wires[1].BoundBox)
        e1 = wires[0].Edges[0]
        e2 = wires[0].Edges[-1]
        e3 = wires[1].Edges[0]
        e4 = wires[1].Edges[-1]
        i1 = DraftGeomUtils.findIntersection(e1, e3, infinite1=True, infinite2=True)
        i2 = DraftGeomUtils.findIntersection(e1, e4, infinite1=True, infinite2=True)
        i3 = DraftGeomUtils.findIntersection(e2, e3, infinite1=True, infinite2=True)
        i4 = DraftGeomUtils.findIntersection(e2, e4, infinite1=True, infinite2=True)
        if i1:
            if (
                i1.sub(e1.Vertexes[0].Point).Length
                < i1.sub(e1.Vertexes[-1].Point).Length
            ):
                if (
                    i1.sub(e3.Vertexes[0].Point).Length
                    < i1.sub(e3.Vertexes[-1].Point).Length
                ):
                    pos1 = "Start"
                    pos2 = "Start"
                    pla1, pos1, pla2, pos2 = self.get_placement_planes(
                        i1, e1, e3, pos1, pos2, w2
                    )
                    dist = i1.sub(bb.Center)
        if i2 and (not dist or i2.sub(bb.Center) > dist):
            if (
                i2.sub(e1.Vertexes[0].Point).Length
                < i2.sub(e1.Vertexes[-1].Point).Length
            ):
                if (
                    i2.sub(e4.Vertexes[-1].Point).Length
                    < i2.sub(e4.Vertexes[0].Point).Length
                ):
                    pos1 = "Start"
                    pos2 = "End"
                    pla1, pos1, pla2, pos2 = self.get_placement_planes(
                        i2, e1, e4, pos1, pos2, w2
                    )
                    dist = i2.sub(bb.Center)
        if i3 and (not dist or i3.sub(bb.Center) > dist):
            if (
                i3.sub(e2.Vertexes[-1].Point).Length
                < i3.sub(e2.Vertexes[0].Point).Length
            ):
                if (
                    i3.sub(e3.Vertexes[0].Point).Length
                    < i3.sub(e3.Vertexes[-1].Point).Length
                ):
                    pos1 = "End"
                    pos2 = "Start"
                    pla1, pos1, pla2, pos2 = self.get_placement_planes(
                        i3, e2, e3, pos1, pos2, w2
                    )
                    dist = i2.sub(bb.Center)
        if i4 and (not dist or i4.sub(bb.Center) > dist):
            if (
                i4.sub(e2.Vertexes[-1].Point).Length
                < i4.sub(e2.Vertexes[0].Point).Length
            ):
                if (
                    i4.sub(e4.Vertexes[-1].Point).Length
                    < i4.sub(e4.Vertexes[0].Point).Length
                ):
                    pos1 = "End"
                    pos2 = "End"
                    pla1, pos1, pla2, pos2 = self.get_placement_planes(
                        i4, e2, e4, pos1, pos2, w2
                    )
        return pla1, pos1, pla2, pos2

    def reverse(self, placement):
        """Reverses the orientation of the placement"""

        newpl = placement.inverse()
        newpl.Base = placement.Base
        return newpl

    def get_placement_planes(self, i1, e1, e2, pos1, pos2):
        """Returns nothing - to be reimplemented"""

        return None, None, None, None


class BIM_Join_Miter(BIM_Join):
    """The BIM_Join_Miter command creates a miter joint between two objects"""

    def __init__(self):
        super().__init__()
        self.JointType = "Miter"

    def GetResources(self):
        return {
            "Pixmap": "BIM_Join_Miter",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Join_Miter", "Miter joint"),
            "Accel": "J, M",
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Join_Miter", "Creates a miter joint between two supported objects."
            )
            + "\n"
            + super().Supported,
        }

    def get_placement_planes(self, i1, e1, e2, pos1, pos2, w2):
        """Returns two placements defining the bissection plane."""

        import DraftGeomUtils

        # find midpoints on each edge
        midp1 = e1.Vertexes[-1].Point.sub(e1.Vertexes[0].Point)
        midp2 = e2.Vertexes[-1].Point.sub(e2.Vertexes[0].Point)
        # build a vector between them and find the midpoint
        vm = midp2.sub(midp1)
        vm.multiply(0.5)
        midp3 = midp1.add(vm)
        # create bissector line
        axis_x = midp3.sub(i1)
        # find Y axis
        axis_y = DraftGeomUtils.vec(e1).cross(DraftGeomUtils.vec(e2))
        # find Z axis
        axis_z = axis_x.cross(axis_y)
        # create placement
        rot = FreeCAD.Rotation(axis_x, axis_y, axis_z, "ZXY")
        pla1 = FreeCAD.Placement(i1, rot)
        pla2 = self.reverse(pla1)
        return pla1, pos1, pla2, pos2


class BIM_Join_Tee(BIM_Join):
    """The BIM_Join_Tee command creates a tee joint between two objects"""

    def __init__(self):
        super().__init__()
        self.JointType = "Tee"

    def GetResources(self):
        return {
            "Pixmap": "BIM_Join_Tee",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Join_Tee", "Tee joint"),
            "Accel": "J, T",
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Join_Tee", "Creates a tee joint between two supported objects."
            )
            + "\n"
            + super().Supported,
        }

    def get_placement_planes(self, i1, e1, e2, pos1, pos2, w2):
        """Returns one placement for the first object"""

        import DraftGeomUtils

        # pla2 is set to null, as wall 2 won't change
        # put pla1 on e2
        axis_x = DraftGeomUtils.vec(e2)
        axis_y = DraftGeomUtils.vec(e1).cross(axis_x)
        axis_z = axis_x.cross(axis_y)
        rot = FreeCAD.Rotation(axis_x, axis_y, axis_z, "ZXY")
        pla1 = FreeCAD.Placement(i1, rot)
        return pla1, pos1, None, None


class BIM_Join_Butt(BIM_Join):
    """The BIM_Join_Butt command creates a butt joint between two objects"""

    def __init__(self):
        super().__init__()
        self.JointType = "Butt"

    def GetResources(self):
        return {
            "Pixmap": "BIM_Join_Butt",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Join_Butt", "Butt joint"),
            "Accel": "J, B",
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Join_Butt", "Creates a butt joint between two supported objects."
            )
            + "\n"
            + super().Supported,
        }

    def get_placement_planes(self, i1, e1, e2, pos1, pos2, w2):
        """Returns two placements creating a butt situation"""

        import DraftGeomUtils

        # e1 will extend pass e2
        # first calculate pla2 to finish at i1 point, but align with e1
        axis_x_2 = DraftGeomUtils.vec(e1)
        axis_y = axis_x_2.cross(DraftGeomUtils.vec(e2))
        axis_z_2 = axis_x_2.cross(axis_z)
        rot2 = FreeCAD.Rotation(axis_x_2, axis_y, axis_z_2, "ZXY")
        pla2 = FreeCAD.Placement(i1, rot2)
        # then calculate the orientation of pla1, aligned with e2
        axis_x_1 = DraftGeomUtils.vec(e2)
        axis_z_1 = axis_x_1.cross(axis_y)
        # then move the insertion point to account for the w2 with
        ang = axis_x_2.getAngle(axis_z_1)
        edist = w2 * math.cos(ang)
        evec = DraftGeomUtils.vec(e1)
        evec.normalize()
        evec.multiply(edist)
        i2 = i1.add(evec)
        # finally build pla1
        rot1 = FreeCAD.Rotation(axis_x_1, axis_y, axis_z_1, "ZXY")
        pla1 = FreeCAD.Placement(i2, rot1)
        return pla1, pos1, pla2, pos2


FreeCADGui.addCommand("BIM_Join_Miter", BIM_Join_Miter())
FreeCADGui.addCommand("BIM_Join_Tee", BIM_Join_Tee())
FreeCADGui.addCommand("BIM_Join_Butt", BIM_Join_Butt())
