# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

from Path.Dressup.Gui.TagPreferences import HoldingTagPreferences
from PathScripts.PathUtils import waiting_effects
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import Path.Dressup.Utils as PathDressup
import PathScripts.PathUtils as PathUtils
import copy
import math


# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


def debugEdge(edge, prefix, force=False):
    if force or Path.Log.getLevel(Path.Log.thisModule()) == Path.Log.Level.DEBUG:
        pf = edge.valueAt(edge.FirstParameter)
        pl = edge.valueAt(edge.LastParameter)
        if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
            print(
                "%s %s((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))"
                % (prefix, type(edge.Curve), pf.x, pf.y, pf.z, pl.x, pl.y, pl.z)
            )
        else:
            pm = edge.valueAt((edge.FirstParameter + edge.LastParameter) / 2)
            print(
                "%s %s((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))"
                % (
                    prefix,
                    type(edge.Curve),
                    pf.x,
                    pf.y,
                    pf.z,
                    pm.x,
                    pm.y,
                    pm.z,
                    pl.x,
                    pl.y,
                    pl.z,
                )
            )


def debugMarker(vector, label, color=None, radius=0.5):
    if Path.Log.getLevel(Path.Log.thisModule()) == Path.Log.Level.DEBUG:
        obj = FreeCAD.ActiveDocument.addObject("Part::Sphere", label)
        obj.Label = label
        obj.Radius = radius
        obj.Placement = FreeCAD.Placement(
            vector, FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
        )
        if color:
            obj.ViewObject.ShapeColor = color


def debugCylinder(vector, r, height, label, color=None):
    if Path.Log.getLevel(Path.Log.thisModule()) == Path.Log.Level.DEBUG:
        obj = FreeCAD.ActiveDocument.addObject("Part::Cylinder", label)
        obj.Label = label
        obj.Radius = r
        obj.Height = height
        obj.Placement = FreeCAD.Placement(
            vector, FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
        )
        obj.ViewObject.Transparency = 90
        if color:
            obj.ViewObject.ShapeColor = color


def debugCone(vector, r1, r2, height, label, color=None):
    if Path.Log.getLevel(Path.Log.thisModule()) == Path.Log.Level.DEBUG:
        obj = FreeCAD.ActiveDocument.addObject("Part::Cone", label)
        obj.Label = label
        obj.Radius1 = r1
        obj.Radius2 = r2
        obj.Height = height
        obj.Placement = FreeCAD.Placement(
            vector, FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
        )
        obj.ViewObject.Transparency = 90
        if color:
            obj.ViewObject.ShapeColor = color


class Tag:
    def __init__(self, nr, x, y, width, height, angle, radius, enabled=True):
        Path.Log.track(
            "%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %d"
            % (x, y, width, height, angle, radius, enabled)
        )
        self.nr = nr
        self.x = x
        self.y = y
        self.width = math.fabs(width)
        self.height = math.fabs(height)
        self.actualHeight = self.height
        self.angle = math.fabs(angle)
        self.radius = getattr(
            radius, "Value", FreeCAD.Units.Quantity(radius, FreeCAD.Units.Length).Value
        )
        self.enabled = enabled
        self.isSquare = False

        # initialized later
        self.toolRadius = None
        self.realRadius = None
        self.r1 = None
        self.r2 = None
        self.solid = None
        self.z = None

    def fullWidth(self):
        return 2 * self.toolRadius + self.width

    def originAt(self, z):
        return FreeCAD.Vector(self.x, self.y, z)

    def bottom(self):
        return self.z

    def top(self):
        return self.z + self.actualHeight

    def createSolidsAt(self, z, R):
        self.z = z
        self.toolRadius = R
        r1 = self.fullWidth() / 2
        self.r1 = r1
        self.r2 = r1
        height = self.height * 1.01
        radius = 0
        if Path.Geom.isRoughly(90, self.angle) and height > 0:
            # cylinder
            self.isSquare = True
            self.solid = Part.makeCylinder(r1, height)
            radius = min(min(self.radius, r1), self.height)
            Path.Log.debug("Part.makeCylinder(%f, %f)" % (r1, height))
        elif self.angle > 0.0 and height > 0.0:
            # cone
            rad = math.radians(self.angle)
            tangens = math.tan(rad)
            dr = height / tangens
            if dr < r1:
                # with top
                r2 = r1 - dr
                s = height / math.sin(rad)
                radius = min(r2, s) * math.tan((math.pi - rad) / 2) * 0.95
            else:
                # triangular
                r2 = 0
                height = r1 * tangens * 1.01
                self.actualHeight = height
            self.r2 = r2
            Path.Log.debug("Part.makeCone(%f, %f, %f)" % (r1, r2, height))
            self.solid = Part.makeCone(r1, r2, height)
        else:
            # degenerated case - no tag
            Path.Log.debug("Part.makeSphere(%f / 10000)" % (r1))
            self.solid = Part.makeSphere(r1 / 10000)
        if not Path.Geom.isRoughly(
            0, R
        ):  # testing is easier if the solid is not rotated
            angle = -Path.Geom.getAngle(self.originAt(0)) * 180 / math.pi
            Path.Log.debug("solid.rotate(%f)" % angle)
            self.solid.rotate(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1), angle)
        orig = self.originAt(z - 0.01 * self.actualHeight)
        Path.Log.debug("solid.translate(%s)" % orig)
        self.solid.translate(orig)
        radius = min(self.radius, radius)
        self.realRadius = radius
        if not Path.Geom.isRoughly(0, radius):
            Path.Log.debug("makeFillet(%.4f)" % radius)
            self.solid = self.solid.makeFillet(radius, [self.solid.Edges[0]])

    def filterIntersections(self, pts, face):
        if (
            type(face.Surface) == Part.Cone
            or type(face.Surface) == Part.Cylinder
            or type(face.Surface) == Part.Toroid
        ):
            Path.Log.track("it's a cone/cylinder, checking z")
            return list(
                [pt for pt in pts if pt.z >= self.bottom() and pt.z <= self.top()]
            )
        if type(face.Surface) == Part.Plane:
            Path.Log.track("it's a plane, checking R")
            c = face.Edges[0].Curve
            if type(c) == Part.Circle:
                return list(
                    [
                        pt
                        for pt in pts
                        if (pt - c.Center).Length <= c.Radius
                        or Path.Geom.isRoughly((pt - c.Center).Length, c.Radius)
                    ]
                )
        Path.Log.error("==== we got a %s" % face.Surface)

    def isPointOnEdge(self, pt, edge):
        param = edge.Curve.parameter(pt)
        if edge.FirstParameter <= param <= edge.LastParameter:
            return True
        if edge.LastParameter <= param <= edge.FirstParameter:
            return True
        if Path.Geom.isRoughly(edge.FirstParameter, param) or Path.Geom.isRoughly(
            edge.LastParameter, param
        ):
            return True
        # print("-------- X %.2f <= %.2f <=%.2f   (%.2f, %.2f, %.2f)   %.2f:%.2f" % (edge.FirstParameter, param, edge.LastParameter, pt.x, pt.y, pt.z, edge.Curve.parameter(edge.valueAt(edge.FirstParameter)), edge.Curve.parameter(edge.valueAt(edge.LastParameter))))
        # p1 = edge.Vertexes[0]
        # f1 = edge.Curve.parameter(FreeCAD.Vector(p1.X, p1.Y, p1.Z))
        # p2 = edge.Vertexes[1]
        # f2 = edge.Curve.parameter(FreeCAD.Vector(p2.X, p2.Y, p2.Z))
        return False

    def nextIntersectionClosestTo(self, edge, solid, refPt):
        # debugEdge(edge, 'intersects_')

        vertexes = edge.common(solid).Vertexes
        if vertexes:
            pt = sorted(vertexes, key=lambda v: (v.Point - refPt).Length)[0].Point
            debugEdge(
                edge,
                "intersects (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)"
                % (refPt.x, refPt.y, refPt.z, pt.x, pt.y, pt.z),
            )
            return pt
        return None

    def intersects(self, edge, param):
        def isDefinitelySmaller(z, zRef):
            # Eliminate false positives of edges that just brush along the top of the tag
            return z < zRef and not Path.Geom.isRoughly(z, zRef, 0.01)

        if self.enabled:
            zFirst = edge.valueAt(edge.FirstParameter).z
            zLast = edge.valueAt(edge.LastParameter).z
            zMax = self.top()
            if isDefinitelySmaller(zFirst, zMax) or isDefinitelySmaller(zLast, zMax):
                return self.nextIntersectionClosestTo(
                    edge, self.solid, edge.valueAt(param)
                )
        return None

    def bbEdges(self):
        edges = []
        for i in range(12):
            p1, p2 = self.solid.BoundBox.getEdge(i)
            edges.append(Part.Edge(Part.LineSegment(p1, p2)))
        return edges

    def bbShow(self):
        for e in self.bbEdges():
            Part.show(e)


