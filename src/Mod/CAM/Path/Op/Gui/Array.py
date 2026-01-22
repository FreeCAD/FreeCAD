# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
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

import DraftVecUtils
import FreeCAD
import FreeCADGui
import Path
import Path.Op.Base as PathOp
import PathScripts.PathUtils as PathUtils
import Path.Base.Util as PathUtil
from Path.Dressup.Utils import toolController

import random
import math

from PySide.QtCore import QT_TRANSLATE_NOOP

__doc__ = """CAM Array object and FreeCAD command"""

translate = FreeCAD.Qt.translate


class ObjectArray:
    def __init__(self, obj):
        # Path properties group
        obj.addProperty(
            "App::PropertyBool",
            "Active",
            "Path",
            QT_TRANSLATE_NOOP("PathOp", "Make False, to prevent operation from generating code"),
        )
        obj.addProperty(
            "App::PropertyLinkList",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The toolpaths to array"),
        )
        obj.addProperty(
            "App::PropertyString",
            "CycleTime",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Operations cycle time estimation"),
        )
        obj.addProperty(
            "App::PropertyLink",
            "ToolController",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The tool controller that will be used to calculate the toolpath\nShould be identical for all base operations",
            ),
        )

        # Pattern properties group
        obj.addProperty(
            "App::PropertyEnumeration",
            "Type",
            "Pattern",
            QT_TRANSLATE_NOOP("App::Property", "Pattern method"),
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "Copies",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property", "The number of copies in Linear1D and Polar pattern"
            ),
        )
        obj.addProperty(
            "App::PropertyVectorDistance",
            "Offset",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The spacing between the array copies in linear pattern",
            ),
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "CopiesX",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property", "The number of copies in X-direction in linear pattern"
            ),
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "CopiesY",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property", "The number of copies in Y-direction in linear pattern"
            ),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "Angle",
            "Pattern",
            QT_TRANSLATE_NOOP("App::Property", "Total angle in polar pattern"),
        )
        obj.addProperty(
            "App::PropertyVector",
            "Centre",
            "Pattern",
            QT_TRANSLATE_NOOP("App::Property", "The centre of rotation in polar pattern"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "SwapDirection",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Make copies in X direction before Y in Linear 2D pattern",
            ),
        )
        obj.addProperty(
            "App::PropertyLinkSubListGlobal",
            "PointsSource",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Define the offsets and angle of rotation for repeats from selected shapes"
                "\n\nIf selected object in tree view (without sub-elements):"
                "\n- shape contain only vertexes: create repeats for each vertex"
                "\n- shape contain edges: create only one repeat (useful for imported nesting shapes)",
            ),
        )
        obj.addProperty(
            "App::PropertyLinkSubGlobal",
            "PointsOrigin",
            "Pattern",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Define the base offsets and angle of rotation from selected shape",
            ),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "PointsSorting",
            "Pattern",
            QT_TRANSLATE_NOOP("App::Property", "Sorting mode"),
        )

        # Random properties group
        obj.addProperty(
            "App::PropertyBool",
            "UseJitter",
            "Random",
            QT_TRANSLATE_NOOP("App::Property", "Use randomly offset"),
        )
        obj.addProperty(
            "App::PropertyVectorDistance",
            "JitterMagnitude",
            "Random",
            QT_TRANSLATE_NOOP("App::Property", "Maximum random offset of copies"),
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "JitterSeed",
            "Random",
            QT_TRANSLATE_NOOP("App::Property", "Seed value for jitter randomness"),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "JitterAngle",
            "Random",
            QT_TRANSLATE_NOOP("App::Property", "Max angle of rotation for jitter randomness"),
        )

        obj.Active = True
        obj.Type = ("Linear1D", "Linear2D", "Points", "Polar")
        obj.PointsSorting = ("Automatic", "Manual")
        obj.Copies = (0, 0, 99999, 1)
        obj.CopiesX = (0, 0, 99999, 1)
        obj.CopiesY = (0, 0, 99999, 1)
        obj.JitterSeed = (0, 0, 2147483647, 1)
        obj.JitterMagnitude = FreeCAD.Vector(10, 10, 0)
        obj.JitterAngle = 10

        self.setEditorModes(obj)
        obj.Proxy = self

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def setEditorModes(self, obj):
        obj.setEditorMode("ToolController", 2)  # hidden
        obj.setEditorMode("CycleTime", 1)  # read-only

        if obj.Type == "Linear1D":
            angleMode = centreMode = copiesXMode = copiesYMode = swapDirectionMode = 2
            pointsMode = 2
            copiesMode = offsetMode = 0
        elif obj.Type == "Linear2D":
            angleMode = copiesMode = centreMode = 2
            pointsMode = 2
            copiesXMode = copiesYMode = offsetMode = swapDirectionMode = 0
        elif obj.Type == "Points":
            angleMode = centreMode = copiesMode = offsetMode = 2
            copiesXMode = copiesYMode = swapDirectionMode = 2
            pointsMode = 0
        elif obj.Type == "Polar":
            copiesXMode = copiesYMode = offsetMode = swapDirectionMode = 2
            pointsMode = 2
            angleMode = copiesMode = centreMode = 0

        obj.setEditorMode("Angle", angleMode)
        obj.setEditorMode("Centre", centreMode)
        obj.setEditorMode("Copies", copiesMode)
        obj.setEditorMode("CopiesX", copiesXMode)
        obj.setEditorMode("CopiesY", copiesYMode)
        obj.setEditorMode("Offset", offsetMode)
        obj.setEditorMode("SwapDirection", swapDirectionMode)

        obj.setEditorMode("PointsOrigin", pointsMode)
        obj.setEditorMode("PointsSource", pointsMode)
        obj.setEditorMode("PointsSorting", pointsMode)

        jitterMode = 0 if obj.UseJitter else 2
        obj.setEditorMode("JitterMagnitude", jitterMode)
        obj.setEditorMode("JitterSeed", jitterMode)
        obj.setEditorMode("JitterAngle", jitterMode)

    def onChanged(self, obj, prop):
        if prop in ("Type", "UseJitter") and not obj.Document.Restoring:
            self.setEditorModes(obj)

        if prop == "Active" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def onDocumentRestored(self, obj):
        """onDocumentRestored(obj) ... Called automatically when document is restored."""
        if not obj.ViewObject.Proxy:
            Path.Op.Gui.Array.ViewProviderArray(obj.ViewObject)

        if not hasattr(obj, "JitterAngle"):
            obj.addProperty(
                "App::PropertyAngle",
                "JitterAngle",
                "Random",
                QT_TRANSLATE_NOOP("App::Property", "Max angle of rotation for jitter randomness"),
            )
        if not hasattr(obj, "UseJitter"):
            obj.addProperty(
                "App::PropertyBool",
                "UseJitter",
                "Random",
                QT_TRANSLATE_NOOP("App::Property", "Use randomly offset"),
            )
            obj.setGroupOfProperty("JitterMagnitude", "Random")
            obj.setGroupOfProperty("JitterSeed", "Random")

            obj.setGroupOfProperty("SwapDirection", "Pattern")
            obj.setGroupOfProperty("CopiesX", "Pattern")
            obj.setGroupOfProperty("CopiesY", "Pattern")
            obj.setGroupOfProperty("Copies", "Pattern")
            obj.setGroupOfProperty("Offset", "Pattern")
            obj.setGroupOfProperty("Type", "Pattern")

        if not hasattr(obj, "CycleTime"):
            obj.addProperty(
                "App::PropertyString",
                "CycleTime",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Operations cycle time estimation"),
            )
            obj.CycleTime = self.getCycleTimeEstimate(obj)

        if not hasattr(obj, "PointsSource"):
            obj.addProperty(
                "App::PropertyLinkSubListGlobal",
                "PointsSource",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "The sources of points for array"),
            )
            type = obj.Type
            obj.Type = ("Linear1D", "Linear2D", "Polar", "Points")
            obj.Type = type

        if not hasattr(obj, "PointsOrigin"):
            obj.addProperty(
                "App::PropertyLinkSubGlobal",
                "PointsOrigin",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "The origin for points"),
            )

        if not hasattr(obj, "PointsSorting"):
            obj.addProperty(
                "App::PropertyEnumeration",
                "PointsSorting",
                "Pattern",
                QT_TRANSLATE_NOOP("App::Property", "Sorting mode"),
            )
            obj.PointsSorting = ("Automatic", "Manual")

        self.setEditorModes(obj)

    def execute(self, obj):
        # backwards compatibility for PathArrays created before support for multiple bases
        if isinstance(obj.Base, list):
            base = obj.Base
        else:
            base = [obj.Base]

        # Do not generate paths and clear current Path data
        # if operation not Active or no base operations or operations not compatible
        if not obj.Active or len(base) == 0 or not self.isBaseCompatible(obj):
            obj.Path = Path.Path()
            return

        obj.ToolController = toolController(base[0])

        # Prepare random function
        if obj.UseJitter:
            random.seed(obj.JitterSeed)
            jitterMagnitude = obj.JitterMagnitude
            jitterAngle = obj.JitterAngle
        else:
            jitterMagnitude = FreeCAD.Vector()
            jitterAngle = 0

        pa = PathArray(
            obj.Base,
            obj.Type,
            obj.Copies,
            obj.Offset,
            obj.CopiesX,
            obj.CopiesY,
            obj.Angle,
            obj.Centre,
            obj.SwapDirection,
            jitterMagnitude,
            jitterAngle,
            self.getPathCenter(obj),
            obj.PointsSource,
            obj.PointsOrigin,
            obj.PointsSorting,
        )

        obj.Path = pa.getPath()
        obj.CycleTime = PathOp.getCycleTimeEstimate(obj)

    def isBaseCompatible(self, obj):
        if not obj.Base:
            return False
        tcs = []
        cms = []
        for sel in obj.Base:
            if not sel.isDerivedFrom("Path::Feature"):
                return False
            tcs.append(toolController(sel))
            cms.append(PathUtil.coolantModeForOp(sel))

        if tcs == {None} or len(set(tcs)) > 1:
            Path.Log.warning(
                translate(
                    "PathArray",
                    "Arrays of toolpaths having different tool controllers or tool controller not selected.",
                )
            )
            return False

        if set(cms) != {"None"}:
            Path.Log.warning(
                translate(
                    "PathArray",
                    "Arrays not compatible with coolant modes.",
                )
            )
            return False

        return True

    # Get center point of all base operations
    def getPathCenter(self, obj):
        path = Path.Path()
        for op in obj.Base:
            path.addCommands(op.Path.Commands)
        return path.BoundBox.Center


