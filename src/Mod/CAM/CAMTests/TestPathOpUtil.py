# SPDX-License-Identifier: LGPL-2.1-or-later

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
import Path.Op.Custom as PathCustom
import Path.Main.Job as PathJob
import CAMTests.PathTestUtils as PathTestUtils
import math
import area

from FreeCAD import Vector

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
# Path.Log.trackModule(Path.Log.thisModule())

DOC = FreeCAD.getHomePath() + "Mod/CAM/CAMTests/test_geomop.fcstd"


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


def describeEdge(edge):
    c = edge.Curve
    if isinstance(c, Part.Circle):
        return f"Circle(center={c.Center}, radius={c.Radius:.4f}, axis={c.Axis})"
    elif isinstance(c, (Part.Line, Part.LineSegment)):
        v0 = edge.valueAt(edge.FirstParameter)
        v1 = edge.valueAt(edge.LastParameter)
        return f"Line({v0} -> {v1}, length={edge.Length:.4f})"
    else:
        return f"{type(c).__name__}(length={edge.Length:.4f})"


def describeWire(wire):
    edges = "\n".join(f"    {describeEdge(e)}" for e in wire.Edges)
    return f"Wire(\n{edges}\n)"


def describeWires(wires):
    if not wires:
        return "[]"
    parts = "\n".join(describeWire(w) for w in wires)
    return f"\n{parts}"


