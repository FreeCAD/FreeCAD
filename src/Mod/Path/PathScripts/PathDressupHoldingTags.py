# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import FreeCAD
import Part
import Path
import PathScripts.PathDressup as PathDressup
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathUtil as PathUtil
import PathScripts.PathUtils as PathUtils
import copy
import math

from PathScripts.PathDressupTagPreferences import HoldingTagPreferences
from PathScripts.PathUtils import waiting_effects
from PySide import QtCore

"""Holding Tags Dressup object and FreeCAD command"""

LOGLEVEL = False

if LOGLEVEL:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule()
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

failures = []

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


def debugEdge(edge, prefix, force=False):
    if force or PathLog.getLevel(PathLog.thisModule()) == PathLog.Level.DEBUG:
        pf = edge.valueAt(edge.FirstParameter)
        pl = edge.valueAt(edge.LastParameter)
        if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
            print("%s %s((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (prefix, type(edge.Curve), pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))
        else:
            pm = edge.valueAt((edge.FirstParameter+edge.LastParameter)/2)
            print("%s %s((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (prefix, type(edge.Curve), pf.x, pf.y, pf.z, pm.x, pm.y, pm.z, pl.x, pl.y, pl.z))


def debugMarker(vector, label, color=None, radius=0.5):
    if PathLog.getLevel(PathLog.thisModule()) == PathLog.Level.DEBUG:
        obj = FreeCAD.ActiveDocument.addObject("Part::Sphere", label)
        obj.Label = label
        obj.Radius = radius
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0))
        if color:
            obj.ViewObject.ShapeColor = color


def debugCylinder(vector, r, height, label, color=None):
    if PathLog.getLevel(PathLog.thisModule()) == PathLog.Level.DEBUG:
        obj = FreeCAD.ActiveDocument.addObject("Part::Cylinder", label)
        obj.Label = label
        obj.Radius = r
        obj.Height = height
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0))
        obj.ViewObject.Transparency = 90
        if color:
            obj.ViewObject.ShapeColor = color


def debugCone(vector, r1, r2, height, label, color=None):
    if PathLog.getLevel(PathLog.thisModule()) == PathLog.Level.DEBUG:
        obj = FreeCAD.ActiveDocument.addObject("Part::Cone", label)
        obj.Label = label
        obj.Radius1 = r1
        obj.Radius2 = r2
        obj.Height = height
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0))
        obj.ViewObject.Transparency = 90
        if color:
            obj.ViewObject.ShapeColor = color