class PathArray:
    """class PathArray ...
    This class receives one or more base operations and repeats those operations
    at set intervals based upon array type requested and the related settings for that type."""

    def __init__(
        self,
        base,
        arrayType,
        copies,
        offsetVector,
        copiesX,
        copiesY,
        angle,
        centre,
        swapDirection,
        jitterMagnitude,
        jitterAngle,
        jitterCentre,
        pointsSource,
        pointsOrigin,
        pointsSorting,
    ):
        self.base = base
        self.arrayType = arrayType  # ['Linear1D', 'Linear2D', 'Polar']
        self.copies = copies
        self.offsetVector = offsetVector
        self.copiesX = copiesX
        self.copiesY = copiesY
        self.polarAngle = angle
        self.polarCentre = centre
        self.swapDirection = swapDirection
        self.jitterMagnitude = jitterMagnitude
        self.jitterAngle = jitterAngle
        self.jitterCentre = jitterCentre
        self.pointsSource = pointsSource
        self.pointsOrigin = pointsOrigin
        self.pointsSorting = pointsSorting

    def getPath(self):
        """getPath() ... Call this method on an instance of the class to generate and return
        path data for the requested path array."""

        commands = []

        if self.arrayType == "Linear1D":
            self.getLinear1DArray(commands)
        elif self.arrayType == "Linear2D":
            if self.swapDirection:
                self.getLinear2DXYArray(commands)
            else:
                self.getLinear2DYXArray(commands)
        elif self.arrayType == "Points":
            self.getPointsArray(commands)
        elif self.arrayType == "Polar":
            self.getPolarArray(commands)

        return Path.Path(commands)

    def calculateJitter(self, pos):
        """calculateJitter(pos) ...
        Returns the position argument with a random vector shift applied and random angle"""

        if self.jitterMagnitude != FreeCAD.Vector():
            pos.x = pos.x + random.uniform(-self.jitterMagnitude.x, self.jitterMagnitude.x)
            pos.y = pos.y + random.uniform(-self.jitterMagnitude.y, self.jitterMagnitude.y)
            pos.z = pos.z + random.uniform(-self.jitterMagnitude.z, self.jitterMagnitude.z)

        alpha = 0
        if self.jitterAngle:
            alpha = random.uniform(-self.jitterAngle, self.jitterAngle)

        return pos, alpha

    def getStartPoint(self, cmds):
        """First tool position in first base operation"""
        x = y = z = None
        for cmd in cmds:
            x = cmd.x if x is None and cmd.x is not None else x
            y = cmd.y if y is None and cmd.y is not None else y
            z = cmd.z if z is None and cmd.z is not None else z
            if x is not None and y is not None and z is not None:
                return FreeCAD.Vector(x, y, z)

        return FreeCAD.Vector()

    def getEndPoint(self, cmds):
        """Last tool position in last base operation"""
        x = y = z = None
        for cmd in reversed(cmds):
            x = cmd.x if x is None and cmd.x is not None else x
            y = cmd.y if y is None and cmd.y is not None else y
            z = cmd.z if z is None and cmd.z is not None else z
            if x is not None and y is not None and z is not None:
                return FreeCAD.Vector(x, y, z)

        return FreeCAD.Vector()

    def getLinear1DArray(self, commands):
        """Array type Linear1D"""
        for i in range(self.copies):
            pos = FreeCAD.Vector(
                self.offsetVector.x * (i + 1),
                self.offsetVector.y * (i + 1),
                self.offsetVector.z * (i + 1),
            )
            pos, alpha = self.calculateJitter(pos)

            for b in self.base:
                pl = FreeCAD.Placement()
                pl.move(pos)
                pl.rotate(self.jitterCentre, FreeCAD.Vector(0, 0, 1), alpha)
                path = PathUtils.getPathWithPlacement(b)
                path = PathUtils.applyPlacementToPath(pl, path)
                commands.extend(path.Commands)

    def getLinear2DXYArray(self, commands):
        """Array type Linear2D with initial X direction"""
        for i in range(self.copiesY + 1):
            for j in range(self.copiesX + 1):
                if (i % 2) == 0:
                    pos = FreeCAD.Vector(
                        self.offsetVector.x * j,
                        self.offsetVector.y * i,
                        self.offsetVector.z * i,
                    )
                else:
                    pos = FreeCAD.Vector(
                        self.offsetVector.x * (self.copiesX - j),
                        self.offsetVector.y * i,
                        self.offsetVector.z * i,
                    )
                pos, alpha = self.calculateJitter(pos)

                for b in self.base:
                    pl = FreeCAD.Placement()
                    # index 0,0 will be processed by the base Paths themselves
                    if i != 0 or j != 0:
                        pl.move(pos)
                        pl.rotate(self.jitterCentre, FreeCAD.Vector(0, 0, 1), alpha)
                        path = PathUtils.getPathWithPlacement(b)
                        path = PathUtils.applyPlacementToPath(pl, path)
                        commands.extend(path.Commands)

    def getLinear2DYXArray(self, commands):
        """Array type Linear2D with initial Y direction"""
        for i in range(self.copiesX + 1):
            for j in range(self.copiesY + 1):
                if (i % 2) == 0:
                    pos = FreeCAD.Vector(
                        self.offsetVector.x * i,
                        self.offsetVector.y * j,
                        self.offsetVector.z * i,
                    )
                else:
                    pos = FreeCAD.Vector(
                        self.offsetVector.x * i,
                        self.offsetVector.y * (self.copiesY - j),
                        self.offsetVector.z * i,
                    )
                pos, alpha = self.calculateJitter(pos)

                for b in self.base:
                    pl = FreeCAD.Placement()
                    # index 0,0 will be processed by the base Paths themselves
                    if i != 0 or j != 0:
                        pl.move(pos)
                        pl.rotate(self.jitterCentre, FreeCAD.Vector(0, 0, 1), alpha)
                        path = PathUtils.getPathWithPlacement(b)
                        path = PathUtils.applyPlacementToPath(pl, path)
                        commands.extend(path.Commands)

    def getPolarArray(self, commands):
        """Array type Polar"""
        for i in range(self.copies):
            ang = 360
            if self.copies > 0:
                ang = self.polarAngle / self.copies * (1 + i)

            # prepare placement for polar pattern
            pl = FreeCAD.Placement()
            pl.rotate(self.polarCentre, FreeCAD.Vector(0, 0, 1), ang)

            # add jitter to placement
            pos, alpha = self.calculateJitter(FreeCAD.Vector())
            pl.move(pos)
            pl.rotate(self.jitterCentre, FreeCAD.Vector(0, 0, 1), alpha)

            for b in self.base:
                path = PathUtils.getPathWithPlacement(b)
                path = PathUtils.applyPlacementToPath(pl, path)
                commands.extend(path.Commands)

    def getPointsArray(self, commands):
        """Array type Points"""
        originPoint = FreeCAD.Vector()
        originAngle = 0
        self.checkDistance = None

        # get offsets and angle from base shape
        if self.pointsOrigin:
            (originObj, originSubNames) = self.pointsOrigin
            if not originSubNames:
                # no sub elements selected
                if originObj.Shape.Edges:
                    # object contain edges
                    originPoint = originObj.Shape.Edges[0].Vertexes[0].Point
                    originAngle = self.getEdgeAngle(originObj.Shape.Edges[0])
                else:
                    # object contain only vertexes
                    originPoint = originObj.Shape.Vertexes[0].Point
            else:
                # sub element selected
                originSub = originObj.Shape.getElement(originSubNames[0])
                originPoint = originSub.Vertexes[0].Point
                if originSub.ShapeType == "Edge":
                    originAngle = self.getEdgeAngle(originSub)
                elif originSub.ShapeType == "Face":
                    originPoint = originSub.CenterOfGravity
                    originAngle = self.getFaceAngle(originSub, originPoint, origin=True)

        # get points from selected shapes
        points = []
        for source in self.pointsSource:
            (sourceObj, sourceSubNames) = source
            if not sourceSubNames or sourceSubNames == ("",):
                # no sub elements selected
                if sourceObj.Shape.Edges:
                    # shape contain edges
                    # use whole shape as one repeat
                    point = sourceObj.Shape.Edges[0].Vertexes[0].Point
                    sourceAngle = self.getEdgeAngle(sourceObj.Shape.Edges[0])
                    points.append({"point": point, "angle": sourceAngle})
                else:
                    # object contain only vertexes
                    # use each point as repeat
                    points.extend(
                        [{"point": v.Point, "angle": 0} for v in sourceObj.Shape.Vertexes]
                    )
            else:
                # sub elements selected
                for sourceSubName in sourceSubNames:
                    sourceSub = sourceObj.Shape.getElement(sourceSubName)
                    sourcePoint = sourceSub.Vertexes[0].Point
                    sourceAngle = 0
                    if sourceSub.ShapeType == "Edge":
                        sourceAngle = self.getEdgeAngle(sourceSub)
                    elif sourceSub.ShapeType == "Face":
                        sourcePoint = sourceSub.CenterOfGravity
                        sourceAngle = self.getFaceAngle(sourceSub, sourcePoint)
                    points.append({"point": sourcePoint, "angle": sourceAngle})

        # Apply origin offset to each point
        if originPoint != FreeCAD.Vector() or originAngle:
            for pos in points:
                pos["point"] -= originPoint
                pos["angle"] -= originAngle

        # remove points which similar with origin
        points = [p for p in points if p["point"] != FreeCAD.Vector() or p["angle"]]

        # get sorted positions for array
        if self.pointsSorting == "Automatic":
            basePathStartPoint = self.getStartPoint(self.base[0].Path.Commands)
            basePathEndPoint = self.getEndPoint(self.base[-1].Path.Commands)
            dirStart = basePathStartPoint - originPoint
            dirEnd = basePathEndPoint - originPoint
            routes = []
            for i, pos in enumerate(points):
                origin = originPoint + pos["point"]
                dirStartOffset = DraftVecUtils.rotate(
                    dirStart, math.radians(pos["angle"]), FreeCAD.Vector(0, 0, 1)
                )
                dirEndOffset = DraftVecUtils.rotate(
                    dirEnd, math.radians(pos["angle"]), FreeCAD.Vector(0, 0, 1)
                )
                routes.append(
                    {
                        "startX": origin.x + dirStartOffset.x,
                        "startY": origin.y + dirStartOffset.y,
                        "endX": origin.x + dirEndOffset.x,
                        "endY": origin.y + dirEndOffset.y,
                        "point": pos["point"],
                        "a": pos["angle"],
                    }
                )
            routes = PathUtils.sort_tunnels_tsp(routes, routeStartPoint=basePathEndPoint)
            if routes:
                points = [{"point": pos["point"], "angle": pos["a"]} for pos in routes]

        for pos in points:
            # apply jitter
            point, alpha = self.calculateJitter(pos["point"])

            for b in self.base:
                pl = FreeCAD.Placement()
                pl.move(point)
                pl.rotate(self.jitterCentre, FreeCAD.Vector(0, 0, 1), alpha)
                pl.rotate(originPoint, FreeCAD.Vector(0, 0, 1), pos["angle"])
                path = PathUtils.getPathWithPlacement(b)
                path = PathUtils.applyPlacementToPath(pl, path)
                commands.extend(path.Commands)

    def getPointsAngle(self, p1, p2=FreeCAD.Vector()):
        """return angle between vector (direction) and Y-axis"""
        direction = p1 - p2
        if Path.Geom.pointsCoincide(direction, FreeCAD.Vector()):
            return 0
        angle = math.degrees(direction.getAngle(FreeCAD.Vector(0, 1, 0)))
        if direction.x > 0:
            angle = -angle

        return angle

    def getEdgeAngle(self, edge):
        """return angle between edge direction and Y-axis
        Edge direction defines from end points"""
        p1 = edge.Vertexes[-1].Point
        p2 = edge.Vertexes[0].Point

        return self.getPointsAngle(p1, p2)

    def getFaceAngle(self, face, centerPoint, origin=False):
        """return angle between face direction and Y-axis
        Face direction defines from center point and farthest point from UV nodes"""
        maxDist = 0
        farthestPoints = []
        # use UV nodes to find farthest point
        face.tessellate(0.1)
        candidates = [face.valueAt(uv[0], uv[1]) for uv in face.getUVNodes()]
        for p in candidates:
            dist = centerPoint.distanceToPoint(p)
            if Path.Geom.isRoughly(dist, maxDist):
                farthestPoints.append(p)
            elif dist > maxDist:
                farthestPoints = [p]
                maxDist = dist

        # check extra distance while processing source face
        if not origin and len(farthestPoints) > 1 and self.checkDistance:
            for i in range(len(farthestPoints) - 1):
                d = farthestPoints[i].distanceToPoint(farthestPoints[i + 1])
                if Path.Geom.isRoughly(d, self.checkDistance):
                    return self.getPointsAngle(centerPoint, farthestPoints[i])

        # defined several farthest points while processing origin face
        if origin and len(farthestPoints) > 1:
            # get distance between first and second farthest points
            self.checkDistance = farthestPoints[0].distanceToPoint(farthestPoints[1])

        return self.getPointsAngle(centerPoint, farthestPoints[0])