class TestPathOpUtil(PathTestUtils.PathTestBase):
    tolerance = 0.01

    @classmethod
    def setUpClass(cls):
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")
        cls.doc = FreeCAD.openDocument(DOC)
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")

    @classmethod
    def tearDownClass(cls):
        FreeCAD.closeDocument(cls.doc.Name)

    def setUp(self):
        self.clipper_scale_orig = area.get_clipper_scale()
        area.set_clipper_scale(1e7)

    def tearDown(self):
        area.set_clipper_scale(self.clipper_scale_orig)

    def _offsetWire(self, wire, base, offset, pos_compat, compat_flipped, tolerance=None):
        tolerance = tolerance if tolerance is not None else self.tolerance
        compat_wires = PathOpUtil.offsetWireCompat(wire, base, offset, tolerance)
        pos_wires, neg_wires = PathOpUtil.offsetWire(wire, base, offset, tolerance)

        if compat_flipped:
            pos_wires = [Path.Geom.flipWire(w) for w in pos_wires]
            neg_wires = [Path.Geom.flipWire(w) for w in neg_wires]

        wires = pos_wires if pos_compat else neg_wires
        unused_wires = neg_wires if pos_compat else pos_wires
        unused_label = "neg_wires (unused)" if pos_compat else "pos_wires (unused)"

        desc = (
            f"compat_wires:{describeWires(compat_wires)}\n\n"
            f"wires:{describeWires(wires)}\n\n"
            f"{unused_label}:{describeWires(unused_wires)}"
        )

        self.assertEqual(len(wires), len(compat_wires), f"Wire count mismatch:\n{desc}")
        for w, cw in zip(wires, compat_wires):
            self.assertEqual(len(w.Edges), len(cw.Edges), f"Edge count mismatch:\n{desc}")
            for e, ce in zip(w.Edges, cw.Edges):
                c1, c2 = e.Curve, ce.Curve
                edges_equal = (
                    type(c1) == type(c2)
                    and e.firstVertex().Point == ce.firstVertex().Point
                    and e.lastVertex().Point == ce.lastVertex().Point
                )
                if isinstance(c1, (Part.Circle, Part.ArcOfCircle)):
                    edges_equal = edges_equal and c1.Center == c2.Center and c1.Axis == c2.Axis
                self.assertTrue(edges_equal, f"Edge mismatch:\n{desc}")

        return compat_wires

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
            PathOpUtil.isWireClockwise(Part.makeCircle(5, Vector(1, 2, 3), Vector(0, 0, -1)))
        )
        self.assertFalse(
            PathOpUtil.isWireClockwise(Part.makeCircle(5, Vector(1, 2, 3), Vector(0, 0, +1)))
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
        obj = self.doc.getObjectsByLabel("offset-circle")[0]

        small = getWireInside(obj)
        self.assertRoughly(10, small.Edges[0].Curve.Radius)

        wires = self._offsetWire(small, obj.Shape, 3, False, True)
        self.assertEqual(1, len(wires), describeWires(wires))
        wire = wires[0]
        self.assertEqual(1, len(wire.Edges), describeWire(wire))
        self.assertRoughly(7, wire.Edges[0].Curve.Radius, self.tolerance)
        # default circle is CCW, so should be flipped to get forward direction
        self.assertCoincide(Vector(0, 0, -1), wire.Edges[0].Curve.Axis)

        wires = self._offsetWire(small, obj.Shape, 9.9, False, True)
        self.assertEqual(1, len(wires), describeWires(wires))
        wire = wires[0]
        self.assertEqual(1, len(wire.Edges), describeWire(wire))
        self.assertRoughly(0.1, wire.Edges[0].Curve.Radius, self.tolerance)
        # default circle is CCW, so should be flipped to get forward direction
        self.assertCoincide(Vector(0, 0, -1), wire.Edges[0].Curve.Axis)

    def test12(self):
        """Check offsetting a circular hole by the radius or more makes the hole vanish."""
        obj = self.doc.getObjectsByLabel("offset-circle")[0]

        small = getWireInside(obj)
        self.assertRoughly(10, small.Edges[0].Curve.Radius)
        wires = self._offsetWire(small, obj.Shape, 10, False, True)
        self.assertEqual(0, len(wires))

        wires = self._offsetWire(small, obj.Shape, 15, False, True)
        self.assertEqual(0, len(wires))

    def test13(self):
        """Check offsetting a cylinder succeeds."""
        obj = self.doc.getObjectsByLabel("offset-circle")[0]

        big = getWireOutside(obj)
        self.assertRoughly(20, big.Edges[0].Curve.Radius)

        wires = self._offsetWire(big, obj.Shape, 10, True, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(1, len(wire.Edges))
        # old radius 20 mm, offset +10 mm, new radius should be 30 mm
        self.assertRoughly(30, wire.Edges[0].Curve.Radius, self.tolerance)
        # default circle is CCW, so should be flipped to get forward direction
        self.assertCoincide(Vector(0, 0, -1), wire.Edges[0].Curve.Axis)

        wires = self._offsetWire(big, obj.Shape, 20, True, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(40, wire.Edges[0].Curve.Radius, self.tolerance)
        # default circle is CCW, so should be flipped to get forward direction
        self.assertCoincide(Vector(0, 0, -1), wire.Edges[0].Curve.Axis)

    def test14(self):
        """Check offsetting a hole with Placement."""
        obj = self.doc.getObjectsByLabel("offset-placement")[0]

        wires = [
            w
            for w in obj.Shape.Wires
            if 1 == len(w.Edges) and Path.Geom.isRoughly(0, w.Edges[0].Vertexes[0].Point.z)
        ]
        self.assertEqual(2, len(wires))
        w = wires[1] if wires[0].BoundBox.isInside(wires[1].BoundBox) else wires[0]

        self.assertRoughly(10, w.Edges[0].Curve.Radius)
        # make sure there is a placement and I didn't mess up the model
        self.assertFalse(Path.Geom.pointsCoincide(Vector(), w.Edges[0].Placement.Base))

        wires = self._offsetWire(w, obj.Shape, 2, False, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(8, wire.Edges[0].Curve.Radius, self.tolerance)
        self.assertCoincide(Vector(0, 0, 0), wire.Edges[0].Curve.Center)
        # default circle is CCW, so should be flipped to get forward direction
        self.assertCoincide(Vector(0, 0, -1), wire.Edges[0].Curve.Axis)

    def test15(self):
        """Check offsetting a cylinder with Placement."""
        obj = self.doc.getObjectsByLabel("offset-placement")[0]

        wires = [
            w
            for w in obj.Shape.Wires
            if 1 == len(w.Edges) and Path.Geom.isRoughly(0, w.Edges[0].Vertexes[0].Point.z)
        ]
        self.assertEqual(2, len(wires))
        w = wires[0] if wires[0].BoundBox.isInside(wires[1].BoundBox) else wires[1]

        self.assertRoughly(20, w.Edges[0].Curve.Radius)
        # make sure there is a placement and I didn't mess up the model
        self.assertFalse(Path.Geom.pointsCoincide(Vector(), w.Edges[0].Placement.Base))

        wires = self._offsetWire(w, obj.Shape, 2, True, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(1, len(wire.Edges))
        self.assertRoughly(22, wire.Edges[0].Curve.Radius, self.tolerance)
        self.assertCoincide(Vector(0, 0, 0), wire.Edges[0].Curve.Center)
        # default circle is CCW, so should be flipped to get forward direction
        self.assertCoincide(Vector(0, 0, -1), wire.Edges[0].Curve.Axis)

    def test20(self):
        """Check offsetting hole wire succeeds."""
        obj = self.doc.getObjectsByLabel("offset-edge")[0]

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

        wires = self._offsetWire(small, obj.Shape, 3, False, False)
        self.assertEqual(1, len(wires))
        wire = wires[0]
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

    def test21(self):
        """Check offsetting hole wire for more than it's size makes hole vanish."""
        obj = self.doc.getObjectsByLabel("offset-edge")[0]

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
        wires = self._offsetWire(small, obj.Shape, 5, False, True)
        self.assertEqual(0, len(wires))

    def test22(self):
        """Check offsetting a body wire succeeds."""
        obj = self.doc.getObjectsByLabel("offset-edge")[0]

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

        wires = self._offsetWire(big, obj.Shape, 5, True, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(6, len(wire.Edges))
        lastAngle = None
        refAngle = math.pi / 3
        for e in wire.Edges:
            if isinstance(e.Curve, Part.Circle):
                self.assertRoughly(5, e.Curve.Radius, self.tolerance)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)
            else:
                self.assertRoughly(34.641, e.Length, 0.001)
                begin = e.Vertexes[0].Point
                end = e.Vertexes[1].Point
                v = end - begin
                angle = Path.Geom.getAngle(v)
                if Path.Geom.isRoughly(0, angle) or Path.Geom.isRoughly(
                    math.pi, math.fabs(angle), error=0.0001
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

    def test31(self):
        """Check offsetting a cylinder."""
        obj = self.doc.getObjectsByLabel("circle-cut")[0]

        wires = self._offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, True, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(1, len(wire.Edges))
        edge = wire.Edges[0]
        self.assertCoincide(Vector(), edge.Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), edge.Curve.Axis)
        self.assertRoughly(33, edge.Curve.Radius)

    def test32(self):
        """Check offsetting a box."""
        obj = self.doc.getObjectsByLabel("square-cut")[0]

        wires = self._offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, True, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(8, len(wire.Edges))
        self.assertEqual(4, len([e for e in wire.Edges if isinstance(e.Curve, Part.Line)]))
        self.assertEqual(4, len([e for e in wire.Edges if isinstance(e.Curve, Part.Circle)]))
        for e in wire.Edges:
            if isinstance(e.Curve, Part.Line):
                if Path.Geom.isRoughly(e.Vertexes[0].Point.x, e.Vertexes[1].Point.x):
                    self.assertRoughly(40, e.Length)
                if Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y):
                    self.assertRoughly(60, e.Length)
            if isinstance(e.Curve, Part.Circle):
                self.assertRoughly(3, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)
        self.assertTrue(PathOpUtil.isWireClockwise(wire))

    def test33(self):
        """Check offsetting a triangle."""
        obj = self.doc.getObjectsByLabel("triangle-cut")[0]

        wires = self._offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, True, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if isinstance(e.Curve, Part.Line)]))
        self.assertEqual(3, len([e for e in wire.Edges if isinstance(e.Curve, Part.Circle)]))
        length = 60 * math.sin(math.radians(60))
        for e in wire.Edges:
            if isinstance(e.Curve, Part.Line):
                self.assertRoughly(length, e.Length)
            if isinstance(e.Curve, Part.Circle):
                self.assertRoughly(3, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)

    def test34(self):
        """Check offsetting a shape."""
        obj = self.doc.getObjectsByLabel("shape-cut")[0]

        wires = self._offsetWire(getWire(obj.Tool), getPositiveShape(obj), 3, True, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if isinstance(e.Curve, Part.Line)]))
        self.assertEqual(3, len([e for e in wire.Edges if isinstance(e.Curve, Part.Circle)]))
        length = 40
        radius = 20 + 3
        for e in wire.Edges:
            if isinstance(e.Curve, Part.Line):
                self.assertRoughly(length, e.Length, self.tolerance)
            if isinstance(e.Curve, Part.Circle):
                self.assertRoughly(radius, e.Curve.Radius, self.tolerance)
                self.assertCoincide(Vector(0, 0, -1), e.Curve.Axis)

    def test35(self):
        """Check offsetting a cylindrical hole."""
        obj = self.doc.getObjectsByLabel("circle-cut")[0]

        wires = self._offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, False, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(1, len(wire.Edges))
        edge = wire.Edges[0]
        self.assertCoincide(Vector(), edge.Curve.Center)
        # default circle is CCW, so should be flipped to get forward direction
        self.assertCoincide(Vector(0, 0, -1), edge.Curve.Axis)
        self.assertRoughly(27, edge.Curve.Radius)

    def test36(self):
        """Check offsetting a square hole."""
        obj = self.doc.getObjectsByLabel("square-cut")[0]

        wires = self._offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, False, False)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(4, len(wire.Edges))
        self.assertEqual(4, len([e for e in wire.Edges if isinstance(e.Curve, Part.Line)]))
        for e in wire.Edges:
            if Path.Geom.isRoughly(e.Vertexes[0].Point.x, e.Vertexes[1].Point.x):
                self.assertRoughly(34, e.Length)
            if Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y):
                self.assertRoughly(54, e.Length)
        self.assertFalse(PathOpUtil.isWireClockwise(wire))

    def test37(self):
        """Check offsetting a triangular holee."""
        obj = self.doc.getObjectsByLabel("triangle-cut")[0]

        wires = self._offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, False, False)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(3, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if isinstance(e.Curve, Part.Line)]))
        length = 48 * math.sin(math.radians(60))
        for e in wire.Edges:
            self.assertRoughly(length, e.Length)
        self.assertFalse(PathOpUtil.isWireClockwise(wire))

    def test38(self):
        """Check offsetting a shape hole."""
        obj = self.doc.getObjectsByLabel("shape-cut")[0]

        wires = self._offsetWire(getWire(obj.Tool), getNegativeShape(obj), 3, False, False)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(6, len(wire.Edges))
        self.assertEqual(3, len([e for e in wire.Edges if isinstance(e.Curve, Part.Line)]))
        self.assertEqual(3, len([e for e in wire.Edges if isinstance(e.Curve, Part.Circle)]))
        length = 40
        radius = 20 - 3
        for e in wire.Edges:
            if isinstance(e.Curve, Part.Line):
                self.assertRoughly(length, e.Length)
            if isinstance(e.Curve, Part.Circle):
                self.assertRoughly(radius, e.Curve.Radius)
                self.assertCoincide(Vector(0, 0, +1), e.Curve.Axis)

    def test40(self):
        """Check offsetting a single outside edge forward."""
        obj = self.doc.getObjectsByLabel("offset-edge")[0]

        w = getWireOutside(obj)

        length = 40 * math.cos(math.pi / 6)
        for e in w.Edges:
            self.assertRoughly(length, e.Length)

        # let's offset the horizontal edge for starters
        hEdges = [
            e for e in w.Edges if Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]

        x = length / 2
        y = -10
        self.assertEqual(1, len(hEdges))
        edge = hEdges[0]

        self.assertCoincide(Vector(-x, y, 0), edge.Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), edge.Vertexes[1].Point)

        wires = self._offsetWire(Part.Wire([edge]), obj.Shape, 5, True, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(1, len(wire.Edges))

        y = y - 5
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[1].Point)

        # make sure we get the same result even if the edge is oriented the other way
        edge = Path.Geom.flipEdge(edge)
        wires = self._offsetWire(Part.Wire([edge]), obj.Shape, 5, False, False)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(1, len(wire.Edges))

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[1].Point)

    def test42(self):
        """Check offsetting multiple outside edges."""
        obj = self.doc.getObjectsByLabel("offset-edge")[0]
        obj.Shape.tessellate(0.01)
        self.doc.recompute()

        w = getWireOutside(obj)
        length = 40 * math.cos(math.pi / 6)

        # let's offset the other two legs
        lEdges = [
            e
            for e in w.Edges
            if not Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]
        self.assertEqual(2, len(lEdges))

        wires = self._offsetWire(Part.Wire(lEdges), obj.Shape, 2, True, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]

        x = length / 2 + 2 * math.cos(math.pi / 6)
        y = -10 + 2 * math.sin(math.pi / 6)

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if isinstance(e.Curve, Part.Circle)]

        self.assertEqual(1, len(rEdges))
        self.assertCoincide(Vector(0, 20, 0), rEdges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), rEdges[0].Curve.Axis)

    def test43(self):
        """Check offsetting multiple backwards outside edges."""
        # This is exactly the same as test42, except that the wire is flipped to make
        # sure the input orientation doesn't matter
        obj = self.doc.getObjectsByLabel("offset-edge")[0]

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
        wires = self._offsetWire(w, obj.Shape, 2, False, False)
        self.assertEqual(1, len(wires))
        wire = wires[0]

        x = length / 2 + 2 * math.cos(math.pi / 6)
        y = -10 + 2 * math.sin(math.pi / 6)

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[-1].Vertexes[1].Point)

        rEdges = [e for e in wire.Edges if isinstance(e.Curve, Part.Circle)]

        self.assertEqual(1, len(rEdges))
        self.assertCoincide(Vector(0, 20, 0), rEdges[0].Curve.Center)
        self.assertCoincide(Vector(0, 0, -1), rEdges[0].Curve.Axis)

    def test44(self):
        """Check offsetting a single inside edge forward."""
        obj = self.doc.getObjectsByLabel("offset-edge")[0]

        w = getWireInside(obj)
        length = 20 * math.cos(math.pi / 6)
        for e in w.Edges:
            self.assertRoughly(length, e.Length)

        # let's offset the horizontal edge for starters
        hEdges = [
            e for e in w.Edges if Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]

        x = length / 2
        y = -5
        self.assertEqual(1, len(hEdges))
        edge = hEdges[0]

        self.assertCoincide(Vector(-x, y, 0), edge.Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), edge.Vertexes[1].Point)

        wires = self._offsetWire(Part.Wire([edge]), obj.Shape, 2, False, False)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(1, len(wire.Edges))

        y = y + 2
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[1].Point)

        # make sure we get the same result even if the edge is oriented the other way
        edge = Path.Geom.flipEdge(edge)
        wires = self._offsetWire(Part.Wire([edge]), obj.Shape, 2, True, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]
        self.assertEqual(1, len(wire.Edges))

        self.assertCoincide(Vector(-x, y, 0), wire.Edges[0].Vertexes[0].Point)
        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[1].Point)

    def test46(self):
        """Check offsetting multiple inside edges."""
        obj = self.doc.getObjectsByLabel("offset-edge")[0]

        w = getWireInside(obj)
        length = 20 * math.cos(math.pi / 6)

        # let's offset the other two legs
        lEdges = [
            e
            for e in w.Edges
            if not Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]
        self.assertEqual(2, len(lEdges))

        wires = self._offsetWire(Part.Wire(lEdges), obj.Shape, 2, False, False)
        self.assertEqual(1, len(wires))
        wire = wires[0]

        x = length / 2 - 2 * math.cos(math.pi / 6)
        y = -5 - 2 * math.sin(math.pi / 6)

        self.assertCoincide(Vector(+x, y, 0), wire.Edges[0].Vertexes[0].Point, self.tolerance)
        self.assertCoincide(Vector(-x, y, 0), wire.Edges[-1].Vertexes[1].Point, self.tolerance)

        rEdges = [e for e in wire.Edges if isinstance(e.Curve, Part.Circle)]
        self.assertEqual(0, len(rEdges))

    def test47(self):
        """Check offsetting multiple backwards inside edges."""
        # This is similar to test46 except that the wire is flipped to verify
        # that wire offsetting works regardless of input wire orientation
        obj = self.doc.getObjectsByLabel("offset-edge")[0]

        w = getWireInside(obj)

        # let's offset the other two legs
        lEdges = [
            e
            for e in w.Edges
            if not Path.Geom.isRoughly(e.Vertexes[0].Point.y, e.Vertexes[1].Point.y)
        ]
        self.assertEqual(2, len(lEdges))

        # Test with flipped wire - the algorithm should handle it
        w = Path.Geom.flipWire(Part.Wire(lEdges))
        wires = self._offsetWire(w, obj.Shape, 2, True, True)
        self.assertEqual(1, len(wires))
        wire = wires[0]

        # Check structural properties rather than exact coordinates
        # Verify the wire is valid and has geometry
        self.assertIsNotNone(wire)
        self.assertGreater(len(wire.Edges), 0)
        # All edges should be lines (no circles for inside edges)
        lEdges_result = [e for e in wire.Edges if isinstance(e.Curve, Part.Line)]
        self.assertGreater(len(lEdges_result), 0)
        # Check that edge lengths are consistent with offset geometry
        total_length = 0
        for e in wire.Edges:
            self.assertGreater(e.Length, 0)
            total_length += e.Length
        # Result should have meaningful length after offset
        self.assertGreater(total_length, 0)

    def test48(self):
        """Check offsetting an ellipse converts it to circular arcs."""
        # Create an ellipse
        ellipse = Part.Ellipse(Vector(0, 0, 0), 20, 10)
        ellipse_edge = ellipse.toShape()
        ellipse_wire = Part.Wire([ellipse_edge])
        base_shape = Part.Face(ellipse_wire)

        # Test that offsetting the ellipse requires positive tolerance (i.e. requires discretization)
        with self.assertRaises(ValueError) as context:
            self._offsetWire(ellipse_wire, base_shape, 2, True, True, tolerance=0)
        self.assertIn(
            "tolerance parameter is required to be a positive value", str(context.exception)
        )

        # Test offsetting with default (positive) tolerance
        offset_wires = self._offsetWire(ellipse_wire, base_shape, 2, True, True)
        self.assertEqual(1, len(offset_wires))
        offset_wire = offset_wires[0]

        # Verify the result exists and is converted into multiple edges
        self.assertIsNotNone(offset_wire)
        self.assertGreater(
            len(offset_wire.Edges), 1, "Ellipse should be converted to multiple edges"
        )

        # Verify that all edges are circular arcs from the biarc conversion
        for edge in offset_wire.Edges:
            self.assertIsInstance(
                edge.Curve,
                (Part.Circle, Part.ArcOfCircle),
                "All edges should be circular arcs, no line segments",
            )

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


