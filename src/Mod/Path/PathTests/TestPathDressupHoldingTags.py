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
from PathScripts.PathDressupHoldingTags import Tag

slack = 0.0000001

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
        #print("assertAbout(%f, %f)" % (v1, v2))
        if math.fabs(v1 - v2) > slack:
            self.fail("%f != %f" % (v1, v2))

    def assertTrapezoid(self, edgs, tail, spec):
        """Check that there are 5 edges forming a trapezoid."""
        edges = list(edgs)
        if tail:
            edges.append(tail)
        self.assertEqual(len(edges), 5)

        p0 = spec[0]
        p1 = Vector(spec[1], p0.y, p0.z)
        p2 = Vector(p1.x, p1.y, spec[2])
        p3 = Vector(-p2.x, p2.y, p2.z)
        p4 = Vector(p3.x, p3.y, p0.z)
        p5 = spec[3]

        self.assertLine(edges[0], p0, p1)
        self.assertLine(edges[1], p1, p2)
        self.assertLine(edges[2], p2, p3)
        self.assertLine(edges[3], p3, p4)
        self.assertLine(edges[4], p4, p5)


class TagTestCases(TagTestCaseBase): # ============= 
    """Unit tests for the HoldingTags dressup."""

    def testTagBasics(self):
        #"""Check Tag origin, serialization and de-serialization."""
        tag = Tag(77, 13, 4, 5, 90, True)
        self.assertCoincide(tag.originAt(3), Vector(77, 13, 3))
        s = tag.toString()
        tagCopy = Tag.FromString(s)
        self.assertEqual(tag.x, tagCopy.x)
        self.assertEqual(tag.y, tagCopy.y)
        self.assertEqual(tag.height, tagCopy.height)
        self.assertEqual(tag.width, tagCopy.width)
        self.assertEqual(tag.enabled, tagCopy.enabled)


    def testTagSolidBasic(self):
        #"""For a 90 degree tag the core and solid are both defined and identical cylinders."""
        tag = Tag(100, 200, 4, 5, 90, True)
        tag.createSolidsAt(17)

        self.assertIsNotNone(tag.solid)
        self.assertCylinderAt(tag.solid, Vector(100, 200, 17), 2, 5)

        self.assertIsNotNone(tag.core)
        self.assertCylinderAt(tag.core, Vector(100, 200, 17), 2, 5)

    def testTagSolidFlatCone(self):
        #"""Tests a Tag that has an angle leaving a flat face on top of the cone."""
        tag = Tag(0, 0, 18, 5, 45, True)
        tag.createSolidsAt(0)

        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0,0,0), 9, 4, 5)

        self.assertIsNotNone(tag.core)
        self.assertCylinderAt(tag.core, Vector(0,0,0), 4, 5)

    def testTagSolidCone(self):
        #"""Tests a Tag who's angled sides coincide at the tag's height."""
        tag = Tag(0, 0, 10, 5, 45, True)
        tag.createSolidsAt(0)
        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0,0,0), 5, 0, 5)

        self.assertIsNone(tag.core)

    def testTagSolidShortCone(self):
        #"""Tests a Tag that's not wide enough to reach full height."""
        tag = Tag(0, 0, 5, 17, 60, True)
        tag.createSolidsAt(0)
        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0,0,0), 2.5, 0, 2.5 * math.tan((60/180.0)*math.pi))

        self.assertIsNone(tag.core)

