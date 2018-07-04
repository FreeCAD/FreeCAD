# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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
import PathScripts.PathGeom as PathGeom
import PathScripts.PathOpTools as PathOpTools
import PathScripts.PathLog as PathLog
import PathTests.PathTestUtils as PathTestUtils
import math

from FreeCAD import Vector

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule(PathLog.thisModule())

def getWire(obj, nr=0):
    return obj.Tip.Profile[0].Shape.Wires[nr]

def getWireInside(obj):
    w1 = getWire(obj, 0)
    w2 = getWire(obj, 1)
    if w2.BoundBox.isInside(w1.BoundBox):
        return w1
    return w2

def getWireOutside(obj):
    w1 = getWire(obj, 0)
    w2 = getWire(obj, 1)
    if w2.BoundBox.isInside(w1.BoundBox):
        return w2
    return w1

def getPositiveShape(obj):
    return obj.Tool.Shape

def getNegativeShape(obj):
    return obj.Shape

doc = None
triangle = None
shape = None

def makeWire(pts):
    edges = []
    first = pts[0]
    last = pts[0]
    for p in pts[1:]:
        edges.append(Part.Edge(Part.LineSegment(last, p)))
        last = p
    edges.append(Part.Edge(Part.LineSegment(last, first)))
    return Part.Wire(edges)


pa = Vector(1, 1, 0)
pb = Vector(1, 5, 0)
pc = Vector(5, 5, 0)
pd = Vector(5, 1, 0)

