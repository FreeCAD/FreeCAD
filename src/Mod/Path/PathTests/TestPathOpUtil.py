# -*- coding: utf-8 -*-
# ***************************************************************************
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
import Path.Op.Util as PathOpUtil
import PathTests.PathTestUtils as PathTestUtils
import math

from FreeCAD import Vector

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
# Path.Log.trackModule(Path.Log.thisModule())

DOC = FreeCAD.getHomePath() + "Mod/Path/PathTests/test_geomop.fcstd"

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


def makeWire(pts):
    edges = []
    first = pts[0]
    last = pts[0]
    for p in pts[1:]:
        edges.append(Part.Edge(Part.LineSegment(last, p)))
        last = p
    edges.append(Part.Edge(Part.LineSegment(last, first)))
    return Part.Wire(edges)


def wireMarkers(wire):
    pts = [wire.Edges[0].valueAt(wire.Edges[0].FirstParameter)]
    for edge in wire.Edges:
        pts.append(edge.valueAt(edge.LastParameter))
    return pts


class TestPathOpUtil(PathTestUtils.PathTestBase):

    def test00(self):
        """Verify isWireClockwise for polygon wires."""
        pa = Vector(1, 1, 0)
        pb = Vector(1, 5, 0)
        pc = Vector(5, 5, 0)
        pd = Vector(5, 1, 0)

        self.assertTrue(PathOpUtil.isWireClockwise(makeWire([pa, pb, pc, pd])))
        self.assertFalse(PathOpUtil.isWireClockwise(makeWire([pa, pd, pc, pb])))

    def test01(self):
        """Verify isWireClockwise for single edge circle wires."""
        self.assertTrue(
            PathOpUtil.isWireClockwise(
                Part.makeCircle(5, Vector(1, 2, 3), Vector(0, 0, -1))
            )
        )
        self.assertFalse(
            PathOpUtil.isWireClockwise(
                Part.makeCircle(5, Vector(1, 2, 3), Vector(0, 0, +1))
            )
        )

    def test02(self):
        """Verify isWireClockwise for two half circle wires."""
        e0 = Part.makeCircle(5, Vector(1, 2, 3), Vector(0, 0, -1), 0, 180)
        e1 = Part.makeCircle(5, Vector(1, 2, 3), Vector(0, 0, -1), 180, 360)
        self.assertTrue(PathOpUtil.isWireClockwise(Part.Wire([e0, e1])))

        e0 = Part.makeCircle(5, Vector(1, 2, 3), Vector(0, 0, +1), 0, 180)
        e1 = Part.makeCircle(5, Vector(1, 2, 3), Vector(0, 0, +1), 180, 360)
        self.assertFalse(PathOpUtil.isWireClockwise(Part.Wire([e0, e1])))

    def test03(self):
        """Verify isWireClockwise for two edge wires with an arc."""
        e0 = Part.makeCircle(5, Vector(1, 2, 3), Vector(0, 0, -1), 0, 180)
        e2 = Part.makeLine(e0.valueAt(e0.LastParameter), e0.valueAt(e0.FirstParameter))
        self.assertTrue(PathOpUtil.isWireClockwise(Part.Wire([e0, e2])))

        e0 = Part.makeCircle(5, Vector(1, 2, 3), Vector(0, 0, +1), 0, 180)
        e2 = Part.makeLine(e0.valueAt(e0.LastParameter), e0.valueAt(e0.FirstParameter))
        self.assertFalse(PathOpUtil.isWireClockwise(Part.Wire([e0, e2])))

    def test04(self):
        """Verify isWireClockwise for unoriented wires."""
        e0 = Part.makeCircle(5, Vector(1, 2, 3), Vector(0, 0, -1), 0, 180)
        e3 = Part.makeLine(e0.valueAt(e0.FirstParameter), e0.valueAt(e0.LastParameter))
        self.assertTrue(PathOpUtil.isWireClockwise(Part.Wire([e0, e3])))

        e0 = Part.makeCircle(5, Vector(1, 2, 3), Vector(0, 0, +1), 0, 180)
        e3 = Part.makeLine(e0.valueAt(e0.FirstParameter), e0.valueAt(e0.LastParameter))
        self.assertFalse(PathOpUtil.isWireClockwise(Part.Wire([e0, e3])))

    def test11(self):
        """Check offsetting a circular hole."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-circle")[0]

        small = getWireInside(obj)
        self.assertRoughly(10, small.Edges[0].Curve.Radius)

        wire = PathOpUtil.offsetWire(small, obj.Shape, 3, True)
        self.assertIsNotNone(wire)
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(7, wire.Edges[0].Curve.Radius)
        self.assertCoincide(Vector(0, 0, 1), wire.Edges[0].Curve.Axis)

        wire = PathOpUtil.offsetWire(small, obj.Shape, 9.9, True)
        self.assertIsNotNone(wire)
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(0.1, wire.Edges[0].Curve.Radius)
        self.assertCoincide(Vector(0, 0, 1), wire.Edges[0].Curve.Axis)
        FreeCAD.closeDocument("test_geomop")

    def test12(self):
        """Check offsetting a circular hole by the radius or more makes the hole vanish."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-circle")[0]

        small = getWireInside(obj)
        self.assertRoughly(10, small.Edges[0].Curve.Radius)
        wire = PathOpUtil.offsetWire(small, obj.Shape, 10, True)
        self.assertIsNone(wire)

        wire = PathOpUtil.offsetWire(small, obj.Shape, 15, True)
        self.assertIsNone(wire)
        FreeCAD.closeDocument("test_geomop")

    def test13(self):
        """Check offsetting a cylinder succeeds."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-circle")[0]

        big = getWireOutside(obj)
        self.assertRoughly(20, big.Edges[0].Curve.Radius)

        wire = PathOpUtil.offsetWire(big, obj.Shape, 10, True)
        self.assertIsNotNone(wire)
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(30, wire.Edges[0].Curve.Radius)
        self.assertCoincide(Vector(0, 0, -1), wire.Edges[0].Curve.Axis)

        wire = PathOpUtil.offsetWire(big, obj.Shape, 20, True)
        self.assertIsNotNone(wire)
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(40, wire.Edges[0].Curve.Radius)
        self.assertCoincide(Vector(0, 0, -1), wire.Edges[0].Curve.Axis)
        FreeCAD.closeDocument("test_geomop")

    def test14(self):
        """Check offsetting a hole with Placement."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-placement")[0]

        wires = [
            w
            for w in obj.Shape.Wires
            if 1 == len(w.Edges)
            and Path.Geom.isRoughly(0, w.Edges[0].Vertexes[0].Point.z)
        ]
        self.assertEqual(2, len(wires))
        w = wires[1] if wires[0].BoundBox.isInside(wires[1].BoundBox) else wires[0]

        self.assertRoughly(10, w.Edges[0].Curve.Radius)
        # make sure there is a placement and I didn't mess up the model
        self.assertFalse(Path.Geom.pointsCoincide(Vector(), w.Edges[0].Placement.Base))

        wire = PathOpUtil.offsetWire(w, obj.Shape, 2, True)
        self.assertIsNotNone(wire)
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(8, wire.Edges[0].Curve.Radius)
        self.assertCoincide(Vector(0, 0, 0), wire.Edges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, 1), wire.Edges[0].Curve.Axis)
        FreeCAD.closeDocument("test_geomop")

    def test15(self):
        """Check offsetting a cylinder with Placement."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-placement")[0]

        wires = [
            w
            for w in obj.Shape.Wires
            if 1 == len(w.Edges)
            and Path.Geom.isRoughly(0, w.Edges[0].Vertexes[0].Point.z)
        ]
        self.assertEqual(2, len(wires))
        w = wires[0] if wires[0].BoundBox.isInside(wires[1].BoundBox) else wires[1]

        self.assertRoughly(20, w.Edges[0].Curve.Radius)
        # make sure there is a placement and I didn't mess up the model
        self.assertFalse(Path.Geom.pointsCoincide(Vector(), w.Edges[0].Placement.Base))

        wire = PathOpUtil.offsetWire(w, obj.Shape, 2, True)
        self.assertIsNotNone(wire)
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(22, wire.Edges[0].Curve.Radius)
        self.assertCoincide(Vector(0, 0, 0), wire.Edges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), wire.Edges[0].Curve.Axis)
        FreeCAD.closeDocument("test_geomop")

    def test20(self):
        """Check offsetting hole wire succeeds."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-edge")[0]

        small = getWireInside(obj)
        # sanity check
        y = 10
        x = 10 * math.cos(math.pi / 6)
        self.assertLines(
            small.Edges,
            False,
            [
                Vector(0, y, 0),
                Vector(-x, -y / 2, 0),
                Vector(x, -y / 2, 0),
                Vector(0, y, 0),
            ],
        )

        wire = PathOpUtil.offsetWire(small, obj.Shape, 3, True)
        self.assertIsNotNone(wire)
        self.assertEqual(3, len(wire.Edges))
        self.assertTrue(wire.isClosed())
        # for holes processing "forward" means CCW
        self.assertFalse(PathOpUtil.isWireClockwise(wire))
        y = 4  # offset works in both directions
        x = 4 * math.cos(math.pi / 6)
        self.assertLines(
            wire.Edges,
            False,
            [Vector(0, 4, 0), Vector(-x, -2, 0), Vector(x, -2, 0), Vector(0, 4, 0)],
        )
        FreeCAD.closeDocument("test_geomop")

    def test21(self):
        """Check offsetting hole wire for more than it's size makes hole vanish."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-edge")[0]

        small = getWireInside(obj)
        # sanity check
        y = 10
        x = 10 * math.cos(math.pi / 6)
        self.assertLines(
            small.Edges,
            False,
            [
                Vector(0, y, 0),
                Vector(-x, -y / 2, 0),
                Vector(x, -y / 2, 0),
                Vector(0, y, 0),
            ],
        )
        wire = PathOpUtil.offsetWire(small, obj.Shape, 5, True)
        self.assertIsNone(wire)
        FreeCAD.closeDocument("test_geomop")

    def test22(self):
        """Check offsetting a body wire succeeds."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-edge")[0]

        big = getWireOutside(obj)
        # sanity check
        y = 20
        x = 20 * math.cos(math.pi / 6)
        self.assertLines(
            big.Edges,
            False,
            [
                Vector(0, y, 0),
                Vector(-x, -y / 2, 0),
                Vector(x, -y / 2, 0),
                Vector(0, y, 0),
            ],
        )

        wire = PathOpUtil.offsetWire(big, obj.Shape, 5, True)
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
                end = e.Vertexes[1].Point
                v = end - begin
                angle = Path.Geom.getAngle(v)
                if Path.Geom.isRoughly(0, angle) or Path.Geom.isRoughly(
                    math.pi, math.fabs(angle)
                ):
                    if lastAngle:
                        self.assertRoughly(-refAngle, lastAngle)
                elif Path.Geom.isRoughly(+refAngle, angle):
                    if lastAngle:
                        self.assertRoughly(math.pi, math.fabs(lastAngle))
                elif Path.Geom.isRoughly(-refAngle, angle):
                    if lastAngle:
                        self.assertRoughly(+refAngle, lastAngle)
                else:
                    self.assertIsNone("%s: angle=%s" % (type(e.Curve), angle))
                lastAngle = angle
        self.assertTrue(PathOpUtil.isWireClockwise(wire))
        FreeCAD.closeDocument("test_geomop")

    def test31(self):
        """Check offsetting a cylinder."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("circle-cut")[0]

        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, True)
        self.assertEqual(1, len(wire.Edges))
        edge = wire.Edges[0]
        self.assertCoincide(Vector(), edge.Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), edge.Curve.Axis)
        self.assertRoughly(33, edge.Curve.Radius)

        # the other way around everything's the same except the axis is negative
        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, False)
        self.assertEqual(1, len(wire.Edges))
        edge = wire.Edges[0]
        self.assertCoincide(Vector(), edge.Curve.Center)
        self.assertCoincide(Vector(0, 0, +1), edge.Curve.Axis)
        self.assertRoughly(33, edge.Curve.Radius)
        FreeCAD.closeDocument("test_geomop")

    def test32(self):
        """Check offsetting a box."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("square-cut")[0]

        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, True)
        self.assertEqual(8, len(wire.Edges))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(
            4, len([e for e in wire.Edges if Part.Circle == type(e.Curve)])
        )
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                if Path.Geom.isRoughly(e.Vertexes[0].Point.x, e.Vertexes[1].Point.x):
                    self.assertEqual(40, e.Length)
                if Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y):
                    self.assertEqual(60, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(3, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)
        self.assertTrue(PathOpUtil.isWireClockwise(wire))

        # change offset orientation
        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, False)
        self.assertEqual(8, len(wire.Edges))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(
            4, len([e for e in wire.Edges if Part.Circle == type(e.Curve)])
        )
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                if Path.Geom.isRoughly(e.Vertexes[0].Point.x, e.Vertexes[1].Point.x):
                    self.assertEqual(40, e.Length)
                if Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y):
                    self.assertEqual(60, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(3, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, +1), e.Curve.Axis)
        self.assertFalse(PathOpUtil.isWireClockwise(wire))
        FreeCAD.closeDocument("test_geomop")

    def test33(self):
        """Check offsetting a triangle."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("triangle-cut")[0]

        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, True)
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(
            3, len([e for e in wire.Edges if Part.Circle == type(e.Curve)])
        )
        length = 60 * math.sin(math.radians(60))
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                self.assertRoughly(length, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(3, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)

        # change offset orientation
        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, False)
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(
            3, len([e for e in wire.Edges if Part.Circle == type(e.Curve)])
        )
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                self.assertRoughly(length, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(3, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, +1), e.Curve.Axis)
        FreeCAD.closeDocument("test_geomop")

    def test34(self):
        """Check offsetting a shape."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("shape-cut")[0]

        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, True)
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(
            3, len([e for e in wire.Edges if Part.Circle == type(e.Curve)])
        )
        length = 40
        radius = 20 + 3
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                self.assertRoughly(length, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(radius, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)

        # change offset orientation
        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, False)
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(
            3, len([e for e in wire.Edges if Part.Circle == type(e.Curve)])
        )
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                self.assertRoughly(length, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(radius, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, +1), e.Curve.Axis)
        FreeCAD.closeDocument("test_geomop")

    def test35(self):
        """Check offsetting a cylindrical hole."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("circle-cut")[0]

        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, True)
        self.assertEqual(1, len(wire.Edges))
        edge = wire.Edges[0]
        self.assertCoincide(Vector(), edge.Curve.Center)
        self.assertCoincide(Vector(0, 0, +1), edge.Curve.Axis)
        self.assertRoughly(27, edge.Curve.Radius)

        # the other way around everything's the same except the axis is negative
        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, False)
        self.assertEqual(1, len(wire.Edges))
        edge = wire.Edges[0]
        self.assertCoincide(Vector(), edge.Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), edge.Curve.Axis)
        self.assertRoughly(27, edge.Curve.Radius)
        FreeCAD.closeDocument("test_geomop")

    def test36(self):
        """Check offsetting a square hole."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("square-cut")[0]

        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, True)
        self.assertEqual(4, len(wire.Edges))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        for e in wire.Edges:
            if Path.Geom.isRoughly(e.Vertexes[0].Point.x, e.Vertexes[1].Point.x):
                self.assertRoughly(34, e.Length)
            if Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y):
                self.assertRoughly(54, e.Length)
        self.assertFalse(PathOpUtil.isWireClockwise(wire))

        # change offset orientation
        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, False)
        self.assertEqual(4, len(wire.Edges))
        self.assertEqual(4, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        for e in wire.Edges:
            if Path.Geom.isRoughly(e.Vertexes[0].Point.x, e.Vertexes[1].Point.x):
                self.assertRoughly(34, e.Length)
            if Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y):
                self.assertRoughly(54, e.Length)
        self.assertTrue(PathOpUtil.isWireClockwise(wire))
        FreeCAD.closeDocument("test_geomop")

    def test37(self):
        """Check offsetting a triangular holee."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("triangle-cut")[0]

        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, True)
        self.assertEqual(3, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        length = 48 * math.sin(math.radians(60))
        for e in wire.Edges:
            self.assertRoughly(length, e.Length)
        self.assertFalse(PathOpUtil.isWireClockwise(wire))

        # change offset orientation
        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, False)
        self.assertEqual(3, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        for e in wire.Edges:
            self.assertRoughly(length, e.Length)
        self.assertTrue(PathOpUtil.isWireClockwise(wire))
        FreeCAD.closeDocument("test_geomop")

    def test38(self):
        """Check offsetting a shape hole."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("shape-cut")[0]

        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, True)
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(
            3, len([e for e in wire.Edges if Part.Circle == type(e.Curve)])
        )
        length = 40
        radius = 20 - 3
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                self.assertRoughly(length, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(radius, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, +1), e.Curve.Axis)

        # change offset orientation
        wire = PathOpUtil.offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, False)
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if Part.Line == type(e.Curve)]))
        self.assertEqual(
            3, len([e for e in wire.Edges if Part.Circle == type(e.Curve)])
        )
        for e in wire.Edges:
            if Part.Line == type(e.Curve):
                self.assertRoughly(length, e.Length)
            if Part.Circle == type(e.Curve):
                self.assertRoughly(radius, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)
        FreeCAD.closeDocument("test_geomop")

    def test40(self):
        """Check offsetting a single outside edge forward."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-edge")[0]

        w = getWireOutside(obj)
        length = 40 * math.cos(math.pi / 6)
        for e in w.Edges:
            self.assertRoughly(length, e.Length)

        # let's offset the horizontal edge for starters
        hEdges = [
            e
            for e in w.Edges
            if Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]

        x = length / 2
        y = -10
        self.assertEqual(1, len(hEdges))
        edge = hEdges[0]

        self.assertCoincide(Vector(-x, y, 0), edge.Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), edge.Vertexes[1].Point)

        wire = PathOpUtil.offsetWire(Part.Wire([edge]), obj.Shape, 5, True)
        self.assertEqual(1, len(wire.Edges))

        y = y - 5
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[1].Point)

        # make sure we get the same result even if the edge is oriented the other way
        edge = Path.Geom.flipEdge(edge)
        wire = PathOpUtil.offsetWire(Part.Wire([edge]), obj.Shape, 5, True)
        self.assertEqual(1, len(wire.Edges))

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[1].Point)
        FreeCAD.closeDocument("test_geomop")

    def test41(self):
        """Check offsetting a single outside edge not forward."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-edge")[0]

        w = getWireOutside(obj)
        length = 40 * math.cos(math.pi / 6)
        for e in w.Edges:
            self.assertRoughly(length, e.Length)

        # let's offset the horizontal edge for starters
        hEdges = [
            e
            for e in w.Edges
            if Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]

        x = length / 2
        y = -10
        self.assertEqual(1, len(hEdges))
        edge = hEdges[0]
        self.assertCoincide(Vector(-x, y, 0), edge.Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), edge.Vertexes[1].Point)

        wire = PathOpUtil.offsetWire(Part.Wire([edge]), obj.Shape, 5, False)
        self.assertEqual(1, len(wire.Edges))

        y = y - 5
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[1].Point)

        # make sure we get the same result on a reversed edge
        edge = Path.Geom.flipEdge(edge)
        wire = PathOpUtil.offsetWire(Part.Wire([edge]), obj.Shape, 5, False)
        self.assertEqual(1, len(wire.Edges))

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[1].Point)
        FreeCAD.closeDocument("test_geomop")

    def test42(self):
        """Check offsetting multiple outside edges."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-edge")[0]
        obj.Shape.tessellate(0.01)
        doc.recompute()

        w = getWireOutside(obj)
        length = 40 * math.cos(math.pi / 6)

        # let's offset the other two legs
        lEdges = [
            e
            for e in w.Edges
            if not Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]
        self.assertEqual(2, len(lEdges))

        wire = PathOpUtil.offsetWire(Part.Wire(lEdges), obj.Shape, 2, True)

        x = length / 2 + 2 * math.cos(math.pi / 6)
        y = -10 + 2 * math.sin(math.pi / 6)

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]

        self.assertEqual(1, len(rEdges))
        self.assertCoincide(Vector(0, 20, 0), rEdges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), rEdges[0].Curve.Axis)

        # offset the other way
        wire = PathOpUtil.offsetWire(Part.Wire(lEdges), obj.Shape, 2, False)

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]

        self.assertEqual(1, len(rEdges))
        self.assertCoincide(Vector(0, 20, 0), rEdges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, +1), rEdges[0].Curve.Axis)
        FreeCAD.closeDocument("test_geomop")

    def test43(self):
        """Check offsetting multiple backwards outside edges."""
        # This is exactly the same as test32, except that the wire is flipped to make
        # sure the input orientation doesn't matter
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-edge")[0]

        w = getWireOutside(obj)
        length = 40 * math.cos(math.pi / 6)

        # let's offset the other two legs
        lEdges = [
            e
            for e in w.Edges
            if not Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]
        self.assertEqual(2, len(lEdges))

        w = Path.Geom.flipWire(Part.Wire(lEdges))
        wire = PathOpUtil.offsetWire(w, obj.Shape, 2, True)

        x = length / 2 + 2 * math.cos(math.pi / 6)
        y = -10 + 2 * math.sin(math.pi / 6)

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]

        self.assertEqual(1, len(rEdges))
        self.assertCoincide(Vector(0, 20, 0), rEdges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), rEdges[0].Curve.Axis)

        # offset the other way
        wire = PathOpUtil.offsetWire(Part.Wire(lEdges), obj.Shape, 2, False)

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]

        self.assertEqual(1, len(rEdges))
        self.assertCoincide(Vector(0, 20, 0), rEdges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, +1), rEdges[0].Curve.Axis)
        FreeCAD.closeDocument("test_geomop")

    def test44(self):
        """Check offsetting a single inside edge forward."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-edge")[0]

        w = getWireInside(obj)
        length = 20 * math.cos(math.pi / 6)
        for e in w.Edges:
            self.assertRoughly(length, e.Length)

        # let's offset the horizontal edge for starters
        hEdges = [
            e
            for e in w.Edges
            if Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]

        x = length / 2
        y = -5
        self.assertEqual(1, len(hEdges))
        edge = hEdges[0]

        self.assertCoincide(Vector(-x, y, 0), edge.Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), edge.Vertexes[1].Point)

        wire = PathOpUtil.offsetWire(Part.Wire([edge]), obj.Shape, 2, True)
        self.assertEqual(1, len(wire.Edges))

        y = y + 2
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[1].Point)

        # make sure we get the same result even if the edge is oriented the other way
        edge = Path.Geom.flipEdge(edge)
        wire = PathOpUtil.offsetWire(Part.Wire([edge]), obj.Shape, 2, True)
        self.assertEqual(1, len(wire.Edges))

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[1].Point)
        FreeCAD.closeDocument("test_geomop")

    def test45(self):
        """Check offsetting a single inside edge not forward."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-edge")[0]

        w = getWireInside(obj)
        length = 20 * math.cos(math.pi / 6)
        for e in w.Edges:
            self.assertRoughly(length, e.Length)

        # let's offset the horizontal edge for starters
        hEdges = [
            e
            for e in w.Edges
            if Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]

        x = length / 2
        y = -5
        self.assertEqual(1, len(hEdges))
        edge = hEdges[0]

        self.assertCoincide(Vector(-x, y, 0), edge.Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), edge.Vertexes[1].Point)

        wire = PathOpUtil.offsetWire(Part.Wire([edge]), obj.Shape, 2, False)
        self.assertEqual(1, len(wire.Edges))

        y = y + 2
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[1].Point)

        # make sure we get the same result even if the edge is oriented the other way
        edge = Path.Geom.flipEdge(edge)
        wire = PathOpUtil.offsetWire(Part.Wire([edge]), obj.Shape, 2, False)
        self.assertEqual(1, len(wire.Edges))

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[1].Point)
        FreeCAD.closeDocument("test_geomop")

    def test46(self):
        """Check offsetting multiple inside edges."""
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-edge")[0]

        w = getWireInside(obj)
        length = 20 * math.cos(math.pi / 6)

        # let's offset the other two legs
        lEdges = [
            e
            for e in w.Edges
            if not Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]
        self.assertEqual(2, len(lEdges))

        wire = PathOpUtil.offsetWire(Part.Wire(lEdges), obj.Shape, 2, True)

        x = length / 2 - 2 * math.cos(math.pi / 6)
        y = -5 - 2 * math.sin(math.pi / 6)

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]
        self.assertEqual(0, len(rEdges))

        # offset the other way
        wire = PathOpUtil.offsetWire(Part.Wire(lEdges), obj.Shape, 2, False)

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]
        self.assertEqual(0, len(rEdges))
        FreeCAD.closeDocument("test_geomop")

    def test47(self):
        """Check offsetting multiple backwards inside edges."""
        # This is exactly the same as test36 except that the wire is flipped to make
        # sure it's orientation doesn't matter
        doc = FreeCAD.openDocument(DOC)
        obj = doc.getObjectsByLabel("offset-edge")[0]

        w = getWireInside(obj)
        length = 20 * math.cos(math.pi / 6)

        # let's offset the other two legs
        lEdges = [
            e
            for e in w.Edges
            if not Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]
        self.assertEqual(2, len(lEdges))

        w = Path.Geom.flipWire(Part.Wire(lEdges))
        wire = PathOpUtil.offsetWire(w, obj.Shape, 2, True)

        x = length / 2 - 2 * math.cos(math.pi / 6)
        y = -5 - 2 * math.sin(math.pi / 6)

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]
        self.assertEqual(0, len(rEdges))

        # offset the other way
        wire = PathOpUtil.offsetWire(Part.Wire(lEdges), obj.Shape, 2, False)

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if Part.Circle == type(e.Curve)]
        self.assertEqual(0, len(rEdges))
        FreeCAD.closeDocument("test_geomop")

    def test50(self):
        """Orient an already oriented wire"""
        p0 = Vector()
        p1 = Vector(1, 2, 3)
        p2 = Vector(2, 3, 4)
        pts = [p0, p1, p2]

        e0 = Part.Edge(Part.LineSegment(p0, p1))
        e1 = Part.Edge(Part.LineSegment(p1, p2))

        wire = PathOpUtil.orientWire(Part.Wire([e0, e1]))
        wirePts = wireMarkers(wire)

        self.assertPointsMatch(wirePts, pts)

    def test51(self):
        """Orient a potentially misoriented wire"""
        p0 = Vector()
        p1 = Vector(1, 2, 3)
        p2 = Vector(2, 3, 4)
        pts = [p0, p1, p2]

        e0p = Part.Edge(Part.LineSegment(p0, p1))
        e0m = Part.Edge(Part.LineSegment(p1, p0))
        e1p = Part.Edge(Part.LineSegment(p1, p2))
        e1m = Part.Edge(Part.LineSegment(p2, p1))

        wire = PathOpUtil.orientWire(Part.Wire([e0p, e1p]))
        self.assertPointsMatch(wireMarkers(wire), pts)

        wire = PathOpUtil.orientWire(Part.Wire([e0p, e1m]))
        self.assertPointsMatch(wireMarkers(wire), pts)

        wire = PathOpUtil.orientWire(Part.Wire([e0m, e1p]))
        self.assertPointsMatch(wireMarkers(wire), pts)

        wire = PathOpUtil.orientWire(Part.Wire([e0m, e1m]))
        self.assertPointsMatch(wireMarkers(wire), pts)

    def test52(self):
        """Orient a potentially misoriented longer wire"""
        p0 = Vector()
        p1 = Vector(1, 2, 3)
        p2 = Vector(4, 5, 6)
        p3 = Vector(7, 8, 9)
        pts = [p0, p1, p2, p3]

        e0p = Part.Edge(Part.LineSegment(p0, p1))
        e0m = Part.Edge(Part.LineSegment(p1, p0))
        e1p = Part.Edge(Part.LineSegment(p1, p2))
        e1m = Part.Edge(Part.LineSegment(p2, p1))
        e2p = Part.Edge(Part.LineSegment(p2, p3))
        e2m = Part.Edge(Part.LineSegment(p3, p2))

        wire = PathOpUtil.orientWire(Part.Wire([e0p, e1p, e2p]))
        self.assertPointsMatch(wireMarkers(wire), pts)

        wire = PathOpUtil.orientWire(Part.Wire([e0p, e1m, e2p]))
        self.assertPointsMatch(wireMarkers(wire), pts)

        wire = PathOpUtil.orientWire(Part.Wire([e0m, e1p, e2p]))
        self.assertPointsMatch(wireMarkers(wire), pts)

        wire = PathOpUtil.orientWire(Part.Wire([e0m, e1m, e2p]))
        self.assertPointsMatch(wireMarkers(wire), pts)

        wire = PathOpUtil.orientWire(Part.Wire([e0p, e1p, e2m]))
        self.assertPointsMatch(wireMarkers(wire), pts)

        wire = PathOpUtil.orientWire(Part.Wire([e0p, e1m, e2m]))
        self.assertPointsMatch(wireMarkers(wire), pts)

        wire = PathOpUtil.orientWire(Part.Wire([e0m, e1p, e2m]))
        self.assertPointsMatch(wireMarkers(wire), pts)

        wire = PathOpUtil.orientWire(Part.Wire([e0m, e1m, e2m]))
        self.assertPointsMatch(wireMarkers(wire), pts)