class MapWireToTag:
    def __init__(self, edge, tag, i, segm, maxZ, hSpeed, vSpeed):
        debugEdge(edge, "MapWireToTag(%.2f, %.2f, %.2f)" % (i.x, i.y, i.z))
        self.tag = tag
        self.segm = segm
        self.maxZ = maxZ
        self.hSpeed = hSpeed
        self.vSpeed = vSpeed
        if Path.Geom.pointsCoincide(edge.valueAt(edge.FirstParameter), i):
            tail = edge
            self.commands = []
            debugEdge(tail, ".........=")
        elif Path.Geom.pointsCoincide(edge.valueAt(edge.LastParameter), i):
            debugEdge(edge, "++++++++ .")
            self.commands = Path.Geom.cmdsForEdge(
                edge, segm=segm, hSpeed=self.hSpeed, vSpeed=self.vSpeed
            )
            tail = None
        else:
            e, tail = Path.Geom.splitEdgeAt(edge, i)
            debugEdge(e, "++++++++ .")
            self.commands = Path.Geom.cmdsForEdge(
                e, segm=segm, hSpeed=self.hSpeed, vSpeed=self.vSpeed
            )
            debugEdge(tail, ".........-")
            self.initialEdge = edge
        self.tail = tail
        self.edges = []
        self.entry = i
        if tail:
            Path.Log.debug(
                "MapWireToTag(%s - %s)" % (i, tail.valueAt(tail.FirstParameter))
            )
        else:
            Path.Log.debug("MapWireToTag(%s - )" % i)
        self.complete = False
        self.haveProblem = False

        # initialized later
        self.edgePoints = None
        self.edgesCleanup = None
        self.edgesOrder = None
        self.entryEdges = None
        self.exit = None
        self.exitEdges = None
        self.finalEdge = None
        self.offendingEdge = None
        self.realEntry = None
        self.realExit = None

    def addEdge(self, edge):
        debugEdge(edge, "..........")
        self.edges.append(edge)

    def needToFlipEdge(self, edge, p):
        if Path.Geom.pointsCoincide(edge.valueAt(edge.LastParameter), p):
            return True, edge.valueAt(edge.FirstParameter)
        return False, edge.valueAt(edge.LastParameter)

    def isEntryOrExitStrut(self, e):
        p1 = e.valueAt(e.FirstParameter)
        p2 = e.valueAt(e.LastParameter)
        if Path.Geom.pointsCoincide(p1, self.entry) and p2.z >= self.entry.z:
            return 1
        if Path.Geom.pointsCoincide(p2, self.entry) and p1.z >= self.entry.z:
            return 1
        if Path.Geom.pointsCoincide(p1, self.exit) and p2.z >= self.exit.z:
            return 2
        if Path.Geom.pointsCoincide(p2, self.exit) and p1.z >= self.exit.z:
            return 2
        return 0

    def cleanupEdges(self, edges):
        # want to remove all edges from the wire itself, and all internal struts
        Path.Log.track("+cleanupEdges")
        Path.Log.debug(" edges:")
        if not edges:
            return edges
        for e in edges:
            debugEdge(e, "   ")
        Path.Log.debug(":")
        self.edgesCleanup = [copy.copy(edges)]

        # remove any edge that has a point inside the tag solid
        # and collect all edges that are connected to the entry and/or exit
        self.entryEdges = []
        self.exitEdges = []
        self.edgePoints = []
        for e in copy.copy(edges):
            p1 = e.valueAt(e.FirstParameter)
            p2 = e.valueAt(e.LastParameter)
            self.edgePoints.append(p1)
            self.edgePoints.append(p2)
            if self.tag.solid.isInside(
                p1, Path.Geom.Tolerance, False
            ) or self.tag.solid.isInside(p2, Path.Geom.Tolerance, False):
                edges.remove(e)
                debugEdge(e, "......... X0", False)
            else:
                if Path.Geom.pointsCoincide(p1, self.entry) or Path.Geom.pointsCoincide(
                    p2, self.entry
                ):
                    self.entryEdges.append(e)
                if Path.Geom.pointsCoincide(p1, self.exit) or Path.Geom.pointsCoincide(
                    p2, self.exit
                ):
                    self.exitEdges.append(e)
        self.edgesCleanup.append(copy.copy(edges))

        # if there are no edges connected to entry/exit, it means the plunge in/out is vertical
        # we need to add in the missing segment and collect the new entry/exit edges.
        if not self.entryEdges:
            Path.Log.debug("fill entryEdges ...")
            self.realEntry = sorted(
                self.edgePoints, key=lambda p: (p - self.entry).Length
            )[0]
            self.entryEdges = list(
                [e for e in edges if Path.Geom.edgeConnectsTo(e, self.realEntry)]
            )
            edges.append(Part.Edge(Part.LineSegment(self.entry, self.realEntry)))
        else:
            self.realEntry = None
        if not self.exitEdges:
            Path.Log.debug("fill exitEdges ...")
            self.realExit = sorted(
                self.edgePoints, key=lambda p: (p - self.exit).Length
            )[0]
            self.exitEdges = list(
                [e for e in edges if Path.Geom.edgeConnectsTo(e, self.realExit)]
            )
            edges.append(Part.Edge(Part.LineSegment(self.realExit, self.exit)))
        else:
            self.realExit = None
        self.edgesCleanup.append(copy.copy(edges))

        # if there are 2 edges attached to entry/exit, throw away the one that is "lower"
        if len(self.entryEdges) > 1:
            debugEdge(self.entryEdges[0], " entry[0]", False)
            debugEdge(self.entryEdges[1], " entry[1]", False)
            if self.entryEdges[0].BoundBox.ZMax < self.entryEdges[1].BoundBox.ZMax:
                edges.remove(self.entryEdges[0])
                debugEdge(e, "......... X1", False)
            else:
                edges.remove(self.entryEdges[1])
                debugEdge(e, "......... X2", False)
        if len(self.exitEdges) > 1:
            debugEdge(self.exitEdges[0], " exit[0]", False)
            debugEdge(self.exitEdges[1], " exit[1]", False)
            if self.exitEdges[0].BoundBox.ZMax < self.exitEdges[1].BoundBox.ZMax:
                if self.exitEdges[0] in edges:
                    edges.remove(self.exitEdges[0])
                debugEdge(e, "......... X3", False)
            else:
                if self.exitEdges[1] in edges:
                    edges.remove(self.exitEdges[1])
                debugEdge(e, "......... X4", False)

        self.edgesCleanup.append(copy.copy(edges))
        return edges

    def orderAndFlipEdges(self, inputEdges):
        Path.Log.track(
            "entry(%.2f, %.2f, %.2f), exit(%.2f, %.2f, %.2f)"
            % (
                self.entry.x,
                self.entry.y,
                self.entry.z,
                self.exit.x,
                self.exit.y,
                self.exit.z,
            )
        )
        self.edgesOrder = []
        outputEdges = []
        p0 = self.entry
        lastP = p0
        edges = copy.copy(inputEdges)
        while edges:
            # print("(%.2f, %.2f, %.2f) %d %d" % (p0.x, p0.y, p0.z))
            for e in copy.copy(edges):
                p1 = e.valueAt(e.FirstParameter)
                p2 = e.valueAt(e.LastParameter)
                if Path.Geom.pointsCoincide(p1, p0):
                    outputEdges.append((e, False))
                    edges.remove(e)
                    lastP = None
                    p0 = p2
                    debugEdge(e, ">>>>> no flip")
                    break
                elif Path.Geom.pointsCoincide(p2, p0):
                    flipped = Path.Geom.flipEdge(e)
                    if not flipped is None:
                        outputEdges.append((flipped, True))
                    else:
                        p0 = None
                        cnt = 0
                        for p in reversed(e.discretize(Deflection=0.01)):
                            if not p0 is None:
                                outputEdges.append(
                                    (Part.Edge(Part.LineSegment(p0, p)), True)
                                )
                                cnt = cnt + 1
                            p0 = p
                        Path.Log.info("replaced edge with %d straight segments" % cnt)
                    edges.remove(e)
                    lastP = None
                    p0 = p1
                    debugEdge(e, ">>>>> flip")
                    break
                else:
                    debugEdge(e, "<<<<< (%.2f, %.2f, %.2f)" % (p0.x, p0.y, p0.z))

            if lastP == p0:
                self.edgesOrder.append(outputEdges)
                self.edgesOrder.append(edges)
                Path.Log.debug("input edges:")
                for e in inputEdges:
                    debugEdge(e, "  ", False)
                Path.Log.debug("ordered edges:")
                for e, flip in outputEdges:
                    debugEdge(e, "  %c " % ("<" if flip else ">"), False)
                Path.Log.debug("remaining edges:")
                for e in edges:
                    debugEdge(e, "    ", False)
                raise ValueError("No connection to %s" % (p0))
            elif lastP:
                Path.Log.debug(
                    "xxxxxx (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)"
                    % (p0.x, p0.y, p0.z, lastP.x, lastP.y, lastP.z)
                )
            else:
                Path.Log.debug("xxxxxx (%.2f, %.2f, %.2f) -" % (p0.x, p0.y, p0.z))
            lastP = p0
        Path.Log.track("-")
        return outputEdges

    def isStrut(self, edge):
        p1 = Path.Geom.xy(edge.valueAt(edge.FirstParameter))
        p2 = Path.Geom.xy(edge.valueAt(edge.LastParameter))
        return Path.Geom.pointsCoincide(p1, p2)

    def shell(self):
        if len(self.edges) > 1:
            wire = Part.Wire(self.initialEdge)
        else:
            edge = self.edges[0]
            if Path.Geom.pointsCoincide(
                edge.valueAt(edge.FirstParameter),
                self.finalEdge.valueAt(self.finalEdge.FirstParameter),
            ):
                wire = Part.Wire(self.finalEdge)
            elif hasattr(self, "initialEdge") and Path.Geom.pointsCoincide(
                edge.valueAt(edge.FirstParameter),
                self.initialEdge.valueAt(self.initialEdge.FirstParameter),
            ):
                wire = Part.Wire(self.initialEdge)
            else:
                wire = Part.Wire(edge)

        for edge in self.edges[1:]:
            if Path.Geom.pointsCoincide(
                edge.valueAt(edge.FirstParameter),
                self.finalEdge.valueAt(self.finalEdge.FirstParameter),
            ):
                wire.add(self.finalEdge)
            else:
                wire.add(edge)

        shell = wire.extrude(FreeCAD.Vector(0, 0, self.tag.height + 1))
        nullFaces = list([f for f in shell.Faces if Path.Geom.isRoughly(f.Area, 0)])
        if nullFaces:
            return shell.removeShape(nullFaces)
        return shell

    def commandsForEdges(self):
        if self.edges:
            try:
                shape = self.shell().common(self.tag.solid)
                commands = []
                rapid = None
                for e, flip in self.orderAndFlipEdges(self.cleanupEdges(shape.Edges)):
                    debugEdge(e, "++++++++ %s" % ("<" if flip else ">"), False)
                    p1 = e.valueAt(e.FirstParameter)
                    p2 = e.valueAt(e.LastParameter)
                    if (
                        self.tag.isSquare
                        and (Path.Geom.isRoughly(p1.z, self.maxZ) or p1.z > self.maxZ)
                        and (Path.Geom.isRoughly(p2.z, self.maxZ) or p2.z > self.maxZ)
                    ):
                        rapid = p1 if flip else p2
                    else:
                        if rapid:
                            commands.append(
                                Path.Command(
                                    "G0", {"X": rapid.x, "Y": rapid.y, "Z": rapid.z}
                                )
                            )
                            rapid = None
                        commands.extend(
                            Path.Geom.cmdsForEdge(
                                e,
                                False,
                                False,
                                self.segm,
                                hSpeed=self.hSpeed,
                                vSpeed=self.vSpeed,
                            )
                        )
                if rapid:
                    commands.append(
                        Path.Command("G0", {"X": rapid.x, "Y": rapid.y, "Z": rapid.z})
                    )
                    # rapid = None  # commented out per LGTM suggestion
                return commands
            except Exception as e:
                Path.Log.error(
                    "Exception during processing tag @(%.2f, %.2f) (%s) - disabling the tag"
                    % (self.tag.x, self.tag.y, e.args[0])
                )
                self.tag.enabled = False
                commands = []
                for e in self.edges:
                    commands.extend(
                        Path.Geom.cmdsForEdge(e, hSpeed=self.hSpeed, vSpeed=self.vSpeed)
                    )
                return commands
        return []

    def add(self, edge):
        self.tail = None
        self.finalEdge = edge
        if self.tag.solid.isInside(
            edge.valueAt(edge.LastParameter), Path.Geom.Tolerance, True
        ):
            Path.Log.track("solid.isInside")
            self.addEdge(edge)
        else:
            i = self.tag.intersects(edge, edge.LastParameter)
            if not i:
                self.offendingEdge = edge
                debugEdge(edge, "offending Edge:", False)
                o = self.tag.originAt(self.tag.z)
                Path.Log.debug("originAt: (%.2f, %.2f, %.2f)" % (o.x, o.y, o.z))
                i = edge.valueAt(edge.FirstParameter)
            if Path.Geom.pointsCoincide(i, edge.valueAt(edge.FirstParameter)):
                Path.Log.track("tail")
                self.tail = edge
            else:
                Path.Log.track("split")
                e, tail = Path.Geom.splitEdgeAt(edge, i)
                self.addEdge(e)
                self.tail = tail
            self.exit = i
            self.complete = True
            self.commands.extend(self.commandsForEdges())

    def mappingComplete(self):
        return self.complete


