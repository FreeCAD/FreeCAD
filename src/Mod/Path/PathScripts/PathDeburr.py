# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2020-2021 Schildkroet                                   *
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

import FreeCAD
import PathScripts.PathEngraveBase as PathEngraveBase
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathOpTools as PathOpTools
import math

from PySide.QtCore import QT_TRANSLATE_NOOP

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

__title__ = "Path Deburr Operation"
__author__ = "sliptonic (Brad Collette), Schildkroet"
__url__ = "http://www.freecadweb.org"
__doc__ = "Deburr operation."


if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

translate = FreeCAD.Qt.translate


def toolDepthAndOffset(width, extraDepth, tool, printInfo):
    """toolDepthAndOffset(width, extraDepth, tool) ... return tuple for given\n
    parameters."""

    if not hasattr(tool, "Diameter"):
        raise ValueError("Deburr requires tool with diameter\n")

    suppressInfo = False
    if hasattr(tool, "CuttingEdgeAngle"):
        angle = float(tool.CuttingEdgeAngle)
        if PathGeom.isRoughly(angle, 180) or PathGeom.isRoughly(angle, 0):
            angle = 180
            toolOffset = float(tool.Diameter) / 2
        else:
            if hasattr(tool, "TipDiameter"):
                toolOffset = float(tool.TipDiameter) / 2
            elif hasattr(tool, "FlatRadius"):
                toolOffset = float(tool.FlatRadius)
            else:
                toolOffset = 0.0
                if printInfo and not suppressInfo:
                    FreeCAD.Console.PrintMessage(
                        translate(
                            "PathDeburr",
                            "The selected tool has no FlatRadius and no TipDiameter property. Assuming {}\n".format(
                                "Endmill" if angle == 180 else "V-Bit"
                            ),
                        )
                    )
                suppressInfo = True
    else:
        angle = 180
        toolOffset = float(tool.Diameter) / 2
        if printInfo:
            FreeCAD.Console.PrintMessage(
                translate(
                    "PathDeburr",
                    "The selected tool has no CuttingEdgeAngle property. Assuming Endmill\n",
                )
            )
        suppressInfo = True

    tan = math.tan(math.radians(angle / 2))

    toolDepth = 0 if PathGeom.isRoughly(tan, 0) else width / tan
    depth = toolDepth + extraDepth
    extraOffset = -width if angle == 180 else (extraDepth / tan)
    offset = toolOffset + extraOffset

    return (depth, offset, extraOffset, suppressInfo)


