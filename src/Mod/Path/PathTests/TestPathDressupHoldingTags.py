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
import Part
import Path
import PathScripts
import math
import unittest

from FreeCAD import Vector
from PathScripts.PathDressupHoldingTags import *

def pointsCoincide(pt1, pt2):
    pt = pt1 - pt2
    if math.fabs(pt.x) > slack:
        return False
    if math.fabs(pt.y) > slack:
        return False
    if math.fabs(pt.z) > slack:
        return False
    return True

class TagTestCaseBase(unittest.TestCase):
    """Base class for all tag test cases providing additional assert functions."""

    def assertCylinderAt(self, solid, pt, r, h):
        """Verify that solid is a cylinder at the specified location."""
        self.assertEqual(len(solid.Edges), 3)

        lid  = solid.Edges[0]
        hull = solid.Edges[1]
        base = solid.Edges[2]

        self.assertCircle(lid, Vector(pt.x, pt.y, pt.z+h), r)
        self.assertLine(hull, Vector(pt.x+r, pt.y, pt.z), Vector(pt.x+r, pt.y, pt.z+h))
        self.assertCircle(base, Vector(pt.x, pt.y, pt.z), r)

    def assertConeAt(self, solid, pt, r1, r2, h):
        """Verify that solid is a cone at the specified location."""
        self.assertEqual(len(solid.Edges), 3)

        lid  = solid.Edges[0]
        hull = solid.Edges[1]
        base = solid.Edges[2]

        self.assertCircle(lid, Vector(pt.x, pt.y, pt.z+h), r2)
        self.assertLine(hull, Vector(pt.x+r1, pt.y, pt.z), Vector(pt.x+r2, pt.y, pt.z+h))
        self.assertCircle(base, Vector(pt.x, pt.y, pt.z), r1)

    def assertCircle(self, edge, pt, r):
        """Verivy that edge is a circle at given location."""
        curve = edge.Curve
        self.assertIs(type(curve), Part.Circle)
        self.assertCoincide(curve.Center, Vector(pt.x, pt.y, pt.z))
        self.assertAbout(curve.Radius, r)

    def assertLine(self, edge, pt1, pt2):
        """Verify that edge is a line from pt1 to pt2."""
        curve = edge.Curve
        self.assertIs(type(curve), Part.Line)
        self.assertCoincide(curve.StartPoint, pt1)
        self.assertCoincide(curve.EndPoint, pt2)

    def assertCoincide(self, pt1, pt2):
        """Verify that 2 points coincide (with tolerance)."""
        self.assertAbout(pt1.x, pt2.x)
        self.assertAbout(pt1.y, pt2.y)
        self.assertAbout(pt1.z, pt2.z)

    def assertAbout(self, v1, v2):
        """Verify that 2 values are the same (accounting for float imprecision)."""
        if math.fabs(v1 - v2) > slack:
            self.fail("%f != %f" % (v1, v2))

    def assertLines(self, edgs, tail, points):
        """Check that there are 5 edges forming a trapezoid."""
        edges = list(edgs)
        if tail:
            edges.append(tail)
        self.assertEqual(len(edges), len(points) - 1)

        for i in range(0, len(edges)):
            self.assertLine(edges[i], points[i], points[i+1])

class TestTag00BasicHolding(TagTestCaseBase):
    """Some basid test cases."""

    def test00(self,x=1, y=1):
        """Test getAngle."""
        self.assertAbout(getAngle(FreeCAD.Vector( 1*x, 0*y, 0)), 0)
        self.assertAbout(getAngle(FreeCAD.Vector( 1*x, 1*y, 0)), math.pi/4)
        self.assertAbout(getAngle(FreeCAD.Vector( 0*x, 1*y, 0)), math.pi/2)
        self.assertAbout(getAngle(FreeCAD.Vector(-1*x, 1*y, 0)), 3*math.pi/4)
        self.assertAbout(getAngle(FreeCAD.Vector(-1*x, 0*y, 0)), math.pi)
        self.assertAbout(getAngle(FreeCAD.Vector(-1*x,-1*y, 0)), -3*math.pi/4)
        self.assertAbout(getAngle(FreeCAD.Vector( 0*x,-1*y, 0)), -math.pi/2)
        self.assertAbout(getAngle(FreeCAD.Vector( 1*x,-1*y, 0)), -math.pi/4)

    def test01(self):
        """Test class Side."""
        self.assertEqual(Side.of(FreeCAD.Vector( 1, 0, 0), FreeCAD.Vector( 1, 0, 0)), Side.On)
        self.assertEqual(Side.of(FreeCAD.Vector( 1, 0, 0), FreeCAD.Vector(-1, 0, 0)), Side.On)
        self.assertEqual(Side.of(FreeCAD.Vector( 1, 0, 0), FreeCAD.Vector( 0, 1, 0)), Side.Left)
        self.assertEqual(Side.of(FreeCAD.Vector( 1, 0, 0), FreeCAD.Vector( 0,-1, 0)), Side.Right)


