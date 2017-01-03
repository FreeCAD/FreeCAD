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
import DraftGeomUtils
import Path
import Part
import copy
import math

import cProfile
import time

from DraftGui import todo
from PathScripts import PathUtils
from PathScripts.PathGeom import *
from PySide import QtCore, QtGui

"""Holding Tags Dressup object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)

except AttributeError:

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

debugDressup = False

def debugPrint(msg):
    if debugDressup:
        print(msg)

def debugEdge(edge, prefix, force = False):
    pf = edge.valueAt(edge.FirstParameter)
    pl = edge.valueAt(edge.LastParameter)
    if force or debugDressup:
        if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
            print("%s %s((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (prefix, type(edge.Curve), pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))
        else:
            pm = edge.valueAt((edge.FirstParameter+edge.LastParameter)/2)
            print("%s %s((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (prefix, type(edge.Curve), pf.x, pf.y, pf.z, pm.x, pm.y, pm.z, pl.x, pl.y, pl.z))

def debugMarker(vector, label, color = None, radius = 0.5):
    if debugDressup:
        obj = FreeCAD.ActiveDocument.addObject("Part::Sphere", label)
        obj.Label = label
        obj.Radius = radius
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        if color:
            obj.ViewObject.ShapeColor = color

def debugCylinder(vector, r, height, label, color = None):
    if debugDressup:
        obj = FreeCAD.ActiveDocument.addObject("Part::Cylinder", label)
        obj.Label = label
        obj.Radius = r
        obj.Height = height
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        obj.ViewObject.Transparency = 90
        if color:
            obj.ViewObject.ShapeColor = color

def debugCone(vector, r1, r2, height, label, color = None):
    if debugDressup:
        obj = FreeCAD.ActiveDocument.addObject("Part::Cone", label)
        obj.Label = label
        obj.Radius1 = r1
        obj.Radius2 = r2
        obj.Height = height
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        obj.ViewObject.Transparency = 90
        if color:
            obj.ViewObject.ShapeColor = color

class Tag:

    @classmethod
    def FromString(cls, string):
        try:
            t = eval(string)
            return Tag(t[0], t[1], t[2], t[3], t[4], t[5])
        except:
            return None

    def toString(self):
        return str((self.x, self.y, self.width, self.height, self.angle, self.enabled))

    def __init__(self, x, y, width, height, angle, enabled=True):
        debugPrint("Tag(%.2f, %.2f, %.2f, %.2f, %.2f, %d)" % (x, y, width, height, angle/math.pi, enabled))
        self.x = x
        self.y = y
        self.width = math.fabs(width)
        self.height = math.fabs(height)
        self.actualHeight = self.height
        self.angle = math.fabs(angle)
        self.enabled = enabled

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
        height = self.height
        if self.angle == 90 and height > 0:
            self.solid = Part.makeCylinder(r1, height)
            debugPrint("Part.makeCone(%f, %f)" % (r1, height))
        elif self.angle > 0.0 and height > 0.0:
            tangens = math.tan(math.radians(self.angle))
            dr = height / tangens
            if dr < r1:
                r2 = r1 - dr
            else:
                r2 = 0
                height = r1 * tangens
                self.actualHeight = height
            self.r2 = r2
            debugPrint("Part.makeCone(%f, %f, %f)" % (r1, r2, height))
            self.solid = Part.makeCone(r1, r2, height)
        else:
            # degenerated case - no tag
            debugPrint("Part.makeSphere(%f / 10000)" % (r1))
            self.solid = Part.makeSphere(r1 / 10000)
        if not R == 0: # testing is easier if the solid is not rotated
            angle = -PathGeom.getAngle(self.originAt(0)) * 180 / math.pi
            debugPrint("solid.rotate(%f)" % angle)
            self.solid.rotate(FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,0,1), angle)
        debugPrint("solid.translate(%s)" % self.originAt(z))
        self.solid.translate(self.originAt(z))

    def filterIntersections(self, pts, face):
        if type(face.Surface) == Part.Cone or type(face.Surface) == Part.Cylinder:
            #print("it's a cone/cylinder, checking z")
            return filter(lambda pt: pt.z >= self.bottom() and pt.z <= self.top(), pts)
        if type(face.Surface) == Part.Plane:
            #print("it's a plane, checking R")
            c = face.Edges[0].Curve
            if (type(c) == Part.Circle):
                return filter(lambda pt: (pt - c.Center).Length <= c.Radius or PathGeom.isRoughly((pt - c.Center).Length, c.Radius), pts)
        #print("==== we got a %s" % face.Surface)

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

        pts = []
        for index, face in enumerate(solid.Faces):
            i = edge.Curve.intersect(face.Surface)[0]
            #print i
            ps = self.filterIntersections([FreeCAD.Vector(p.X, p.Y, p.Z) for p in i], face)
            pts.extend(filter(lambda pt: self.isPointOnEdge(pt, edge), ps))
            if len(ps)  != len(filter(lambda pt: self.isPointOnEdge(pt, edge), ps)):
                filtered = filter(lambda pt: self.isPointOnEdge(pt, edge), ps)
                #print("-------- ++ len(ps)=%d, len(filtered)=%d" % (len(ps), len(filtered)))
                for p in ps:
                    included = '+' if p in filtered else '-'
                    #print("--------     %s (%.2f, %.2f, %.2f)" % (included, p.x, p.y, p.z))
        if pts:
            closest = sorted(pts, key=lambda pt: (pt - refPt).Length)[0]
            #for p in pts:
            #    print("-------- - intersect pt : (%.2f, %.2f, %.2f)" % (p.x, p.y, p.z))
            #print("-------- -> (%.2f, %.2f, %.2f)" % (closest.x, closest.y, closest.z))
            return closest
        
        #print("-------- -> None")
        return None

    def intersects(self, edge, param):
        if self.enabled:
            if edge.valueAt(edge.FirstParameter).z < self.top() or edge.valueAt(edge.LastParameter).z < self.top():
                return self.nextIntersectionClosestTo(edge, self.solid, edge.valueAt(param))
        return None

class MapWireToTag:
    def __init__(self, edge, tag, i):
        debugEdge(edge, 'MapWireToTag(%.2f, %.2f, %.2f)' % (i.x, i.y, i.z))
        self.tag = tag
        if PathGeom.pointsCoincide(edge.valueAt(edge.FirstParameter), i):
            tail = edge
            self.commands = []
            debugEdge(tail, '.........=')
        elif PathGeom.pointsCoincide(edge.valueAt(edge.LastParameter), i):
            debugEdge(edge, '++++++++ .')
            self.commands = PathGeom.cmdsForEdge(edge)
            tail = None
        else:
            e, tail = PathGeom.splitEdgeAt(edge, i)
            debugEdge(e, '++++++++ .')
            self.commands = PathGeom.cmdsForEdge(e)
            debugEdge(tail, '.........-')
        self.tail = tail
        self.edges = []
        self.entry = i
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

    def cleanupEdges(self, edges, baseEdge):
        # want to remove all edges from the wire itself, and all internal struts
        #print("+cleanupEdges")
        #print(" base:")
        debugEdge(baseEdge, '   ')
        #print(" edges:")
        for e in edges:
            debugEdge(e, '   ')
        #print(":")

        haveEntry = False
        for e in copy.copy(edges):
            if PathGeom.edgesMatch(e, baseEdge):
                debugEdge(e, '......... X0')
                edges.remove(e)
            elif self.isStrut(e):
                typ = self.isEntryOrExitStrut(e)
                debugEdge(e, '......... |%d' % typ)
                if 0 == typ: # neither entry nor exit
                    debugEdge(e, '......... X1')
                    edges.remove(e)
                elif 1 == typ:
                    haveEntry = True

        #print("entry(%.2f, %.2f, %.2f), exit(%.2f, %.2f, %.2f)" % (self.entry.x, self.entry.y, self.entry.z, self.exit.x, self.exit.y, self.exit.z))
        # the remaininng edges for the path from xy(baseEdge) along the tags surface
        outputEdges = []
        p0 = baseEdge.valueAt(baseEdge.FirstParameter)
        ignoreZ = False
        if not haveEntry:
            ignoreZ = True
            p0 = PathGeom.xy(p0)
        lastP = p0
        while edges:
            #print("(%.2f, %.2f, %.2f) %d %d" % (p0.x, p0.y, p0.z, haveEntry, ignoreZ))
            for e in edges:
                p1 = e.valueAt(e.FirstParameter)
                p2 = e.valueAt(e.LastParameter)
                if PathGeom.pointsCoincide(PathGeom.xy(p1) if ignoreZ else p1, p0):
                    outputEdges.append((e, False))
                    edges.remove(e)
                    lastP = None
                    ignoreZ = False
                    p0 = p2
                    debugEdge(e, ">>>>> no flip")
                    break
                elif PathGeom.pointsCoincide(PathGeom.xy(p2) if ignoreZ else p2, p0):
                    outputEdges.append((e, True))
                    edges.remove(e)
                    lastP = None
                    ignoreZ = False
                    p0 = p1
                    debugEdge(e, ">>>>> flip")
                    break
                else:
                    debugEdge(e, "<<<<< (%.2f, %.2f, %.2f)" % (p0.x, p0.y, p0.z))
            if lastP == p0:
                raise ValueError("No connection to %s" % (p0))
            elif lastP:
                debugPrint("xxxxxx (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)" % (p0.x, p0.y, p0.z, lastP.x, lastP.y, lastP.z))
            else:
                debugPrint("xxxxxx (%.2f, %.2f, %.2f) -" % (p0.x, p0.y, p0.z))
            lastP = p0
        #print("-cleanupEdges")
        return outputEdges

    def isStrut(self, edge):
        p1 = PathGeom.xy(edge.valueAt(edge.FirstParameter))
        p2 = PathGeom.xy(edge.valueAt(edge.LastParameter))
        return PathGeom.pointsCoincide(p1, p2)

    def cmdsForEdge(self, edge):
        cmds = []

        # OCC doesn't like it very much if the shapes align with each other. So if we have a slightly
        # extended edge for the last edge in list we'll use that instead for stable results.
        if PathGeom.pointsCoincide(edge.valueAt(edge.FirstParameter), self.lastEdge.valueAt(self.lastEdge.FirstParameter)):
            shell = self.lastEdge.extrude(FreeCAD.Vector(0, 0, self.tag.height + 1))
        else:
            shell = edge.extrude(FreeCAD.Vector(0, 0, self.tag.height + 1))
        shape = shell.common(self.tag.solid)

        if not shape.Edges:
            self.haveProblem = True

        for e,flip in self.cleanupEdges(shape.Edges, edge):
            debugEdge(e, '++++++++ %s' % ('.' if not flip else '@'))
            cmds.extend(PathGeom.cmdsForEdge(e, flip, False))
        return cmds

    def commandsForEdges(self):
        commands = []
        for e in self.edges:
            if self.isStrut(e):
                continue
            commands.extend(self.cmdsForEdge(e))
        return commands

    def add(self, edge):
        self.tail = None
        self.lastEdge = edge # see cmdsForEdge
        if self.tag.solid.isInside(edge.valueAt(edge.LastParameter), 0.000001, True):
            self.addEdge(edge)
        else:
            i = self.tag.intersects(edge, edge.LastParameter)
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

    def isRapid(self, edge, removeIfFound=True):
        if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
            v0 = edge.Vertexes[0]
            v1 = edge.Vertexes[1]
            for r in self.rapid:
                r0 = r.Vertexes[0]
                r1 = r.Vertexes[1]
                if PathGeom.isRoughly(r0.X, v0.X) and PathGeom.isRoughly(r0.Y, v0.Y) and PathGeom.isRoughly(r0.Z, v0.Z) and PathGeom.isRoughly(r1.X, v1.X) and PathGeom.isRoughly(r1.Y, v1.Y) and PathGeom.isRoughly(r1.Z, v1.Z):
                    if removeIfFound:
                        self.rapid.remove(r)
                    return True
        return False

    def p(self):
        print('rapid:')
        for r in self.rapid:
            debugEdge(r, '  ', True)

class PathData:
    def __init__(self, obj):
        debugPrint("PathData(%s)" % obj.Base.Name)
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
        bottom = [e for e in edges if e.Vertexes[0].Point.z == minZ and e.Vertexes[1].Point.z == minZ]
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

    def generateTags(self, obj, count=None, width=None, height=None, angle=90, spacing=None):
        debugPrint("generateTags(%s, %s, %s, %s, %s)" % (count, width, height, angle, spacing))
        #for e in self.base.Edges:
        #    debugMarker(e.Vertexes[0].Point, 'base', (0.0, 1.0, 1.0), 0.2)

        if spacing:
            tagDistance = spacing
        else:
            if count:
                tagDistance = self.base.Length / count
            else:
                tagDistance = self.base.Length / 4
        if width:
            W = width
        else:
            W = self.defaultTagWidth()
        if height:
            H = height
        else:
            H = self.defaultTagHeight()


        # start assigning tags on the longest segment
        (shortestEdge, longestEdge) = self.shortestAndLongestPathEdge()
        startIndex = 0
        for i in range(0, len(self.base.Edges)):
            edge = self.base.Edges[i]
            debugPrint('  %d: %.2f' % (i, edge.Length))
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

        debugPrint("length=%.2f shortestEdge=%.2f(%.2f) longestEdge=%.2f(%.2f) minLength=%.2f" % (self.base.Length, shortestEdge.Length, shortestEdge.Length/self.base.Length, longestEdge.Length, longestEdge.Length / self.base.Length, minLength))
        debugPrint("   start: index=%-2d count=%d (length=%.2f, distance=%.2f)" % (startIndex, startCount, startEdge.Length, tagDistance))
        debugPrint("               -> lastTagLength=%.2f)" % lastTagLength)
        debugPrint("               -> currentLength=%.2f)" % currentLength)

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
            debugPrint(" %d: %d" % (i, count))
            #debugMarker(edge.Vertexes[0].Point, 'base', (1.0, 0.0, 0.0), 0.2)
            #debugMarker(edge.Vertexes[1].Point, 'base', (0.0, 1.0, 0.0), 0.2)
            if 0 != count:
                distance = (edge.LastParameter - edge.FirstParameter) / count
                for j in range(0, count):
                    tag = edge.Curve.value((j+0.5) * distance)
                    tags.append(Tag(tag.x, tag.y, W, H, angle, True))

        return tags

    def processEdge(self, index, edge, currentLength, lastTagLength, tagDistance, minLength, edgeDict):
        tagCount = 0
        currentLength += edge.Length
        if edge.Length >= minLength:
            while lastTagLength + tagDistance < currentLength:
                tagCount += 1
                lastTagLength += tagDistance
            if tagCount > 0:
                debugPrint("      index=%d -> count=%d" % (index, tagCount))
                edgeDict[index] = tagCount
        else:
            debugPrint("      skipping=%-2d (%.2f)" % (index, edge.Length))

        return (currentLength, lastTagLength)

    def defaultTagHeight(self):
        if hasattr(self.obj, 'Base') and hasattr(self.obj.Base, 'StartDepth') and hasattr(self.obj.Base, 'FinalDepth'):
            return (self.obj.Base.StartDepth - self.obj.Base.FinalDepth).Value / 2
        return (self.maxZ - self.minZ) / 2

    def defaultTagWidth(self):
        return self.shortestAndLongestPathEdge()[1].Length / 10

    def defaultTagAngle(self):
        return 45

    def sortedTags(self, tags):
        ordered = []
        for edge in self.base.Edges:
            ts = [t for t in tags if DraftGeomUtils.isPtOnEdge(t.originAt(self.minZ), edge)]
            for t in sorted(ts, key=lambda t: (t.originAt(self.minZ) - edge.valueAt(edge.FirstParameter)).Length):
                tags.remove(t)
                ordered.append(t)
        if tags:
            raise ValueError("There's something really wrong here")
        return ordered


class ObjectDressup:

    def __init__(self, obj):
        self.obj = obj
        obj.addProperty("App::PropertyLink", "Base","Base", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "The base path to modify"))
        obj.addProperty("App::PropertyFloat", "Width", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Width of tags."))
        obj.addProperty("App::PropertyFloat", "Height", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Height of tags."))
        obj.addProperty("App::PropertyFloat", "Angle", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Angle of tag plunge and ascent."))
        obj.addProperty("App::PropertyVectorList", "Positions", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Locations of insterted holding tags"))
        obj.addProperty("App::PropertyIntegerList", "Disabled", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Ids of disabled holding tags"))
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def generateTags(self, obj, count):
        if hasattr(self, "pathData"):
            self.tags = self.pathData.generateTags(obj, count, obj.Width, obj.Height, obj.Angle, None)
            obj.Positions = [tag.originAt(0) for tag in self.tags]
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

    def createPath(self, edges, tags, rapid):
        print("createPath")
        commands = []
        lastEdge = 0
        lastTag = 0
        sameTag = None
        t = 0
        inters = None
        edge = None

        self.mappers = []
        mapper = None

        while edge or lastEdge < len(edges):
            debugPrint("------- lastEdge = %d/%d.%d/%d" % (lastEdge, lastTag, t, len(tags)))
            if not edge:
                edge = edges[lastEdge]
                debugEdge(edge, "=======  new edge: %d/%d" % (lastEdge, len(edges)))
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
                    mapper = MapWireToTag(edge, tags[tIndex], i)
                    self.mappers.append(mapper)
                    edge = mapper.tail


            if not mapper and t >= len(tags):
                # gone through all tags, consume edge and move on
                if edge:
                    debugEdge(edge, '++++++++')
                    if rapid.isRapid(edge, True):
                        v = edge.Vertexes[1]
                        commands.append(Path.Command('G0', {'X': v.X, 'Y': v.Y, 'Z': v.Z}))
                    else:
                        commands.extend(PathGeom.cmdsForEdge(edge))
                edge = None
                t = 0

        #for cmd in commands:
        #    print(cmd)
        return Path.Path(commands)

    def problems(self):
        return filter(lambda m: m.haveProblem, self.mappers)

    def execute(self, obj):
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
            for i, pos in enumerate(obj.Positions):
                tag = Tag(pos.x, pos.y, obj.Width, obj.Height, obj.Angle, not i in obj.Disabled)
                tag.createSolidsAt(pathData.minZ, self.toolRadius)
                self.tags.append(tag)

        if not self.tags:
            print("execute - no tags")
            obj.Path = obj.Base.Path
            return

        tagID = 0
        if debugDressup:
            for tag in self.tags:
                tagID += 1
                if tag.enabled:
                    print("x=%s, y=%s, z=%s" % (tag.x, tag.y, pathData.minZ))
                    debugMarker(FreeCAD.Vector(tag.x, tag.y, pathData.minZ), "tag-%02d" % tagID , (1.0, 0.0, 1.0), 0.5)
                    if tag.angle != 90:
                        debugCone(tag.originAt(pathData.minZ), tag.r1, tag.r2, tag.actualHeight, "tag-%02d" % tagID)
                    else:
                        debugCylinder(tag.originAt(pathData.minZ), tag.fullWidth()/2, tag.actualHeight, "tag-%02d" % tagID)

        obj.Path = self.createPath(pathData.edges, self.tags, pathData.rapid)
        print("execute - done")

    def setup(self, obj, generate=None):
        print("setup")
        self.obj = obj
        try:
            pathData = PathData(obj)
        except ValueError:
            FreeCAD.Console.PrintError(translate("PathDressup_HoldingTags", "Cannot insert holding tags for this path - please select a Profile path\n"))
            return None

        self.toolRadius = 5
        toolLoad = PathUtils.getLastToolLoad(obj)
        if toolLoad is None or toolLoad.ToolNumber == 0:
            self.toolRadius = 5
        else:
            tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
            if not tool or tool.Diameter == 0:
                self.toolRadius = 5
            else:
                self.toolRadius = tool.Diameter / 2
        self.pathData = pathData
        if generate:
            obj.Height = self.pathData.defaultTagHeight()
            obj.Width  = self.pathData.defaultTagWidth()
            obj.Angle  = self.pathData.defaultTagAngle()
            self.generateTags(obj, generate)
        return self.pathData

    def setXyEnabled(self, triples):
        positions = []
        disabled = []
        for i, (x, y, enabled) in enumerate(triples):
            positions.append(FreeCAD.Vector(x, y, 0))
            if not enabled:
                disabled.append(i)
        self.obj.Positions = positions
        self.obj.Disabled = disabled
        self.execute(self.obj)

class TaskPanel:
    DataX = QtCore.Qt.ItemDataRole.UserRole
    DataY = QtCore.Qt.ItemDataRole.UserRole + 1

    def __init__(self, obj, jvoVisibility=None):
        self.obj = obj
        self.obj.Proxy.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/HoldingTagsEdit.ui")
        self.jvo = PathUtils.findParentJob(obj).ViewObject
        if jvoVisibility is None:
            FreeCAD.ActiveDocument.openTransaction(translate("PathDressup_HoldingTags", "Edit HoldingTags Dress-up"))
            self.jvoVisible = self.jvo.isVisible()
            if self.jvoVisible:
                self.jvo.hide()
        else:
            self.jvoVisible = jvoVisibility

    def reject(self):
        print("reject")
        FreeCAD.ActiveDocument.abortTransaction()
        self.cleanup()

    def accept(self):
        print("accept")
        self.getFields()
        FreeCAD.ActiveDocument.commitTransaction()
        self.cleanup()
        FreeCAD.ActiveDocument.recompute()

    def cleanup(self):
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)
        if self.jvoVisible:
            self.jvo.show()

    def closeDialog(self):
        print("closed")

    def open(self):
        self.s = SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)

    def getTags(self, includeCurrent):
        tags = []
        index = self.form.lwTags.currentRow()
        for i in range(0, self.form.lwTags.count()):
            item = self.form.lwTags.item(i)
            enabled = item.checkState() == QtCore.Qt.CheckState.Checked
            x = item.data(self.DataX)
            y = item.data(self.DataY)
            print("(%.2f, %.2f) i=%d/%s" % (x, y, i, index))
            if includeCurrent or i != index:
                tags.append((x, y, enabled))
        return tags

    def getTagParameters(self):
        self.obj.Width  = self.form.dsbWidth.value()
        self.obj.Height = self.form.dsbHeight.value()
        self.obj.Angle  = self.form.dsbAngle.value()

    def getFields(self):
        self.getTagParameters()
        tags = self.getTags(True)
        self.obj.Proxy.setXyEnabled(tags)

    def updateTagsView(self):
        print("updateTagsView")
        self.form.lwTags.blockSignals(True)
        self.form.lwTags.clear()
        for i, pos in enumerate(self.obj.Positions):
            lbl = "%d: (%.2f, %.2f)" % (i, pos.x, pos.y)
            item = QtGui.QListWidgetItem(lbl)
            item.setData(self.DataX, pos.x)
            item.setData(self.DataY, pos.y)
            if i in self.obj.Disabled:
                item.setCheckState(QtCore.Qt.CheckState.Unchecked)
            else:
                item.setCheckState(QtCore.Qt.CheckState.Checked)
            flags = QtCore.Qt.ItemFlag.ItemIsSelectable
            flags |= QtCore.Qt.ItemFlag.ItemIsEnabled
            flags |= QtCore.Qt.ItemFlag.ItemIsUserCheckable
            item.setFlags(flags)
            self.form.lwTags.addItem(item)
        self.form.lwTags.blockSignals(False)

    def cleanupUI(self):
        print("cleanupUI")
        if debugDressup:
            for obj in FreeCAD.ActiveDocument.Objects:
                if obj.Name.startswith('tag'):
                    FreeCAD.ActiveDocument.removeObject(obj.Name)

    def generateNewTags(self):
        print("generateNewTags")
        self.cleanupUI()

        count = self.form.sbCount.value()
        if not self.obj.Proxy.generateTags(self.obj, count):
            self.obj.Proxy.execute(self.obj)

        self.updateTagsView()
        #if debugDressup:
        #    # this causes a big of an echo and a double click on the spin buttons, don't know why though
        #    FreeCAD.ActiveDocument.recompute()


    def updateModel(self):
        self.getFields()
        self.updateTagsView()
        #FreeCAD.ActiveDocument.recompute()

    def whenCountChanged(self):
        print("whenCountChanged")
        count = self.form.sbCount.value()
        self.form.pbGenerate.setEnabled(count)

    def whenTagSelectionChanged(self):
        print('whenTagSelectionChanged')
        item = self.form.lwTags.currentItem()
        self.form.pbDelete.setEnabled(not item is None)

    def deleteSelectedTag(self):
        self.obj.Proxy.setXyEnabled(self.getTags(False))
        self.updateTagsView()

    def addNewTagAt(self, point, what):
        if what == self.obj:
            print("%s '%s'" %( point, what.Name))
            tags = self.tags
            tags.append((point.x, point.y, True))
            self.obj.Proxy.setXyEnabled(tags)
        panel = TaskPanel(self.obj, self.jvoVisible)
        todo.delay(FreeCADGui.Control.closeDialog, None)
        todo.delay(FreeCADGui.Control.showDialog, panel)
        todo.delay(panel.setupUi, None)

    def addNewTag(self):
        self.tags = self.getTags(True)
        FreeCADGui.Snapper.getPoint(callback=self.addNewTagAt, extradlg=[self])

    def setupSpinBox(self, widget, val, decimals = 2):
        widget.setMinimum(0)
        if decimals:
            widget.setDecimals(decimals)
        widget.setValue(val)

    def setFields(self):
        self.updateTagsView()
        self.setupSpinBox(self.form.sbCount, len(self.obj.Positions), None)
        self.setupSpinBox(self.form.dsbHeight, self.obj.Height)
        self.setupSpinBox(self.form.dsbWidth, self.obj.Width)
        self.setupSpinBox(self.form.dsbAngle, self.obj.Angle, 0)
        self.form.dsbAngle.setMaximum(90)
        self.form.dsbAngle.setSingleStep(5.)

    def updateModelHeight(self):
        print('updateModelHeight')
        self.updateModel()

    def updateModelWidth(self):
        print('updateModelWidth')
        self.updateModel()

    def updateModelAngle(self):
        print('updateModelAngle')
        self.updateModel()

    def updateModelTags(self):
        print('updateModelTags')
        self.updateModel()

    def setupUi(self):
        self.setFields()
        self.whenCountChanged()

        self.form.sbCount.valueChanged.connect(self.whenCountChanged)
        self.form.pbGenerate.clicked.connect(self.generateNewTags)

        self.form.dsbHeight.editingFinished.connect(self.updateModelHeight)
        self.form.dsbWidth.editingFinished.connect(self.updateModelWidth)
        self.form.dsbAngle.editingFinished.connect(self.updateModelAngle)
        self.form.lwTags.itemChanged.connect(self.updateModelTags)
        self.form.lwTags.itemSelectionChanged.connect(self.whenTagSelectionChanged)

        self.form.pbDelete.clicked.connect(self.deleteSelectedTag)
        self.form.pbAdd.clicked.connect(self.addNewTag)

class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST
        PST.eselect()

    def __del__(self):
        import PathScripts.PathSelection as PST
        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):
        #FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
        FreeCADGui.updateGui()

class ViewProviderDressup:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def claimChildren(self):
        for i in self.Object.Base.InList:
            if hasattr(i, "Group"):
                group = i.Group
                for g in group:
                    if g.Name == self.Object.Base.Name:
                        group.remove(g)
                i.Group = group
                print i.Group
        #FreeCADGui.ActiveDocument.getObject(obj.Base.Name).Visibility = False
        return [self.Object.Base]

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        panel = TaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(panel)
        panel.setupUi()
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onDelete(self, arg1=None, arg2=None):
        '''this makes sure that the base operation is added back to the project and visible'''
        FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
        PathUtils.addToJob(arg1.Object.Base)
        return True

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
            FreeCAD.Console.PrintError(translate("PathDressup_HoldingTags", "Please select one path object\n"))
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(translate("PathDressup_HoldingTags", "The selected object is not a path\n"))
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            FreeCAD.Console.PrintError(translate("PathDressup_HoldingTags", "Please select a Profile object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate("PathDressup_HoldingTags", "Create HoldingTags Dress-up"))
        FreeCADGui.addModule("PathScripts.PathDressupHoldingTags")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "HoldingTagsDressup")')
        FreeCADGui.doCommand('dbo = PathScripts.PathDressupHoldingTags.ObjectDressup(obj)')
        FreeCADGui.doCommand('obj.Base = FreeCAD.ActiveDocument.' + selection[0].Name)
        FreeCADGui.doCommand('PathScripts.PathDressupHoldingTags.ViewProviderDressup(obj.ViewObject)')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('Gui.ActiveDocument.getObject(obj.Base.Name).Visibility = False')
        FreeCADGui.doCommand('dbo.setup(obj, 4.)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('PathDressup_HoldingTags', CommandPathDressupHoldingTags())

FreeCAD.Console.PrintLog("Loading PathDressupHoldingTags... done\n")