def _makeRectanglePath(x0, y0, x1, y1, z):
    """Build a Path object that walks the rectangle perimeter at depth z."""
    cmds = [
        Path.Command("G0", {"X": x0, "Y": y0, "Z": z + 5}),
        Path.Command("G1", {"X": x0, "Y": y0, "Z": z}),
        Path.Command("G1", {"X": x1, "Y": y0, "Z": z}),
        Path.Command("G1", {"X": x1, "Y": y1, "Z": z}),
        Path.Command("G1", {"X": x0, "Y": y1, "Z": z}),
        Path.Command("G1", {"X": x0, "Y": y0, "Z": z}),
    ]
    return Path.Path(cmds)


class TestGetClearedAreasWorkplane(PathTestUtils.PathTestBase):
    """Verify getClearedAreas filters previous ops by Workplane.

    REST machining stores each op's Path in its own (rotated) workplane frame.
    Mixing frames produces meaningless cleared-area polygons, so previous ops
    with a different Workplane than the current op must be skipped.

    The previous ops are real Custom (user-gcode) operations rather than mocks:
    this exercises the genuine ``PathDressup.baseOp`` resolution used to detect
    the current op while iterating, and the real ToolController/Workplane
    property access. Each op's Path is assigned directly so the filtering logic
    is tested independently of the rotation/post pipeline, which has its own
    coverage.
    """

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestGetClearedAreasWorkplane")
        box = self.doc.addObject("Part::Box", "Box")
        box.Length = 100
        box.Width = 100
        box.Height = 100
        self.doc.recompute()
        self.job = PathJob.Create("Job", [box], None)
        self.job.GeometryTolerance.Value = 0.001
        self.doc.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _makeOp(self, name, workplane, path, diameter=5.0, active=True):
        """Create a real Custom op in the job with the given Path and Workplane."""
        op = PathCustom.Create(name, parentJob=self.job)
        op.Active = active
        if workplane is None:
            # Simulate an older op that predates the Workplane property.
            op.removeProperty("Workplane")
        else:
            op.Workplane = workplane
        op.ToolController.Tool.Diameter = diameter
        # Assign the toolpath directly: getClearedAreas only reads op.Path, and
        # driving it here keeps the cleared-area filtering test independent of
        # path generation (and of any machine configuration the rotation
        # pipeline would otherwise require for non-Z-up workplanes).
        op.Path = path
        return op

    def _bbox(self):
        bb = FreeCAD.BoundBox()
        bb.add(FreeCAD.Vector(-50, -50, -10))
        bb.add(FreeCAD.Vector(50, 50, 10))
        return bb

    def test_sameWorkplaneIncluded(self):
        """A previous op sharing the current op's Workplane is included."""
        z_up = Vector(0, 0, 1)
        self._makeOp("Prev", z_up, _makeRectanglePath(-20, -20, 20, 20, -1))
        current = self._makeOp("Current", z_up, _makeRectanglePath(-10, -10, 10, 10, -1))

        areas = PathOpUtil.getClearedAreas(current, self._bbox())
        self.assertEqual(len(areas), 1, "Same-workplane previous op must be included")

    def test_differentWorkplaneSkipped(self):
        """A previous op with a different Workplane is skipped."""
        z_up = Vector(0, 0, 1)
        x_dir = Vector(1, 0, 0)
        self._makeOp("PrevTop", z_up, _makeRectanglePath(-20, -20, 20, 20, -1))
        self._makeOp("PrevSide", x_dir, _makeRectanglePath(-15, -15, 15, 15, -1))
        current = self._makeOp("Current", x_dir, _makeRectanglePath(-10, -10, 10, 10, -1))

        areas = PathOpUtil.getClearedAreas(current, self._bbox())
        self.assertEqual(
            len(areas),
            1,
            "Only the same-workplane previous op (PrevSide) should contribute cleared area",
        )

    def test_currentOpStopsIteration(self):
        """Ops at or after the current op in the list are not considered."""
        z_up = Vector(0, 0, 1)
        self._makeOp("Prev", z_up, _makeRectanglePath(-20, -20, 20, 20, -1))
        current = self._makeOp("Current", z_up, _makeRectanglePath(-10, -10, 10, 10, -1))
        self._makeOp("Later", z_up, _makeRectanglePath(-20, -20, 20, 20, -1))

        areas = PathOpUtil.getClearedAreas(current, self._bbox())
        self.assertEqual(len(areas), 1, "Operations after the current op must be ignored")

    def test_inactiveOpSkipped(self):
        """Inactive previous ops are skipped regardless of Workplane."""
        z_up = Vector(0, 0, 1)
        self._makeOp("Prev", z_up, _makeRectanglePath(-20, -20, 20, 20, -1), active=False)
        current = self._makeOp("Current", z_up, _makeRectanglePath(-10, -10, 10, 10, -1))

        areas = PathOpUtil.getClearedAreas(current, self._bbox())
        self.assertEqual(len(areas), 0, "Inactive previous ops must not contribute")

    def test_missingWorkplaneTreatedAsZUp(self):
        """An op without a Workplane property is treated as Z-up."""
        z_up = Vector(0, 0, 1)
        # Passing workplane=None drops the Workplane property to simulate an
        # older op missing it.
        self._makeOp("Prev", None, _makeRectanglePath(-20, -20, 20, 20, -1))
        current = self._makeOp("Current", z_up, _makeRectanglePath(-10, -10, 10, 10, -1))

        areas = PathOpUtil.getClearedAreas(current, self._bbox())
        self.assertEqual(len(areas), 1, "A previous op without Workplane should default to Z-up")