class Tag:
    def __init__(self, id, x, y, width, height, angle, radius, enabled=True):
        PathLog.track("%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %d" % (x, y, width, height, angle, radius, enabled))
        self.id = id
        self.x = x
        self.y = y
        self.width = math.fabs(width)
        self.height = math.fabs(height)
        self.actualHeight = self.height
        self.angle = math.fabs(angle)
        self.radius = radius if FreeCAD.Units.Quantity == type(radius) else FreeCAD.Units.Quantity(radius, FreeCAD.Units.Length)
        self.enabled = enabled
        self.isSquare = False

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
        if PathGeom.isRoughly(90, self.angle) and height > 0:
            # cylinder
            self.isSquare = True
            self.solid = Part.makeCylinder(r1, height)
            radius = min(min(self.radius, r1), self.height)
            PathLog.debug("Part.makeCone(%f, %f)" % (r1, height))
        elif self.angle > 0.0 and height > 0.0:
            # cone
            rad = math.radians(self.angle)
            tangens = math.tan(rad)
            dr = height / tangens
            if dr < r1:
                # with top
                r2 = r1 - dr
                s = height / math.sin(rad)
                radius = min(r2, s) * math.tan((math.pi - rad)/2) * 0.95
            else:
                # triangular
                r2 = 0
                height = r1 * tangens * 1.01
                self.actualHeight = height
            self.r2 = r2
            PathLog.debug("Part.makeCone(%f, %f, %f)" % (r1, r2, height))
            self.solid = Part.makeCone(r1, r2, height)
        else:
            # degenerated case - no tag
            PathLog.debug("Part.makeSphere(%f / 10000)" % (r1))
            self.solid = Part.makeSphere(r1 / 10000)
        if not PathGeom.isRoughly(0, R):  # testing is easier if the solid is not rotated
            angle = -PathGeom.getAngle(self.originAt(0)) * 180 / math.pi
            PathLog.debug("solid.rotate(%f)" % angle)
            self.solid.rotate(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1), angle)
        orig = self.originAt(z - 0.01 * self.actualHeight)
        PathLog.debug("solid.translate(%s)" % orig)
        self.solid.translate(orig)
        radius = min(self.radius, radius)
        self.realRadius = radius
        if not PathGeom.isRoughly(0, radius.Value):
            PathLog.debug("makeFillet(%.4f)" % radius)
            self.solid = self.solid.makeFillet(radius, [self.solid.Edges[0]])

    def filterIntersections(self, pts, face):
        if type(face.Surface) == Part.Cone or type(face.Surface) == Part.Cylinder or type(face.Surface) == Part.Toroid:
            PathLog.track("it's a cone/cylinder, checking z")
            return list(filter(lambda pt: pt.z >= self.bottom() and pt.z <= self.top(), pts))
        if type(face.Surface) == Part.Plane:
            PathLog.track("it's a plane, checking R")
            c = face.Edges[0].Curve
            if (type(c) == Part.Circle):
                return list(filter(lambda pt: (pt - c.Center).Length <= c.Radius or PathGeom.isRoughly((pt - c.Center).Length, c.Radius), pts))
        print("==== we got a %s" % face.Surface)

    def isPointOnEdge(self, pt, edge):
        param = edge.Curve.parameter(pt)
        if edge.FirstParameter <= param <= edge.LastParameter:
            return True
        if edge.LastParameter <= param <= edge.FirstParameter:
            return True
        if PathGeom.isRoughly(edge.FirstParameter, param) or PathGeom.isRoughly(edge.LastParameter, param):
            return True
        # print("-------- X %.2f <= %.2f <=%.2f   (%.2f, %.2f, %.2f)   %.2f:%.2f" % (edge.FirstParameter, param, edge.LastParameter, pt.x, pt.y, pt.z, edge.Curve.parameter(edge.valueAt(edge.FirstParameter)), edge.Curve.parameter(edge.valueAt(edge.LastParameter))))
        # p1 = edge.Vertexes[0]
        # f1 = edge.Curve.parameter(FreeCAD.Vector(p1.X, p1.Y, p1.Z))
        # p2 = edge.Vertexes[1]
        # f2 = edge.Curve.parameter(FreeCAD.Vector(p2.X, p2.Y, p2.Z))
        return False

    def nextIntersectionClosestTo(self, edge, solid, refPt):
        # ef = edge.valueAt(edge.FirstParameter)
        # em = edge.valueAt((edge.FirstParameter+edge.LastParameter)/2)
        # el = edge.valueAt(edge.LastParameter)
        # print("-------- intersect %s (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)  refp=(%.2f, %.2f, %.2f)" % (type(edge.Curve), ef.x, ef.y, ef.z, em.x, em.y, em.z, el.x, el.y, el.z, refPt.x, refPt.y, refPt.z))

        vertexes = edge.common(solid).Vertexes
        if vertexes:
            return sorted(vertexes, key=lambda v: (v.Point - refPt).Length)[0].Point
        return None

    def intersects(self, edge, param):
        def isDefinitelySmaller(z, zRef):
            # Eliminate false positives of edges that just brush along the top of the tag
            return z < zRef and not PathGeom.isRoughly(z, zRef, 0.01)

        if self.enabled:
            zFirst = edge.valueAt(edge.FirstParameter).z
            zLast  = edge.valueAt(edge.LastParameter).z
            zMax = self.top()
            if isDefinitelySmaller(zFirst, zMax) or isDefinitelySmaller(zLast, zMax):
                return self.nextIntersectionClosestTo(edge, self.solid, edge.valueAt(param))
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
        debugEdge(edge, 'MapWireToTag(%.2f, %.2f, %.2f)' % (i.x, i.y, i.z))
        self.tag = tag
        self.segm = segm
        self.maxZ = maxZ
        self.hSpeed = hSpeed
        self.vSpeed = vSpeed
        if PathGeom.pointsCoincide(edge.valueAt(edge.FirstParameter), i):
            tail = edge
            self.commands = []
            debugEdge(tail, '.........=')
        elif PathGeom.pointsCoincide(edge.valueAt(edge.LastParameter), i):
            debugEdge(edge, '++++++++ .')
            self.commands = PathGeom.cmdsForEdge(edge, segm=segm, hSpeed = self.hSpeed, vSpeed = self.vSpeed)
            tail = None
        else:
            e, tail = PathGeom.splitEdgeAt(edge, i)
            debugEdge(e, '++++++++ .')
            self.commands = PathGeom.cmdsForEdge(e, segm=segm, hSpeed = self.hSpeed, vSpeed = self.vSpeed)
            debugEdge(tail, '.........-')
            self.initialEdge = edge
        self.tail = tail
        self.edges = []
        self.entry = i
        if tail:
            PathLog.debug("MapWireToTag(%s - %s)" % (i, tail.valueAt(tail.FirstParameter)))
        else:
            PathLog.debug("MapWireToTag(%s - )" % i)
        self.complete = False
        self.haveProblem = False

    def addEdge(self, edge):
        debugEdge(edge, '..........')
        self.edges.append(edge)

    def needToFlipEdge(self, edge, p):
        if PathGeom.pointsCoincide(edge.valueAt(edge.LastParameter), p):
            return True, edge.valueAt(edge.FirstParameter)
        return False, edge.valueAt(edge.LastParameter)

    def isEntryOrExitStrut(self, e):
        p1 = e.valueAt(e.FirstParameter)
        p2 = e.valueAt(e.LastParameter)
        if PathGeom.pointsCoincide(p1, self.entry) and p2.z >= self.entry.z:
            return 1
        if PathGeom.pointsCoincide(p2, self.entry) and p1.z >= self.entry.z:
            return 1
        if PathGeom.pointsCoincide(p1, self.exit) and p2.z >= self.exit.z:
            return 2
        if PathGeom.pointsCoincide(p2, self.exit) and p1.z >= self.exit.z:
            return 2
        return 0

    def cleanupEdges(self, edges):
        # want to remove all edges from the wire itself, and all internal struts
        PathLog.track("+cleanupEdges")
        PathLog.debug(" edges:")
        if not edges:
            return edges
        for e in edges:
            debugEdge(e, '   ')
        PathLog.debug(":")
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
            if self.tag.solid.isInside(p1, PathGeom.Tolerance, False) or self.tag.solid.isInside(p2, PathGeom.Tolerance, False):
                edges.remove(e)
                debugEdge(e, '......... X0', False)
            else:
                if PathGeom.pointsCoincide(p1, self.entry) or PathGeom.pointsCoincide(p2, self.entry):
                    self.entryEdges.append(e)
                if PathGeom.pointsCoincide(p1, self.exit) or PathGeom.pointsCoincide(p2, self.exit):
                    self.exitEdges.append(e)
        self.edgesCleanup.append(copy.copy(edges))

        # if there are no edges connected to entry/exit, it means the plunge in/out is vertical
        # we need to add in the missing segment and collect the new entry/exit edges.
        if not self.entryEdges:
            print("fill entryEdges ...")
            self.realEntry = sorted(self.edgePoints, key=lambda p: (p - self.entry).Length)[0]
            self.entryEdges = list(filter(lambda e: PathGeom.edgeConnectsTo(e, self.realEntry), edges))
            edges.append(Part.Edge(Part.LineSegment(self.entry, self.realEntry)))
        else:
            self.realEntry = None
        if not self.exitEdges:
            print("fill exitEdges ...")
            self.realExit = sorted(self.edgePoints, key=lambda p: (p - self.exit).Length)[0]
            self.exitEdges = list(filter(lambda e: PathGeom.edgeConnectsTo(e, self.realExit), edges))
            edges.append(Part.Edge(Part.LineSegment(self.realExit, self.exit)))
        else:
            self.realExit = None
        self.edgesCleanup.append(copy.copy(edges))

        # if there are 2 edges attached to entry/exit, throw away the one that is "lower"
        if len(self.entryEdges) > 1:
            debugEdge(self.entryEdges[0], ' entry[0]', False)
            debugEdge(self.entryEdges[1], ' entry[1]', False)
            if self.entryEdges[0].BoundBox.ZMax < self.entryEdges[1].BoundBox.ZMax:
                edges.remove(self.entryEdges[0])
                debugEdge(e, '......... X1', False)
            else:
                edges.remove(self.entryEdges[1])
                debugEdge(e, '......... X2', False)
        if len(self.exitEdges) > 1:
            debugEdge(self.exitEdges[0], ' exit[0]', False)
            debugEdge(self.exitEdges[1], ' exit[1]', False)
            if self.exitEdges[0].BoundBox.ZMax < self.exitEdges[1].BoundBox.ZMax:
                if self.exitEdges[0] in edges:
                    edges.remove(self.exitEdges[0])
                debugEdge(e, '......... X3', False)
            else:
                if self.exitEdges[1] in edges:
                    edges.remove(self.exitEdges[1])
                debugEdge(e, '......... X4', False)

        self.edgesCleanup.append(copy.copy(edges))
        return edges

    def orderAndFlipEdges(self, inputEdges):
        PathLog.track("entry(%.2f, %.2f, %.2f), exit(%.2f, %.2f, %.2f)" % (self.entry.x, self.entry.y, self.entry.z, self.exit.x, self.exit.y, self.exit.z))
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
                if PathGeom.pointsCoincide(p1, p0):
                    outputEdges.append((e, False))
                    edges.remove(e)
                    lastP = None
                    p0 = p2
                    debugEdge(e, ">>>>> no flip")
                    break
                elif PathGeom.pointsCoincide(p2, p0):
                    flipped = PathGeom.flipEdge(e)
                    if not flipped is None:
                        outputEdges.append((flipped, True))
                    else:
                        p0 = None
                        cnt = 0
                        for p in reversed(e.discretize(Deflection=0.01)):
                            if not p0 is None:
                                outputEdges.append((Part.Edge(Part.LineSegment(p0, p)), True))
                                cnt = cnt + 1
                            p0 = p
                        PathLog.info("replaced edge with %d straight segments" % cnt)
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
                print('input edges:')
                for e in inputEdges:
                    debugEdge(e, '  ', False)
                print('ordered edges:')
                for e, flip in outputEdges:
                    debugEdge(e, '  %c ' % ('<' if flip else '>'), False)
                print('remaining edges:')
                for e in edges:
                    debugEdge(e, '    ', False)
                raise ValueError("No connection to %s" % (p0))
            elif lastP:
                PathLog.debug("xxxxxx (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)" % (p0.x, p0.y, p0.z, lastP.x, lastP.y, lastP.z))
            else:
                PathLog.debug("xxxxxx (%.2f, %.2f, %.2f) -" % (p0.x, p0.y, p0.z))
            lastP = p0
        PathLog.track("-")
        return outputEdges

    def isStrut(self, edge):
        p1 = PathGeom.xy(edge.valueAt(edge.FirstParameter))
        p2 = PathGeom.xy(edge.valueAt(edge.LastParameter))
        return PathGeom.pointsCoincide(p1, p2)

    def shell(self):
        if len(self.edges) > 1:
            wire = Part.Wire(self.initialEdge)
        else:
            edge = self.edges[0]
            if PathGeom.pointsCoincide(edge.valueAt(edge.FirstParameter), self.finalEdge.valueAt(self.finalEdge.FirstParameter)):
                wire = Part.Wire(self.finalEdge)
            elif hasattr(self, 'initialEdge') and PathGeom.pointsCoincide(edge.valueAt(edge.FirstParameter), self.initialEdge.valueAt(self.initialEdge.FirstParameter)):
                wire = Part.Wire(self.initialEdge)
            else:
                wire = Part.Wire(edge)

        for edge in self.edges[1:]:
            if PathGeom.pointsCoincide(edge.valueAt(edge.FirstParameter), self.finalEdge.valueAt(self.finalEdge.FirstParameter)):
                wire.add(self.finalEdge)
            else:
                wire.add(edge)

        shell = wire.extrude(FreeCAD.Vector(0, 0, self.tag.height + 1))
        nullFaces = list(filter(lambda f: PathGeom.isRoughly(f.Area, 0), shell.Faces))
        if nullFaces:
            return shell.removeShape(nullFaces)
        return shell

    def commandsForEdges(self):
        global failures
        if self.edges:
            try:
                shape = self.shell().common(self.tag.solid)
                commands = []
                rapid = None
                for e, flip in self.orderAndFlipEdges(self.cleanupEdges(shape.Edges)):
                    debugEdge(e, '++++++++ %s' % ('<' if flip else '>'), False)
                    p1 = e.valueAt(e.FirstParameter)
                    p2 = e.valueAt(e.LastParameter)
                    if self.tag.isSquare and (PathGeom.isRoughly(p1.z, self.maxZ) or p1.z > self.maxZ) and (PathGeom.isRoughly(p2.z, self.maxZ) or p2.z > self.maxZ):
                        rapid = p1 if flip else p2
                    else:
                        if rapid:
                            commands.append(Path.Command('G0', {'X': rapid.x, 'Y': rapid.y, 'Z': rapid.z}))
                            rapid = None
                        commands.extend(PathGeom.cmdsForEdge(e, False, False, self.segm, hSpeed = self.hSpeed, vSpeed = self.vSpeed))
                if rapid:
                    commands.append(Path.Command('G0', {'X': rapid.x, 'Y': rapid.y, 'Z': rapid.z}))
                    rapid = None
                return commands
            except Exception as e:
                PathLog.error("Exception during processing tag @(%.2f, %.2f) (%s) - disabling the tag" % (self.tag.x, self.tag.y, e.args[0]))
                #if sys.version_info.major < 3:
                #    traceback.print_exc(e)
                #else:
                #    traceback.print_exc()
                self.tag.enabled = False
                commands = []
                for e in self.edges:
                    commands.extend(PathGeom.cmdsForEdge(e, hSpeed = self.hSpeed, vSpeed = self.vSpeed))
                failures.append(self)
                return commands
        return []

    def add(self, edge):
        self.tail = None
        self.finalEdge = edge
        if self.tag.solid.isInside(edge.valueAt(edge.LastParameter), PathGeom.Tolerance, True):
            self.addEdge(edge)
        else:
            i = self.tag.intersects(edge, edge.LastParameter)
            if not i:
                self.offendingEdge = edge
                debugEdge(edge, 'offending Edge:', False)
                o = self.tag.originAt(self.tag.z)
                print('originAt: (%.2f, %.2f, %.2f)' % (o.x, o.y, o.z))
                i = edge.valueAt(edge.FirstParameter)
            if PathGeom.pointsCoincide(i, edge.valueAt(edge.FirstParameter)):
                self.tail = edge
            else:
                e, tail = PathGeom.splitEdgeAt(edge, i)
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
                if PathGeom.isRoughly(r0.X, v0.X) and PathGeom.isRoughly(r0.Y, v0.Y) and PathGeom.isRoughly(r0.Z, v0.Z) and PathGeom.isRoughly(r1.X, v1.X) and PathGeom.isRoughly(r1.Y, v1.Y) and PathGeom.isRoughly(r1.Z, v1.Z):
                    return True
        return False