class TestTag01BasicTag(TagTestCaseBase): # ============= 
    """Unit tests for the HoldingTags dressup."""

    def test00(self):
        """Check Tag origin, serialization and de-serialization."""
        tag = Tag(77, 13, 4, 5, 90, True)
        self.assertCoincide(tag.originAt(3), Vector(77, 13, 3))
        s = tag.toString()
        tagCopy = Tag.FromString(s)
        self.assertEqual(tag.x, tagCopy.x)
        self.assertEqual(tag.y, tagCopy.y)
        self.assertEqual(tag.height, tagCopy.height)
        self.assertEqual(tag.width, tagCopy.width)
        self.assertEqual(tag.enabled, tagCopy.enabled)


    def test01(self):
        """Verify solid and core for a 90 degree tag are identical cylinders."""
        tag = Tag(100, 200, 4, 5, 90, True)
        tag.createSolidsAt(17)

        self.assertIsNotNone(tag.solid)
        self.assertCylinderAt(tag.solid, Vector(100, 200, 17), 2, 5)

        self.assertIsNotNone(tag.core)
        self.assertCylinderAt(tag.core, Vector(100, 200, 17), 2, 5)

    def test02(self):
        """Verify trapezoidal tag has a cone shape with a lid, and cylinder core."""
        tag = Tag(0, 0, 18, 5, 45, True)
        tag.createSolidsAt(0)

        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0,0,0), 9, 4, 5)

        self.assertIsNotNone(tag.core)
        self.assertCylinderAt(tag.core, Vector(0,0,0), 4, 5)

    def test03(self):
        """Verify pointy cone shape of tag with pointy end if width, angle and height match up."""
        tag = Tag(0, 0, 10, 5, 45, True)
        tag.createSolidsAt(0)
        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0,0,0), 5, 0, 5)

        self.assertIsNone(tag.core)

    def test04(self):
        """Verify height adjustment if tag isn't wide eough for angle."""
        tag = Tag(0, 0, 5, 17, 60, True)
        tag.createSolidsAt(0)
        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0,0,0), 2.5, 0, 2.5 * math.tan((60/180.0)*math.pi))

        self.assertIsNone(tag.core)