class ObjectDeburr(PathEngraveBase.ObjectOp):
    """Proxy class for Deburr operation."""

    def opFeatures(self, obj):
        return (
            PathOp.FeatureTool
            | PathOp.FeatureHeights
            | PathOp.FeatureStepDown
            | PathOp.FeatureBaseEdges
            | PathOp.FeatureBaseFaces
            | PathOp.FeatureCoolant
            | PathOp.FeatureBaseGeometry
        )

    def initOperation(self, obj):
        PathLog.track(obj.Label)
        obj.addProperty(
            "App::PropertyDistance",
            "Width",
            "Deburr",
            QT_TRANSLATE_NOOP("App::Property", "The desired width of the chamfer"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "ExtraDepth",
            "Deburr",
            QT_TRANSLATE_NOOP("App::Property", "The additional depth of the tool path"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "Join",
            "Deburr",
            QT_TRANSLATE_NOOP("App::Property", "How to join chamfer segments"),
        )
        # obj.Join = ["Round", "Miter"]
        obj.setEditorMode("Join", 2)  # hide for now
        obj.addProperty(
            "App::PropertyEnumeration",
            "Direction",
            "Deburr",
            QT_TRANSLATE_NOOP("App::Property", "Direction of Operation"),
        )
        # obj.Direction = ["CW", "CCW"]
        obj.addProperty(
            "App::PropertyEnumeration",
            "Side",
            "Deburr",
            QT_TRANSLATE_NOOP("App::Property", "Side of Operation"),
        )
        obj.Side = ["Outside", "Inside"]
        obj.setEditorMode("Side", 2)  # Hide property, it's calculated by op
        obj.addProperty(
            "App::PropertyInteger",
            "EntryPoint",
            "Deburr",
            QT_TRANSLATE_NOOP(
                "App::Property", "Select the segment, there the operations starts"
            ),
        )

        ENUMS = self.propertyEnumerations()
        for n in ENUMS:
            setattr(obj, n[0], n[1])

    @classmethod
    def propertyEnumerations(self, dataType="data"):

        """opPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        # Enumeration lists for App::PropertyEnumeration properties
        enums = {
            "Direction": [
                (translate("Path", "CW"), "CW"),
                (translate("Path", "CCW"), "CCW"),
            ],  # this is the direction that the profile runs
            "Join": [
                (translate("PathDeburr", "Round"), "Round"),
                (translate("PathDeburr", "Miter"), "Miter"),
            ],  # this is the direction that the profile runs
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        PathLog.debug(enums)

        for k, v in enumerate(enums):
            # data[k] = [tup[idx] for tup in v]
            data.append((v, [tup[idx] for tup in enums[v]]))
        PathLog.debug(data)

        return data

    def opOnDocumentRestored(self, obj):
        obj.setEditorMode("Join", 2)  # hide for now

    def opExecute(self, obj):
        PathLog.track(obj.Label)
        if not hasattr(self, "printInfo"):
            self.printInfo = True
        try:
            (depth, offset, extraOffset, suppressInfo) = toolDepthAndOffset(
                obj.Width.Value, obj.ExtraDepth.Value, self.tool, self.printInfo
            )
            self.printInfo = not suppressInfo
        except ValueError as e:
            msg = "{} \n No path will be generated".format(e)
            raise ValueError(msg)
            # QtGui.QMessageBox.information(None, "Tool Error", msg)
            # return

        PathLog.track(obj.Label, depth, offset)

        self.basewires = []  # pylint: disable=attribute-defined-outside-init
        self.adjusted_basewires = []  # pylint: disable=attribute-defined-outside-init
        wires = []

        for base, subs in obj.Base:
            edges = []
            basewires = []
            max_h = -99999
            radius_top = 0
            radius_bottom = 0

            for f in subs:
                sub = base.Shape.getElement(f)

                if type(sub) == Part.Edge:  # Edge
                    edges.append(sub)

                elif type(sub) == Part.Face and sub.normalAt(0, 0) != FreeCAD.Vector(
                    0, 0, 1
                ):  # Angled face
                    # If an angled face is selected, the lower edge is projected to the height of the upper edge,
                    # to simulate an edge

                    # Find z value of upper edge
                    for edge in sub.Edges:
                        for p0 in edge.Vertexes:
                            if p0.Point.z > max_h:
                                max_h = p0.Point.z

                    # Find biggest radius for top/bottom
                    for edge in sub.Edges:
                        if Part.Circle == type(edge.Curve):
                            if edge.Vertexes[0].Point.z == max_h:
                                if edge.Curve.Radius > radius_top:
                                    radius_top = edge.Curve.Radius
                            else:
                                if edge.Curve.Radius > radius_bottom:
                                    radius_bottom = edge.Curve.Radius

                    # Search for lower edge and raise it to height of upper edge
                    for edge in sub.Edges:
                        if Part.Circle == type(edge.Curve):  # Edge is a circle
                            if edge.Vertexes[0].Point.z < max_h:

                                if edge.Closed:  # Circle
                                    # New center
                                    center = FreeCAD.Vector(
                                        edge.Curve.Center.x, edge.Curve.Center.y, max_h
                                    )
                                    new_edge = Part.makeCircle(
                                        edge.Curve.Radius,
                                        center,
                                        FreeCAD.Vector(0, 0, 1),
                                    )
                                    edges.append(new_edge)

                                    # Modify offset for inner angled faces
                                    if radius_bottom < radius_top:
                                        offset -= 2 * extraOffset

                                    break

                                else:  # Arc
                                    if (
                                        edge.Vertexes[0].Point.z
                                        == edge.Vertexes[1].Point.z
                                    ):
                                        # Arc vertexes are on same layer
                                        l1 = math.sqrt(
                                            (
                                                edge.Vertexes[0].Point.x
                                                - edge.Curve.Center.x
                                            )
                                            ** 2
                                            + (
                                                edge.Vertexes[0].Point.y
                                                - edge.Curve.Center.y
                                            )
                                            ** 2
                                        )
                                        l2 = math.sqrt(
                                            (
                                                edge.Vertexes[1].Point.x
                                                - edge.Curve.Center.x
                                            )
                                            ** 2
                                            + (
                                                edge.Vertexes[1].Point.y
                                                - edge.Curve.Center.y
                                            )
                                            ** 2
                                        )

                                        # New center
                                        center = FreeCAD.Vector(
                                            edge.Curve.Center.x,
                                            edge.Curve.Center.y,
                                            max_h,
                                        )

                                        # Calculate angles based on x-axis (0 - PI/2)
                                        start_angle = math.acos(
                                            (
                                                edge.Vertexes[0].Point.x
                                                - edge.Curve.Center.x
                                            )
                                            / l1
                                        )
                                        end_angle = math.acos(
                                            (
                                                edge.Vertexes[1].Point.x
                                                - edge.Curve.Center.x
                                            )
                                            / l2
                                        )

                                        # Angles are based on x-axis (Mirrored on x-axis) -> negative y value means negative angle
                                        if (
                                            edge.Vertexes[0].Point.y
                                            < edge.Curve.Center.y
                                        ):
                                            start_angle *= -1
                                        if (
                                            edge.Vertexes[1].Point.y
                                            < edge.Curve.Center.y
                                        ):
                                            end_angle *= -1

                                        # Create new arc
                                        new_edge = Part.ArcOfCircle(
                                            Part.Circle(
                                                center,
                                                FreeCAD.Vector(0, 0, 1),
                                                edge.Curve.Radius,
                                            ),
                                            start_angle,
                                            end_angle,
                                        ).toShape()
                                        edges.append(new_edge)

                                        # Modify offset for inner angled faces
                                        if radius_bottom < radius_top:
                                            offset -= 2 * extraOffset

                                        break

                        else:  # Line
                            if (
                                edge.Vertexes[0].Point.z == edge.Vertexes[1].Point.z
                                and edge.Vertexes[0].Point.z < max_h
                            ):
                                new_edge = Part.Edge(
                                    Part.LineSegment(
                                        FreeCAD.Vector(
                                            edge.Vertexes[0].Point.x,
                                            edge.Vertexes[0].Point.y,
                                            max_h,
                                        ),
                                        FreeCAD.Vector(
                                            edge.Vertexes[1].Point.x,
                                            edge.Vertexes[1].Point.y,
                                            max_h,
                                        ),
                                    )
                                )
                                edges.append(new_edge)

                elif sub.Wires:
                    basewires.extend(sub.Wires)

                else:  # Flat face
                    basewires.append(Part.Wire(sub.Edges))

            self.edges = edges  # pylint: disable=attribute-defined-outside-init
            for edgelist in Part.sortEdges(edges):
                basewires.append(Part.Wire(edgelist))

            self.basewires.extend(basewires)

            # Set default side
            side = ["Outside"]

            for w in basewires:
                self.adjusted_basewires.append(w)
                wire = PathOpTools.offsetWire(w, base.Shape, offset, True, side)
                if wire:
                    wires.append(wire)

        # Set direction of op
        forward = obj.Direction == "CW"

        # Set value of side
        obj.Side = side[0]
        # Check side extra for angled faces
        if radius_top > radius_bottom:
            obj.Side = "Inside"

        zValues = []
        z = 0
        if obj.StepDown.Value != 0:
            while z + obj.StepDown.Value < depth:
                z = z + obj.StepDown.Value
                zValues.append(z)

        zValues.append(depth)
        PathLog.track(obj.Label, depth, zValues)

        if obj.EntryPoint < 0:
            obj.EntryPoint = 0

        self.wires = wires  # pylint: disable=attribute-defined-outside-init
        self.buildpathocc(obj, wires, zValues, True, forward, obj.EntryPoint)

    def opRejectAddBase(self, obj, base, sub):
        """The chamfer op can only deal with features of the base model, all others are rejected."""
        return base not in self.model

    def opSetDefaultValues(self, obj, job):
        PathLog.track(obj.Label, job.Label)
        obj.Width = "1 mm"
        obj.ExtraDepth = "0.5 mm"
        obj.Join = "Round"
        obj.setExpression("StepDown", "0 mm")
        obj.StepDown = "0 mm"
        obj.Direction = "CW"
        obj.Side = "Outside"
        obj.EntryPoint = 0


def SetupProperties():
    setup = []
    setup.append("Width")
    setup.append("ExtraDepth")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Deburr operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectDeburr(obj, name, parentJob)
    return obj