class PathData:
    def __init__(self, obj):
        PathLog.track(obj.Base.Name)
        self.obj = obj
        self.wire, rapid = PathGeom.wireForPath(obj.Base.Path)
        self.rapid = _RapidEdges(rapid)
        self.edges = self.wire.Edges
        self.baseWire = self.findBottomWire(self.edges)

    def findBottomWire(self, edges):
        (minZ, maxZ) = self.findZLimits(edges)
        self.minZ = minZ
        self.maxZ = maxZ
        bottom = [e for e in edges if PathGeom.isRoughly(e.Vertexes[0].Point.z, minZ) and PathGeom.isRoughly(e.Vertexes[1].Point.z, minZ)]
        self.bottomEdges = bottom
        try:
            wire = Part.Wire(bottom)
            if wire.isClosed():
                return wire
        except Exception:
            #if sys.version_info.major < 3:
            #    traceback.print_exc(e)
            #else:
            #    traceback.print_exc()
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

    def generateTags(self, obj, count, width=None, height=None, angle=None, radius=None, spacing=None):
        PathLog.track(count, width, height, angle, spacing)
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
            PathLog.debug('  %d: %.2f' % (i, edge.Length))
            if PathGeom.isRoughly(edge.Length, longestEdge.Length):
                startIndex = i
                break

        startEdge = self.baseWire.Edges[startIndex]
        startCount = int(startEdge.Length / tagDistance)
        if (longestEdge.Length - shortestEdge.Length) > shortestEdge.Length:
            startCount = int(startEdge.Length / tagDistance) + 1

        lastTagLength = (startEdge.Length + (startCount - 1) * tagDistance) / 2
        currentLength = startEdge.Length

        minLength = min(2. * W, longestEdge.Length)

        PathLog.debug("length=%.2f shortestEdge=%.2f(%.2f) longestEdge=%.2f(%.2f) minLength=%.2f" % (self.baseWire.Length, shortestEdge.Length, shortestEdge.Length/self.baseWire.Length, longestEdge.Length, longestEdge.Length / self.baseWire.Length, minLength))
        PathLog.debug("   start: index=%-2d count=%d (length=%.2f, distance=%.2f)" % (startIndex, startCount, startEdge.Length, tagDistance))
        PathLog.debug("               -> lastTagLength=%.2f)" % lastTagLength)
        PathLog.debug("               -> currentLength=%.2f)" % currentLength)

        edgeDict = {startIndex: startCount}

        for i in range(startIndex + 1, len(self.baseWire.Edges)):
            edge = self.baseWire.Edges[i]
            (currentLength, lastTagLength) = self.processEdge(i, edge, currentLength, lastTagLength, tagDistance, minLength, edgeDict)
        for i in range(0, startIndex):
            edge = self.baseWire.Edges[i]
            (currentLength, lastTagLength) = self.processEdge(i, edge, currentLength, lastTagLength, tagDistance, minLength, edgeDict)

        tags = []

        for (i, count) in PathUtil.keyValueIter(edgeDict):
            edge = self.baseWire.Edges[i]
            PathLog.debug(" %d: %d" % (i, count))
            # debugMarker(edge.Vertexes[0].Point, 'base', (1.0, 0.0, 0.0), 0.2)
            # debugMarker(edge.Vertexes[1].Point, 'base', (0.0, 1.0, 0.0), 0.2)
            if 0 != count:
                distance = (edge.LastParameter - edge.FirstParameter) / count
                for j in range(0, count):
                    tag = edge.Curve.value((j+0.5) * distance)
                    tags.append(Tag(j, tag.x, tag.y, W, H, A, R, True))

        return tags

    def processEdge(self, index, edge, currentLength, lastTagLength, tagDistance, minLength, edgeDict):
        tagCount = 0
        currentLength += edge.Length
        if edge.Length >= minLength:
            while lastTagLength + tagDistance < currentLength:
                tagCount += 1
                lastTagLength += tagDistance
            if tagCount > 0:
                PathLog.debug("      index=%d -> count=%d" % (index, tagCount))
                edgeDict[index] = tagCount
        else:
            PathLog.debug("      skipping=%-2d (%.2f)" % (index, edge.Length))

        return (currentLength, lastTagLength)

    def defaultTagHeight(self):
        if hasattr(self.obj, 'Base') and hasattr(self.obj.Base, 'StartDepth') and hasattr(self.obj.Base, 'FinalDepth'):
            pathHeight = (self.obj.Base.StartDepth - self.obj.Base.FinalDepth).Value
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
            ts = [t for t in tags if PathGeom.isRoughly(0, Part.Vertex(t.originAt(self.minZ)).distToShape(edge)[0], 0.1)]
            for t in sorted(ts, key=lambda t: (t.originAt(self.minZ) - edge.valueAt(edge.FirstParameter)).Length):
                tags.remove(t)
                ordered.append(t)
        # disable all tags that are not on the base wire.
        for tag in tags:
            PathLog.info("Tag #%d (%.2f, %.2f, %.2f) not on base wire - disabling\n" % (len(ordered), tag.x, tag.y, self.minZ))
            tag.enabled = False
            ordered.append(tag)
        return ordered

    def pointIsOnPath(self, p):
        v = Part.Vertex(self.pointAtBottom(p))
        PathLog.debug("pt = (%f, %f, %f)" % (v.X, v.Y, v.Z))
        for e in self.bottomEdges:
            indent = "{} ".format(e.distToShape(v)[0])
            debugEdge(e, indent, True)
            if PathGeom.isRoughly(0.0, v.distToShape(e)[0], 0.1):
                return True
        return False

    def pointAtBottom(self, p):
        return FreeCAD.Vector(p.x, p.y, self.minZ)