class TestTag02SquareTag(TagTestCaseBase): # ============= 
    """Unit tests for square tags."""

    def test00(self):
        """Verify no intersection."""
        tag = Tag( 0, 0, 4, 7, 90, True, 0)
        pt1 = Vector(+5, 5, 0)
        pt2 = Vector(-5, 5, 0)
        edge = Part.Edge(Part.Line(pt1, pt2))

        i = tag.intersect(edge)
        self.assertIsNotNone(i)
        self.assertTrue(i.isComplete())
        self.assertIsNotNone(i.edges)
        self.assertFalse(i.edges)
        self.assertLine(i.tail, pt1, pt2)

    def test01(self):
        """Verify intersection of square tag with line ending at tag start."""
        tag = Tag( 0, 0, 8, 3, 90, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(4, 0, 0)))

        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P0)
        self.assertEqual(len(i.edges), 1)
        self.assertLine(i.edges[0], edge.Curve.StartPoint, edge.Curve.EndPoint)
        self.assertIsNone(i.tail)

    def test02(self):
        """Verify intersection of square tag with line ending between P1 and P2."""
        tag = Tag( 0, 0, 8, 3, 90, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(1, 0, 0)))

        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P1)
        self.assertEqual(len(i.edges), 3)
        p1 = Vector(4, 0, 0)
        p2 = Vector(4, 0, 3)
        p3 = Vector(1, 0, 3)
        self.assertLine(i.edges[0], edge.Curve.StartPoint, p1)
        self.assertLine(i.edges[1], p1, p2)
        self.assertLine(i.edges[2], p2, p3)
        self.assertIsNone(i.tail)

        # verify we stay in P1 if we add another segment
        edge = Part.Edge(Part.Line(edge.Curve.EndPoint, Vector(0, 0, 0)))
        i = i.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P1)
        self.assertEqual(len(i.edges), 4)
        p4 = Vector(0, 0, 3)
        self.assertLine(i.edges[3], p3, p4)
        self.assertIsNone(i.tail)

    def test03(self):
        """Verify intesection of square tag with line ending on P2."""
        tag = Tag( 0, 0, 8, 3, 90, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(-4, 0, 0)))

        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P2)
        self.assertEqual(len(i.edges), 3)
        p0 = edge.Curve.StartPoint
        p1 = Vector( 4, 0, 0)
        p2 = Vector( 4, 0, 3)
        p3 = Vector(-4, 0, 3)
        self.assertLine(i.edges[0], p0, p1)
        self.assertLine(i.edges[1], p1, p2)
        self.assertLine(i.edges[2], p2, p3)
        self.assertIsNone(i.tail)

        # make sure it also works if we get there not directly
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(0, 0, 0)))
        i = tag.intersect(edge)
        edge = Part.Edge(Part.Line(edge.Curve.EndPoint, Vector(-4, 0, 0)))
        i = i.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P2)
        self.assertEqual(len(i.edges), 4)
        p2a = Vector( 0, 0, 3)
        self.assertLine(i.edges[0], p0, p1)
        self.assertLine(i.edges[1], p1, p2)
        self.assertLine(i.edges[2], p2, p2a)
        self.assertLine(i.edges[3], p2a, p3)
        self.assertIsNone(i.tail)

    def test04(self):
        """Verify plunge down is inserted for square tag on exit."""
        tag = Tag( 0, 0, 8, 3, 90, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(-5, 0, 0)))

        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P3)
        self.assertTrue(i.isComplete())
        self.assertEqual(len(i.edges), 4)
        p0 = edge.Curve.StartPoint
        p1 = Vector( 4, 0, 0)
        p2 = Vector( 4, 0, 3)
        p3 = Vector(-4, 0, 3)
        p4 = Vector(-4, 0, 0)
        p5 = edge.Curve.EndPoint
        self.assertLine(i.edges[0], p0, p1)
        self.assertLine(i.edges[1], p1, p2)
        self.assertLine(i.edges[2], p2, p3)
        self.assertLine(i.edges[3], p3, p4)
        self.assertIsNotNone(i.tail)
        self.assertLine(i.tail, p4, p5)

    def test05(self):
        """Verify all lines between P0 and P3 are added."""
        tag = Tag( 0, 0, 4, 7, 90, True, 0)
        e0 = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(+2, 0, 0)))
        e1 = Part.Edge(Part.Line(e0.Curve.EndPoint, Vector(+1, 0, 0)))
        e2 = Part.Edge(Part.Line(e1.Curve.EndPoint, Vector(+0.5, 0, 0)))
        e3 = Part.Edge(Part.Line(e2.Curve.EndPoint, Vector(-0.5, 0, 0)))
        e4 = Part.Edge(Part.Line(e3.Curve.EndPoint, Vector(-1, 0, 0)))
        e5 = Part.Edge(Part.Line(e4.Curve.EndPoint, Vector(-2, 0, 0)))
        e6 = Part.Edge(Part.Line(e5.Curve.EndPoint, Vector(-5, 0, 0)))

        i = tag
        for e in [e0, e1, e2, e3, e4, e5]:
            i = i.intersect(e)
            self.assertFalse(i.isComplete())
        i = i.intersect(e6)
        self.assertTrue(i.isComplete())

        pt0 = Vector(2, 0, 0)
        pt1 = Vector(2, 0, 7)
        pt2 = Vector(1, 0, 7)
        pt3 = Vector(0.5, 0, 7)
        pt4 = Vector(-0.5, 0, 7)
        pt5 = Vector(-1, 0, 7)
        pt6 = Vector(-2, 0, 7)

        self.assertEqual(len(i.edges), 8)
        self.assertLines(i.edges, i.tail, [e0.Curve.StartPoint, pt0, pt1, pt2, pt3, pt4, pt5, pt6, e6.Curve.StartPoint, e6.Curve.EndPoint])
        self.assertIsNotNone(i.tail)

    def test06(self):
        """Verify intersection of different z levels."""
        tag = Tag( 0, 0, 4, 7, 90, True, 0)
        # for all lines below 7 we get the trapezoid
        for i in range(0, 7):
            p0 = Vector(5, 0, i)
            p1 = Vector(2, 0, i)
            p2 = Vector(2, 0, 7)
            p3 = Vector(-2, 0, 7)
            p4 = Vector(-2, 0, i)
            p5 = Vector(-5, 0, i)
            edge = Part.Edge(Part.Line(p0, p5))
            s = tag.intersect(edge)
            self.assertTrue(s.isComplete())
            self.assertLines(s.edges, s.tail, [p0, p1, p2, p3, p4, p5])

        # for all edges at height or above the original line is used
        for i in range(7, 9):
            edge = Part.Edge(Part.Line(Vector(5, 0, i), Vector(-5, 0, i)))
            s = tag.intersect(edge)
            self.assertTrue(s.isComplete())
            self.assertLine(s.tail, edge.Curve.StartPoint, edge.Curve.EndPoint)