class TestPathOpTools(PathTestUtils.PathTestBase):

    @classmethod
    def setUpClass(cls):
        global doc
        doc = FreeCAD.openDocument(FreeCAD.getHomePath() + 'Mod/Path/PathTests/test_geomop.fcstd')

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument("test_geomop")

    def test00(self):
        w = makeWire([pa, pb, pc, pd])
        f = Part.Face(w)
        self.assertCoincide(Vector(0, 0, -1), f.Surface.Axis)

    def test01(self):
        w = makeWire([pa, pb, pc, pd])
        f = Part.Face(w)
        self.assertEqual('Forward', f.Orientation)

    def test10(self):
        w = makeWire([pa, pd, pc, pb])
        f = Part.Face(w)
        self.assertCoincide(Vector(0, 0, +1), f.Surface.Axis)

    def test11(self):
        w = makeWire([pa, pb, pc, pd])
        f = Part.Face(w)
        self.assertEqual('Forward', f.Orientation)


    def xtest01(self):
        '''Check offsetting a circular hole.'''
        obj = doc.getObjectsByLabel('offset-circle')[0]

        small = getWireInside(obj)
        self.assertRoughly(10, small.Edges[0].Curve.Radius)

        wire = PathOpTools.offsetWire(small, obj.Shape, 3, True)
        self.assertIsNotNone(wire)
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(7, wire.Edges[0].Curve.Radius)
        self.assertCoincide(Vector(0, 0, 1), wire.Edges[0].Curve.Axis)
        
        wire = PathOpTools.offsetWire(small, obj.Shape, 9.9, True)
        self.assertIsNotNone(wire)
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(0.1, wire.Edges[0].Curve.Radius)
        self.assertCoincide(Vector(0, 0, 1), wire.Edges[0].Curve.Axis)

    def xtest02(self):
        '''Check offsetting a circular hole by the radius or more makes the hole vanish.'''
        obj = doc.getObjectsByLabel('offset-circle')[0]

        small = getWireInside(obj)
        self.assertRoughly(10, small.Edges[0].Curve.Radius)
        wire = PathOpTools.offsetWire(small, obj.Shape, 10, True)
        self.assertIsNone(wire)

        wire = PathOpTools.offsetWire(small, obj.Shape, 15, True)
        self.assertIsNone(wire)

    def xtest03(self):
        '''Check offsetting a cylinder succeeds.'''
        obj = doc.getObjectsByLabel('offset-circle')[0]

        big = getWireOutside(obj)
        self.assertRoughly(20, big.Edges[0].Curve.Radius)

        wire = PathOpTools.offsetWire(big, obj.Shape, 10, True)
        self.assertIsNotNone(wire)
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(30, wire.Edges[0].Curve.Radius)
        self.assertCoincide(Vector(0, 0, -1), wire.Edges[0].Curve.Axis)

        wire = PathOpTools.offsetWire(big, obj.Shape, 20, True)
        self.assertIsNotNone(wire)
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(40, wire.Edges[0].Curve.Radius)
        self.assertCoincide(Vector(0, 0, -1), wire.Edges[0].Curve.Axis)

    def xtest04(self):
        '''Check offsetting a hole with Placement.'''
        obj = doc.getObjectsByLabel('offset-placement')[0]

        wires = [w for w in obj.Shape.Wires if 1 == len(w.Edges) and PathGeom.isRoughly(0, w.Edges[0].Vertexes[0].Point.z)]
        self.assertEqual(2, len(wires))
        w = wires[1] if wires[0].BoundBox.isInside(wires[1].BoundBox) else wires[0]

        self.assertRoughly(10, w.Edges[0].Curve.Radius)
        # make sure there is a placement and I didn't mess up the model
        self.assertFalse(PathGeom.pointsCoincide(Vector(), w.Edges[0].Placement.Base))

        wire = PathOpTools.offsetWire(w, obj.Shape, 2, True)
        self.assertIsNotNone(wire)
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(8, wire.Edges[0].Curve.Radius)
        self.assertCoincide(Vector(0, 0, 0), wire.Edges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, 1), wire.Edges[0].Curve.Axis)

    def xtest05(self):
        '''Check offsetting a cylinder with Placement.'''
        obj = doc.getObjectsByLabel('offset-placement')[0]

        wires = [w for w in obj.Shape.Wires if 1 == len(w.Edges) and PathGeom.isRoughly(0, w.Edges[0].Vertexes[0].Point.z)]
        self.assertEqual(2, len(wires))
        w = wires[0] if wires[0].BoundBox.isInside(wires[1].BoundBox) else wires[1]

        self.assertRoughly(20, w.Edges[0].Curve.Radius)
        # make sure there is a placement and I didn't mess up the model
        self.assertFalse(PathGeom.pointsCoincide(Vector(), w.Edges[0].Placement.Base))

        wire = PathOpTools.offsetWire(w, obj.Shape, 2, True)
        self.assertIsNotNone(wire)
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(22, wire.Edges[0].Curve.Radius)
        self.assertCoincide(Vector(0, 0, 0), wire.Edges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), wire.Edges[0].Curve.Axis)

    def xtest10(self):
        '''Check offsetting hole wire succeeds.'''
        obj = doc.getObjectsByLabel('offset-edge')[0]

        small = getWireInside(obj)
        # sanity check
        y = 10
        x = 10 * math.cos(math.pi/6)
        self.assertLines(small.Edges, False, [Vector(0, y, 0), Vector(-x, -y/2, 0), Vector(x, -y/2, 0), Vector(0, y, 0)])

        wire = PathOpTools.offsetWire(small, obj.Shape, 3, True)
        self.assertIsNotNone(wire)
        self.assertEqual(3, len(wire.Edges))
        self.assertTrue(wire.isClosed())
        y = 4  # offset works in both directions
        x = 4 * math.cos(math.pi/6)
        self.assertLines(wire.Edges, False, [Vector(0, 4, 0), Vector(x, -2, 0), Vector(-x, -2, 0), Vector(0, 4, 0)])
        f = Part.Face(wire)
        self.assertCoincide(Vector(0, 0, -1), f.Surface.Axis)

    def xtest11(self):
        '''Check offsetting hole wire for more than it's size makes hole vanish.'''
        obj = doc.getObjectsByLabel('offset-edge')[0]

        small = getWireInside(obj)
        # sanity check
        y = 10
        x = 10 * math.cos(math.pi/6)
        self.assertLines(small.Edges, False, [Vector(0, y, 0), Vector(-x, -y/2, 0), Vector(x, -y/2, 0), Vector(0, y, 0)])
        wire = PathOpTools.offsetWire(small, obj.Shape, 5, True)
        self.assertIsNone(wire)

    def xtest12(self):
        '''Check offsetting a body wire succeeds.'''
        obj = doc.getObjectsByLabel('offset-edge')[0]

        big = getWireOutside(obj)
        # sanity check
        y = 20
        x = 20 * math.cos(math.pi/6)
        self.assertLines(big.Edges, False, [Vector(0, y, 0), Vector(-x, -y/2, 0), Vector(x, -y/2, 0), Vector(0, y, 0)])

        wire = PathOpTools.offsetWire(big, obj.Shape, 5, True)
        self.assertIsNotNone(wire)
        self.assertEqual(6, len(wire.Edges))
        lastAngle = None
        refAngle = math.pi / 3
        for e in wire.Edges:
            if Part.Circle == type(e.Curve):
                self.assertRoughly(5, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)
            else:
                self.assertRoughly(34.641, e.Length, 0.001)
                begin = e.Vertexes[0].Point
                end   = e.Vertexes[1].Point
                v = end - begin
                angle = PathGeom.getAngle(v)
                if PathGeom.isRoughly(0, angle) or PathGeom.isRoughly(math.pi, math.fabs(angle)):
                    if lastAngle:
                        self.assertRoughly(-refAngle, lastAngle)
                elif PathGeom.isRoughly(+refAngle, angle):
                    if lastAngle:
                        self.assertRoughly(math.pi, math.fabs(lastAngle))
                elif PathGeom.isRoughly(-refAngle, angle):
                    if lastAngle:
                        self.assertRoughly(+refAngle, lastAngle)
                else:
                    self.assertIsNone("%s: angle=%s" % (type(e.Curve), angle))
                lastAngle = angle
        f = Part.Face(wire)
        self.assertCoincide(Vector(0, 0, -1), f.Surface.Axis)

    def xtest21(self):
        '''Check offsetting a cylinder.'''
        obj = doc.getObjectsByLabel('circle-cut')[0]

        wire = PathOpTools.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, True)
        self.assertEqual(1, len(wire.Edges))
        edge = wire.Edges[0]
        self.assertCoincide(Vector(), edge.Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), edge.Curve.Axis)
        self.assertRoughly(33, edge.Curve.Radius)

        # the other way around everything's the same except the axis is negative
        wire = PathOpTools.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, False)
        self.assertEqual(1, len(wire.Edges))
        edge = wire.Edges[0]
        self.assertCoincide(Vector(), edge.Curve.Center)
        self.assertCoincide(Vector(0, 0, +1), edge.Curve.Axis)
        self.assertRoughly(33, edge.Curve.Radius)


    def xtest22(self):
        '''Check offsetting a box.'''
        obj = doc.getObjectsByLabel('square-cut')[0]

        wire = PathOpTools.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, True)
        self.assertEqual(8, len(wire.Edges))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Circle == type(e.Curve)]))
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                if PathGeom.isRoughly(e.Vertexes[0].Point.x, e.Vertexes[1].Point.x):
                    self.assertEqual(40, e.Length)
                if PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y):
                    self.assertEqual(60, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(3, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)

        # change offset orientation
        wire = PathOpTools.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, False)
        self.assertEqual(8, len(wire.Edges))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Circle == type(e.Curve)]))
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                if PathGeom.isRoughly(e.Vertexes[0].Point.x, e.Vertexes[1].Point.x):
                    self.assertEqual(40, e.Length)
                if PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y):
                    self.assertEqual(60, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(3, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, +1), e.Curve.Axis)


    def xtest23(self):
        '''Check offsetting a triangle.'''
        obj = doc.getObjectsByLabel('triangle-cut')[0]

        wire = PathOpTools.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, True)
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Circle == type(e.Curve)]))
        length = 60 * math.sin(math.radians(60))
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                self.assertRoughly(length, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(3, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)

        # change offset orientation
        wire = PathOpTools.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, False)
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Circle == type(e.Curve)]))
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                self.assertRoughly(length, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(3, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, +1), e.Curve.Axis)

    def xtest24(self):
        '''Check offsetting a shape.'''
        obj = doc.getObjectsByLabel('shape-cut')[0]

        wire = PathOpTools.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, True)
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Circle == type(e.Curve)]))
        length = 40
        radius = 20 + 3
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                self.assertRoughly(length, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(radius, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)

        # change offset orientation
        wire = PathOpTools.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, False)
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Circle == type(e.Curve)]))
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                self.assertRoughly(length, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(radius, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, +1), e.Curve.Axis)

    def xtest25(self):
        '''Check offsetting a cylindrical hole.'''
        obj = doc.getObjectsByLabel('circle-cut')[0]

        wire = PathOpTools.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, True)
        self.assertEqual(1, len(wire.Edges))
        edge = wire.Edges[0]
        self.assertCoincide(Vector(), edge.Curve.Center)
        self.assertCoincide(Vector(0, 0, +1), edge.Curve.Axis)
        self.assertRoughly(27, edge.Curve.Radius)

        # the other way around everything's the same except the axis is negative
        wire = PathOpTools.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, False)
        self.assertEqual(1, len(wire.Edges))
        edge = wire.Edges[0]
        self.assertCoincide(Vector(), edge.Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), edge.Curve.Axis)
        self.assertRoughly(27, edge.Curve.Radius)


    def xtest26(self):
        '''Check offsetting a square hole.'''
        obj = doc.getObjectsByLabel('square-cut')[0]

        wire = PathOpTools.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, True)
        self.assertEqual(4, len(wire.Edges))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        for e in wire.Edges:
            if PathGeom.isRoughly(e.Vertexes[0].Point.x, e.Vertexes[1].Point.x):
                self.assertRoughly(34, e.Length)
            if PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y):
                self.assertRoughly(54, e.Length)

        # change offset orientation
        wire = PathOpTools.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, False)
        self.assertEqual(4, len(wire.Edges))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        for e in wire.Edges:
            if PathGeom.isRoughly(e.Vertexes[0].Point.x, e.Vertexes[1].Point.x):
                self.assertRoughly(34, e.Length)
            if PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y):
                self.assertRoughly(54, e.Length)


    def xtest27(self):
        '''Check offsetting a triangular holee.'''
        obj = doc.getObjectsByLabel('triangle-cut')[0]

        wire = PathOpTools.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, True)
        self.assertEqual(3, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        length = 48 * math.sin(math.radians(60))
        for e in wire.Edges:
            self.assertRoughly(length, e.Length)
        f = Part.Face(wire)
        self.assertCoincide(Vector(0, 0, -1), f.Surface.Axis)

        # change offset orientation
        wire = PathOpTools.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, False)
        self.assertEqual(3, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        for e in wire.Edges:
            self.assertRoughly(length, e.Length)
        f = Part.Face(wire)
        self.assertCoincide(Vector(0, 0, +1), f.Surface.Axis)

    def xtest28(self):
        '''Check offsetting a shape hole.'''
        obj = doc.getObjectsByLabel('shape-cut')[0]

        wire = PathOpTools.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, True)
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Circle == type(e.Curve)]))
        length = 40
        radius = 20 - 3
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                self.assertRoughly(length, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(radius, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)

        # change offset orientation
        wire = PathOpTools.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, False)
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Circle == type(e.Curve)]))
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                self.assertRoughly(length, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(radius, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, +1), e.Curve.Axis)


    def xtest30(self):
        '''Check offsetting a single outside edge forward.'''
        obj = doc.getObjectsByLabel('offset-edge')[0]

        w = getWireOutside(obj)
        length = 40 * math.cos(math.pi/6)
        for e in w.Edges:
            self.assertRoughly(length, e.Length)

        # let's offset the horizontal edge for starters
        hEdges = [e for e in w.Edges if PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)]

        x = length / 2
        y = -10
        self.assertEqual(1, len(hEdges))
        edge = hEdges[0]

        self.assertCoincide(Vector(-x, y, 0), edge.Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), edge.Vertexes[1].Point)

        wire = PathOpTools.offsetWire(Part.Wire([edge]), obj.Shape, 5, True)
        self.assertEqual(1, len(wire.Edges))

        y = y - 5
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[1].Point)

        # make sure we get the same result even if the edge is oriented the other way
        edge = PathGeom.flipEdge(edge)
        wire = PathOpTools.offsetWire(Part.Wire([edge]), obj.Shape, 5, True)
        self.assertEqual(1, len(wire.Edges))

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[1].Point)

    def xtest31(self):
        '''Check offsetting a single outside edge not forward.'''
        obj = doc.getObjectsByLabel('offset-edge')[0]

        w = getWireOutside(obj)
        length = 40 * math.cos(math.pi/6)
        for e in w.Edges:
            self.assertRoughly(length, e.Length)

        # let's offset the horizontal edge for starters
        hEdges = [e for e in w.Edges if PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)]

        x = length / 2
        y = -10
        self.assertEqual(1, len(hEdges))
        edge = hEdges[0]
        self.assertCoincide(Vector(-x, y, 0), edge.Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), edge.Vertexes[1].Point)

        wire = PathOpTools.offsetWire(Part.Wire([edge]), obj.Shape, 5, False)
        self.assertEqual(1, len(wire.Edges))

        y = y - 5
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[1].Point)

        # make sure we get the same result on a reversed edge
        edge = PathGeom.flipEdge(edge)
        wire = PathOpTools.offsetWire(Part.Wire([edge]), obj.Shape, 5, False)
        self.assertEqual(1, len(wire.Edges))

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[1].Point)

    def xtest32(self):
        '''Check offsetting multiple outside edges.'''
        obj = doc.getObjectsByLabel('offset-edge')[0]

        w = getWireOutside(obj)
        length = 40 * math.cos(math.pi/6)

        # let's offset the other two legs
        lEdges = [e for e in w.Edges if not PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)]
        self.assertEqual(2, len(lEdges))

        wire = PathOpTools.offsetWire(Part.Wire(lEdges), obj.Shape, 2, True)

        x = length/2 + 2 * math.cos(math.pi/6)
        y = -10 + 2 * math.sin(math.pi/6)

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]

        self.assertEqual(1, len(rEdges))
        self.assertCoincide(Vector(0, 20, 0), rEdges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), rEdges[0].Curve.Axis)

        #offset the other way
        wire = PathOpTools.offsetWire(Part.Wire(lEdges), obj.Shape, 2, False)

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]

        self.assertEqual(1, len(rEdges))
        self.assertCoincide(Vector(0, 20, 0), rEdges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, +1), rEdges[0].Curve.Axis)

    def xtest33(self):
        '''Check offsetting multiple backwards outside edges.'''
        # This is exactly the same as test32, except that the wire is flipped to make
        # sure the input orientation doesn't matter
        obj = doc.getObjectsByLabel('offset-edge')[0]

        w = getWireOutside(obj)
        length = 40 * math.cos(math.pi/6)

        # let's offset the other two legs
        lEdges = [e for e in w.Edges if not PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)]
        self.assertEqual(2, len(lEdges))

        w = PathGeom.flipWire(Part.Wire(lEdges))
        wire = PathOpTools.offsetWire(w, obj.Shape, 2, True)

        x = length/2 + 2 * math.cos(math.pi/6)
        y = -10 + 2 * math.sin(math.pi/6)

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]

        self.assertEqual(1, len(rEdges))
        self.assertCoincide(Vector(0, 20, 0), rEdges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), rEdges[0].Curve.Axis)

        #offset the other way
        wire = PathOpTools.offsetWire(Part.Wire(lEdges), obj.Shape, 2, False)

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]

        self.assertEqual(1, len(rEdges))
        self.assertCoincide(Vector(0, 20, 0), rEdges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, +1), rEdges[0].Curve.Axis)

    def xtest34(self):
        '''Check offsetting a single inside edge forward.'''
        obj = doc.getObjectsByLabel('offset-edge')[0]

        w = getWireInside(obj)
        length = 20 * math.cos(math.pi/6)
        for e in w.Edges:
            self.assertRoughly(length, e.Length)

        # let's offset the horizontal edge for starters
        hEdges = [e for e in w.Edges if PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)]

        x = length / 2
        y = -5
        self.assertEqual(1, len(hEdges))
        edge = hEdges[0]

        self.assertCoincide(Vector(-x, y, 0), edge.Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), edge.Vertexes[1].Point)

        wire = PathOpTools.offsetWire(Part.Wire([edge]), obj.Shape, 2, True)
        self.assertEqual(1, len(wire.Edges))

        y = y + 2
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[1].Point)

        # make sure we get the same result even if the edge is oriented the other way
        edge = PathGeom.flipEdge(edge)
        wire = PathOpTools.offsetWire(Part.Wire([edge]), obj.Shape, 2, True)
        self.assertEqual(1, len(wire.Edges))

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[1].Point)

    def xtest35(self):
        '''Check offsetting a single inside edge not forward.'''
        obj = doc.getObjectsByLabel('offset-edge')[0]

        w = getWireInside(obj)
        length = 20 * math.cos(math.pi/6)
        for e in w.Edges:
            self.assertRoughly(length, e.Length)

        # let's offset the horizontal edge for starters
        hEdges = [e for e in w.Edges if PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)]

        x = length / 2
        y = -5
        self.assertEqual(1, len(hEdges))
        edge = hEdges[0]

        self.assertCoincide(Vector(-x, y, 0), edge.Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), edge.Vertexes[1].Point)

        wire = PathOpTools.offsetWire(Part.Wire([edge]), obj.Shape, 2, False)
        self.assertEqual(1, len(wire.Edges))

        y = y + 2
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[1].Point)

        # make sure we get the same result even if the edge is oriented the other way
        edge = PathGeom.flipEdge(edge)
        wire = PathOpTools.offsetWire(Part.Wire([edge]), obj.Shape, 2, False)
        self.assertEqual(1, len(wire.Edges))

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[1].Point)

    def xtest36(self):
        '''Check offsetting multiple inside edges.'''
        obj = doc.getObjectsByLabel('offset-edge')[0]

        w = getWireInside(obj)
        length = 20 * math.cos(math.pi/6)

        # let's offset the other two legs
        lEdges = [e for e in w.Edges if not PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)]
        self.assertEqual(2, len(lEdges))

        wire = PathOpTools.offsetWire(Part.Wire(lEdges), obj.Shape, 2, True)

        x = length/2 - 2 * math.cos(math.pi/6)
        y = -5 - 2 * math.sin(math.pi/6)

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]
        self.assertEqual(0, len(rEdges))

        #offset the other way
        wire = PathOpTools.offsetWire(Part.Wire(lEdges), obj.Shape, 2, False)

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]
        self.assertEqual(0, len(rEdges))

    def xtest37(self):
        '''Check offsetting multiple backwards inside edges.'''
        # This is exactly the same as test36 except that the wire is flipped to make
        # sure it's orientation doesn't matter
        obj = doc.getObjectsByLabel('offset-edge')[0]

        w = getWireInside(obj)
        length = 20 * math.cos(math.pi/6)

        # let's offset the other two legs
        lEdges = [e for e in w.Edges if not PathGeom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)]
        self.assertEqual(2, len(lEdges))

        w = PathGeom.flipWire(Part.Wire(lEdges))
        wire = PathOpTools.offsetWire(w, obj.Shape, 2, True)

        x = length/2 - 2 * math.cos(math.pi/6)
        y = -5 - 2 * math.sin(math.pi/6)

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]
        self.assertEqual(0, len(rEdges))

        #offset the other way
        wire = PathOpTools.offsetWire(Part.Wire(lEdges), obj.Shape, 2, False)

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]
        self.assertEqual(0, len(rEdges))