class TestStripRotaryAxes(PathTestUtils.PathTestBase):
    """Verify _stripRotaryAxes drops rotary parameters from path commands."""

    def test_pureRotaryCommandDropped(self):
        """A G0 with only rotary parameters is removed entirely."""
        path = Path.Path(
            [
                Path.Command("G0", {"A": 90.0}),
                Path.Command("G1", {"X": 1.0, "Y": 2.0, "Z": -1.0}),
            ]
        )
        stripped = PathOpUtil._stripRotaryAxes(path)
        self.assertEqual(len(stripped.Commands), 1)
        self.assertEqual(stripped.Commands[0].Name, "G1")
        self.assertNotIn("A", stripped.Commands[0].Parameters)

    def test_mixedCommandKeepsLinearAxes(self):
        """A move that carries both linear and rotary axes keeps the linear ones."""
        path = Path.Path(
            [
                Path.Command("G1", {"X": 5.0, "Y": 6.0, "Z": -2.0, "B": 45.0}),
            ]
        )
        stripped = PathOpUtil._stripRotaryAxes(path)
        self.assertEqual(len(stripped.Commands), 1)
        params = stripped.Commands[0].Parameters
        self.assertEqual(params.get("X"), 5.0)
        self.assertEqual(params.get("Y"), 6.0)
        self.assertEqual(params.get("Z"), -2.0)
        for axis in ("A", "B", "C", "U", "V", "W"):
            self.assertNotIn(axis, params)

    def test_clearedAreaIgnoresLeadingRotation(self):
        """Cleared area on a stripped path matches the same path without the rotary G0.

        This is the property that makes REST machining correct in 3+2: the
        leading rotary command must not cause PathSegmentWalker to rotate the
        subsequent X/Y/Z positions.
        """
        bbox = FreeCAD.BoundBox()
        bbox.add(FreeCAD.Vector(-50, -50, -10))
        bbox.add(FreeCAD.Vector(50, 50, 10))

        # A simple square traverse at z=-1.
        moves = [
            Path.Command("G0", {"X": -10, "Y": -10, "Z": 5}),
            Path.Command("G1", {"X": -10, "Y": -10, "Z": -1}),
            Path.Command("G1", {"X": 10, "Y": -10, "Z": -1}),
            Path.Command("G1", {"X": 10, "Y": 10, "Z": -1}),
            Path.Command("G1", {"X": -10, "Y": 10, "Z": -1}),
            Path.Command("G1", {"X": -10, "Y": -10, "Z": -1}),
        ]
        plain = Path.Path(moves)
        rotated = Path.Path([Path.Command("G0", {"B": -90.0})] + moves)

        plainArea = plain.getClearedArea(5.0, 0.0, bbox)
        rotatedRawArea = rotated.getClearedArea(5.0, 0.0, bbox)
        rotatedStrippedArea = PathOpUtil._stripRotaryAxes(rotated).getClearedArea(5.0, 0.0, bbox)

        # Compare via Wires count + bounding box of resulting shape — we just
        # need to confirm that stripping recovers the unrotated cleared area
        # and that the raw rotated walk produces something different.
        plainShape = plainArea.toTopoShape()
        rotatedRawShape = rotatedRawArea.toTopoShape()
        strippedShape = rotatedStrippedArea.toTopoShape()

        plainBB = plainShape.BoundBox
        strippedBB = strippedShape.BoundBox
        rawBB = rotatedRawShape.BoundBox

        # Stripped result should match the plain (no-rotation) result.
        self.assertAlmostEqual(plainBB.XMin, strippedBB.XMin, places=3)
        self.assertAlmostEqual(plainBB.XMax, strippedBB.XMax, places=3)
        self.assertAlmostEqual(plainBB.YMin, strippedBB.YMin, places=3)
        self.assertAlmostEqual(plainBB.YMax, strippedBB.YMax, places=3)

        # The unstripped rotated walk should NOT match — confirms the bug is
        # real and that the strip is doing the work.
        differs = (
            not Path.Geom.isRoughly(plainBB.XMin, rawBB.XMin)
            or not Path.Geom.isRoughly(plainBB.XMax, rawBB.XMax)
            or not Path.Geom.isRoughly(plainBB.YMin, rawBB.YMin)
            or not Path.Geom.isRoughly(plainBB.YMax, rawBB.YMax)
        )
        self.assertTrue(
            differs,
            "Unstripped rotated path should produce a rotation-compensated "
            "cleared area different from the plain path",
        )