class TestTag03TrapezoidTag(TagTestCaseBase): # ============= 
    """Unit tests for trapezoid tags."""

    def test00(self):
        """Verify no intersection."""
        tag = Tag( 0, 0, 8, 3, 45, True, 0)
        pt1 = Vector(+5, 5, 0)
        pt2 = Vector(-5, 5, 0)
        edge = Part.Edge(Part.Line(pt1, pt2))

        i = tag.intersect(edge)
        self.assertIsNotNone(i)
        self.assertTrue(i.isComplete())
        self.assertIsNotNone(i.edges)
        self.assertFalse(i.edges)
        self.assertLine(i.tail, pt1, pt2)

    def test01(self):
        """Veify intersection of trapezoid tag with line ending before P1."""
        tag = Tag( 0, 0, 8, 3, 45, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(4, 0, 0)))

        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P0)
        self.assertEqual(len(i.edges), 1)
        self.assertLine(i.edges[0], edge.Curve.StartPoint, edge.Curve.EndPoint)
        self.assertIsNone(i.tail)

        # now add another segment that doesn't reach the top of the cone
        edge = Part.Edge(Part.Line(edge.Curve.EndPoint, Vector(3, 0, 0)))
        i = i.intersect(edge)
        # still a P0 and edge fully consumed
        p1 = Vector(edge.Curve.StartPoint)
        p1.z = 0
        p2 = Vector(edge.Curve.EndPoint)
        p2.z = 1  # height of cone @ (3,0)
        self.assertEqual(i.state, Tag.Intersection.P0)
        self.assertEqual(len(i.edges), 2)
        self.assertLine(i.edges[1], p1, p2)
        self.assertIsNone(i.tail)

        # add another segment to verify starting point offset
        edge = Part.Edge(Part.Line(edge.Curve.EndPoint, Vector(2, 0, 0)))
        i = i.intersect(edge)
        # still a P0 and edge fully consumed
        p3 = Vector(edge.Curve.EndPoint)
        p3.z = 2  # height of cone @ (2,0)
        self.assertEqual(i.state, Tag.Intersection.P0)
        self.assertEqual(len(i.edges), 3)
        self.assertLine(i.edges[2], p2, p3)
        self.assertIsNone(i.tail)

    def test02(self):
        """Verify intersection of trapezoid tag with line ending between P1 and P2"""
        tag = Tag( 0, 0, 8, 3, 45, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(1, 0, 0)))

        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P1)
        self.assertEqual(len(i.edges), 2)
        p1 = Vector(4, 0, 0)
        p2 = Vector(1, 0, 3)
        self.assertLine(i.edges[0], edge.Curve.StartPoint, p1)
        self.assertLine(i.edges[1], p1, p2)
        self.assertIsNone(i.tail)

        # verify we stay in P1 if we add another segment
        edge = Part.Edge(Part.Line(edge.Curve.EndPoint, Vector(0, 0, 0)))
        i = i.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P1)
        self.assertEqual(len(i.edges), 3)
        p3 = Vector(0, 0, 3)
        self.assertLine(i.edges[2], p2, p3)
        self.assertIsNone(i.tail)

    def test03(self):
        """Verify intersection of trapezoid tag with edge ending on P2."""
        tag = Tag( 0, 0, 8, 3, 45, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(-1, 0, 0)))

        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P2)
        p0 = Vector(edge.Curve.StartPoint)
        p1 = Vector(4, 0, 0)
        p2 = Vector(1, 0, 3)
        p3 = Vector(-1, 0, 3)
        self.assertLines(i.edges, i.tail, [p0, p1, p2, p3])
        self.assertIsNone(i.tail)

        # make sure we get the same result if there's another edge
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(1, 0, 0)))
        i = tag.intersect(edge)
        edge = Part.Edge(Part.Line(edge.Curve.EndPoint, Vector(-1, 0, 0)))
        i = i.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P2)
        self.assertLines(i.edges, i.tail, [p0, p1, p2, p3])
        self.assertIsNone(i.tail)

        # and also if the last segment doesn't cross the entire plateau
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(0.5, 0, 0)))
        i = tag.intersect(edge)
        edge = Part.Edge(Part.Line(edge.Curve.EndPoint, Vector(-1, 0, 0)))
        i = i.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P2)
        p2a = Vector(0.5, 0, 3)
        self.assertLines(i.edges, i.tail, [p0, p1, p2, p2a, p3])
        self.assertIsNone(i.tail)

    def test04(self):
        """Verify proper down plunge on trapezoid tag exit."""
        tag = Tag( 0, 0, 8, 3, 45, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(-2, 0, 0)))

        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P2)
        p0 = Vector(5, 0, 0)
        p1 = Vector(4, 0, 0)
        p2 = Vector(1, 0, 3)
        p3 = Vector(-1, 0, 3)
        p4 = Vector(-2, 0, 2)
        self.assertLines(i.edges, i.tail, [p0, p1, p2, p3, p4])
        self.assertIsNone(i.tail)

        # make sure adding another segment doesn't change the state
        edge = Part.Edge(Part.Line(edge.Curve.EndPoint, Vector(-3, 0, 0)))
        i = i.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P2)
        self.assertEqual(len(i.edges), 5)
        p5 = Vector(-3, 0, 1)
        self.assertLine(i.edges[4], p4, p5)
        self.assertIsNone(i.tail)

        # now if we complete to P3 ....
        edge = Part.Edge(Part.Line(edge.Curve.EndPoint, Vector(-4, 0, 0)))
        i = i.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P3)
        self.assertTrue(i.isComplete())
        self.assertEqual(len(i.edges), 6)
        p6 = Vector(-4, 0, 0)
        self.assertLine(i.edges[5], p5, p6)
        self.assertIsNone(i.tail)

        # verify proper operation if there is a single edge going through all
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(-4, 0, 0)))
        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P3)
        self.assertTrue(i.isComplete())
        self.assertLines(i.edges, i.tail, [p0, p1, p2, p3, p6])
        self.assertIsNone(i.tail)

        # verify tail is added as well
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(-5, 0, 0)))
        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P3)
        self.assertTrue(i.isComplete())
        self.assertLines(i.edges, i.tail, [p0, p1, p2, p3, p6, edge.Curve.EndPoint])
        self.assertIsNotNone(i.tail)

    def test05(self):
        """Verify all lines between P0 and P3 are added."""
        tag = Tag( 0, 0, 8, 3, 45, True, 0)
        e0 = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(+4, 0, 0)))
        e1 = Part.Edge(Part.Line(e0.Curve.EndPoint, Vector(+2, 0, 0)))
        e2 = Part.Edge(Part.Line(e1.Curve.EndPoint, Vector(+0.5, 0, 0)))
        e3 = Part.Edge(Part.Line(e2.Curve.EndPoint, Vector(-0.5, 0, 0)))
        e4 = Part.Edge(Part.Line(e3.Curve.EndPoint, Vector(-1, 0, 0)))
        e5 = Part.Edge(Part.Line(e4.Curve.EndPoint, Vector(-2, 0, 0)))
        e6 = Part.Edge(Part.Line(e5.Curve.EndPoint, Vector(-5, 0, 0)))

        i = tag
        for e in [e0, e1, e2, e3, e4, e5]:
            i = i.intersect(e)
            self.assertFalse(i.isComplete())
        i = i.intersect(e6)
        self.assertTrue(i.isComplete())

        p0 = Vector(4, 0, 0)
        p1 = Vector(2, 0, 2)
        p2 = Vector(1, 0, 3)
        p3 = Vector(0.5, 0, 3)
        p4 = Vector(-0.5, 0, 3)
        p5 = Vector(-1, 0, 3)
        p6 = Vector(-2, 0, 2)
        p7 = Vector(-4, 0, 0)

        self.assertLines(i.edges, i.tail, [e0.Curve.StartPoint, p0, p1, p2, p3, p4, p5, p6, p7, e6.Curve.EndPoint])
        self.assertIsNotNone(i.tail)

    def test06(self):
        """Verify intersection for different z levels."""
        tag = Tag( 0, 0, 8, 3, 45, True, 0)
        # for all lines below 3 we get the trapezoid
        for i in range(0, 3):
            p0 = Vector(5, 0, i)
            p1 = Vector(4-i, 0, i)
            p2 = Vector(1, 0, 3)
            p3 = Vector(-1, 0, 3)
            p4 = Vector(-4+i, 0, i)
            p5 = Vector(-5, 0, i)
            edge = Part.Edge(Part.Line(p0, p5))
            s = tag.intersect(edge)
            self.assertTrue(s.isComplete())
            self.assertLines(s.edges, s.tail, [p0, p1, p2, p3, p4, p5])

        # for all edges at height or above the original line is used
        for i in range(3, 5):
            edge = Part.Edge(Part.Line(Vector(5, 0, i), Vector(-5, 0, i)))
            s = tag.intersect(edge)
            self.assertTrue(s.isComplete())
            self.assertLine(s.tail, edge.Curve.StartPoint, edge.Curve.EndPoint)


