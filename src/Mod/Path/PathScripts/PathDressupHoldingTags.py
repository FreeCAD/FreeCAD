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

debugDressup = True
debugComponents = ['P0', 'P1', 'P2', 'P3']

def debugPrint(comp, msg):
    if debugDressup and comp in debugComponents:
        print(msg)

def debugEdge(edge, prefix, comp = None):
    pf = edge.valueAt(edge.FirstParameter)
    pl = edge.valueAt(edge.LastParameter)
    if comp:
        debugPrint(comp, "%s %s((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (prefix, type(edge.Curve), pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))
    else:
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

movecommands = ['G0', 'G00', 'G1', 'G01', 'G2', 'G02', 'G3', 'G03']
movestraight = ['G1', 'G01']
movecw =       ['G2', 'G02']
moveccw =      ['G3', 'G03']
movearc = movecw + moveccw

slack = 0.0000001

def pathCommandForEdge(edge):
    pt = edge.valueAt(edge.LastParameter)
    params = {'X': pt.x, 'Y': pt.y, 'Z': pt.z}
    if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
        command =  Path.Command('G1', params)
    else:
        p1 = edge.valueAt(edge.FirstParameter)
        p2 = edge.valueAt((edge.FirstParameter + edge.LastParameter)/2)
        p3 = pt
        if Side.Left == Side.of(p2 - p1, p3 - p2):
            cmd = 'G3'
        else:
            cmd = 'G2'
        pa = PathGeom.xy(p1)
        pb = PathGeom.xy(p2)
        pc = PathGeom.xy(p3)
        pd = Part.Circle(PathGeom.xy(p1), PathGeom.xy(p2), PathGeom.xy(p3)).Center
        print("**** (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)" % (pa.x, pa.y, pa.z, pc.x, pc.y, pc.z))
        print("**** (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)" % (pb.x, pb.y, pb.z, pd.x, pd.y, pd.z))
        offset = Part.Circle(PathGeom.xy(p1), PathGeom.xy(p2), PathGeom.xy(p3)).Center - p1
        print("**** (%.2f, %.2f, %.2f)" % (offset.x, offset.y, offset.z))
        params.update({'I': offset.x, 'J': offset.y, 'K': (p3.z - p1.z)/2})
        command = Path.Command(cmd, params)
    print command
    return command


class Tag:

    @classmethod
    def FromString(cls, string):
        try:
            t = eval(string)
            return Tag(t[0], t[1], t[2], t[3], t[4], t[5])
        except:
            return None

    def __init__(self, x, y, width, height, angle, enabled=True, z=None):
        self.x = x
        self.y = y
        self.width = math.fabs(width)
        self.height = math.fabs(height)
        self.actualHeight = self.height
        self.angle = math.fabs(angle)
        self.enabled = enabled
        if z is not None:
            self.createSolidsAt(z)

    def toString(self):
        return str((self.x, self.y, self.width, self.height, self.angle, self.enabled))

    def originAt(self, z):
        return FreeCAD.Vector(self.x, self.y, z)

    def bottom(self):
        return self.z

    def top(self):
        return self.z + self.actualHeight

    def centerLine(self):
        return Part.LineSegment(self.originAt(self.bottom() - 1), self.originAt(self.top() + 1))

    def createSolidsAt(self, z):
        self.z = z
        r1 = self.width / 2
        height = self.height
        if self.angle == 90 and height > 0:
            self.solid = Part.makeCylinder(r1, height)
            self.core  = self.solid.copy()
        elif self.angle > 0.0 and height > 0.0:
            tangens = math.tan(math.radians(self.angle))
            dr = height / tangens
            if dr < r1:
                r2 = r1 - dr
                self.core = Part.makeCylinder(r2, height)
            else:
                r2 = 0
                height = r1 * tangens
                self.core = None
                self.actualHeight = height
            self.solid = Part.makeCone(r1, r2, height)
        else:
            # degenerated case - no tag
            self.solid = Part.makeSphere(r1 / 10000)
            self.core = None
        self.solid.translate(self.originAt(z))
        if self.core:
            self.core.translate(self.originAt(z))

    class Intersection:
        # An intersection with a tag has 4 markant points, where one might be optional.
        #
        #    P1---P2             P1---P2               P2
        #    |    |              /     \               /\
        #    |    |             /       \             /  \
        #    |    |            /         \           /    \
        # ---P0   P3---    ---P0         P3---   ---P0    P3---
        #
        # If no intersection occured the Intersection can be viewed as being
        # at P3 with no additional edges.
        Pnone = 1
        P0 = 2
        P1 = 3
        P2 = 4
        P3 = 5

        def __init__(self, tag):
            self.tag = tag
            self.state = self.Pnone
            self.edges = []
            self.tail = None

        def isComplete(self):
            return self.state == self.Pnone or self.state == self.P3

        def hasEdges(self):
            return self.state != self.Pnone

        def moveEdgeToPlateau(self, edge):
            if type(edge.Curve) is Part.Line or type(edge.Curve) is Part.LineSegment:
                e = copy.copy(edge)
                z = edge.valueAt(edge.FirstParameter).z
            elif type(edge.Curve) is Part.Circle:
                # it's an arc
                e = copy.copy(edge)
                z = edge.Curve.Center.z
            else:
                # it's a helix -> transform to arc
                z = 0
                p1 = PathGeom.xy(edge.valueAt(edge.FirstParameter))
                p2 = PathGeom.xy(edge.valueAt((edge.FirstParameter + edge.LastParameter)/2))
                p3 = PathGeom.xy(edge.valueAt(edge.LastParameter))
                e = Part.Edge(Part.Arc(p1, p2, p3))
            print("-------- moveEdgeToPlateau")
            e.translate(Vector(0, 0, self.tag.top() - z))
            return e

        def intersectP0Core(self, edge):
            debugPrint('P0', "----- P0-core")

            i = self.tag.nextIntersectionClosestTo(edge, self.tag.core, edge.valueAt(edge.FirstParameter))
            if i:
                if PathGeom.pointsCoincide(i, edge.valueAt(edge.FirstParameter)):
                    # if P0 and P1 are the same, we need to insert a segment for the rise
                    debugPrint('P0', "-------  insert vertical rise (%s)" % i)
                    self.edges.append(Part.Edge(Part.LineSegment(i, FreeCAD.Vector(i.x, i.y, self.tag.top()))))
                    self.p1 = i
                    self.state = self.P1
                    return edge
                if PathGeom.pointsCoincide(i, edge.valueAt(edge.LastParameter)):
                    debugPrint('P0', "-------  consumed (%s)" % i)
                    e = edge
                    tail = None
                else:
                    debugPrint('P0', "-------  split at (%s)" % i)
                    e, tail = self.tag.splitEdgeAt(edge, i)
                self.p1 = e.valueAt(edge.LastParameter)
                self.edges.extend(self.tag.mapEdgeToSolid(e, 'P0-core-1'))
                self.state = self.P1
                return tail
            # no intersection, the entire edge fits between P0 and P1
            debugPrint('P0', "-------  no intersection")
            self.edges.extend(self.tag.mapEdgeToSolid(edge, 'P0-core-2'))
            return None

        def intersectP0(self, edge):
            pf = edge.valueAt(edge.FirstParameter)
            pl = edge.valueAt(edge.LastParameter)
            debugPrint('P0', "----- P0 %s(%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (type(edge.Curve), pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))

            if self.tag.core:
                return self.intersectP0Core(edge)

            # if we have no core the tip is the origin of the Tag
            line = Part.Edge(self.tag.centerLine())
            debugEdge(line, "------- center line", 'P0')
            if type(edge.Curve) != Part.Circle and type(edge.Curve) != Part.Line:
                p1 = edge.valueAt(edge.FirstParameter)
                p2 = edge.valueAt((edge.FirstParameter + edge.LastParameter)/2)
                p3 = edge.valueAt(edge.LastParameter)
                p1.z = 0
                p2.z = 0
                p3.z = 0
                arc = Part.Edge(Part.Arc(p1, p2, p3))
                aps = DraftGeomUtils.findIntersection(line, arc)
                for p in aps:
                    print("%s - p=%.2f" % (p, arc.Curve.parameter(p)))
                i = [edge.valueAt(arc.Curve.parameter(p)) for p in aps]
            else:
                i = DraftGeomUtils.findIntersection(line, edge)
            #i = line.Curve.intersect(edge)
            if i:
                debugPrint('P0', '------- P0 split @ (%.2f, %.2f, %.2f)' % (i[0].x, i[0].y, i[0].z))
                if PathGeom.pointsCoincide(i[0], edge.valueAt(edge.LastParameter)):
                    e = edge
                    tail = None
                else:
                    e, tail = self.tag.splitEdgeAt(edge, i[0])
                self.state = self.P2 # P1 and P2 are identical for triangular tags
                self.p1 = i[0]
                self.p2 = i[0]
            else:
                debugPrint('P0', '------- P0 no intersect')
                e = edge
                tail = None
            self.edges.extend(self.tag.mapEdgeToSolid(e, 'P0'))
            return tail



        def intersectP1(self, edge):
            pf = edge.valueAt(edge.FirstParameter)
            pl = edge.valueAt(edge.LastParameter)
            debugPrint('P1', "----- P1 (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))
            i = self.tag.nextIntersectionClosestTo(edge, self.tag.core, edge.valueAt(edge.LastParameter))
            if i:
                if PathGeom.pointsCoincide(i, edge.valueAt(edge.FirstParameter)):
                    debugPrint('P1', "----- P1 edge too short")
                    #self.edges.extend(self.tag.mapEdgeToSolid(edge, 'P1'))
                    self.edges.append(self.moveEdgeToPlateau(edge))
                    return None
                if PathGeom.pointsCoincide(i, edge.valueAt(edge.LastParameter)):
                    debugPrint('P1', "----- P1 edge at end")
                    e = edge
                    tail = None
                else:
                    debugPrint('P1', "----- P1 split edge @ (%.2f, %.2f, %.2f)" % (i.x, i.y, i.z))
                    e, tail = self.tag.splitEdgeAt(edge, i)
                    f = e.valueAt(e.FirstParameter)
                    l = e.valueAt(e.LastParameter)
                    debugPrint('P1', "----- P1 (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)" % (f.x, f.y, f.z, l.x, l.y, l.z))
                self.p2 = e.valueAt(e.LastParameter)
                self.state = self.P2
            else:
                debugPrint('P1', "----- P1 no intersect")
                e = edge
                tail = None
            f = e.valueAt(e.FirstParameter)
            l = e.valueAt(e.LastParameter)
            debugPrint('P1', "----- P1 (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)" % (f.x, f.y, f.z, l.x, l.y, l.z))
            self.edges.append(self.moveEdgeToPlateau(e))
            return tail

        def intersectP2(self, edge):
            pf = edge.valueAt(edge.FirstParameter)
            pl = edge.valueAt(edge.LastParameter)
            debugPrint('P2', "----- P2 (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))
            i = self.tag.nextIntersectionClosestTo(edge, self.tag.solid, edge.valueAt(edge.LastParameter))
            if i:
                if PathGeom.pointsCoincide(i, edge.valueAt(edge.FirstParameter)):
                    debugPrint('P2', "------- insert exit plunge (%s)"  % i)
                    self.edges.append(Part.Edge(Part.LineSegment(FreeCAD.Vector(i.x, i.y, self.tag.top()), i)))
                    e = None
                    tail = edge
                elif PathGeom.pointsCoincide(i, edge.valueAt(edge.LastParameter)):
                    debugPrint('P2', "------- entire segment added (%s)"  % i)
                    e = edge
                    tail = None
                else:
                    e, tail = self.tag.splitEdgeAt(edge, i)
                if tail:
                    pf = tail.valueAt(tail.FirstParameter)
                    pl = tail.valueAt(tail.LastParameter)
                    debugPrint('P3', "----- P3 (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))
                else:
                    debugPrint('P3', "----- P3 (---)")
                self.state = self.P3
                self.tail = tail
            else:
                debugPrint('P2', "----- P2 no intersection")
                e = edge
                tail = None
            if e:
                pf = e.valueAt(e.FirstParameter)
                pl = e.valueAt(e.LastParameter)
                s = 'P2' if self.state == self.P2 else 'P3'
                debugPrint(s, "----- %s (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (s, pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))
                self.edges.extend(self.tag.mapEdgeToSolid(e, 'P2'))
            return tail

        def intersect(self, edge):
            #print("")
            #print(" >>> (%s - %s)" % (edge.valueAt(edge.FirstParameter), edge.valueAt(edge.LastParameter)))
            if edge and self.state == self.P0:
                edge = self.intersectP0(edge)
            if edge and self.state == self.P1:
                edge = self.intersectP1(edge)
            if edge and self.state == self.P2:
                edge = self.intersectP2(edge)
            return self


    def splitEdgeAt(self, edge, pt):
        # I'm getting rather tired of this interface, so I decided to implement this myself.
        # How hard can it be?
        # There are only 3 types of edges passing through here, Line, Circle and Helix ...
        if False:
            p = edge.Curve.parameter(pt)
            #p = edge.parameterAt(Part.Vertex(pt.x, pt.y, pt.z))

            pf = edge.valueAt(edge.FirstParameter)
            pl = edge.valueAt(edge.LastParameter)
            print("-------- splitAt((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f): (%.2f, %.2f, %.2f)) -> param[%.2f -> %.2f]: %.2f" % (pf.x, pf.y, pf.z, pl.x, pl.y, pl.z, pt.x, pt.y, pt.z, edge.FirstParameter, edge.LastParameter, p))

            print("-------- splitAt(%.2f <= %.2f <= %.2f" % (edge.FirstParameter, p, edge.LastParameter))
            wire = edge.split(p)
            # split does not carry the Placement of the original curve foward ...
            wire.transformShape(edge.Placement.toMatrix())
            return wire.Edges
        p1 = edge.valueAt(edge.FirstParameter)
        p2 = pt
        p3 = edge.valueAt(edge.LastParameter)
        edges = []
        if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
            edges.append(Part.LineSegment(p1, p2))
            edges.append(Part.LineSegment(p2, p3))
        elif type(edge.Curve) == Part.Circle:
            # it's an arc
            p = edge.Curve.parameterAt(p2)
            p12 = edge.Curve.value((edge.Curve.FirstParameter + p)/2)
            p23 = edge.Curve.value((p + edge.Curve.LastParameter)/2)
            edges.append(Part.Edge(Part.Arc(p1, p12, p2)))
            edges.append(Part.Edge(Part.Arc(p2, p23, p3)))
        else:
            # it's a helix
            # convert to arc
            p01 = FreeCAD.Vector(p1.x, p1.y, 0)
            p02 = FreeCAD.Vector(p2.x, p2.y, 0)
            p03 = FreeCAD.Vector(p3.x, p3.y, 0)
            e0 = Part.Edge(Part.Arc(p01, p02, p03))
            # split arc
            p0 = e0.Curve.parameterAt(p02)
            p012 = e0.Curve.value((e0.Curve.FirstParameter + p0)/2)
            p023 = e0.Curve.value((p0 + e0.Curve.LastParameter)/2)
            e01 = Part.Edge(Part.Arc(p01, p012, p02))
            e02 = Part.Edge(Part.Arc(p02, p023, p03))
            # transform arcs into helical form
            edges.append(self.arcToHelix(e01, p1.z, p2.z))
            edges.append(self.arcToHelix(e02, p2.z, p3.z))
        return edges

    def mapEdgeToSolid(self, edge, label):
        pf = edge.valueAt(edge.FirstParameter)
        pl = edge.valueAt(edge.LastParameter)
        print("--------- mapEdgeToSolid-%s: %s((%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f))" % (label, type(edge.Curve), pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))

        p1a = edge.valueAt(edge.FirstParameter)
        p1b = FreeCAD.Vector(p1a.x, p1a.y, p1a.z + self.height)
        p1a.z = self.bottom()
        e1 = Part.Edge(Part.LineSegment(p1a, p1b))
        p1 = self.nextIntersectionClosestTo(e1, self.solid, p1b) # top most intersection
        print("---------- p1: (%s %s) -> %s %d" % (p1a, p1b, p1, self.solid.isInside(p1, 0.0000001, True)))
        if not p1:
            raise Exception('no p1')
            return []

        p2a = edge.valueAt(edge.LastParameter)
        p2b = FreeCAD.Vector(p2a.x, p2a.y, p2a.z + self.height)
        p2a.z = self.bottom()
        e2 = Part.Edge(Part.LineSegment(p2a, p2b))
        p2 = self.nextIntersectionClosestTo(e2, self.solid, p2b) # top most intersection
        if not p2:
            p1 = edge.valueAt(edge.FirstParameter)
            p2 = edge.valueAt(edge.LastParameter)
            print("---------- p1: %d%d" % (self.solid.isInside(p1, 0.0000001, True), self.solid.isInside(p1, 0.0000001, False)))
            print("---------- p2: %d%d" % (self.solid.isInside(p2, 0.0000001, True), self.solid.isInside(p2, 0.0000001, False)))
            #if not self.solid.isInside(p1, 0.0000001, False):
                # p1 is on the solid - 
            raise Exception('no p2')
            return []

        print("---------- %s - %s" % (p1, p2))
        print("---------- (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)" % (p2.x, p2.y, p2.z, p1.x, p1.y, p1.z))

        if PathGeom.pointsCoincide(p1, p2):
            return []

        if type(edge.Curve) == Part.Line or type(edge.Curve) == Part.LineSegment:
            e = Part.Edge(Part.LineSegment(p1, p2))
            debugEdge(e, "-------- >>")
            return [e]

        m = FreeCAD.Matrix()
        m.unity()
        pd = p2 - p1

        if type(edge.Curve) == Part.Circle:
            m.A32 = pd.z / pd.y
            m.A34 = - m.A32
            if pd.z < 0:
                m.A34 *= p2.y
            else:
                m.A34 *= p1.y
            e = edge.transformGeometry(m).Edges[0]
            debugEdge(e, "-------- >>")
            return [e]

        # it's already a helix, just need to lift it to the plateau
        m.A33 = pd.z / (edge.valueAt(edge.LastParameter).z - edge.valueAt(edge.FirstParameter).z)
        m.A34 = (1 - m.A33)
        if pd.z < 0:
            m.A34 *= edge.valueAt(edge.LastParameter).z
        else:
            m.A34 *= edge.valueAt(edge.FirstParameter).z

        #print
        pf = edge.valueAt(edge.FirstParameter)
        pl = edge.valueAt(edge.LastParameter)
        #print("(%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f):  %.2f" % (pf.x, pf.y, pf.z, pl.x, pl.y, pl.z, m.A33))
        #print("**** %.2f %.2f (%.2f - %.2f)" % (pd.z, p2a.z-p1a.z, p2a.z, p1a.z))
        e = edge.transformGeometry(m).Edges[0]
        pf = e.valueAt(e.FirstParameter)
        pl = e.valueAt(e.LastParameter)
        #print("(%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)" % (pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))
        #raise Exception("mensch")
        debugEdge(e, "-------- >>")
        return [e]


    def filterIntersections(self, pts, face):
        if type(face.Surface) == Part.Cone or type(face.Surface) == Part.Cylinder:
            #print("it's a cone/cylinder, checking z")
            return filter(lambda pt: pt.z >= self.bottom() and pt.z <= self.top(), pts)
        if type(face.Surface) == Part.Plane:
            #print("it's a plane, checking R")
            c = face.Edges[0].Curve
            if (type(c) == Part.Circle):
                return filter(lambda pt: (pt - c.Center).Length <= c.Radius, pts)
        print("==== we got a %s" % face.Surface)

    def isPointOnEdge(self, pt, edge):
        param = edge.Curve.parameter(pt)
        if edge.FirstParameter <= param <= edge.LastParameter:
            return True
        if edge.LastParameter <= param <= edge.FirstParameter:
            return True
        if PathGeom.isRoughly(edge.FirstParameter, param) or PathGeom.isRoughly(edge.LastParameter, param):
            return True
        print("-------- X %.2f <= %.2f <=%.2f   (%.2f, %.2f, %.2f)" % (edge.FirstParameter, param, edge.LastParameter, pt.x, pt.y, pt.z))
        return False


    def nextIntersectionClosestTo(self, edge, solid, refPt):
        ef = edge.valueAt(edge.FirstParameter)
        el = edge.valueAt(edge.LastParameter)
        print("-------- intersect %s (%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)  refp=(%.2f, %.2f, %.2f)" % (type(edge.Curve), ef.x, ef.y, ef.z, el.x, el.y, el.z, refPt.x, refPt.y, refPt.z))

        pts = []
        for index, face in enumerate(solid.Faces):
            i = edge.Curve.intersect(face.Surface)[0]
            ps = self.filterIntersections([FreeCAD.Vector(p.X, p.Y, p.Z) for p in i], face)
            pts.extend(filter(lambda pt: self.isPointOnEdge(pt, edge), ps))
            if len(ps)  != len(filter(lambda pt: self.isPointOnEdge(pt, edge), ps)):
                filtered = filter(lambda pt: self.isPointOnEdge(pt, edge), ps)
                print("-------- ++ len(ps)=%d, len(filtered)=%d" % (len(ps), len(filtered)))
                for p in ps:
                    included = '+' if p in filtered else '-'
                    print("--------     %s (%.2f, %.2f, %.2f)" % (included, p.x, p.y, p.z))
        if pts:
            closest = sorted(pts, key=lambda pt: (pt - refPt).Length)[0]
            for p in pts:
                print("-------- - intersect pt : (%.2f, %.2f, %.2f)" % (p.x, p.y, p.z))
            print("-------- -> (%.2f, %.2f, %.2f)" % (closest.x, closest.y, closest.z))
            return closest
        
        print("-------- -> None")
        return None

    def intersect(self, edge, check = True):
        print("--- intersect")
        inters = self.Intersection(self)
        if check:
            if edge.valueAt(edge.FirstParameter).z < self.top() or edge.valueAt(edge.LastParameter).z < self.top():
                i = self.nextIntersectionClosestTo(edge, self.solid, edge.valueAt(edge.FirstParameter))
                if i:
                    print("---- (%.2f, %.2f, %.2f)" % (i.x, i.y, i.z))
                    inters.state = self.Intersection.P0
                    inters.p0 = i
                    if PathGeom.pointsCoincide(i, edge.valueAt(edge.LastParameter)):
                        print("---- entire edge consumed.")
                        inters.edges.append(edge)
                        return inters
                    if PathGeom.pointsCoincide(i, edge.valueAt(edge.FirstParameter)):
                        print("---- nothing of edge consumed.")
                        tail = edge
                    else:
                        print("---- split edge")
                        e,tail = self.splitEdgeAt(edge, i)
                        inters.edges.append(e)
                    return inters.intersect(tail)
                else:
                    print("---- No intersection found.")
            else:
                print("---- Fly by")
        else:
            print("---- skipped")
        # if we get here there is no intersection with the tag
        inters.state = self.Intersection.Pnone
        inters.tail = edge
        return inters

class PathData:
    def __init__(self, obj):
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
            #return Part.Wire(self.sortedBase(bottom))
            return wire
        # if we get here there are already holding tags, or we're not looking at a profile
        # let's try and insert the missing pieces - another day
        raise ValueError("Selected path doesn't seem to be a Profile operation.")

    def sortedBase(self, base):
        # first find the exit point, where base wire is closed
        edges = [e for e in self.edges if e.valueAt(e.FirstParameter).z == self.minZ and e.valueAt(e.LastParameter).z != self.maxZ]
        exit = sorted(edges, key=lambda e: -e.valueAt(e.LastParameter).z)[0]
        pt = exit.valueAt(exit.FirstParameter)
        # then find the first base edge, and sort them until done
        ordered = []
        while base:
            edges = [e for e in base if e.valueAt(e.FirstParameter) == pt]
            if not edges:
                print ordered
                print base
                print("(%.2f, %.2f, %.2f)" % (pt.x, pt.y, pt.z))
                for e in base:
                    pf = e.valueAt(e.FirstParameter)
                    pl = e.valueAt(e.LastParameter)
                    print("(%.2f, %.2f, %.2f) - (%.2f, %.2f, %.2f)" % (pf.x, pf.y, pf.z, pl.x, pl.y, pl.z))
            edge = edges[0]
            ordered.append(edge)
            base.remove(edge)
            pt = edge.valueAt(edge.LastParameter)
        return ordered


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

        #print("length=%.2f shortestEdge=%.2f(%.2f) longestEdge=%.2f(%.2f)" % (self.base.Length, shortestEdge.Length, shortestEdge.Length/self.base.Length, longestEdge.Length, longestEdge.Length / self.base.Length))
        #print("   start: index=%-2d count=%d (length=%.2f, distance=%.2f)" % (startIndex, startCount, startEdge.Length, tagDistance))
        #print("               -> lastTagLength=%.2f)" % lastTagLength)
        #print("               -> currentLength=%.2f)" % currentLength)

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
            #print(" %d: %d" % (i, count))
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
        if edge.Length > minLength:
            while lastTagLength + tagDistance < currentLength:
                tagCount += 1
                lastTagLength += tagDistance
            if tagCount > 0:
                #print("      index=%d -> count=%d" % (index, tagCount))
                edgeDict[index] = tagCount
        #else:
            #print("      skipping=%-2d (%.2f)" % (index, edge.Length))

        return (currentLength, lastTagLength)

    def tagHeight(self):
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


    def tagIntersection(self, face, edge):
        p1 = edge.valueAt(edge.FirstParameter)
        pts = edge.Curve.intersect(face.Surface)
        if pts[0]:
            closest = sorted(pts[0], key=lambda pt: (pt - p1).Length)[0]
            return closest
        return None

    def createPath(self, edges, tags):
        commands = []
        lastEdge = 0
        lastTag = 0
        sameTag = None
        t = 0
        inters = None
        edge = None

        while edge or lastEdge < len(edges):
            print("------- lastEdge = %d/%d.%d/%d" % (lastEdge, lastTag, t, len(tags)))
            if not edge:
                edge = edges[lastEdge]
                debugEdge(edge, "=======  new edge: %d/%d" % (lastEdge, len(edges)))
                lastEdge += 1
                sameTag = None

            if inters:
                inters = inters.intersect(edge)
            else:
                tIndex = (t + lastTag) % len(tags)
                t += 1
                print("<<<<< lastTag=%d, t=%d, tIndex=%d, sameTag=%s >>>>>>" % (lastTag, t, tIndex, sameTag))
                inters = tags[tIndex].intersect(edge, True or tIndex != sameTag)
            edge = inters.tail

            if inters.isComplete():
                if inters.hasEdges():
                    sameTag = (t + lastTag - 1) % len(tags)
                    lastTag = sameTag
                    t = 1
                    for e in inters.edges:
                        commands.append(pathCommandForEdge(e))
                inters = None

            if t >= len(tags):
                # gone through all tags, consume edge and move on
                if edge:
                    commands.append(pathCommandForEdge(edge))
                edge = None
                t = 0

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
            tags = self.generateTags(obj, 2.)

        if not tags:
            print("execute - no tags")
            self.tags = []
            obj.Path = obj.Base.Path
            return

        print("execute - %d tags" % (len(tags)))
        tagID = 0
        for tag in tags:
            tagID += 1
            if tag.enabled:
                #print("x=%s, y=%s, z=%s" % (tag.x, tag.y, pathData.minZ))
                #debugMarker(FreeCAD.Vector(tag.x, tag.y, pathData.minZ), "tag-%02d" % tagID , (1.0, 0.0, 1.0), 0.5)
                debugCylinder(tag.originAt(pathData.minZ), tag.width/2, tag.actualHeight, "tag-%02d" % tagID)

        tags = pathData.sortedTags(tags)
        for tag in tags:
            tag.createSolidsAt(pathData.minZ)

        self.fingerprint = [tag.toString() for tag in tags]
        self.tags = tags

        obj.Path = self.createPath(pathData.edges, tags)

    def setTags(self, obj, tags):
        obj.Tags = [tag.toString() for tag in tags]
        self.execute(obj)

    def getTags(self, obj):
        if hasattr(self, 'tags'):
            return self.tags
        return self.setup(obj).generateTags(obj, 4)

    def setup(self, obj):
        if False or not hasattr(self, "pathData") or not self.pathData:
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
