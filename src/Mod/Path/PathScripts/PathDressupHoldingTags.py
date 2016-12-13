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

def debugEdge(edge, prefix, comp = None):
    pf = edge.valueAt(edge.FirstParameter)
    pl = edge.valueAt(edge.LastParameter)
    if comp:
        debugPrint(comp, "%s %s((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (prefix, type(edge.Curve), pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))
    elif debugDressup:
        print("%s %s((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (prefix, type(edge.Curve), pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))

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

    def __init__(self, x, y, width, height, angle, enabled=True, z=None):
        debugPrint("Tag(%.2f, %.2f, %.2f, %.2f, %.2f, %d, %s)" % (x, y, width, height, angle/math.pi, enabled, z))
        self.x = x
        self.y = y
        self.z = z
        self.width = math.fabs(width)
        self.height = math.fabs(height)
        self.actualHeight = self.height
        self.angle = math.fabs(angle)
        self.enabled = enabled
        if z is not None:
            self.createSolidsAt(z)

    def originAt(self, z):
        return FreeCAD.Vector(self.x, self.y, z)

    def bottom(self):
        return self.z

    def top(self):
        return self.z + self.actualHeight

    def createSolidsAt(self, z):
        self.z = z
        r1 = self.width / 2
        self.r1 = r1
        self.r2 = r1
        height = self.height
        if self.angle == 90 and height > 0:
            self.solid = Part.makeCylinder(r1, height)
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
            self.solid = Part.makeCone(r1, r2, height)
        else:
            # degenerated case - no tag
            self.solid = Part.makeSphere(r1 / 10000)
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
        self.wire = None

    def addEdge(self, edge):
        debugEdge(edge, '..........')
        if self.wire:
            self.wire.add(edge)
        else:
            self.wire = Part.Wire(edge)

    def needToFlipEdge(self, edge, p):
        if PathGeom.pointsCoincide(edge.valueAt(edge.LastParameter), p):
            return True, edge.valueAt(edge.FirstParameter)
        return False, edge.valueAt(edge.LastParameter)

    def isEntryOrExitStrut(self, p1, p2):
        p = PathGeom.xy(p1)
        pEntry0 = PathGeom.xy(self.entry)
        pExit0 = PathGeom.xy(self.exit)
        # it can only be an entry strut if the strut coincides with the entry point and is above it
        if PathGeom.pointsCoincide(p, pEntry0) and p1.z >= self.entry.z and p2.z >= self.entry.z:
            return True
        if PathGeom.pointsCoincide(p, pExit0) and p1.z >= self.exit.z and p2.z >= self.exit.z:
            return True
        return False


    def cleanupEdges(self, edges):
        # first remove all internal struts
        debugEdge(Part.Edge(Part.LineSegment(self.entry, self.exit)), '------> cleanupEdges')
        inputEdges = copy.copy(edges)
        plinths = []
        for e in edges:
            debugEdge(e, '........ cleanup')
            p1 = e.valueAt(e.FirstParameter)
            p2 = e.valueAt(e.LastParameter)
            if PathGeom.pointsCoincide(PathGeom.xy(p1), PathGeom.xy(p2)):
                #it's a strut
                if not self.isEntryOrExitStrut(p1, p2):
                    debugEdge(e, '......... X0 %d/%d' % (PathGeom.edgeConnectsTo(e, self.entry), PathGeom.edgeConnectsTo(e, self.exit)))
                    inputEdges.remove(e)
                    if p1.z > p2.z:
                        plinths.append(p2)
                    else:
                        plinths.append(p1)
        # remove all edges that are connected to the plinths of the (former) internal struts
        for e in copy.copy(inputEdges):
            for p in plinths:
                if PathGeom.edgeConnectsTo(e, p):
                    debugEdge(e, '......... X1')
                    inputEdges.remove(e)
                    break
        # if there are any edges beside a direct edge remaining, the direct edge between
        # entry and exit is redundant
        if len(inputEdges) > 1:
            for e in copy.copy(inputEdges):
                if PathGeom.edgeConnectsTo(e, self.entry) and PathGeom.edgeConnectsTo(e, self.exit):
                    debugEdge(e, '......... X2')
                    inputEdges.remove(e)

        # the remaining edges form a walk around the tag
        # they need to be ordered and potentially flipped though
        outputEdges = []
        p = self.entry
        lastP = p
        while inputEdges:
            for e in inputEdges:
                p1 = e.valueAt(e.FirstParameter)
                p2 = e.valueAt(e.LastParameter)
                if PathGeom.pointsCoincide(p1, p):
                    outputEdges.append((e,False))
                    inputEdges.remove(e)
                    lastP = p
                    p = p2
                    debugEdge(e, ">>>>> no flip")
                    break
                elif PathGeom.pointsCoincide(p2, p):
                    outputEdges.append((e,True))
                    inputEdges.remove(e)
                    lastP = p
                    p = p1
                    debugEdge(e, ">>>>> flip")
                    break
                #else:
                #    debugEdge(e, "<<<<< (%.2f, %.2f, %.2f)" % (p.x, p.y, p.z))
            if lastP == p:
                raise ValueError("No connection to %s" % (p))
            #else:
            #    print("xxxxxx (%.2f, %.2f, %.2f)" % (p.x, p.y, p.z))
        return outputEdges

    def shell(self):
        shell = self.wire.extrude(FreeCAD.Vector(0, 0, 10))
        redundant = filter(lambda f: f.Area == 0, shell.childShapes())
        if redundant:
            return shell.removeShape(redundant)
        return shell

    def add(self, edge):
        self.tail = None
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
            if self.wire:
                face = self.shell().common(self.tag.solid)

                for e,flip in self.cleanupEdges(face.Edges):
                    debugEdge(e, '++++++++ %s' % ('.' if not flip else '@'))
                    self.commands.extend(PathGeom.cmdsForEdge(e, flip, False))
            self.complete = True

    def mappingComplete(self):
        return self.complete

class PathData:
    def __init__(self, obj):
        debugPrint("PathData(%s)" % obj.Base.Name)
        self.obj = obj
        self.wire = PathGeom.wireForPath(obj.Base.Path)
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
        minZ = edges[0].Vertexes[0].Point.z
        maxZ = minZ
        for e in edges:
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
        print("generateTags(%s, %s, %s, %s, %s)" % (count, width, height, angle, spacing))
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
            W = self.tagWidth()
        if height:
            H = height
        else:
            H = self.tagHeight()


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

    def tagHeight(self):
        if hasattr(self.obj, 'Base') and hasattr(self.obj.Base, 'StartDepth') and hasattr(self.obj.Base, 'FinalDepth'):
            return self.obj.Base.StartDepth - self.obj.Base.FinalDepth
        return self.maxZ - self.minZ

    def tagWidth(self):
        return self.shortestAndLongestPathEdge()[1].Length / 10

    def tagAngle(self):
        return 90

    def pathLength(self):
        return self.base.Length

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
        obj.addProperty("App::PropertyStringList", "Tags", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_holdingTags", "Inserted tags"))
        obj.setEditorMode("Tags", 2)
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def generateTags(self, obj, count=None, width=None, height=None, angle=90, spacing=None):
        return self.pathData.generateTags(obj, count, width, height, angle, spacing)

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

    def createPath(self, edges, tags):
        commands = []
        lastEdge = 0
        lastTag = 0
        sameTag = None
        t = 0
        inters = None
        edge = None

        mapper = None

        while edge or lastEdge < len(edges):
            #print("------- lastEdge = %d/%d.%d/%d" % (lastEdge, lastTag, t, len(tags)))
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
                    edge = mapper.tail


            if not mapper and t >= len(tags):
                # gone through all tags, consume edge and move on
                if edge:
                    debugEdge(edge, '++++++++')
                    commands.extend(PathGeom.cmdsForEdge(edge))
                edge = None
                t = 0

        #for cmd in commands:
        #    print(cmd)
        return Path.Path(commands)


    def execute(self, obj):
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

        if hasattr(obj, 'Tags') and obj.Tags:
            if False and self.fingerprint == obj.Tags:
                print("execute - cache valid")
                return
            print("execute - tags from property")
            tags = [Tag.FromString(tag) for tag in obj.Tags]
        else:
            print("execute - default tags")
            tags = self.generateTags(obj, 4.)

        if not tags:
            print("execute - no tags")
            self.tags = []
            obj.Path = obj.Base.Path
            return

        print("execute - %d tags" % (len(tags)))
        tags = pathData.sortedTags(tags)
        self.setTags(obj, tags, False)
        for tag in tags:
            tag.createSolidsAt(pathData.minZ)

        tagID = 0
        for tag in tags:
            tagID += 1
            if tag.enabled:
                #print("x=%s, y=%s, z=%s" % (tag.x, tag.y, pathData.minZ))
                #debugMarker(FreeCAD.Vector(tag.x, tag.y, pathData.minZ), "tag-%02d" % tagID , (1.0, 0.0, 1.0), 0.5)
                if tag.angle != 90:
                    debugCone(tag.originAt(pathData.minZ), tag.r1, tag.r2, tag.actualHeight, "tag-%02d" % tagID)
                else:
                    debugCylinder(tag.originAt(pathData.minZ), tag.width/2, tag.actualHeight, "tag-%02d" % tagID)

        self.fingerprint = [tag.toString() for tag in tags]
        self.tags = tags

        obj.Path = self.createPath(pathData.edges, tags)

    def setTags(self, obj, tags, update = True):
        print("setTags(%d, %d)" % (len(tags), update))
        for t in tags:
            print(" .... %s" % t.toString())
        obj.Tags = [tag.toString() for tag in tags]
        if update:
            self.execute(obj)

    def getTags(self, obj):
        if hasattr(self, 'tags'):
            return self.tags
        return self.setup(obj).generateTags(obj, 4)

    def setup(self, obj):
        if True or not hasattr(self, "pathData") or not self.pathData:
            try:
                pathData = PathData(obj)
            except ValueError:
                FreeCAD.Console.PrintError(translate("PathDressup_HoldingTags", "Cannot insert holding tags for this path - please select a Profile path\n"))
                return None

            ## setup the object's properties, in case they're not set yet
            #obj.Count = self.tagCount(obj)
            #obj.Angle = self.tagAngle(obj)
            #obj.Blacklist = self.tagBlacklist(obj)

            # if the heigt isn't set, use the height of the path
            #if not hasattr(obj, "Height") or not obj.Height:
            #    obj.Height = pathData.maxZ - pathData.minZ
            # try and take an educated guess at the width
            #if not hasattr(obj, "Width") or not obj.Width:
            #    width = sorted(pathData.base.Edges, key=lambda e: -e.Length)[0].Length / 10
            #    while obj.Count > len([e for e in pathData.base.Edges if e.Length > 3*width]):
            #        width = widht / 2
            #    obj.Width = width

            # and the tool radius, not sure yet if it's needed
            #self.toolRadius = 5
            #toolLoad = PathUtils.getLastToolLoad(obj)
            #if toolLoad is None or toolLoad.ToolNumber == 0:
            #    self.toolRadius = 5
            #else:
            #    tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
            #    if not tool or tool.Diameter == 0:
            #        self.toolRadius = 5
            #    else:
            #        self.toolRadius = tool.Diameter / 2
            self.pathData = pathData
        return self.pathData

    def getHeight(self, obj):
        return self.pathData.tagHeight()

    def getWidth(self, obj):
        return self.pathData.tagWidth()

    def getAngle(self, obj):
        return self.pathData.tagAngle()

    def getPathLength(self, obj):
        return self.pathData.pathLength()

class TaskPanel:
    DataTag   = QtCore.Qt.ItemDataRole.UserRole
    DataValue = QtCore.Qt.ItemDataRole.DisplayRole

    def __init__(self, obj):
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/HoldingTagsEdit.ui")
        FreeCAD.ActiveDocument.openTransaction(translate("PathDressup_HoldingTags", "Edit HoldingTags Dress-up"))

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def accept(self):
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)
        FreeCAD.ActiveDocument.recompute()

    def open(self):
        self.s = SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)

    def tableWidgetItem(self, tag, val):
        item = QtGui.QTableWidgetItem()
        item.setTextAlignment(QtCore.Qt.AlignRight)
        item.setData(self.DataTag, tag)
        item.setData(self.DataValue, val)
        return item

    def getFields(self):
        tags = []
        for row in range(0, self.form.twTags.rowCount()):
            x = self.form.twTags.item(row, 0).data(self.DataValue)
            y = self.form.twTags.item(row, 1).data(self.DataValue)
            w = self.form.twTags.item(row, 2).data(self.DataValue)
            h = self.form.twTags.item(row, 3).data(self.DataValue)
            a = self.form.twTags.item(row, 4).data(self.DataValue)
            tags.append(Tag(x, y, w, h, a, True))
        print("getFields: %d" % (len(tags)))
        self.obj.Proxy.setTags(self.obj, tags)

    def updateTags(self):
        self.tags = self.obj.Proxy.getTags(self.obj)
        self.form.twTags.blockSignals(True)
        self.form.twTags.setSortingEnabled(False)
        self.form.twTags.clearSpans()
        print("updateTags: %d" % (len(self.tags)))
        self.form.twTags.setRowCount(len(self.tags))
        for row, tag in enumerate(self.tags):
            self.form.twTags.setItem(row, 0, self.tableWidgetItem(tag, tag.x))
            self.form.twTags.setItem(row, 1, self.tableWidgetItem(tag, tag.y))
            self.form.twTags.setItem(row, 2, self.tableWidgetItem(tag, tag.width))
            self.form.twTags.setItem(row, 3, self.tableWidgetItem(tag, tag.height))
            self.form.twTags.setItem(row, 4, self.tableWidgetItem(tag, tag.angle))
        self.form.twTags.setSortingEnabled(True)
        self.form.twTags.blockSignals(False)

    def cleanupUI(self):
        print("cleanupUI")
        if debugDressup:
            for obj in FreeCAD.ActiveDocument.Objects:
                if obj.Name.startswith('tag'):
                    FreeCAD.ActiveDocument.removeObject(obj.Name)

    def updateUI(self):
        print("updateUI")
        self.cleanupUI()
        self.getFields()
        if debugDressup:
            FreeCAD.ActiveDocument.recompute()


    def whenApplyClicked(self):
        print("whenApplyClicked")
        self.cleanupUI()

        count = self.form.sbCount.value()
        spacing = self.form.dsbSpacing.value()
        width = self.form.dsbWidth.value()
        height = self.form.dsbHeight.value()
        angle = self.form.dsbAngle.value()

        tags = self.obj.Proxy.generateTags(self.obj, count, width, height, angle, spacing * 0.99)

        self.obj.Proxy.setTags(self.obj, tags)
        self.updateTags()
        if debugDressup:
            # this causes a big of an echo and a double click on the spin buttons, don't know why though
            FreeCAD.ActiveDocument.recompute()

    def autoApply(self):
        print("autoApply")
        if self.form.cbAutoApply.checkState() == QtCore.Qt.CheckState.Checked:
            self.whenApplyClicked()

    def updateTagSpacing(self, count):
        print("updateTagSpacing")
        if count == 0:
            spacing = 0
        else:
            spacing = self.pathLength / count
        self.form.dsbSpacing.blockSignals(True)
        self.form.dsbSpacing.setValue(spacing)
        self.form.dsbSpacing.blockSignals(False)

    def whenCountChanged(self):
        print("whenCountChanged")
        self.updateTagSpacing(self.form.sbCount.value())
        self.autoApply()

    def whenSpacingChanged(self):
        print("whenSpacingChanged")
        if self.form.dsbSpacing.value() == 0:
            count = 0
        else:
            count = int(self.pathLength / self.form.dsbSpacing.value())
        self.form.sbCount.blockSignals(True)
        self.form.sbCount.setValue(count)
        self.form.sbCount.blockSignals(False)
        self.autoApply()

    def whenOkClicked(self):
        print("whenOkClicked")
        self.whenApplyClicked()
        self.form.toolBox.setCurrentWidget(self.form.tbpTags)

    def setupSpinBox(self, widget, val, decimals = 2):
        widget.setMinimum(0)
        if decimals:
            widget.setDecimals(decimals)
        widget.setValue(val)

    def setFields(self):
        self.pathLength = self.obj.Proxy.getPathLength(self.obj)
        vHeader = self.form.twTags.verticalHeader()
        vHeader.setResizeMode(QtGui.QHeaderView.Fixed)
        vHeader.setDefaultSectionSize(20)
        self.updateTags()
        self.setupSpinBox(self.form.sbCount, self.form.twTags.rowCount(), None)
        self.setupSpinBox(self.form.dsbSpacing, 0)
        self.setupSpinBox(self.form.dsbHeight, self.obj.Proxy.getHeight(self.obj))
        self.setupSpinBox(self.form.dsbWidth, self.obj.Proxy.getWidth(self.obj))
        self.setupSpinBox(self.form.dsbAngle, self.obj.Proxy.getAngle(self.obj))
        self.updateTagSpacing(self.form.twTags.rowCount())

    def setupUi(self):
        self.setFields()
        self.form.sbCount.valueChanged.connect(self.whenCountChanged)
        self.form.dsbSpacing.valueChanged.connect(self.whenSpacingChanged)
        self.form.dsbHeight.valueChanged.connect(self.autoApply)
        self.form.dsbWidth.valueChanged.connect(self.autoApply)
        self.form.dsbAngle.valueChanged.connect(self.autoApply)
        #self.form.pbAdd.clicked.connect(self.)
        self.form.buttonBox.button(QtGui.QDialogButtonBox.Apply).clicked.connect(self.whenApplyClicked)
        self.form.buttonBox.button(QtGui.QDialogButtonBox.Ok).clicked.connect(self.whenOkClicked)
        self.form.twTags.itemChanged.connect(self.updateUI)

class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST
        PST.eselect()

    def __del__(self):
        import PathScripts.PathSelection as PST
        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
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
        FreeCADGui.doCommand('dbo.setup(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('PathDressup_HoldingTags', CommandPathDressupHoldingTags())

FreeCAD.Console.PrintLog("Loading PathDressupHoldingTags... done\n")
