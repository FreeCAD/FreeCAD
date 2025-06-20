# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 Pekka Roivainen <pekkaroi@gmail.com>               *
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

from PathScripts import PathUtils
from PySide.QtCore import QT_TRANSLATE_NOOP
import copy
import FreeCAD
import Path
import Path.Dressup.Utils as PathDressup
import math


# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

if FreeCAD.GuiUp:
    import FreeCADGui


translate = FreeCAD.Qt.translate


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class AnnotatedGCode:
    def __init__(self, command, start_point):
        self.start_point = start_point
        self.command = command
        self.end_point = (
            command.Parameters.get("X", start_point[0]),
            command.Parameters.get("Y", start_point[1]),
            command.Parameters.get("Z", start_point[2]),
        )
        self.is_line = command.Name in Path.Geom.CmdMoveStraight
        self.is_arc = command.Name in Path.Geom.CmdMoveArc
        self.xy_length = None
        if self.is_line:
            self.xy_length = (
                (start_point[0] - self.end_point[0]) ** 2
                + (start_point[1] - self.end_point[1]) ** 2
            ) ** 0.5
        elif self.is_arc:
            self.center_xy = (
                start_point[0] + command.Parameters.get("I", 0),
                start_point[1] + command.Parameters.get("J", 0),
            )
            self.start_angle = math.atan2(
                start_point[1] - self.center_xy[1],
                start_point[0] - self.center_xy[0],
            )
            self.end_angle = math.atan2(
                self.end_point[1] - self.center_xy[1],
                self.end_point[0] - self.center_xy[0],
            )
            if self.command.Name in Path.Geom.CmdMoveCCW and self.end_angle < self.start_angle:
                self.end_angle += 2 * math.pi
            if self.command.Name in Path.Geom.CmdMoveCW and self.end_angle > self.start_angle:
                self.end_angle -= 2 * math.pi
            self.radius = (
                (start_point[0] - self.center_xy[0]) ** 2
                + (start_point[1] - self.center_xy[1]) ** 2
            ) ** 0.5
            self.xy_length = self.radius * abs(self.end_angle - self.start_angle)

    """Makes a copy of this annotated gcode at the given z height"""

    def clone(self, z_start=None, z_end=None, reverse=False):
        z_start = z_start if z_start is not None else self.start_point[2]
        z_end = z_end if z_end is not None else self.end_point[2]

        other = copy.copy(self)
        otherParams = copy.copy(self.command.Parameters)
        otherCommandName = self.command.Name
        other.start_point = (self.start_point[0], self.start_point[1], z_start)
        other.end_point = (self.end_point[0], self.end_point[1], z_end)
        otherParams.update({"Z": z_end})
        if reverse:
            other.start_point, other.end_point = other.end_point, other.start_point
            otherParams.update(
                {"X": other.end_point[0], "Y": other.end_point[1], "Z": other.end_point[2]}
            )
            if other.is_arc:
                other.start_angle, other.end_angle = other.end_angle, other.start_angle
                otherCommandName = (
                    Path.Geom.CmdMoveCW[0]
                    if self.command.Name in Path.Geom.CmdMoveCCW
                    else Path.Geom.CmdMoveCCW[0]
                )
                otherParams.update(
                    {
                        "I": other.center_xy[0] - other.start_point[0],
                        "J": other.center_xy[1] - other.start_point[1],
                    }
                )
        other.command = Path.Command(otherCommandName, otherParams)
        return other

    """Splits the edge into two parts, the first split_length (if less than xy_length) long. Only supported for lines and arcs (no rapids)"""

    def split(self, split_length):
        split_length = min(split_length, self.xy_length)
        p = split_length / self.xy_length
        firstParams = copy.copy(self.command.Parameters)
        secondParams = copy.copy(self.command.Parameters)
        split_point = None
        if self.is_line:
            split_point = (
                self.start_point[0] * (1 - p) + self.end_point[0] * p,
                self.start_point[1] * (1 - p) + self.end_point[1] * p,
                self.start_point[2] * (1 - p) + self.end_point[2] * p,
            )
        elif self.is_arc:
            angle = self.start_angle * (1 - p) + self.end_angle * p
            split_point = (
                self.center_xy[0] + self.radius * math.cos(angle),
                self.center_xy[1] + self.radius * math.sin(angle),
                self.start_point[2] * (1 - p) + self.end_point[2] * p,
            )
            secondParams.update(
                {
                    "I": self.center_xy[0] - split_point[0],
                    "J": self.center_xy[1] - split_point[1],
                }
            )
        else:
            raise Exception("Invalid type, can only split (non-rapid) lines and arcs")

        firstParams.update({"X": split_point[0], "Y": split_point[1], "Z": split_point[2]})
        first_command = Path.Command(self.command.Name, firstParams)
        second_command = Path.Command(self.command.Name, secondParams)
        return AnnotatedGCode(first_command, self.start_point), AnnotatedGCode(
            second_command, split_point
        )