class TestTag04TriangularTag(TagTestCaseBase): # ========================
    """Unit tests for tags that take on a triangular shape."""

    def test00(self):
        """Verify intersection of triangular tag with line ending at tag start."""
        tag = Tag( 0, 0, 8, 7, 45, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(4, 0, 0)))

        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P0)
        self.assertEqual(len(i.edges), 1)
        self.assertLine(i.edges[0], edge.Curve.StartPoint, edge.Curve.EndPoint)
        self.assertIsNone(i.tail)

    def test01(self):
        """Verify intersection of triangular tag with line ending between P0 and P1."""
        tag = Tag( 0, 0, 8, 7, 45, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(3, 0, 0)))

        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P0)
        p1 = Vector(4, 0, 0)
        p2 = Vector(3, 0, 1)
        self.assertLines(i.edges, i.tail, [edge.Curve.StartPoint, p1, p2])
        self.assertIsNone(i.tail)

        # verify we stay in P1 if we add another segment
        edge = Part.Edge(Part.Line(edge.Curve.EndPoint, Vector(1, 0, 0)))
        i = i.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P0)
        self.assertEqual(len(i.edges), 3)
        p3 = Vector(1, 0, 3)
        self.assertLine(i.edges[2], p2, p3)
        self.assertIsNone(i.tail)

    def test02(self):
        """Verify proper down plunge on exit of triangular tag."""
        tag = Tag( 0, 0, 8, 7, 45, True, 0)

        p0 = Vector(5, 0, 0)
        p1 = Vector(4, 0, 0)
        p2 = Vector(0, 0, 4)
        edge = Part.Edge(Part.Line(p0, FreeCAD.Vector(0,0,0)))
        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P2)
        self.assertEqual(len(i.edges), 2)
        self.assertLines(i.edges, i.tail, [p0, p1, p2])

        # adding another segment doesn't make a difference
        edge = Part.Edge(Part.Line(edge.Curve.EndPoint, FreeCAD.Vector(-3,0,0)))
        i = i.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P2)
        self.assertEqual(len(i.edges), 3)
        p3 = Vector(-3, 0, 1)
        self.assertLines(i.edges, i.tail, [p0, p1, p2, p3])

        # same result if all is one line
        edge = Part.Edge(Part.Line(p0, edge.Curve.EndPoint))
        i = tag.intersect(edge)
        self.assertEqual(i.state, Tag.Intersection.P2)
        self.assertLines(i.edges, i.tail, [p0, p1, p2, p3])

    def test03(self):
        """Verify triangular tag shap on intersection."""
        tag = Tag( 0, 0, 8, 7, 45, True, 0)
        
        p0 = Vector(5, 0, 0)
        p1 = Vector(4, 0, 0)
        p2 = Vector(0, 0, 4)
        p3 = Vector(-4, 0, 0)
        edge = Part.Edge(Part.Line(p0, p3))
        i = tag.intersect(edge)
        self.assertTrue(i.isComplete())
        self.assertLines(i.edges, i.tail, [p0, p1, p2, p3])
        self.assertIsNone(i.tail)

        # this should also work if there is some excess, aka tail
        p4 = Vector(-5, 0, 0)
        edge = Part.Edge(Part.Line(p0, p4))
        i = tag.intersect(edge)
        self.assertTrue(i.isComplete())
        self.assertLines(i.edges, i.tail, [p0, p1, p2, p3, p4])
        self.assertIsNotNone(i.tail)