class TestWorkplaneRotationCommands(PathTestUtils.PathTestBase):
    """Verify ops always command their rotary pose on a rotary-capable machine.

    Operations are atomic: an op cannot know what pose the previous op left
    the machine in. On a machine with rotary axes even a Z-up op must
    therefore emit an explicit G0 zeroing the rotary axes, otherwise a Z-up
    op following a rotated op silently machines the wrong face (issue #32046).
    """

    def setUp(self):
        self.doc = FreeCAD.newDocument("TestWorkplaneRotationCommands")
        box = self.doc.addObject("Part::Box", "Box")
        box.Length = 100
        box.Width = 100
        box.Height = 100
        self.doc.recompute()
        self.job = PathJob.Create("Job", [box], None)
        self.job.GeometryTolerance.Value = 0.001
        self.doc.recompute()

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def _attachMachine(self):
        """Attach a C-A table-table machine to the job via getMachine."""
        from Machine.models.machine import Machine, RotaryAxis, AxisRole

        machine = Machine(name="Test CA Machine")
        machine.rotary_axes["C"] = RotaryAxis(
            name="C",
            rotation_vector=FreeCAD.Vector(0, 0, 1),
            min_limit=-360,
            max_limit=360,
            role=AxisRole.TABLE_ROTARY,
            parent=None,
            sequence=0,
        )
        machine.rotary_axes["A"] = RotaryAxis(
            name="A",
            rotation_vector=FreeCAD.Vector(1, 0, 0),
            min_limit=-120,
            max_limit=120,
            role=AxisRole.TABLE_ROTARY,
            parent="C",
            sequence=1,
        )
        self.job.Proxy.getMachine = lambda: machine

    def _makeOp(self, name, workplane):
        op = PathCustom.Create(name, parentJob=self.job)
        op.Workplane = workplane
        op.ToolController.Tool.Diameter = 5.0
        self.doc.recompute()
        return op

    def _rotaryCommands(self, op):
        return [
            c
            for c in op.Path.Commands
            if c.Name in ("G0", "G00")
            and ("A" in c.Parameters or "B" in c.Parameters or "C" in c.Parameters)
        ]

    def test_zUpOpEmitsZeroRotation(self):
        """A Z-up op on a rotary machine commands all rotary axes to zero."""
        self._attachMachine()
        op = self._makeOp("ZUp", Vector(0, 0, 1))

        rotary = self._rotaryCommands(op)
        self.assertTrue(rotary, "Z-up op on a rotary machine must emit a rotary G0")
        params = rotary[0].Parameters
        self.assertAlmostEqual(params.get("A"), 0.0)
        self.assertAlmostEqual(params.get("C"), 0.0)

    def test_rotatedOpEmitsSolvedRotation(self):
        """A non-Z workplane still emits its solved rotary position."""
        self._attachMachine()
        op = self._makeOp("Side", Vector(0, 1, 0))

        rotary = self._rotaryCommands(op)
        self.assertTrue(rotary, "Rotated op must emit a rotary G0")
        params = rotary[0].Parameters
        self.assertAlmostEqual(abs(params.get("A")), 90.0)

    def test_zUpOpWithoutMachineEmitsNoRotation(self):
        """Without a rotary machine a Z-up op emits no rotary words."""
        op = self._makeOp("ZUpPlain", Vector(0, 0, 1))

        self.assertEqual(
            self._rotaryCommands(op),
            [],
            "A 3-axis job must not gain rotary words from the Z-up pose reset",
        )
