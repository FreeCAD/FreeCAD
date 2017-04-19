# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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
import FreeCADGui
import Draft
import DraftGeomUtils
import Path
import PathScripts.PathLog as PathLog
import PathScripts.PathPreferencesPathDressup as PathPreferencesPathDressup
import Part
import copy
import math

from PathScripts import PathUtils
from PathScripts.PathGeom import PathGeom
from PathScripts.PathPreferences import PathPreferences
from PySide import QtCore

"""Holding Tags Dressup object and FreeCAD command"""

# Qt tanslation handling
def translate(text, context = "PathDressup_HoldingTags", disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


LOG_MODULE = PathLog.thisModule()
PathLog.setLevel(PathLog.Level.INFO, LOG_MODULE)

if FreeCAD.GuiUp:
    from pivy import coin
    from PySide import QtGui

def debugEdge(edge, prefix, force = False):
    if force or PathLog.getLevel(LOG_MODULE) == PathLog.Level.DEBUG:
        pf = edge.valueAt(edge.FirstParameter)
        pl = edge.valueAt(edge.LastParameter)
        if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
            print("%s %s((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (prefix, type(edge.Curve), pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))
        else:
            pm = edge.valueAt((edge.FirstParameter+edge.LastParameter)/2)
            print("%s %s((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (prefix, type(edge.Curve), pf.x, pf.y, pf.z, pm.x, pm.y, pm.z, pl.x, pl.y, pl.z))

def debugMarker(vector, label, color = None, radius = 0.5):
    if PathLog.getLevel(LOG_MODULE) == PathLog.Level.DEBUG:
        obj = FreeCAD.ActiveDocument.addObject("Part::Sphere", label)
        obj.Label = label
        obj.Radius = radius
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        if color:
            obj.ViewObject.ShapeColor = color

def debugCylinder(vector, r, height, label, color = None):
    if PathLog.getLevel(LOG_MODULE) == PathLog.Level.DEBUG:
        obj = FreeCAD.ActiveDocument.addObject("Part::Cylinder", label)
        obj.Label = label
        obj.Radius = r
        obj.Height = height
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        obj.ViewObject.Transparency = 90
        if color:
            obj.ViewObject.ShapeColor = color

def debugCone(vector, r1, r2, height, label, color = None):
    if PathLog.getLevel(LOG_MODULE) == PathLog.Level.DEBUG:
        obj = FreeCAD.ActiveDocument.addObject("Part::Cone", label)
        obj.Label = label
        obj.Radius1 = r1
        obj.Radius2 = r2
        obj.Height = height
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        obj.ViewObject.Transparency = 90
        if color:
            obj.ViewObject.ShapeColor = color


class HoldingTagsPreferences:
    DefaultHoldingTagWidth   = 'DefaultHoldingTagWidth'
    DefaultHoldingTagHeight  = 'DefaultHoldingTagHeight'
    DefaultHoldingTagAngle   = 'DefaultHoldingTagAngle'
    DefaultHoldingTagRadius  = 'DefaultHoldingTagRadius'
    DefaultHoldingTagCount   = 'DefaultHoldingTagCount'

    @classmethod
    def defaultWidth(cls, ifNotSet):
        value = PathPreferences.preferences().GetFloat(cls.DefaultHoldingTagWidth, ifNotSet)
        if value == 0.0:
            return ifNotSet
        return value

    @classmethod
    def defaultHeight(cls, ifNotSet):
        value = PathPreferences.preferences().GetFloat(cls.DefaultHoldingTagHeight, ifNotSet)
        if value == 0.0:
            return ifNotSet
        return value

    @classmethod
    def defaultAngle(cls, ifNotSet = 45.0):
        value = PathPreferences.preferences().GetFloat(cls.DefaultHoldingTagAngle, ifNotSet)
        if value < 10.0:
            return ifNotSet
        return value

    @classmethod
    def defaultCount(cls, ifNotSet = 4):
        value = PathPreferences.preferences().GetUnsigned(cls.DefaultHoldingTagCount, ifNotSet)
        if value < 2:
            return float(ifNotSet)
        return float(value)

    @classmethod
    def defaultRadius(cls, ifNotSet = 0.0):
        return PathPreferences.preferences().GetFloat(cls.DefaultHoldingTagRadius, ifNotSet)


    def __init__(self):
        self.form = FreeCADGui.PySideUic.loadUi(":/preferences/PathDressupHoldingTags.ui")
        self.label = translate('Holding Tags')

    def loadSettings(self):
        self.form.ifWidth.setText(FreeCAD.Units.Quantity(self.defaultWidth(0), FreeCAD.Units.Length).UserString)
        self.form.ifHeight.setText(FreeCAD.Units.Quantity(self.defaultHeight(0), FreeCAD.Units.Length).UserString)
        self.form.dsbAngle.setValue(self.defaultAngle())
        self.form.ifRadius.setText(FreeCAD.Units.Quantity(self.defaultRadius(), FreeCAD.Units.Length).UserString)
        self.form.sbCount.setValue(self.defaultCount())

    def saveSettings(self):
        pref = PathPreferences.preferences()
        pref.SetFloat(self.DefaultHoldingTagWidth, FreeCAD.Units.Quantity(self.form.ifWidth.text()).Value)
        pref.SetFloat(self.DefaultHoldingTagHeight, FreeCAD.Units.Quantity(self.form.ifHeight.text()).Value)
        pref.SetFloat(self.DefaultHoldingTagAngle, self.form.dsbAngle.value())
        pref.SetFloat(self.DefaultHoldingTagRadius, FreeCAD.Units.Quantity(self.form.ifRadius.text()))
        pref.SetUnsigned(self.DefaultHoldingTagCount, self.form.sbCount.value())

    @classmethod
    def preferencesPage(cls):
        return HoldingTagsPreferences()

class Tag:
    def __init__(self, x, y, width, height, angle, radius, enabled=True):
        PathLog.track("%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %d" % (x, y, width, height, angle, radius, enabled))
        self.x = x
        self.y = y
        self.width = math.fabs(width)
        self.height = math.fabs(height)
        self.actualHeight = self.height
        self.angle = math.fabs(angle)
        self.radius = radius
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
        if self.angle == 90 and height > 0:
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
        if not R == 0: # testing is easier if the solid is not rotated
            angle = -PathGeom.getAngle(self.originAt(0)) * 180 / math.pi
            PathLog.debug("solid.rotate(%f)" % angle)
            self.solid.rotate(FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,0,1), angle)
        orig = self.originAt(z - 0.01 * self.actualHeight)
        PathLog.debug("solid.translate(%s)" % orig)
        self.solid.translate(orig)
        radius = min(self.radius, radius)
        self.realRadius = radius
        if radius != 0:
            PathLog.debug("makeFillet(%.4f)" % radius)
            self.solid = self.solid.makeFillet(radius, [self.solid.Edges[0]])

    def filterIntersections(self, pts, face):
        if type(face.Surface) == Part.Cone or type(face.Surface) == Part.Cylinder or type(face.Surface) == Part.Toroid:
            PathLog.track("it's a cone/cylinder, checking z")
            return filter(lambda pt: pt.z >= self.bottom() and pt.z <= self.top(), pts)
        if type(face.Surface) == Part.Plane:
            PathLog.track("it's a plane, checking R")
            c = face.Edges[0].Curve
            if (type(c) == Part.Circle):
                return filter(lambda pt: (pt - c.Center).Length <= c.Radius or PathGeom.isRoughly((pt - c.Center).Length, c.Radius), pts)
        print("==== we got a %s" % face.Surface)

    def isPointOnEdge(self, pt, edge):
        param = edge.Curve.parameter(pt)
        if edge.FirstParameter <= param <= edge.LastParameter:
            return True
        if edge.LastParameter <= param <= edge.FirstParameter:
            return True
        if PathGeom.isRoughly(edge.FirstParameter, param) or PathGeom.isRoughly(edge.LastParameter, param):
            return True
        #print("-------- X %.2f <= %.2f <=%.2f   (%.2f, %.2f, %.2f)   %.2f:%.2f" % (edge.FirstParameter, param, edge.LastParameter, pt.x, pt.y, pt.z, edge.Curve.parameter(edge.valueAt(edge.FirstParameter)), edge.Curve.parameter(edge.valueAt(edge.LastParameter))))
        p1 = edge.Vertexes[0]
        f1 = edge.Curve.parameter(FreeCAD.Vector(p1.X, p1.Y, p1.Z))
        p2 = edge.Vertexes[1]
        f2 = edge.Curve.parameter(FreeCAD.Vector(p2.X, p2.Y, p2.Z))
        return False


    def nextIntersectionClosestTo(self, edge, solid, refPt):
        ef = edge.valueAt(edge.FirstParameter)
        em = edge.valueAt((edge.FirstParameter+edge.LastParameter)/2)
        el = edge.valueAt(edge.LastParameter)
        #print("-------- intersect %s (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)  refp=(%.2f, %.2f, %.2f)" % (type(edge.Curve), ef.x, ef.y, ef.z, em.x, em.y, em.z, el.x, el.y, el.z, refPt.x, refPt.y, refPt.z))

        vertexes = edge.common(solid).Vertexes
        if vertexes:
            return sorted(vertexes, key=lambda v: (v.Point - refPt).Length)[0].Point
        return None

    def intersects(self, edge, param):
        if self.enabled:
            if edge.valueAt(edge.FirstParameter).z < self.top() or edge.valueAt(edge.LastParameter).z < self.top():
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
    def __init__(self, edge, tag, i, segm, maxZ):
        debugEdge(edge, 'MapWireToTag(%.2f, %.2f, %.2f)' % (i.x, i.y, i.z))
        self.tag = tag
        self.segm = segm
        self.maxZ = maxZ
        if PathGeom.pointsCoincide(edge.valueAt(edge.FirstParameter), i):
            tail = edge
            self.commands = []
            debugEdge(tail, '.........=')
        elif PathGeom.pointsCoincide(edge.valueAt(edge.LastParameter), i):
            debugEdge(edge, '++++++++ .')
            self.commands = PathGeom.cmdsForEdge(edge, segm=segm)
            tail = None
        else:
            e, tail = PathGeom.splitEdgeAt(edge, i)
            debugEdge(e, '++++++++ .')
            self.commands = PathGeom.cmdsForEdge(e, segm=segm)
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
            self.entryEdges = filter(lambda e: PathGeom.edgeConnectsTo(e, self.realEntry), edges)
            edges.append(Part.Edge(Part.LineSegment(self.entry, self.realEntry)))
        else:
            self.realEntry = None
        if not self.exitEdges:
            print("fill exitEdges ...")
            self.realExit = sorted(self.edgePoints, key=lambda p: (p - self.exit).Length)[0]
            self.exitEdges = filter(lambda e: PathGeom.edgeConnectsTo(e, self.realExit), edges)
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

    def orderAndFlipEdges(self, edges):
        PathLog.track("entry(%.2f, %.2f, %.2f), exit(%.2f, %.2f, %.2f)" % (self.entry.x, self.entry.y, self.entry.z, self.exit.x, self.exit.y, self.exit.z))
        self.edgesOrder = []
        outputEdges = []
        p0 = self.entry
        lastP = p0
        while edges:
            #print("(%.2f, %.2f, %.2f) %d %d" % (p0.x, p0.y, p0.z))
            for e in edges:
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
                    outputEdges.append((e, True))
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
        return shell.removeShape(filter(lambda f: PathGeom.isRoughly(f.Area, 0), shell.Faces))

    def commandsForEdges(self):
        if self.edges:
            shape = self.shell().common(self.tag.solid)
            commands = []
            rapid = None
            for e,flip in self.orderAndFlipEdges(self.cleanupEdges(shape.Edges)):
                debugEdge(e, '++++++++ %s' % ('<' if flip else '>'), False)
                p1 = e.valueAt(e.FirstParameter)
                p2 = e.valueAt(e.LastParameter)
                if self.tag.isSquare and (PathGeom.isRoughly(p1.z, self.maxZ) or p1.z > self.maxZ) and (PathGeom.isRoughly(p2.z, self.maxZ) or p2.z > self.maxZ):
                    rapid = p1 if flip else p2
                else:
                    if rapid:
                        commands.append(Path.Command('G0', {'X': rapid.x, 'Y': rapid.y, 'Z': rapid.z}))
                        rapid = None
                    commands.extend(PathGeom.cmdsForEdge(e, flip, False, self.segm))
            if rapid:
                commands.append(Path.Command('G0', {'X': rapid.x, 'Y': rapid.y, 'Z': rapid.z}))
                rapid = None
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
        self.base = self.findBottomWire(self.edges)
        # determine overall length
        self.length = self.base.Length

    def findBottomWire(self, edges):
        (minZ, maxZ) = self.findZLimits(edges)
        self.minZ = minZ
        self.maxZ = maxZ
        bottom = [e for e in edges if PathGeom.isRoughly(e.Vertexes[0].Point.z, minZ) and PathGeom.isRoughly(e.Vertexes[1].Point.z, minZ)]
        wire = Part.Wire(bottom)
        if wire.isClosed():
            return wire
        # if we get here there are already holding tags, or we're not looking at a profile
        # let's try and insert the missing pieces - another day
        raise ValueError("Selected path doesn't seem to be a Profile operation.")

    def findZLimits(self, edges):
        # not considering arcs and spheres in Z direction, find the highes and lowest Z values
        minZ =  99999999999
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
        edges = sorted(self.base.Edges, key=lambda e: e.Length)
        return (edges[0], edges[-1])

    def generateTags(self, obj, count, width=None, height=None, angle=None, radius=None, spacing=None):
        PathLog.track(count, width, height, angle, spacing)
        #for e in self.base.Edges:
        #    debugMarker(e.Vertexes[0].Point, 'base', (0.0, 1.0, 1.0), 0.2)

        if spacing:
            tagDistance = spacing
        else:
            tagDistance = self.base.Length / (count if count else 4)

        W = width if width else self.defaultTagWidth()
        H = height if height else self.defaultTagHeight()
        A = angle if angle else self.defaultTagAngle()
        R = radius if radius else self.defaultTagRadius()


        # start assigning tags on the longest segment
        (shortestEdge, longestEdge) = self.shortestAndLongestPathEdge()
        startIndex = 0
        for i in range(0, len(self.base.Edges)):
            edge = self.base.Edges[i]
            PathLog.debug('  %d: %.2f' % (i, edge.Length))
            if edge.Length == longestEdge.Length:
                startIndex = i
                break

        startEdge = self.base.Edges[startIndex]
        startCount = int(startEdge.Length / tagDistance)
        if (longestEdge.Length - shortestEdge.Length) > shortestEdge.Length:
            startCount = int(startEdge.Length / tagDistance) + 1

        lastTagLength = (startEdge.Length + (startCount - 1) * tagDistance) / 2
        currentLength = startEdge.Length

        minLength = min(2. * W, longestEdge.Length)

        PathLog.debug("length=%.2f shortestEdge=%.2f(%.2f) longestEdge=%.2f(%.2f) minLength=%.2f" % (self.base.Length, shortestEdge.Length, shortestEdge.Length/self.base.Length, longestEdge.Length, longestEdge.Length / self.base.Length, minLength))
        PathLog.debug("   start: index=%-2d count=%d (length=%.2f, distance=%.2f)" % (startIndex, startCount, startEdge.Length, tagDistance))
        PathLog.debug("               -> lastTagLength=%.2f)" % lastTagLength)
        PathLog.debug("               -> currentLength=%.2f)" % currentLength)

        edgeDict = { startIndex: startCount }

        for i in range(startIndex + 1, len(self.base.Edges)):
            edge = self.base.Edges[i]
            (currentLength, lastTagLength) = self.processEdge(i, edge, currentLength, lastTagLength, tagDistance, minLength, edgeDict)
        for i in range(0, startIndex):
            edge = self.base.Edges[i]
            (currentLength, lastTagLength) = self.processEdge(i, edge, currentLength, lastTagLength, tagDistance, minLength, edgeDict)

        tags = []

        for (i, count) in edgeDict.iteritems():
            edge = self.base.Edges[i]
            PathLog.debug(" %d: %d" % (i, count))
            #debugMarker(edge.Vertexes[0].Point, 'base', (1.0, 0.0, 0.0), 0.2)
            #debugMarker(edge.Vertexes[1].Point, 'base', (0.0, 1.0, 0.0), 0.2)
            if 0 != count:
                distance = (edge.LastParameter - edge.FirstParameter) / count
                for j in range(0, count):
                    tag = edge.Curve.value((j+0.5) * distance)
                    tags.append(Tag(tag.x, tag.y, W, H, A, R, True))

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
        height = HoldingTagsPreferences.defaultHeight(pathHeight / 2)
        if height > pathHeight:
            return pathHeight
        return height

    def defaultTagWidth(self):
        width = self.shortestAndLongestPathEdge()[1].Length / 10
        return HoldingTagsPreferences.defaultWidth(width)

    def defaultTagAngle(self):
        return HoldingTagsPreferences.defaultAngle()

    def defaultTagRadius(self):
        return HoldingTagsPreferences.defaultRadius()

    def sortedTags(self, tags):
        ordered = []
        for edge in self.base.Edges:
            ts = [t for t in tags if DraftGeomUtils.isPtOnEdge(t.originAt(self.minZ), edge)]
            for t in sorted(ts, key=lambda t: (t.originAt(self.minZ) - edge.valueAt(edge.FirstParameter)).Length):
                tags.remove(t)
                ordered.append(t)
        # disable all tags that are not on the base wire.
        for tag in tags:
            PathLog.notice("Tag #%d not on base wire - disabling\n" % len(ordered))
            tag.enabled = False
            ordered.append(tag)
        return ordered

    def pointIsOnPath(self, p):
        for e in self.edges:
            if DraftGeomUtils.isPtOnEdge(p, e):
                return True
        return False


class ObjectDressup:

    def __init__(self, obj):
        self.obj = obj
        obj.addProperty("App::PropertyLink", "ToolController", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))
        obj.addProperty("App::PropertyLink", "Base","Base", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "The base path to modify"))
        obj.addProperty("App::PropertyLength", "Width", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Width of tags."))
        obj.addProperty("App::PropertyLength", "Height", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Height of tags."))
        obj.addProperty("App::PropertyAngle", "Angle", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Angle of tag plunge and ascent."))
        obj.addProperty("App::PropertyLength", "Radius", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Radius of the fillet for the tag."))
        obj.addProperty("App::PropertyVectorList", "Positions", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Locations of insterted holding tags"))
        obj.addProperty("App::PropertyIntegerList", "Disabled", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Ids of disabled holding tags"))
        obj.addProperty("App::PropertyInteger", "SegmentationFactor", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Factor determining the # segments used to approximate rounded tags."))
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def generateTags(self, obj, count):
        if hasattr(self, "pathData"):
            self.tags = self.pathData.generateTags(obj, count, obj.Width.Value, obj.Height.Value, obj.Angle, obj.Radius.Value, None)
            obj.Positions = [tag.originAt(self.pathData.minZ) for tag in self.tags]
            obj.Disabled  = []
            return False
        else:
            self.setup(obj, count)
            self.execute(obj)
            return True

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
        sameTag = None
        t = 0
        inters = None
        edge = None

        segm = 50
        if hasattr(obj, 'SegmentationFactor'):
            segm = obj.SegmentationFactor
            if segm <= 0:
                segm = 50
                obj.SegmentationFactor = 50

        self.mappers = []
        mapper = None

        while edge or lastEdge < len(pathData.edges):
            PathLog.debug("------- lastEdge = %d/%d.%d/%d" % (lastEdge, lastTag, t, len(tags)))
            if not edge:
                edge = pathData.edges[lastEdge]
                debugEdge(edge, "=======  new edge: %d/%d" % (lastEdge, len(pathData.edges)))
                lastEdge += 1
                sameTag = None

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
                    mapper = MapWireToTag(edge, tags[tIndex], i, segm, pathData.maxZ)
                    self.mappers.append(mapper)
                    edge = mapper.tail


            if not mapper and t >= len(tags):
                # gone through all tags, consume edge and move on
                if edge:
                    debugEdge(edge, '++++++++')
                    if pathData.rapid.isRapid(edge):
                        v = edge.Vertexes[1]
                        commands.append(Path.Command('G0', {'X': v.X, 'Y': v.Y, 'Z': v.Z}))
                    else:
                        commands.extend(PathGeom.cmdsForEdge(edge, segm=segm))
                edge = None
                t = 0

        lastCmd = Path.Command('G0', {'X': 0.0, 'Y': 0.0, 'Z': 0.0});
        outCommands = []

        horizFeed = obj.ToolController.HorizFeed.Value
        vertFeed = obj.ToolController.VertFeed.Value
        horizRapid = obj.ToolController.HorizRapid.Value
        vertRapid = obj.ToolController.VertRapid.Value

        for cmd in commands:
            params = cmd.Parameters
            zVal = params.get('Z', None)
            zVal2 = lastCmd.Parameters.get('Z', None)

            zVal = zVal and round(zVal, 8)
            zVal2 = zVal2 and round(zVal2, 8)

            if cmd.Name in ['G1', 'G2', 'G3', 'G01', 'G02', 'G03']:
                if zVal is not None and zVal2 != zVal:
                    params['F'] = vertFeed
                else:
                    params['F'] = horizFeed
                lastCmd = cmd

            elif cmd.Name in ['G0', 'G00']:
                if zVal is not None and zVal2 != zVal:
                    params['F'] = vertRapid
                else:
                    params['F'] = horizRapid
                lastCmd = cmd

            outCommands.append(Path.Command(cmd.Name, params))
            
        return Path.Path(outCommands)

    def problems(self):
        return filter(lambda m: m.haveProblem, self.mappers)

    def createTagsPositionDisabled(self, obj, positionsIn, disabledIn):
        rawTags = []
        for i, pos in enumerate(positionsIn):
            tag = Tag(pos.x, pos.y, obj.Width.Value, obj.Height.Value, obj.Angle, obj.Radius, not i in disabledIn)
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
                        PathLog.notice("Tag #%d intersects with previous tag - disabling\n" % i)
                        PathLog.debug("this tag = %d [%s]" % (i, tag.solid.BoundBox))
                        tag.enabled = False
                elif self.pathData.edges:
                    e = self.pathData.edges[0]
                    p0 = e.valueAt(e.FirstParameter)
                    p1 = e.valueAt(e.LastParameter)
                    if tag.solid.isInside(p0, PathGeom.Tolerance, True) or tag.solid.isInside(p1, PathGeom.Tolerance, True):
                        PathLog.notice("Tag #%d intersects with starting point - disabling\n" % i)
                        tag.enabled = False
            if tag.enabled:
                prev = tag
                PathLog.debug("previousTag = %d [%s]" % (i, prev))
            else:
                disabled.append(i)
            tags.append(tag)
            positions.append(tag.originAt(self.pathData.minZ))
        return (tags, positions, disabled)

    def execute(self, obj):
        #import cProfile
        #pr = cProfile.Profile()
        #pr.enable()
        self.doExecute(obj)
        #pr.disable()
        #pr.print_stats()

    def doExecute(self,obj):
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

        self.processTags(obj)

    def processTags(self, obj):
        tagID = 0
        if PathLog.getLevel(LOG_MODULE) == PathLog.Level.DEBUG:
            for tag in self.tags:
                tagID += 1
                if tag.enabled:
                    PathLog.debug("x=%s, y=%s, z=%s" % (tag.x, tag.y, self.pathData.minZ))
                    #debugMarker(FreeCAD.Vector(tag.x, tag.y, self.pathData.minZ), "tag-%02d" % tagID , (1.0, 0.0, 1.0), 0.5)
                    #if tag.angle != 90:
                    #    debugCone(tag.originAt(self.pathData.minZ), tag.r1, tag.r2, tag.actualHeight, "tag-%02d" % tagID)
                    #else:
                    #    debugCylinder(tag.originAt(self.pathData.minZ), tag.fullWidth()/2, tag.actualHeight, "tag-%02d" % tagID)

        obj.Path = self.createPath(obj, self.pathData, self.tags)

    def setup(self, obj, generate=False):
        PathLog.debug("setup")
        self.obj = obj
        try:
            pathData = PathData(obj)
        except ValueError:
            PathLog.error(translate("Cannot insert holding tags for this path - please select a Profile path\n"))
            return None

        self.toolRadius = 5
        # toolLoad = PathUtils.getLastToolLoad(obj)
        # if toolLoad is None or toolLoad.ToolNumber == 0:
        #     self.toolRadius = 5
        # else:
        #     tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
        #     if not tool or tool.Diameter == 0:
        #         self.toolRadius = 5
        #     else:
        #         self.toolRadius = tool.Diameter / 2
        toolLoad = obj.ToolController
        if toolLoad is None or toolLoad.ToolNumber == 0:
            PathLog.error(translate("No Tool Controller is selected. We need a tool to build a Path\n"))
            #return
        else:
            # self.vertFeed = toolLoad.VertFeed.Value
            # self.horizFeed = toolLoad.HorizFeed.Value
            # self.vertRapid = toolLoad.VertRapid.Value
            # self.horizRapid = toolLoad.HorizRapid.Value
            tool = toolLoad.Proxy.getTool(toolLoad)
            if not tool or tool.Diameter == 0:
                PathLog.error(translate("No Tool found or diameter is zero. We need a tool to build a Path.\n"))
                return
            else:
                self.toolRadius = tool.Diameter/2

        self.pathData = pathData
        if generate:
            obj.Height = self.pathData.defaultTagHeight()
            obj.Width  = self.pathData.defaultTagWidth()
            obj.Angle  = self.pathData.defaultTagAngle()
            obj.Radius = self.pathData.defaultTagRadius()
            count = HoldingTagsPreferences.defaultCount()
            self.generateTags(obj, count)
        return self.pathData

    def setXyEnabled(self, triples):
        PathLog.track()
        if not hasattr(self, 'pathData'):
            self.setup(self.obj)
        positions = []
        disabled = []
        for i, (x, y, enabled) in enumerate(triples):
            #print("%d: (%.2f, %.2f) %d" % (i, x, y, enabled))
            positions.append(FreeCAD.Vector(x, y, 0))
            if not enabled:
                disabled.append(i)
        self.tags, self.obj.Positions, self.obj.Disabled = self.createTagsPositionDisabled(self.obj, positions, disabled)
        self.processTags(self.obj)

    def pointIsOnPath(self, obj, point):
        if not hasattr(self, 'pathData'):
            self.setup(obj)
        return self.pathData.pointIsOnPath(point)

    @classmethod
    def preferencesPage(cls):
        return HoldingTagsPreferences()

PathPreferencesPathDressup.RegisterDressup(ObjectDressup)

class TaskPanel:
    DataX = QtCore.Qt.ItemDataRole.UserRole
    DataY = QtCore.Qt.ItemDataRole.UserRole + 1
    DataZ = QtCore.Qt.ItemDataRole.UserRole + 2
    DataID = QtCore.Qt.ItemDataRole.UserRole + 3

    def __init__(self, obj, viewProvider, jvoVisibility=None):
        self.obj = obj
        self.obj.Proxy.obj = obj
        self.viewProvider = viewProvider
        self.form = QtGui.QWidget()
        self.formTags  = FreeCADGui.PySideUic.loadUi(":/panels/HoldingTagsEdit.ui")
        self.formPoint = FreeCADGui.PySideUic.loadUi(":/panels/PointEdit.ui")
        self.layout = QtGui.QVBoxLayout(self.form)
        #self.form.setGeometry(self.formTags.geometry())
        self.form.setWindowTitle(self.formTags.windowTitle())
        self.form.setSizePolicy(self.formTags.sizePolicy())
        self.formTags.setParent(self.form)
        self.formPoint.setParent(self.form)
        self.layout.addWidget(self.formTags)
        self.layout.addWidget(self.formPoint)
        self.formPoint.hide()
        self.jvo = PathUtils.findParentJob(obj).ViewObject
        if jvoVisibility is None:
            FreeCAD.ActiveDocument.openTransaction(translate("Edit HoldingTags Dress-up"))
            self.jvoVisible = self.jvo.isVisible()
            if self.jvoVisible:
                self.jvo.hide()
        else:
            self.jvoVisible = jvoVisibility
        self.pt = FreeCAD.Vector(0, 0, 0)

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        self.cleanup()

    def accept(self):
        self.getFields()
        FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup()
        FreeCAD.ActiveDocument.recompute()

    def cleanup(self):
        self.removeGlobalCallbacks()
        self.viewProvider.clearTaskPanel()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        if self.jvoVisible:
            self.jvo.show()

    def getTags(self, includeCurrent):
        tags = []
        index = self.formTags.lwTags.currentRow()
        for i in range(0, self.formTags.lwTags.count()):
            item = self.formTags.lwTags.item(i)
            enabled = item.checkState() == QtCore.Qt.CheckState.Checked
            x = item.data(self.DataX)
            y = item.data(self.DataY)
            #print("(%.2f, %.2f) i=%d/%s" % (x, y, i, index))
            if includeCurrent or i != index:
                tags.append((x, y, enabled))
        return tags

    def getTagParameters(self):
        self.obj.Width  = FreeCAD.Units.Quantity(self.formTags.ifWidth.text()).Value
        self.obj.Height = FreeCAD.Units.Quantity(self.formTags.ifHeight.text()).Value
        self.obj.Angle  = self.formTags.dsbAngle.value()
        self.obj.Radius = FreeCAD.Units.Quantity(self.formTags.ifRadius.text()).Value

    def getFields(self):
        self.getTagParameters()
        tags = self.getTags(True)
        self.obj.Proxy.setXyEnabled(tags)

    def updateTagsView(self):
        PathLog.track()
        self.formTags.lwTags.blockSignals(True)
        self.formTags.lwTags.clear()
        for i, pos in enumerate(self.obj.Positions):
            lbl = "%d: (%.2f, %.2f)" % (i, pos.x, pos.y)
            item = QtGui.QListWidgetItem(lbl)
            item.setData(self.DataX, pos.x)
            item.setData(self.DataY, pos.y)
            item.setData(self.DataZ, pos.z)
            item.setData(self.DataID, i)
            if i in self.obj.Disabled:
                item.setCheckState(QtCore.Qt.CheckState.Unchecked)
            else:
                item.setCheckState(QtCore.Qt.CheckState.Checked)
            flags = QtCore.Qt.ItemFlag.ItemIsSelectable
            flags |= QtCore.Qt.ItemFlag.ItemIsEnabled
            flags |= QtCore.Qt.ItemFlag.ItemIsUserCheckable
            item.setFlags(flags)
            self.formTags.lwTags.addItem(item)
        self.formTags.lwTags.blockSignals(False)
        self.whenTagSelectionChanged()

    def generateNewTags(self):
        count = self.formTags.sbCount.value()
        if not self.obj.Proxy.generateTags(self.obj, count):
            self.obj.Proxy.execute(self.obj)

        self.updateTagsView()
        #if PathLog.getLevel(LOG_MODULE) == PathLog.Level.DEBUG:
        #    # this causes a big of an echo and a double click on the spin buttons, don't know why though
        #    FreeCAD.ActiveDocument.recompute()


    def updateModel(self):
        self.getFields()
        self.updateTagsView()
        #FreeCAD.ActiveDocument.recompute()

    def whenCountChanged(self):
        count = self.formTags.sbCount.value()
        self.formTags.pbGenerate.setEnabled(count)

    def selectTagWithId(self, index):
        self.formTags.lwTags.setCurrentRow(index)

    def whenTagSelectionChanged(self):
        index = self.formTags.lwTags.currentRow()
        count = self.formTags.lwTags.count()
        self.formTags.pbDelete.setEnabled(index != -1 and count > 2)
        self.formTags.pbEdit.setEnabled(index != -1)
        self.viewProvider.selectTag(index)

    def deleteSelectedTag(self):
        self.obj.Proxy.setXyEnabled(self.getTags(False))
        self.updateTagsView()

    def addNewTagAt(self, point, obj):
        if self.obj.Proxy.pointIsOnPath(self.obj, point):
            #print("addNewTagAt(%s)" % (point))
            tags = self.tags
            tags.append((point.x, point.y, True))
            self.obj.Proxy.setXyEnabled(tags)
            self.updateTagsView()
        else:
            print("ignore new tag at %s" % (point))
        self.formPoint.hide()
        self.formTags.show()

    def addNewTag(self):
        self.tags = self.getTags(True)
        self.getPoint(self.addNewTagAt)

    def editTagAt(self, point, obj):
        if (obj or point != FreeCAD.Vector()) and self.obj.Proxy.pointIsOnPath(self.obj, point):
            tags = []
            for i, (x, y, enabled) in enumerate(self.tags):
                if i == self.editItem:
                    tags.append((point.x, point.y, enabled))
                else:
                    tags.append((x, y, enabled))
            self.obj.Proxy.setXyEnabled(tags)
            self.updateTagsView()
        self.formPoint.hide()
        self.formTags.show()

    def editTag(self, item):
        if item:
            self.tags = self.getTags(True)
            self.editItem = item.data(self.DataID)
            x = item.data(self.DataX)
            y = item.data(self.DataY)
            z = item.data(self.DataZ)
            self.getPoint(self.editTagAt, FreeCAD.Vector(x, y, z))

    def editSelectedTag(self):
        self.editTag(self.formTags.lwTags.currentItem())

    def removeGlobalCallbacks(self):
        if hasattr(self, 'view') and self.view:
            if self.pointCbClick:
                self.view.removeEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), self.pointCbClick)
                self.pointCbClick = None
            if self.pointCbMove:
                self.view.removeEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), self.pointCbMove)
                self.pointCbMove = None
            self.view = None

    def getPoint(self, whenDone, start=None):

        def displayPoint(p):
            self.formPoint.ifValueX.setText(FreeCAD.Units.Quantity(p.x, FreeCAD.Units.Length).UserString)
            self.formPoint.ifValueY.setText(FreeCAD.Units.Quantity(p.y, FreeCAD.Units.Length).UserString)
            self.formPoint.ifValueZ.setText(FreeCAD.Units.Quantity(p.z, FreeCAD.Units.Length).UserString)
            self.formPoint.ifValueX.setFocus()
            self.formPoint.ifValueX.selectAll()

        def mouseMove(cb):
            event = cb.getEvent()
            pos = event.getPosition()
            cntrl = event.wasCtrlDown()
            shift = event.wasShiftDown()
            self.pt = FreeCADGui.Snapper.snap(pos, lastpoint=start, active=cntrl, constrain=shift)
            plane = FreeCAD.DraftWorkingPlane
            p = plane.getLocalCoords(self.pt)
            displayPoint(p)

        def click(cb):
            event = cb.getEvent()
            if event.getButton() == 1 and event.getState() == coin.SoMouseButtonEvent.DOWN:
                accept()

        def accept():
            self.pointAccept()

        def cancel():
            self.pointCancel()

        self.pointWhenDone = whenDone
        self.formTags.hide()
        self.formPoint.show()
        if start:
            displayPoint(start)
        else:
            displayPoint(FreeCAD.Vector(0,0,0))

        self.view = Draft.get3DView()
        self.pointCbClick = self.view.addEventCallbackPivy(coin.SoMouseButtonEvent.getClassTypeId(), click)
        self.pointCbMove = self.view.addEventCallbackPivy(coin.SoLocation2Event.getClassTypeId(), mouseMove)

    def setupSpinBox(self, widget, val, decimals = 2):
        if decimals:
            widget.setDecimals(decimals)
        widget.setValue(val)

    def setFields(self):
        self.updateTagsView()
        self.formTags.sbCount.setValue(len(self.obj.Positions))
        self.formTags.ifHeight.setText(FreeCAD.Units.Quantity(self.obj.Height, FreeCAD.Units.Length).UserString)
        self.formTags.ifWidth.setText(FreeCAD.Units.Quantity(self.obj.Width, FreeCAD.Units.Length).UserString)
        self.formTags.dsbAngle.setValue(self.obj.Angle)
        self.formTags.ifRadius.setText(FreeCAD.Units.Quantity(self.obj.Radius, FreeCAD.Units.Length).UserString)

    def setupUi(self):
        self.setFields()
        self.whenCountChanged()

        self.formTags.sbCount.valueChanged.connect(self.whenCountChanged)
        self.formTags.pbGenerate.clicked.connect(self.generateNewTags)

        self.formTags.ifHeight.editingFinished.connect(self.updateModel)
        self.formTags.ifWidth.editingFinished.connect(self.updateModel)
        self.formTags.dsbAngle.editingFinished.connect(self.updateModel)
        self.formTags.ifRadius.editingFinished.connect(self.updateModel)
        self.formTags.lwTags.itemChanged.connect(self.updateModel)
        self.formTags.lwTags.itemSelectionChanged.connect(self.whenTagSelectionChanged)
        self.formTags.lwTags.itemActivated.connect(self.editTag)

        self.formTags.pbDelete.clicked.connect(self.deleteSelectedTag)
        self.formTags.pbEdit.clicked.connect(self.editSelectedTag)
        self.formTags.pbAdd.clicked.connect(self.addNewTag)

        self.formPoint.buttonBox.accepted.connect(self.pointAccept)
        self.formPoint.ifValueX.editingFinished.connect(self.updatePoint)
        self.formPoint.ifValueY.editingFinished.connect(self.updatePoint)
        self.formPoint.ifValueZ.editingFinished.connect(self.updatePoint)

        self.viewProvider.turnMarkerDisplayOn(True)

    def pointFinish(self, ok):
        self.removeGlobalCallbacks();
        obj = FreeCADGui.Snapper.lastSnappedObject
        FreeCADGui.Snapper.off()
        self.pointWhenDone(self.pt if ok else None, obj if ok else None)

    def pointReject(self):
        self.pointFinish(False)

    def pointAccept(self):
        self.pointFinish(True)

    def updatePoint(self):
        x = FreeCAD.Units.Quantity(self.formPoint.ifValueX.text()).Value
        y = FreeCAD.Units.Quantity(self.formPoint.ifValueY.text()).Value
        z = FreeCAD.Units.Quantity(self.formPoint.ifValueZ.text()).Value
        self.pt = FreeCAD.Vector(x, y, z)

class HoldingTagMarker:
    def __init__(self, point, colors):
        self.point = point
        self.color = colors
        self.sep = coin.SoSeparator()
        self.pos = coin.SoTranslation()
        self.pos.translation = (point.x, point.y, point.z)
        self.sphere = coin.SoSphere()
        self.material = coin.SoMaterial()
        self.sep.addChild(self.pos)
        self.sep.addChild(self.material)
        self.sep.addChild(self.sphere)
        self.enabled = True
        self.selected = False

    def setSelected(self, select):
        self.selected = select
        self.sphere.radius = 1.5 if select else 1.0
        self.setEnabled(self.enabled)

    def setEnabled(self, enabled):
        self.enabled = enabled
        if enabled:
            self.material.diffuseColor = self.color[0] if not self.selected else self.color[2]
            self.material.transparency = 0.0
        else:
            self.material.diffuseColor = self.color[1] if not self.selected else self.color[2]
            self.material.transparency = 0.6

class ViewProviderDressup:

    def __init__(self, vobj):
        vobj.Proxy = self

    def setupColors(self):
        def colorForColorValue(val):
            v = [((val >> n) & 0xff) / 255. for n in [24, 16, 8, 0]]
            return coin.SbColor(v[0], v[1], v[2])

        pref = PathPreferences.preferences()
        #                                                      R         G          B          A
        npc = pref.GetUnsigned("DefaultPathMarkerColor",    (( 85*256 + 255)*256 +   0)*256 + 255)
        hpc = pref.GetUnsigned("DefaultHighlightPathColor", ((255*256 + 125)*256 +   0)*256 + 255)
        dpc = pref.GetUnsigned("DefaultDisabledPathColor",  ((205*256 + 205)*256 + 205)*256 + 154)
        self.colors = [colorForColorValue(npc), colorForColorValue(dpc), colorForColorValue(hpc)]

    def attach(self, vobj):
        self.setupColors()
        self.obj = vobj.Object
        self.tags = []
        self.switch = coin.SoSwitch()
        vobj.RootNode.addChild(self.switch)
        self.turnMarkerDisplayOn(False)

    def turnMarkerDisplayOn(self, display):
        sw = coin.SO_SWITCH_ALL if display else coin.SO_SWITCH_NONE
        self.switch.whichChild = sw


    def claimChildren(self):
        for i in self.obj.Base.InList:
            if hasattr(i, "Group"):
                group = i.Group
                for g in group:
                    if g.Name == self.obj.Base.Name:
                        group.remove(g)
                i.Group = group
                #print i.Group
        #FreeCADGui.ActiveDocument.getObject(obj.Base.Name).Visibility = False
        return [self.obj.Base]

    def setEdit(self, vobj, mode=0):
        panel = TaskPanel(vobj.Object, self)
        self.setupTaskPanel(panel)
        return True

    def setupTaskPanel(self, panel):
        self.panel = panel
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(panel)
        panel.setupUi()
        FreeCADGui.Selection.addSelectionGate(self)
        FreeCADGui.Selection.addObserver(self)

    def clearTaskPanel(self):
        self.panel = None
        FreeCADGui.Selection.removeObserver(self)
        FreeCADGui.Selection.removeSelectionGate()
        self.turnMarkerDisplayOn(False)

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onDelete(self, arg1=None, arg2=None):
        '''this makes sure that the base operation is added back to the project and visible'''
        FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
        PathUtils.addToJob(arg1.Object.Base)
        return True

    def updateData(self, obj, propName):
        if 'Disabled' == propName:
            for tag in self.tags:
                self.switch.removeChild(tag.sep)
            tags = []
            for i, p in enumerate(obj.Positions):
                tag = HoldingTagMarker(p, self.colors)
                tag.setEnabled(not i in obj.Disabled)
                tags.append(tag)
                self.switch.addChild(tag.sep)
            self.tags = tags

    def selectTag(self, index):
        PathLog.track(index)
        for i, tag in enumerate(self.tags):
            tag.setSelected(i == index)

    def tagAtPoint(self, point):
        p = FreeCAD.Vector(point[0], point[1], point[2])
        for i, tag in enumerate(self.tags):
            if PathGeom.pointsCoincide(p, tag.point, tag.sphere.radius.getValue() * 1.1):
                return i
        return -1

    # SelectionObserver interface
    def allow(self, doc, obj, sub):
        if obj == self.obj:
            return True
        return False

    def addSelection(self, doc, obj, sub, point):
        i = self.tagAtPoint(point)
        if self.panel:
            self.panel.selectTagWithId(i)
        FreeCADGui.updateGui()

class CommandPathDressupHoldingTags:

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "HoldingTags Dress-up"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Creates a HoldingTags Dress-up object from a selected path")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            PathLog.error(translate("Please select one path object\n"))
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            PathLog.error(translate("The selected object is not a path\n"))
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            PathLog.error(translate("Please select a Profile object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate("Create HoldingTags Dress-up"))
        FreeCADGui.addModule("PathScripts.PathDressupHoldingTags")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "HoldingTagsDressup")')
        FreeCADGui.doCommand('dbo = PathScripts.PathDressupHoldingTags.ObjectDressup(obj)')
        FreeCADGui.doCommand('obj.Base = FreeCAD.ActiveDocument.' + selection[0].Name)
        FreeCADGui.doCommand('PathScripts.PathDressupHoldingTags.ViewProviderDressup(obj.ViewObject)')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('Gui.ActiveDocument.getObject(obj.Base.Name).Visibility = False')
        FreeCADGui.doCommand('obj.ToolController = PathScripts.PathUtils.findToolController(obj)')
        FreeCADGui.doCommand('dbo.setup(obj, True)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('PathDressup_HoldingTags', CommandPathDressupHoldingTags())

PathLog.notice("Loading PathDressupHoldingTags... done\n")