class ObjectTagDressup:

    def __init__(self, obj, base):

        obj.addProperty("App::PropertyLink", "Base", "Base", QtCore.QT_TRANSLATE_NOOP("Path_DressupTag", "The base path to modify"))
        obj.addProperty("App::PropertyLength", "Width", "Tag", QtCore.QT_TRANSLATE_NOOP("Path_DressupTag", "Width of tags."))
        obj.addProperty("App::PropertyLength", "Height", "Tag", QtCore.QT_TRANSLATE_NOOP("Path_DressupTag", "Height of tags."))
        obj.addProperty("App::PropertyAngle", "Angle", "Tag", QtCore.QT_TRANSLATE_NOOP("Path_DressupTag", "Angle of tag plunge and ascent."))
        obj.addProperty("App::PropertyLength", "Radius", "Tag", QtCore.QT_TRANSLATE_NOOP("Path_DressupTag", "Radius of the fillet for the tag."))
        obj.addProperty("App::PropertyVectorList", "Positions", "Tag", QtCore.QT_TRANSLATE_NOOP("Path_DressupTag", "Locations of inserted holding tags"))
        obj.addProperty("App::PropertyIntegerList", "Disabled", "Tag", QtCore.QT_TRANSLATE_NOOP("Path_DressupTag", "IDs of disabled holding tags"))
        obj.addProperty("App::PropertyInteger", "SegmentationFactor", "Tag", QtCore.QT_TRANSLATE_NOOP("Path_DressupTag", "Factor determining the # of segments used to approximate rounded tags."))

        obj.Proxy = self
        obj.Base = base

        self.obj = obj
        self.solids = []

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def supportsTagGeneration(self, obj):
        if not hasattr(self, 'pathData'):
            self.setup(obj)
        return self.pathData.supportsTagGeneration()

    def generateTags(self, obj, count):
        if self.supportsTagGeneration(obj):
            if hasattr(self, "pathData"):
                self.tags = self.pathData.generateTags(obj, count, obj.Width.Value, obj.Height.Value, obj.Angle, obj.Radius.Value, None)
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

    def isValidTagStartIntersection(self, edge, i):
        if PathGeom.pointsCoincide(i, edge.valueAt(edge.LastParameter)):
            return False
        p1 = edge.valueAt(edge.FirstParameter)
        p2 = edge.valueAt(edge.LastParameter)
        if PathGeom.pointsCoincide(PathGeom.xy(p1), PathGeom.xy(p2)):
            # if this vertical goes up, it can't be the start of a tag intersection
            if p1.z < p2.z:
                return False
        return True

    def createPath(self, obj, pathData, tags):
        PathLog.track()
        commands = []
        lastEdge = 0
        lastTag = 0
        # sameTag = None
        t = 0
        # inters = None
        edge = None

        segm = 50
        if hasattr(obj, 'SegmentationFactor'):
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
            PathLog.debug("------- lastEdge = %d/%d.%d/%d" % (lastEdge, lastTag, t, len(tags)))
            if not edge:
                edge = pathData.edges[lastEdge]
                debugEdge(edge, "=======  new edge: %d/%d" % (lastEdge, len(pathData.edges)))
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
                    mapper = MapWireToTag(edge, tags[tIndex], i, segm, pathData.maxZ, hSpeed = horizFeed, vSpeed = vertFeed)
                    self.mappers.append(mapper)
                    edge = mapper.tail

            if not mapper and t >= len(tags):
                # gone through all tags, consume edge and move on
                if edge:
                    debugEdge(edge, '++++++++')
                    if pathData.rapid.isRapid(edge):
                        v = edge.Vertexes[1]
                        if not commands and PathGeom.isRoughly(0, v.X) and PathGeom.isRoughly(0, v.Y) and not PathGeom.isRoughly(0, v.Z):
                            # The very first move is just to move to ClearanceHeight
                            commands.append(Path.Command('G0', {'Z': v.Z, 'F': horizRapid}))
                        else:
                            commands.append(Path.Command('G0', {'X': v.X, 'Y': v.Y, 'Z': v.Z, 'F': vertRapid}))
                    else:
                        commands.extend(PathGeom.cmdsForEdge(edge, segm=segm, hSpeed = horizFeed, vSpeed = vertFeed))
                edge = None
                t = 0

        return Path.Path(commands)

    def problems(self):
        return list(filter(lambda m: m.haveProblem, self.mappers))

    def createTagsPositionDisabled(self, obj, positionsIn, disabledIn):
        rawTags = []
        for i, pos in enumerate(positionsIn):
            tag = Tag(i, pos.x, pos.y, obj.Width.Value, obj.Height.Value, obj.Angle, obj.Radius, not i in disabledIn)
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
                        PathLog.info("Tag #%d intersects with previous tag - disabling\n" % i)
                        PathLog.debug("this tag = %d [%s]" % (i, tag.solid.BoundBox))
                        tag.enabled = False
                elif self.pathData.edges:
                    e = self.pathData.edges[0]
                    p0 = e.valueAt(e.FirstParameter)
                    p1 = e.valueAt(e.LastParameter)
                    if tag.solid.isInside(p0, PathGeom.Tolerance, True) or tag.solid.isInside(p1, PathGeom.Tolerance, True):
                        PathLog.info("Tag #%d intersects with starting point - disabling\n" % i)
                        tag.enabled = False

            if tag.enabled:
                prev = tag
                PathLog.debug("previousTag = %d [%s]" % (i, prev))
            else:
                disabled.append(i)
            tag.id = i  # assigne final id
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
            print("execute - no pathData")
            return

        self.tags = []
        if hasattr(obj, "Positions"):
            self.tags, positions, disabled = self.createTagsPositionDisabled(obj, obj.Positions, obj.Disabled)
            if obj.Disabled != disabled:
                PathLog.debug("Updating properties.... %s vs. %s" % (obj.Disabled, disabled))
                obj.Positions = positions
                obj.Disabled = disabled

        if not self.tags:
            print("execute - no tags")
            obj.Path = obj.Base.Path
            return

        try:
            self.processTags(obj)
        except Exception as e:
            PathLog.error("processing tags failed clearing all tags ... '%s'" % (e.args[0]))
            #if sys.version_info.major < 3:
            #    traceback.print_exc(e)
            #else:
            #    traceback.print_exc()
            obj.Path = obj.Base.Path

        # update disabled in case there are some additional ones
        disabled = copy.copy(self.obj.Disabled)
        solids = []
        for tag in self.tags:
            solids.append(tag.solid)
            if not tag.enabled and tag.id not in disabled:
                disabled.append(tag.id)
        self.solids = solids
        if obj.Disabled != disabled:
            obj.Disabled = disabled

    @waiting_effects
    def processTags(self, obj):
        global failures
        failures = []
        tagID = 0
        if PathLog.getLevel(PathLog.thisModule()) == PathLog.Level.DEBUG:
            for tag in self.tags:
                tagID += 1
                if tag.enabled:
                    PathLog.debug("x=%s, y=%s, z=%s" % (tag.x, tag.y, self.pathData.minZ))
                    # debugMarker(FreeCAD.Vector(tag.x, tag.y, self.pathData.minZ), "tag-%02d" % tagID , (1.0, 0.0, 1.0), 0.5)
                    # if not PathGeom.isRoughly(90, tag.angle):
                    #    debugCone(tag.originAt(self.pathData.minZ), tag.r1, tag.r2, tag.actualHeight, "tag-%02d" % tagID)
                    # else:
                    #    debugCylinder(tag.originAt(self.pathData.minZ), tag.fullWidth()/2, tag.actualHeight, "tag-%02d" % tagID)

        obj.Path = self.createPath(obj, self.pathData, self.tags)

    def setup(self, obj, generate=False):
        PathLog.debug("setup")
        self.obj = obj
        try:
            pathData = PathData(obj)
        except ValueError:
            PathLog.error(translate("Path_DressupTag", "Cannot insert holding tags for this path - please select a Profile path")+"\n")
            #if sys.version_info.major < 3:
            #    traceback.print_exc(e)
            #else:
            #    traceback.print_exc()
            return None

        self.toolRadius = PathDressup.toolController(obj.Base).Tool.Diameter / 2
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
        PathLog.track()
        if not hasattr(self, 'pathData'):
            self.setup(self.obj)
        positions = []
        disabled = []
        for i, (x, y, enabled) in enumerate(triples):
            # print("%d: (%.2f, %.2f) %d" % (i, x, y, enabled))
            positions.append(FreeCAD.Vector(x, y, 0))
            if not enabled:
                disabled.append(i)
        self.tags, self.obj.Positions, self.obj.Disabled = self.createTagsPositionDisabled(self.obj, positions, disabled)
        self.processTags(self.obj)

    def pointIsOnPath(self, obj, point):
        if not hasattr(self, 'pathData'):
            self.setup(obj)
        return self.pathData.pointIsOnPath(point)

    def pointAtBottom(self, obj, point):
        if not hasattr(self, 'pathData'):
            self.setup(obj)
        return self.pathData.pointAtBottom(point)


def Create(baseObject, name='DressupTag'):
    '''
    Create(basePath, name='DressupTag') ... create tag dressup object for the given base path.
    '''
    if not baseObject.isDerivedFrom('Path::Feature'):
        PathLog.error(translate('Path_DressupTag', 'The selected object is not a path')+'\n')
        return None

    if baseObject.isDerivedFrom('Path::FeatureCompoundPython'):
        PathLog.error(translate('Path_DressupTag', 'Please select a Profile object'))
        return None

    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "TagDressup")
    dbo = ObjectTagDressup(obj, baseObject)
    job = PathUtils.findParentJob(baseObject)
    job.Proxy.addOperation(obj, baseObject)
    dbo.setup(obj, True)
    return obj

PathLog.notice("Loading Path_DressupTag... done\n")