class ViewProviderArray:
    def __init__(self, vobj):
        self.attach(vobj)
        vobj.Proxy = self

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, vobj, prop):
        return None

    def claimChildren(self):
        return []

    def onDelete(self, vobj, args):
        return True

    def getIcon(self):
        if self.obj.Active:
            return ":/icons/CAM_Array.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


class CommandPathArray:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Array",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Array", "Array"),
            "ToolTip": QT_TRANSLATE_NOOP("CAM_Array", "Creates an array from selected toolpaths"),
        }

    def IsActive(self):
        selection = FreeCADGui.Selection.getSelection()
        if not selection:
            return False
        tcs = []
        for sel in selection:
            if not sel.isDerivedFrom("Path::Feature"):
                return False
            tc = toolController(sel)
            if tc:
                # Active only for operations with identical tool controller
                tcs.append(tc)
                if len(set(tcs)) != 1:
                    return False
            else:
                return False
            if PathUtil.coolantModeForOp(sel) != "None":
                # Active only for operations without cooling
                return False
        return True

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()

        for sel in selection:
            if not (sel.isDerivedFrom("Path::Feature")):
                FreeCAD.Console.PrintError(
                    translate("CAM_Array", "Arrays can be created only from toolpath operations.")
                    + "\n"
                )
                return

        # if everything is ok, execute and register the transaction in the
        # undo/redo stack
        FreeCAD.ActiveDocument.openTransaction("Create Array")
        FreeCADGui.addModule("Path.Op.Gui.Array")
        FreeCADGui.addModule("PathScripts.PathUtils")

        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Array")'
        )

        FreeCADGui.doCommand("Path.Op.Gui.Array.ObjectArray(obj)")

        baseString = "[%s]" % ",".join(
            ["FreeCAD.ActiveDocument.%s" % sel.Name for sel in selection]
        )
        FreeCADGui.doCommand("obj.Base = %s" % baseString)

        FreeCADGui.doCommand("Path.Op.Gui.Array.ViewProviderArray(obj.ViewObject)")
        FreeCADGui.doCommand("PathScripts.PathUtils.addToJob(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Array", CommandPathArray())