class _RapidEdges:
    def __init__(self, rapid):
        self.rapid = rapid

    def isRapid(self, edge):
        if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
            v0 = edge.Vertexes[0]
            v1 = edge.Vertexes[1]
            for r in self.rapid:
                r0 = r.Vertexes[0]
                r1 = r.Vertexes[1]
                if (
                    Path.Geom.isRoughly(r0.X, v0.X)
                    and Path.Geom.isRoughly(r0.Y, v0.Y)
                    and Path.Geom.isRoughly(r0.Z, v0.Z)
                    and Path.Geom.isRoughly(r1.X, v1.X)
                    and Path.Geom.isRoughly(r1.Y, v1.Y)
                    and Path.Geom.isRoughly(r1.Z, v1.Z)
                ):
                    return True
        return False


class PathData:
    def __init__(self, obj):
        Path.Log.track(obj.Base.Name)
        self.obj = obj
        path = PathUtils.getPathWithPlacement(obj.Base)
        self.wire, rapid = Path.Geom.wireForPath(path)
        self.rapid = _RapidEdges(rapid)
        if self.wire:
            self.edges = self.wire.Edges
        else:
            self.edges = []
        self.baseWire = self.findBottomWire(self.edges)

    def findBottomWire(self, edges):
        (minZ, maxZ) = self.findZLimits(edges)
        self.minZ = minZ
        self.maxZ = maxZ
        bottom = [
            e
            for e in edges
            if Path.Geom.isRoughly(e.Vertexes[0].Point.z, minZ)
            and Path.Geom.isRoughly(e.Vertexes[1].Point.z, minZ)
        ]
        self.bottomEdges = bottom
        try:
            wire = Part.Wire(bottom)
            if wire.isClosed():
                return wire
        except Exception:
            return None

    def supportsTagGeneration(self):
        return self.baseWire is not None

    def findZLimits(self, edges):
        # not considering arcs and spheres in Z direction, find the highest and lowest Z values
        minZ = 99999999999
        maxZ = -99999999999
        for e in edges:
            if self.rapid.isRapid(e):
                continue
            for v in e.Vertexes:
                if v.Point.z < minZ:
                    minZ = v.Point.z
                if v.Point.z > maxZ:
                    maxZ = v.Point.z
        return (minZ, maxZ)

    def shortestAndLongestPathEdge(self):
        edges = sorted(self.bottomEdges, key=lambda e: e.Length)
        return (edges[0], edges[-1])

    def generateTags(
        self, obj, count, width=None, height=None, angle=None, radius=None, spacing=None
    ):
        Path.Log.track(count, width, height, angle, spacing)
        # for e in self.baseWire.Edges:
        #    debugMarker(e.Vertexes[0].Point, 'base', (0.0, 1.0, 1.0), 0.2)

        if spacing:
            tagDistance = spacing
        else:
            tagDistance = self.baseWire.Length / (count if count else 4)

        W = width if width else self.defaultTagWidth()
        H = height if height else self.defaultTagHeight()
        A = angle if angle else self.defaultTagAngle()
        R = radius if radius else self.defaultTagRadius()

        # start assigning tags on the longest segment
        (shortestEdge, longestEdge) = self.shortestAndLongestPathEdge()
        startIndex = 0
        for i in range(0, len(self.baseWire.Edges)):
            edge = self.baseWire.Edges[i]
            Path.Log.debug("  %d: %.2f" % (i, edge.Length))
            if Path.Geom.isRoughly(edge.Length, longestEdge.Length):
                startIndex = i
                break

        startEdge = self.baseWire.Edges[startIndex]
        startCount = int(startEdge.Length / tagDistance)
        if (longestEdge.Length - shortestEdge.Length) > shortestEdge.Length:
            startCount = int(startEdge.Length / tagDistance) + 1

        lastTagLength = (startEdge.Length + (startCount - 1) * tagDistance) / 2
        currentLength = startEdge.Length

        minLength = min(2.0 * W, longestEdge.Length)

        Path.Log.debug(
            "length=%.2f shortestEdge=%.2f(%.2f) longestEdge=%.2f(%.2f) minLength=%.2f"
            % (
                self.baseWire.Length,
                shortestEdge.Length,
                shortestEdge.Length / self.baseWire.Length,
                longestEdge.Length,
                longestEdge.Length / self.baseWire.Length,
                minLength,
            )
        )
        Path.Log.debug(
            "   start: index=%-2d count=%d (length=%.2f, distance=%.2f)"
            % (startIndex, startCount, startEdge.Length, tagDistance)
        )
        Path.Log.debug("               -> lastTagLength=%.2f)" % lastTagLength)
        Path.Log.debug("               -> currentLength=%.2f)" % currentLength)

        edgeDict = {startIndex: startCount}

        for i in range(startIndex + 1, len(self.baseWire.Edges)):
            edge = self.baseWire.Edges[i]
            (currentLength, lastTagLength) = self.processEdge(
                i, edge, currentLength, lastTagLength, tagDistance, minLength, edgeDict
            )
        for i in range(0, startIndex):
            edge = self.baseWire.Edges[i]
            (currentLength, lastTagLength) = self.processEdge(
                i, edge, currentLength, lastTagLength, tagDistance, minLength, edgeDict
            )

        tags = []

        for (i, count) in edgeDict.items():
            edge = self.baseWire.Edges[i]
            Path.Log.debug(" %d: %d" % (i, count))
            # debugMarker(edge.Vertexes[0].Point, 'base', (1.0, 0.0, 0.0), 0.2)
            # debugMarker(edge.Vertexes[1].Point, 'base', (0.0, 1.0, 0.0), 0.2)
            if 0 != count:
                distance = (edge.LastParameter - edge.FirstParameter) / count
                for j in range(0, count):
                    tag = edge.Curve.value((j + 0.5) * distance)
                    tags.append(Tag(j, tag.x, tag.y, W, H, A, R, True))

        return tags

    def copyTags(self, obj, fromObj, width, height, angle, radius, production=True):
        print(
            "copyTags(%s, %s, %.2f, %.2f, %.2f, %.2f"
            % (obj.Label, fromObj.Label, width, height, angle, radius)
        )
        W = width if width else self.defaultTagWidth()
        H = height if height else self.defaultTagHeight()
        A = angle if angle else self.defaultTagAngle()
        R = radius if radius else self.defaultTagRadius()

        tags = []
        j = 0
        for i, pos in enumerate(fromObj.Positions):
            print("tag[%d]" % i)
            if not i in fromObj.Disabled:
                dist = self.baseWire.distToShape(
                    Part.Vertex(FreeCAD.Vector(pos.x, pos.y, self.minZ))
                )
                if production or dist[0] < W:
                    # russ4262:: `production` variable was a `True` declaration, forcing True branch to be processed always
                    #   The application of the `production` argument/variable is to appease LGTM
                    print(
                        "tag[%d/%d]: (%.2f, %.2f, %.2f)"
                        % (i, j, pos.x, pos.y, self.minZ)
                    )
                    at = dist[1][0][0]
                    tags.append(Tag(j, at.x, at.y, W, H, A, R, True))
                    j += 1
                else:
                    Path.Log.warning(
                        "Tag[%d] (%.2f, %.2f, %.2f) is too far away to copy: %.2f (%.2f)"
                        % (i, pos.x, pos.y, self.minZ, dist[0], W)
                    )
            else:
                Path.Log.info("tag[%d]: not enabled, skipping" % i)
        print("copied %d tags" % len(tags))
        return tags

    def processEdge(
        self,
        index,
        edge,
        currentLength,
        lastTagLength,
        tagDistance,
        minLength,
        edgeDict,
    ):
        tagCount = 0
        currentLength += edge.Length
        if edge.Length >= minLength:
            while lastTagLength + tagDistance < currentLength:
                tagCount += 1
                lastTagLength += tagDistance
            if tagCount > 0:
                Path.Log.debug("      index=%d -> count=%d" % (index, tagCount))
                edgeDict[index] = tagCount
        else:
            Path.Log.debug("      skipping=%-2d (%.2f)" % (index, edge.Length))

        return (currentLength, lastTagLength)

    def defaultTagHeight(self):
        op = PathDressup.baseOp(self.obj.Base)
        if hasattr(op, "StartDepth") and hasattr(op, "FinalDepth"):
            pathHeight = (op.StartDepth - op.FinalDepth).Value
        else:
            pathHeight = self.maxZ - self.minZ
        height = HoldingTagPreferences.defaultHeight(pathHeight / 2)
        if height > pathHeight:
            return pathHeight
        return height

    def defaultTagWidth(self):
        width = self.shortestAndLongestPathEdge()[1].Length / 10
        return HoldingTagPreferences.defaultWidth(width)

    def defaultTagAngle(self):
        return HoldingTagPreferences.defaultAngle()

    def defaultTagRadius(self):
        return HoldingTagPreferences.defaultRadius()

    def sortedTags(self, tags):
        ordered = []
        for edge in self.bottomEdges:
            ts = [
                t
                for t in tags
                if Path.Geom.isRoughly(
                    0, Part.Vertex(t.originAt(self.minZ)).distToShape(edge)[0], 0.1
                )
            ]
            for t in sorted(
                ts,
                key=lambda t, edge=edge: (
                    t.originAt(self.minZ) - edge.valueAt(edge.FirstParameter)
                ).Length,
            ):
                tags.remove(t)
                ordered.append(t)
        # disable all tags that are not on the base wire.
        for tag in tags:
            Path.Log.info(
                "Tag #%d (%.2f, %.2f, %.2f) not on base wire - disabling\n"
                % (len(ordered), tag.x, tag.y, self.minZ)
            )
            tag.enabled = False
            ordered.append(tag)
        return ordered

    def pointIsOnPath(self, p):
        v = Part.Vertex(self.pointAtBottom(p))
        Path.Log.debug("pt = (%f, %f, %f)" % (v.X, v.Y, v.Z))
        for e in self.bottomEdges:
            indent = "{} ".format(e.distToShape(v)[0])
            debugEdge(e, indent, True)
            if Path.Geom.isRoughly(0.0, v.distToShape(e)[0], 0.1):
                return True
        return False

    def pointAtBottom(self, p):
        return FreeCAD.Vector(p.x, p.y, self.minZ)