class SquareTagTestCases(TagTestCaseBase): # ============= 
    """Unit tests for square tags."""

    def testTagNoIntersect(self):
        #"""Check that the returned tail if no intersection occurs matches the input."""
        tag = Tag( 0, 0, 4, 7, 90, True, 0)
        pt1 = Vector(+5, 3, 0)
        pt2 = Vector(-5, 3, 0)
        edge = Part.Edge(Part.Line(pt1, pt2))

        i = tag.intersect(edge)
        self.assertIsNotNone(i)
        self.assertTrue(i.isComplete())
        self.assertIsNotNone(i.edges)
        self.assertFalse(i.edges)
        self.assertLine(i.tail, pt1, pt2)

    def testTagIntersectLine(self):
        #"""Test that a straight line passing through a cylindrical tag is split up into 5 segments."""
        tag = Tag( 0, 0, 4, 7, 90, True, 0)
        pt1 = Vector(+5, 0, 0)
        pt2 = Vector(-5, 0, 0)
        edge = Part.Edge(Part.Line(pt1, pt2))

        i = tag.intersect(edge)
        self.assertIsNotNone(i)
        self.assertTrue(i.isComplete())

        pt0a = Vector(+2, 0, 0)
        pt0b = Vector(+2, 0, 7)
        pt0c = Vector(-2, 0, 7)
        pt0d = Vector(-2, 0, 0)

        self.assertEqual(len(i.edges), 4)
        self.assertLine(i.edges[0], pt1,  pt0a)
        self.assertLine(i.edges[1], pt0a, pt0b)
        self.assertLine(i.edges[2], pt0b, pt0c)
        self.assertLine(i.edges[3], pt0c, pt0d)
        self.assertLine(i.tail,     pt0d, pt2)


    def testTagIntersectPartialLineP0(self):
        #"""Make sure line is accounted for if it reaches P0."""
        tag = Tag( 0, 0, 4, 7, 90, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(2, 0, 0)))

        i = tag.intersect(edge)
        self.assertFalse(i.isComplete())

        self.assertEqual(len(i.edges), 1)
        self.assertLine(i.edges[0], edge.Curve.StartPoint, edge.Curve.EndPoint)
        self.assertIsNone(i.tail)


    def testTagIntersectPartialLineP1(self):
        #"""Make sure line is accounted for if it reaches beyond P1."""
        tag = Tag( 0, 0, 4, 7, 90, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(1, 0, 0)))

        i = tag.intersect(edge)
        self.assertFalse(i.isComplete())

        pt0a = Vector(+2, 0, 0)
        pt0b = Vector(+2, 0, 7)
        pt1a = Vector(+1, 0, 7)

        self.assertEqual(len(i.edges), 3)
        self.assertLine(i.edges[0], edge.Curve.StartPoint, pt0a)
        self.assertLine(i.edges[1], pt0a, pt0b)
        self.assertLine(i.edges[2], pt0b, pt1a)
        self.assertIsNone(i.tail)


    def testTagIntersectPartialLineP2(self):
        #"""Make sure line is accounted for if it reaches beyond P2."""
        tag = Tag( 0, 0, 4, 7, 90, True, 0)
        edge = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(-1, 0, 0)))

        i = tag.intersect(edge)
        self.assertFalse(i.isComplete())

        pt0a = Vector(+2, 0, 0)
        pt0b = Vector(+2, 0, 7)
        pt1a = Vector(-1, 0, 7)

        self.assertEqual(len(i.edges), 3)
        self.assertLine(i.edges[0], edge.Curve.StartPoint, pt0a)
        self.assertLine(i.edges[1], pt0a, pt0b)
        self.assertLine(i.edges[2], pt0b, pt1a)
        self.assertIsNone(i.tail)

    def testTagIntersectPartialLineP11(self):
        #"""Make sure a line is accounted for if it lies entirely between P1 and P2."""
        tag = Tag( 0, 0, 4, 7, 90, True, 0)
        e1 = Part.Edge(Part.Line(Vector(5, 0, 0), Vector(+1, 0, 0)))

        i = tag.intersect(e1)
        self.assertFalse(i.isComplete())

        e2 = Part.Edge(Part.Line(e1.Curve.EndPoint, Vector(0,0,0)))
        i = i.intersect(e2)

        pt0a = Vector(+2, 0, 0)
        pt0b = Vector(+2, 0, 7)
        pt1a = Vector(+1, 0, 7)
        pt1b = Vector( 0, 0, 7)

        self.assertEqual(len(i.edges), 4)
        self.assertLine(i.edges[0], e1.Curve.StartPoint, pt0a)
        self.assertLine(i.edges[1], pt0a, pt0b)
        self.assertLine(i.edges[2], pt0b, pt1a)
        self.assertLine(i.edges[3], pt1a, pt1b)
        self.assertIsNone(i.tail)

    def testTagIntersectPartialLinesP11223(self):
        #"""Verify all lines between P0 and P3 are added."""
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

        self.assertLine(i.edges[0], e0.Curve.StartPoint, pt0)
        self.assertLine(i.edges[1], pt0, pt1)
        self.assertLine(i.edges[2], pt1, pt2)
        self.assertLine(i.edges[3], pt2, pt3)
        self.assertLine(i.edges[4], pt3, pt4)
        self.assertLine(i.edges[5], pt4, pt5)
        self.assertLine(i.edges[6], pt5, pt6)
        self.assertLine(i.edges[7], pt6, e5.Curve.EndPoint)
        self.assertTrue(i.isComplete())

        self.assertIsNotNone(i.tail)
        self.assertLine(i.tail, e6.Curve.StartPoint, e6.Curve.EndPoint)

    def testTagIntersectLineAt(self):
        tag = Tag( 0, 0, 4, 7, 90, True, 0)
        # for all lines below 7 we get the trapezoid
        for i in range(0, 7):
            edge = Part.Edge(Part.Line(Vector(5, 0, i), Vector(-5, 0, i)))
            s = tag.intersect(edge)
            self.assertTrue(s.isComplete())
            self.assertTrapezoid(s.edges, s.tail, [edge.Curve.StartPoint, 2, 7, edge.Curve.EndPoint])

        # for all edges at height or above the original line is used
        for i in range(7, 9):
            edge = Part.Edge(Part.Line(Vector(5, 0, i), Vector(-5, 0, i)))
            s = tag.intersect(edge)
            self.assertTrue(s.isComplete())
            self.assertLine(s.tail, edge.Curve.StartPoint, edge.Curve.EndPoint)