class ObjectDressup:
    def __init__(self, obj):
        self.obj = obj
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base toolpath to modify"),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "Angle",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Angle of ramp."),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "Method",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Ramping Method"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "RampFeedRate",
            "FeedRate",
            QT_TRANSLATE_NOOP("App::Property", "Which feed rate to use for ramping"),
        )
        obj.addProperty(
            "App::PropertySpeed",
            "CustomFeedRate",
            "FeedRate",
            QT_TRANSLATE_NOOP("App::Property", "Custom feed rate"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "UseStartDepth",
            "StartDepth",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Should the dressup ignore motion commands above DressupStartDepth",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "DressupStartDepth",
            "StartDepth",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The depth where the ramp dressup is enabled. Above this ramps are not generated, but motion commands are passed through as is.",
            ),
        )

        # populate the enumerations
        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

        obj.Proxy = self
        self.setEditorProperties(obj)

        # initialized later
        self.wire = None
        self.angle = None
        self.rapids = None
        self.method = None
        self.outedges = None
        self.ignoreAboveEnabled = None
        self.ignoreAbove = None

    @classmethod
    def propertyEnumerations(self, dataType="data"):
        """PropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        enums = {
            "Method": [
                (translate("CAM_DressupRampEntry", "RampMethod1"), "RampMethod1"),
                (translate("CAM_DressupRampEntry", "RampMethod2"), "RampMethod2"),
                (translate("CAM_DressupRampEntry", "RampMethod3"), "RampMethod3"),
                (translate("CAM_DressupRampEntry", "Helix"), "Helix"),
            ],
            "RampFeedRate": [
                (
                    translate("CAM_DressupRampEntry", "Horizontal Feed Rate"),
                    "Horizontal Feed Rate",
                ),
                (
                    translate("CAM_DressupRampEntry", "Vertical Feed Rate"),
                    "Vertical Feed Rate",
                ),
                (
                    translate("CAM_DressupRampEntry", "Ramp Feed Rate"),
                    "Ramp Feed Rate",
                ),
                (translate("CAM_DressupRampEntry", "Custom"), "Custom"),
            ],
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, obj, prop):
        if prop in ["RampFeedRate", "UseStartDepth"]:
            self.setEditorProperties(obj)

    def setEditorProperties(self, obj):
        if hasattr(obj, "UseStartDepth"):
            if obj.UseStartDepth:
                obj.setEditorMode("DressupStartDepth", 0)
            else:
                obj.setEditorMode("DressupStartDepth", 2)

        if obj.RampFeedRate == "Custom":
            obj.setEditorMode("CustomFeedRate", 0)
        else:
            obj.setEditorMode("CustomFeedRate", 2)

    def onDocumentRestored(self, obj):
        self.setEditorProperties(obj)

    def setup(self, obj):
        obj.Angle = 60
        obj.Method = 2
        if PathDressup.baseOp(obj.Base).StartDepth is not None:
            obj.DressupStartDepth = PathDressup.baseOp(obj.Base).StartDepth

    def execute(self, obj):
        if not obj.Base:
            return
        if not obj.Base.isDerivedFrom("Path::Feature"):
            return
        if not obj.Base.Path:
            return

        if not PathDressup.baseOp(obj.Base).Active:
            path = Path.Path("(inactive operation)")
            obj.Path = path
            return

        if obj.Angle >= 90:
            obj.Angle = 89.9
        elif obj.Angle <= 0:
            obj.Angle = 0.1

        if hasattr(obj, "UseStartDepth"):
            self.ignoreAboveEnabled = obj.UseStartDepth
            self.ignoreAbove = obj.DressupStartDepth
        else:
            self.ignoreAboveEnabled = False
            self.ignoreAbove = 0

        self.angle = obj.Angle
        self.method = obj.Method
        positioned_path = PathUtils.getPathWithPlacement(obj.Base)
        cmds = positioned_path.Commands if hasattr(positioned_path, "Commands") else []
        self.edges = []
        start_point = (0, 0, 0)
        for cmd in cmds:
            annotated = AnnotatedGCode(cmd, start_point)
            self.edges.append(annotated)
            start_point = annotated.end_point
        # TODO combine all ramp methods? Or else rewrite the others
        # TODO rewrite helix
        if self.method in ["RampMethod1", "RampMethod2", "RampMethod3"]:
            self.outedges = self.generateRamps()
        else:
            self.outedges = self.generateHelix()
        obj.Path = self.createCommands(obj, self.outedges)

    def generateRamps(self, allowBounce=True):
        edges = self.edges
        outedges = []
        for edgei, edge in enumerate(edges):
            if edge.is_line or edge.is_arc:
                rampangle = self.angle
                # check for plunge
                if edge.xy_length < 1e-6 and edge.end_point[2] < edge.start_point[2]:
                    # check if above ignoreAbove parameter - do not generate ramp if it is
                    orig_edge = edge
                    noramp_edge, edge = self.processIgnoreAbove(edge)
                    if noramp_edge is not None:
                        outedges.append(noramp_edge)
                    if edge is None:
                        continue

                    plungelen = abs(edge.start_point[2] - edge.end_point[2])
                    projectionlen = plungelen * math.tan(
                        math.radians(rampangle)
                    )  # length of the forthcoming ramp projected to XY plane
                    # Path.Log.debug(
                    #    "Found plunge move at X:{} Y:{} From Z:{} to Z{}, length of ramp: {}".format(
                    #        p0.x, p0.y, p0.z, p1.z, projectionlen
                    #    )
                    # )
                    if self.method == "RampMethod3":
                        projectionlen = projectionlen / 2

                    # next need to determine how many edges in the path after
                    # plunge are needed to cover the length:
                    covered = False
                    coveredlen = 0
                    rampedges = []
                    i = edgei + 1
                    while not covered and i < len(edges):
                        candidate = edges[i]
                        if abs(candidate.start_point[2] - candidate.end_point[2]) > 1e-6 or (
                            not candidate.is_line and not candidate.is_arc
                        ):
                            # this edge is not an edge/arc in the XY plane; not qualified for ramping
                            break
                        # Path.Log.debug("Next edge length {}".format(candidate.Length))
                        rampedges.append(candidate)
                        coveredlen = coveredlen + candidate.xy_length

                        if coveredlen > projectionlen:
                            covered = True
                        i = i + 1
                    if len(rampedges) == 0:
                        Path.Log.debug("No suitable edges for ramping, plunge will remain as such")
                        outedges.append(edge)
                    else:
                        if not covered:
                            if (not allowBounce) or self.method == "RampMethod2":
                                if self.method == "RampMethod3":
                                    rampangle = math.degrees(
                                        math.atan(coveredlen / (plungelen / 2))
                                    )
                                else:
                                    rampangle = math.degrees(math.atan(coveredlen / plungelen))
                                Path.Log.warning(
                                    "Cannot cover with desired angle, tightening angle to: {}".format(
                                        rampangle
                                    )
                                )

                        # Path.Log.debug("Doing ramp to edges: {}".format(rampedges))
                        if self.method == "RampMethod1":
                            outedges.extend(
                                self.createRampMethod1(
                                    rampedges, edge.start_point, projectionlen, rampangle
                                )
                            )
                        elif self.method == "RampMethod2":
                            outedges.extend(
                                self.createRampMethod2(
                                    rampedges, edge.start_point, projectionlen, rampangle
                                )
                            )
                        else:
                            # if the ramp cannot be covered with Method3, revert to Method1
                            # because Method1 support going back-and-forth and thus results in same path as Method3 when
                            # length of the ramp is smaller than needed for single ramp.
                            if (not covered) and allowBounce:
                                projectionlen = projectionlen * 2
                                outedges.extend(
                                    self.createRampMethod1(
                                        rampedges, edge.start_point, projectionlen, rampangle
                                    )
                                )
                            else:
                                outedges.extend(
                                    self.createRampMethod3(
                                        rampedges, edge.start_point, projectionlen, rampangle
                                    )
                                )
                else:
                    outedges.append(edge)
            else:
                outedges.append(edge)
        return outedges

    def generateHelix(self):
        edges = self.wire.Edges
        minZ = self.findMinZ(edges)
        Path.Log.debug("Minimum Z in this path is {}".format(minZ))
        outedges = []
        i = 0
        while i < len(edges):
            edge = edges[i]
            israpid = i in self.rapid_indexes
            if not israpid:
                bb = edge.BoundBox
                p0 = edge.Vertexes[0].Point
                p1 = edge.Vertexes[1].Point
                if bb.XLength < 1e-6 and bb.YLength < 1e-6 and bb.ZLength > 0 and p0.z > p1.z:
                    # plungelen = abs(p0.z-p1.z)
                    Path.Log.debug(
                        "Found plunge move at X:{} Y:{} From Z:{} to Z{}, Searching for closed loop".format(
                            p0.x, p0.y, p0.z, p1.z
                        )
                    )
                    # check if above ignoreAbove parameter - do not generate helix if it is
                    newEdge, cont = self.checkIgnoreAbove(edge)
                    if newEdge is not None:
                        outedges.append((newEdge, israpid))
                        p0.z = self.ignoreAbove
                    if cont:
                        i = i + 1
                        continue
                    # next need to determine how many edges in the path after plunge are needed to cover the length:
                    loopFound = False
                    rampedges = []
                    j = i + 1
                    while not loopFound:
                        candidate = edges[j]
                        cp0 = candidate.Vertexes[0].Point
                        cp1 = candidate.Vertexes[1].Point
                        if Path.Geom.pointsCoincide(p1, cp1):
                            # found closed loop
                            loopFound = True
                            rampedges.append(candidate)
                            break
                        if abs(cp0.z - cp1.z) > 1e-6:
                            # this edge is not parallel to XY plane, not qualified for ramping.
                            break
                        # Path.Log.debug("Next edge length {}".format(candidate.Length))
                        rampedges.append(candidate)
                        j = j + 1
                        if j >= len(edges):
                            break
                    if len(rampedges) == 0 or not loopFound:
                        Path.Log.debug("No suitable helix found")
                        outedges.append((edge, israpid))
                    else:
                        outedges.extend(self.createHelix(rampedges, p0, p1))
                        if not Path.Geom.isRoughly(p1.z, minZ):
                            # the edges covered by the helix not handled again,
                            # unless reached the bottom height
                            i = j

                else:
                    outedges.append((edge, israpid))
            else:
                outedges.append((edge, israpid))
            i = i + 1
        return outedges

    """
    Edges, or parts of edges, above self.ignoreAbove should not be ramped.
    This method is a helper for splitting edges into a portion that should be
    ramped and a portion that should not be ramped.

    Returns (noramp_edge, ramp_edge). Either of these variables may be None
    """

    def processIgnoreAbove(self, edge):
        if not self.ignoreAboveEnabled:
            return None, edge
        z0, z1 = edge.start_point[2], edge.end_point[2]
        if z0 > self.ignoreAbove.Value:
            if z1 > self.ignoreAbove.Value or Path.Geom.isRoughly(z1, self.ignoreAbove.Value):
                # Entire plunge is above ignoreAbove
                return edge, None
            elif not Path.Geom.isRoughly(z0, self.ignoreAbove.Value):
                # Split the edge into regions above and below
                return (
                    edge.clone(z0, self.ignoreAbove.Value),
                    edge.clone(self.ignoreAbove.Value, z1),
                )
        # Entire plunge is below ignoreAbove
        return None, edge

    def checkIgnoreAbove(self, edge):
        if self.ignoreAboveEnabled:
            p0 = edge.Vertexes[0].Point
            p1 = edge.Vertexes[1].Point
            if p0.z > self.ignoreAbove and (
                p1.z > self.ignoreAbove or Path.Geom.isRoughly(p1.z, self.ignoreAbove.Value)
            ):
                Path.Log.debug("Whole plunge move above 'ignoreAbove', ignoring")
                return (edge, True)
            elif p0.z > self.ignoreAbove and not Path.Geom.isRoughly(p0.z, self.ignoreAbove.Value):
                Path.Log.debug("Plunge move partially above 'ignoreAbove', splitting into two")
                newPoint = FreeCAD.Base.Vector(p0.x, p0.y, self.ignoreAbove)
                return (Part.makeLine(p0, newPoint), False)
            else:
                return None, False
        else:
            return None, False

    def createHelix(self, rampedges, startPoint, endPoint):
        outedges = []
        ramplen = 0
        for redge in rampedges:
            ramplen = ramplen + redge.Length
        rampheight = abs(endPoint.z - startPoint.z)
        rampangle_rad = math.atan(ramplen / rampheight)
        curPoint = startPoint
        for i, redge in enumerate(rampedges):
            if i < len(rampedges) - 1:
                deltaZ = redge.Length / math.tan(rampangle_rad)
                newPoint = FreeCAD.Base.Vector(
                    redge.valueAt(redge.LastParameter).x,
                    redge.valueAt(redge.LastParameter).y,
                    curPoint.z - deltaZ,
                )
                outedges.append(self.createRampEdge(redge, curPoint, newPoint))
                curPoint = newPoint
            else:
                # on the last edge, force it to end to the endPoint
                # this should happen automatically, but this avoids any rounding error
                outedges.append(self.createRampEdge(redge, curPoint, endPoint))
        return [(e, False) for e in outedges]

    def createRampEdge(self, originalEdge, startPoint, endPoint):
        # Path.Log.debug("Create edge from [{},{},{}] to [{},{},{}]".format(startPoint.x,startPoint.y, startPoint.z, endPoint.x, endPoint.y, endPoint.z))
        if type(originalEdge.Curve) == Part.Line or type(originalEdge.Curve) == Part.LineSegment:
            return Part.makeLine(startPoint, endPoint)
        elif type(originalEdge.Curve) == Part.Circle:
            firstParameter = originalEdge.Curve.parameter(startPoint)
            lastParameter = originalEdge.Curve.parameter(endPoint)
            arcMid = originalEdge.valueAt((firstParameter + lastParameter) / 2)
            arcMid.z = (startPoint.z + endPoint.z) / 2
            return Part.Arc(startPoint, arcMid, endPoint).toShape()
        else:
            Path.Log.error("Edge should not be helix")

    def getreversed(self, edges):
        """
        Reverses the edge array and the direction of each edge
        """
        outedges = []
        for edge in reversed(edges):
            # reverse the start and end points
            startPoint = edge.valueAt(edge.LastParameter)
            endPoint = edge.valueAt(edge.FirstParameter)
            if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
                outedges.append(Part.makeLine(startPoint, endPoint))
            elif type(edge.Curve) == Part.Circle:
                arcMid = edge.valueAt((edge.FirstParameter + edge.LastParameter) / 2)
                outedges.append(Part.Arc(startPoint, arcMid, endPoint).toShape())
            else:
                Path.Log.error("Edge should not be helix")
        return outedges

    def findMinZ(self, edges):
        minZ = 99999999999
        for edge in edges[1:]:
            for v in edge.Vertexes:
                if v.Point.z < minZ:
                    minZ = v.Point.z
        return minZ

    def getSplitPoint(self, edge, remaining):
        if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
            return edge.valueAt(remaining)
        elif type(edge.Curve) == Part.Circle:
            param = remaining / edge.Curve.Radius
            return edge.valueAt(param)

    def createRampMethod1(self, rampedges, p0, projectionlen, rampangle):
        """
        This method generates ramp with following pattern:
        1. Start from the original startpoint of the plunge
        2. Ramp down along the path that comes after the plunge
        3. When reaching the Z level of the original plunge, return back to the beginning
           by going the path backwards until the original plunge end point is reached
        4. Continue with the original path

        This method causes many unnecessary moves with tool down.
        """
        outedges = []
        reversed_edges = [redge.clone(reverse=True) for redge in rampedges]
        rampremaining = projectionlen
        z = p0[2]  # start from the upper point of plunge
        goingForward = True
        i = 0  # current position = start of this edge. May be len(rampremaining) if going backwards
        while rampremaining > 0:
            redge = rampedges[i] if goingForward else reversed_edges[i - 1]
            # for i, redge in enumerate(rampedges):
            if redge.xy_length > rampremaining:
                # will reach end of ramp within this edge, needs to be split
                split_first, split_remaining = redge.split(rampremaining)
                outedges.append(split_first.clone(z_start=z))
                # now we have reached the end of the ramp. Go back to plunge position with constant Z
                # start that by going to the beginning of this splitEdge
                if goingForward:
                    outedges.append(split_first.clone(reverse=True))
                else:
                    # if we were reversing, we continue to the same direction as the ramp
                    outedges.append(split_remaining)
                    i = i - 1
                rampremaining = 0
                break
            else:
                deltaZ = redge.xy_length / math.tan(math.radians(rampangle))
                new_z = z - deltaZ
                outedges.append(redge.clone(z, new_z))
                z = new_z
                rampremaining = rampremaining - redge.xy_length
                i = i + 1 if goingForward else i - 1
                if i == 0:
                    goingForward = True
                if i == len(rampedges):
                    goingForward = False

        # now we need to return to original position.
        while i >= 1:
            outedges.append(reversed_edges[i - 1])
            i = i - 1

        return outedges

    def createRampMethod3(self, rampedges, p0, projectionlen, rampangle):
        """
        This method generates ramp with following pattern:
        1. Start from the original startpoint of the plunge
        2. Ramp down along the path that comes after the plunge until
           traveled half of the Z distance
        3. Change direction and ramp backwards to the original plunge end point
        4. Continue with the original path

        This method causes many unnecessary moves with tool down.
        """
        outedges = []
        rampremaining = projectionlen
        curPoint = p0  # start from the upper point of plunge
        done = False

        i = 0
        while not done:
            for i, redge in enumerate(rampedges):
                if redge.Length >= rampremaining:
                    # will reach end of ramp within this edge, needs to be split
                    p1 = self.getSplitPoint(redge, rampremaining)
                    splitEdge = Path.Geom.splitEdgeAt(redge, p1)
                    Path.Log.debug(
                        "Got split edge (index: {}) with lengths: {}, {}".format(
                            i, splitEdge[0].Length, splitEdge[1].Length
                        )
                    )
                    # ramp ends to the last point of first edge
                    p1 = splitEdge[0].valueAt(splitEdge[0].LastParameter)
                    deltaZ = splitEdge[0].Length / math.tan(math.radians(rampangle))
                    p1.z = curPoint.z - deltaZ
                    outedges.append(self.createRampEdge(splitEdge[0], curPoint, p1))
                    curPoint.z = p1.z - deltaZ
                    # now we have reached the end of the ramp. Reverse direction of ramp
                    # start that by going back to the beginning of this splitEdge
                    outedges.append(self.createRampEdge(splitEdge[0], p1, curPoint))

                    done = True
                    break
                elif i == len(rampedges) - 1:
                    # last ramp element but still did not reach the full length?
                    # Probably a rounding issue on floats.
                    p1 = redge.valueAt(redge.LastParameter)
                    deltaZ = redge.Length / math.tan(math.radians(rampangle))
                    p1.z = curPoint.z - deltaZ
                    outedges.append(self.createRampEdge(redge, curPoint, p1))
                    # and go back that edge
                    newPoint = FreeCAD.Base.Vector(
                        redge.valueAt(redge.FirstParameter).x,
                        redge.valueAt(redge.FirstParameter).y,
                        p1.z - deltaZ,
                    )
                    outedges.append(self.createRampEdge(redge, p1, newPoint))
                    curPoint = newPoint
                    done = True
                else:
                    deltaZ = redge.Length / math.tan(math.radians(rampangle))
                    newPoint = FreeCAD.Base.Vector(
                        redge.valueAt(redge.LastParameter).x,
                        redge.valueAt(redge.LastParameter).y,
                        curPoint.z - deltaZ,
                    )
                    outedges.append(self.createRampEdge(redge, curPoint, newPoint))
                    curPoint = newPoint
                    rampremaining = rampremaining - redge.Length

        returnedges = self.getreversed(rampedges[:i])

        # ramp backwards to the plunge position
        for i, redge in enumerate(returnedges):
            deltaZ = redge.Length / math.tan(math.radians(rampangle))
            newPoint = FreeCAD.Base.Vector(
                redge.valueAt(redge.LastParameter).x,
                redge.valueAt(redge.LastParameter).y,
                curPoint.z - deltaZ,
            )
            if i == len(rampedges) - 1:
                # make sure that the last point of the ramps ends to the original position
                newPoint = redge.valueAt(redge.LastParameter)
            outedges.append(self.createRampEdge(redge, curPoint, newPoint))
            curPoint = newPoint

        return [(e, False) for e in outedges]

    def createRampMethod2(self, rampedges, p0, projectionlen, rampangle):
        """
        This method generates ramp with following pattern:
        1. Start from the original startpoint of the plunge
        2. Calculate the distance on the path which is needed to implement the ramp
           and travel that distance while maintaining start depth
        3. Start ramping while traveling the original path backwards until reaching the
           original plunge end point
        4. Continue with the original path
        """
        outedges = []
        rampremaining = projectionlen
        curPoint = p0  # start from the upper point of plunge
        if Path.Geom.pointsCoincide(
            Path.Geom.xy(p0),
            Path.Geom.xy(rampedges[-1].valueAt(rampedges[-1].LastParameter)),
        ):
            Path.Log.debug("The ramp forms a closed wire, needless to move on original Z height")
        else:
            for i, redge in enumerate(rampedges):
                if redge.Length >= rampremaining:
                    # this edge needs to be split
                    p1 = self.getSplitPoint(redge, rampremaining)
                    splitEdge = Path.Geom.splitEdgeAt(redge, p1)
                    Path.Log.debug(
                        "Got split edges with lengths: {}, {}".format(
                            splitEdge[0].Length, splitEdge[1].Length
                        )
                    )
                    # ramp starts at the last point of first edge
                    p1 = splitEdge[0].valueAt(splitEdge[0].LastParameter)
                    p1.z = p0.z
                    outedges.append(self.createRampEdge(splitEdge[0], curPoint, p1))
                    # now we have reached the beginning of the ramp.
                    # start that by going to the beginning of this splitEdge
                    deltaZ = splitEdge[0].Length / math.tan(math.radians(rampangle))
                    newPoint = FreeCAD.Base.Vector(
                        splitEdge[0].valueAt(splitEdge[0].FirstParameter).x,
                        splitEdge[0].valueAt(splitEdge[0].FirstParameter).y,
                        p1.z - deltaZ,
                    )
                    outedges.append(self.createRampEdge(splitEdge[0], p1, newPoint))
                    curPoint = newPoint
                elif i == len(rampedges) - 1:
                    # last ramp element but still did not reach the full length?
                    # Probably a rounding issue on floats.
                    # Lets start the ramp anyway
                    p1 = redge.valueAt(redge.LastParameter)
                    p1.z = p0.z
                    outedges.append(self.createRampEdge(redge, curPoint, p1))
                    # and go back that edge
                    deltaZ = redge.Length / math.tan(math.radians(rampangle))
                    newPoint = FreeCAD.Base.Vector(
                        redge.valueAt(redge.FirstParameter).x,
                        redge.valueAt(redge.FirstParameter).y,
                        p1.z - deltaZ,
                    )
                    outedges.append(self.createRampEdge(redge, p1, newPoint))
                    curPoint = newPoint

                else:
                    # we are traveling on start depth
                    newPoint = FreeCAD.Base.Vector(
                        redge.valueAt(redge.LastParameter).x,
                        redge.valueAt(redge.LastParameter).y,
                        p0.z,
                    )
                    outedges.append(self.createRampEdge(redge, curPoint, newPoint))
                    curPoint = newPoint
                    rampremaining = rampremaining - redge.Length

            # the last edge got handled previously
            rampedges.pop()
        # ramp backwards to the plunge position
        for i, redge in enumerate(reversed(rampedges)):
            deltaZ = redge.Length / math.tan(math.radians(rampangle))
            newPoint = FreeCAD.Base.Vector(
                redge.valueAt(redge.FirstParameter).x,
                redge.valueAt(redge.FirstParameter).y,
                curPoint.z - deltaZ,
            )
            if i == len(rampedges) - 1:
                # make sure that the last point of the ramps ends to the original position
                newPoint = redge.valueAt(redge.FirstParameter)
            outedges.append(self.createRampEdge(redge, curPoint, newPoint))
            curPoint = newPoint

        return [(e, False) for e in outedges]

    def createCommands(self, obj, edges):
        commands = [edge.command for edge in edges]

        lastCmd = Path.Command("G0", {"X": 0.0, "Y": 0.0, "Z": 0.0})

        outCommands = []

        tc = PathDressup.toolController(obj.Base)

        horizFeed = tc.HorizFeed.Value
        vertFeed = tc.VertFeed.Value

        if obj.RampFeedRate == "Horizontal Feed Rate":
            rampFeed = tc.HorizFeed.Value
        elif obj.RampFeedRate == "Vertical Feed Rate":
            rampFeed = tc.VertFeed.Value
        elif obj.RampFeedRate == "Ramp Feed Rate":
            rampFeed = math.sqrt(pow(tc.VertFeed.Value, 2) + pow(tc.HorizFeed.Value, 2))
        else:
            rampFeed = obj.CustomFeedRate.Value

        horizRapid = tc.HorizRapid.Value
        vertRapid = tc.VertRapid.Value

        for cmd in commands:
            params = cmd.Parameters
            zVal = params.get("Z", None)
            zVal2 = lastCmd.Parameters.get("Z", None)

            xVal = params.get("X", None)
            xVal2 = lastCmd.Parameters.get("X", None)

            yVal2 = lastCmd.Parameters.get("Y", None)
            yVal = params.get("Y", None)

            zVal = zVal and round(zVal, 8)
            zVal2 = zVal2 and round(zVal2, 8)

            if cmd.Name in ["G1", "G2", "G3", "G01", "G02", "G03"]:
                if zVal is not None and zVal2 != zVal:
                    if Path.Geom.isRoughly(xVal, xVal2) and Path.Geom.isRoughly(yVal, yVal2):
                        # this is a straight plunge
                        params["F"] = vertFeed
                    else:
                        # this is a ramp
                        params["F"] = rampFeed
                else:
                    params["F"] = horizFeed
                lastCmd = cmd

            elif cmd.Name in ["G0", "G00"]:
                if zVal is not None and zVal2 != zVal:
                    params["F"] = vertRapid
                else:
                    params["F"] = horizRapid
                lastCmd = cmd

            outCommands.append(Path.Command(cmd.Name, params))

        return Path.Path(outCommands)


class ViewProviderDressup:
    def __init__(self, vobj):
        self.obj = vobj.Object

    def attach(self, vobj):
        self.obj = vobj.Object

    def claimChildren(self):
        if hasattr(self.obj.Base, "InList"):
            for i in self.obj.Base.InList:
                if hasattr(i, "Group"):
                    group = i.Group
                    for g in group:
                        if g.Name == self.obj.Base.Name:
                            group.remove(g)
                    i.Group = group
                    print(i.Group)
        # FreeCADGui.ActiveDocument.getObject(obj.Base.Name).Visibility = False
        return [self.obj.Base]

    def onDelete(self, arg1=None, arg2=None):
        """this makes sure that the base operation is added back to the project and visible"""
        Path.Log.debug("Deleting Dressup")
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(self.obj)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None


class CommandPathDressupRampEntry:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupRampEntry", "RampEntry"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_DressupRampEntry",
                "Creates a Ramp Entry Dress-up object from a selected toolpath",
            ),
        }

    def IsActive(self):
        op = PathDressup.selection()
        if op:
            return not PathDressup.hasEntryMethod(op)
        return False

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            Path.Log.error(
                translate("CAM_DressupRampEntry", "Please select one toolpath object") + "\n"
            )
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            Path.Log.error(
                translate("CAM_DressupRampEntry", "The selected object is not a toolpath") + "\n"
            )
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            Path.Log.error(translate("CAM_DressupRampEntry", "Please select a Profile object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create RampEntry Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.RampEntry")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "RampEntryDressup")'
        )
        FreeCADGui.doCommand("dbo = Path.Dressup.Gui.RampEntry.ObjectDressup(obj)")
        FreeCADGui.doCommand("base = FreeCAD.ActiveDocument." + selection[0].Name)
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(base)")
        FreeCADGui.doCommand("obj.Base = base")
        FreeCADGui.doCommand("job.Proxy.addOperation(obj, base)")
        FreeCADGui.doCommand(
            "obj.ViewObject.Proxy = Path.Dressup.Gui.RampEntry.ViewProviderDressup(obj.ViewObject)"
        )
        FreeCADGui.doCommand("Gui.ActiveDocument.getObject(base.Name).Visibility = False")
        FreeCADGui.doCommand("dbo.setup(obj)")
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()` called via TaskPanel.accept()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupRampEntry", CommandPathDressupRampEntry())

Path.Log.notice("Loading CAM_DressupRampEntry... done\n")