class ObjectTagDressup:
    def __init__(self, obj, base):

        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "The base path to modify"),
        )
        obj.addProperty(
            "App::PropertyLength",
            "Width",
            "Tag",
            QT_TRANSLATE_NOOP("App::Property", "Width of tags."),
        )
        obj.addProperty(
            "App::PropertyLength",
            "Height",
            "Tag",
            QT_TRANSLATE_NOOP("App::Property", "Height of tags."),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "Angle",
            "Tag",
            QT_TRANSLATE_NOOP("App::Property", "Angle of tag plunge and ascent."),
        )
        obj.addProperty(
            "App::PropertyLength",
            "Radius",
            "Tag",
            QT_TRANSLATE_NOOP("App::Property", "Radius of the fillet for the tag."),
        )
        obj.addProperty(
            "App::PropertyVectorList",
            "Positions",
            "Tag",
            QT_TRANSLATE_NOOP("App::Property", "Locations of inserted holding tags"),
        )
        obj.addProperty(
            "App::PropertyIntegerList",
            "Disabled",
            "Tag",
            QT_TRANSLATE_NOOP("App::Property", "IDs of disabled holding tags"),
        )
        obj.addProperty(
            "App::PropertyInteger",
            "SegmentationFactor",
            "Tag",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Factor determining the # of segments used to approximate rounded tags.",
            ),
        )

        self.obj = obj
        self.solids = []
        self.tags = []
        self.pathData = None
        self.toolRadius = None
        self.mappers = []

        obj.Proxy = self
        obj.Base = base

    def dumps(self):
        return None

    def loads(self, state):
        self.obj = state
        self.solids = []
        self.tags = []
        self.pathData = None
        self.toolRadius = None
        self.mappers = []
        return None

    def onDocumentRestored(self, obj):
        self.obj = obj

    def supportsTagGeneration(self, obj):
        if not self.pathData:
            self.setup(obj)
        return self.pathData.supportsTagGeneration()

    def generateTags(self, obj, count):
        if self.supportsTagGeneration(obj):
            if self.pathData:
                self.tags = self.pathData.generateTags(
                    obj,
                    count,
                    obj.Width.Value,
                    obj.Height.Value,
                    obj.Angle,
                    obj.Radius.Value,
                    None,
                )
                obj.Positions = [tag.originAt(self.pathData.minZ) for tag in self.tags]
                obj.Disabled = []
                return False
            else:
                self.setup(obj, count)
                self.execute(obj)
                return True
        else:
            self.tags = []
            obj.Positions = []
            obj.Disabled = []
            return False

    def copyTags(self, obj, fromObj):
        obj.Width = fromObj.Width
        obj.Height = fromObj.Height
        obj.Angle = fromObj.Angle
        obj.Radius = fromObj.Radius
        obj.SegmentationFactor = fromObj.SegmentationFactor

        self.tags = self.pathData.copyTags(
            obj, fromObj, obj.Width.Value, obj.Height.Value, obj.Angle, obj.Radius.Value
        )
        obj.Positions = [tag.originAt(self.pathData.minZ) for tag in self.tags]
        obj.Disabled = []
        return False

    def isValidTagStartIntersection(self, edge, i):
        if Path.Geom.pointsCoincide(i, edge.valueAt(edge.LastParameter)):
            return False
        p1 = edge.valueAt(edge.FirstParameter)
        p2 = edge.valueAt(edge.LastParameter)
        if Path.Geom.pointsCoincide(Path.Geom.xy(p1), Path.Geom.xy(p2)):
            # if this vertical goes up, it can't be the start of a tag intersection
            if p1.z < p2.z:
                return False
        return True

    def createPath(self, obj, pathData, tags):
        Path.Log.track()
        commands = []
        lastEdge = 0
        lastTag = 0
        # sameTag = None
        t = 0
        # inters = None
        edge = None

        segm = 50
        if hasattr(obj, "SegmentationFactor"):
            segm = obj.SegmentationFactor
            if segm <= 0:
                segm = 50
                obj.SegmentationFactor = 50

        self.mappers = []
        mapper = None

        tc = PathDressup.toolController(obj.Base)
        horizFeed = tc.HorizFeed.Value
        vertFeed = tc.VertFeed.Value
        horizRapid = tc.HorizRapid.Value
        vertRapid = tc.VertRapid.Value

        while edge or lastEdge < len(pathData.edges):
            Path.Log.debug(
                "------- lastEdge = %d/%d.%d/%d" % (lastEdge, lastTag, t, len(tags))
            )
            if not edge:
                edge = pathData.edges[lastEdge]
                debugEdge(
                    edge, "=======  new edge: %d/%d" % (lastEdge, len(pathData.edges))
                )
                lastEdge += 1
                # sameTag = None

            if mapper:
                mapper.add(edge)
                if mapper.mappingComplete():
                    commands.extend(mapper.commands)
                    edge = mapper.tail
                    mapper = None
                else:
                    edge = None

            if edge:
                tIndex = (t + lastTag) % len(tags)
                t += 1
                i = tags[tIndex].intersects(edge, edge.FirstParameter)
                if i and self.isValidTagStartIntersection(edge, i):
                    mapper = MapWireToTag(
                        edge,
                        tags[tIndex],
                        i,
                        segm,
                        pathData.maxZ,
                        hSpeed=horizFeed,
                        vSpeed=vertFeed,
                    )
                    self.mappers.append(mapper)
                    edge = mapper.tail

            if not mapper and t >= len(tags):
                # gone through all tags, consume edge and move on
                if edge:
                    debugEdge(edge, "++++++++")
                    if pathData.rapid.isRapid(edge):
                        v = edge.Vertexes[1]
                        if (
                            not commands
                            and Path.Geom.isRoughly(0, v.X)
                            and Path.Geom.isRoughly(0, v.Y)
                            and not Path.Geom.isRoughly(0, v.Z)
                        ):
                            # The very first move is just to move to ClearanceHeight
                            commands.append(
                                Path.Command("G0", {"Z": v.Z, "F": horizRapid})
                            )
                        else:
                            commands.append(
                                Path.Command(
                                    "G0", {"X": v.X, "Y": v.Y, "Z": v.Z, "F": vertRapid}
                                )
                            )
                    else:
                        commands.extend(
                            Path.Geom.cmdsForEdge(
                                edge, segm=segm, hSpeed=horizFeed, vSpeed=vertFeed
                            )
                        )
                edge = None
                t = 0

        return Path.Path(commands)

    def problems(self):
        return list([m for m in self.mappers if m.haveProblem])

    def createTagsPositionDisabled(self, obj, positionsIn, disabledIn):
        rawTags = []
        for i, pos in enumerate(positionsIn):
            tag = Tag(
                i,
                pos.x,
                pos.y,
                obj.Width.Value,
                obj.Height.Value,
                obj.Angle,
                obj.Radius,
                not i in disabledIn,
            )
            tag.createSolidsAt(self.pathData.minZ, self.toolRadius)
            rawTags.append(tag)
        # disable all tags that intersect with their previous tag
        prev = None
        tags = []
        positions = []
        disabled = []
        for i, tag in enumerate(self.pathData.sortedTags(rawTags)):
            if tag.enabled:
                if prev:
                    if prev.solid.common(tag.solid).Faces:
                        Path.Log.info(
                            "Tag #%d intersects with previous tag - disabling\n" % i
                        )
                        Path.Log.debug("this tag = %d [%s]" % (i, tag.solid.BoundBox))
                        tag.enabled = False
                elif self.pathData.edges:
                    e = self.pathData.edges[0]
                    p0 = e.valueAt(e.FirstParameter)
                    p1 = e.valueAt(e.LastParameter)
                    if tag.solid.isInside(
                        p0, Path.Geom.Tolerance, True
                    ) or tag.solid.isInside(p1, Path.Geom.Tolerance, True):
                        Path.Log.info(
                            "Tag #%d intersects with starting point - disabling\n" % i
                        )
                        tag.enabled = False

            if tag.enabled:
                prev = tag
                Path.Log.debug("previousTag = %d [%s]" % (i, prev))
            else:
                disabled.append(i)
            tag.nr = i  # assign final nr
            tags.append(tag)
            positions.append(tag.originAt(self.pathData.minZ))
        return (tags, positions, disabled)

    def execute(self, obj):
        # import cProfile
        # pr = cProfile.Profile()
        # pr.enable()
        self.doExecute(obj)
        # pr.disable()
        # pr.print_stats()

    def doExecute(self, obj):
        if not obj.Base:
            return
        if not obj.Base.isDerivedFrom("Path::Feature"):
            return
        if not obj.Base.Path:
            return
        if not obj.Base.Path.Commands:
            return

        pathData = self.setup(obj)
        if not pathData:
            Path.Log.debug("execute - no pathData")
            return

        self.tags = []
        if hasattr(obj, "Positions"):
            self.tags, positions, disabled = self.createTagsPositionDisabled(
                obj, obj.Positions, obj.Disabled
            )
            if obj.Disabled != disabled:
                Path.Log.debug(
                    "Updating properties.... %s vs. %s" % (obj.Disabled, disabled)
                )
                obj.Positions = positions
                obj.Disabled = disabled

        if not self.tags:
            Path.Log.debug("execute - no tags")
            obj.Path = PathUtils.getPathWithPlacement(obj.Base)
            return

        try:
            self.processTags(obj)
        except Exception as e:
            Path.Log.error(
                "processing tags failed clearing all tags ... '%s'" % (e.args[0])
            )
            obj.Path = PathUtils.getPathWithPlacement(obj.Base)

        # update disabled in case there are some additional ones
        disabled = copy.copy(self.obj.Disabled)
        solids = []
        for tag in self.tags:
            solids.append(tag.solid)
            if not tag.enabled and tag.nr not in disabled:
                disabled.append(tag.nr)
        self.solids = solids
        if obj.Disabled != disabled:
            obj.Disabled = disabled

    @waiting_effects
    def processTags(self, obj):
        tagID = 0
        if Path.Log.getLevel(Path.Log.thisModule()) == Path.Log.Level.DEBUG:
            for tag in self.tags:
                tagID += 1
                if tag.enabled:
                    Path.Log.debug(
                        "x=%s, y=%s, z=%s" % (tag.x, tag.y, self.pathData.minZ)
                    )
                    # debugMarker(FreeCAD.Vector(tag.x, tag.y, self.pathData.minZ), "tag-%02d" % tagID , (1.0, 0.0, 1.0), 0.5)
                    # if not Path.Geom.isRoughly(90, tag.angle):
                    #    debugCone(tag.originAt(self.pathData.minZ), tag.r1, tag.r2, tag.actualHeight, "tag-%02d" % tagID)
                    # else:
                    #    debugCylinder(tag.originAt(self.pathData.minZ), tag.fullWidth()/2, tag.actualHeight, "tag-%02d" % tagID)

        obj.Path = self.createPath(obj, self.pathData, self.tags)

    def setup(self, obj, generate=False):
        Path.Log.debug("setup")
        self.obj = obj
        try:
            pathData = PathData(obj)
        except ValueError:
            Path.Log.error(
                translate(
                    "CAM_DressupTag",
                    "Cannot insert holding tags for this path - please select a Profile path",
                )
                + "\n"
            )
            return None

        self.toolRadius = float(PathDressup.toolController(obj.Base).Tool.Diameter) / 2
        self.pathData = pathData
        if generate:
            obj.Height = self.pathData.defaultTagHeight()
            obj.Width = self.pathData.defaultTagWidth()
            obj.Angle = self.pathData.defaultTagAngle()
            obj.Radius = self.pathData.defaultTagRadius()
            count = HoldingTagPreferences.defaultCount()
            self.generateTags(obj, count)
        return self.pathData

    def setXyEnabled(self, triples):
        Path.Log.track()
        if not self.pathData:
            self.setup(self.obj)
        positions = []
        disabled = []
        for i, (x, y, enabled) in enumerate(triples):
            # print("%d: (%.2f, %.2f) %d" % (i, x, y, enabled))
            positions.append(FreeCAD.Vector(x, y, 0))
            if not enabled:
                disabled.append(i)
        (
            self.tags,
            self.obj.Positions,
            self.obj.Disabled,
        ) = self.createTagsPositionDisabled(self.obj, positions, disabled)
        self.processTags(self.obj)

    def pointIsOnPath(self, obj, point):
        if not self.pathData:
            self.setup(obj)
        return self.pathData.pointIsOnPath(point)

    def pointAtBottom(self, obj, point):
        if not self.pathData:
            self.setup(obj)
        return self.pathData.pointAtBottom(point)


def Create(baseObject, name="DressupTag"):
    """
    Create(basePath, name='DressupTag') ... create tag dressup object for the given base path.
    """
    if not baseObject.isDerivedFrom("Path::Feature"):
        Path.Log.error(
            translate("CAM_DressupTag", "The selected object is not a path") + "\n"
        )
        return None

    if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
        Path.Log.error(translate("CAM_DressupTag", "Please select a Profile object"))
        return None

    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    dbo = ObjectTagDressup(obj, baseObject)
    job = PathUtils.findParentJob(baseObject)
    job.Proxy.addOperation(obj, baseObject)
    dbo.setup(obj, True)
    return obj


Path.Log.notice("Loading CAM_DressupTag... done\n")
