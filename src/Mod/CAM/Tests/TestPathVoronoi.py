# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
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
import Tests.PathTestUtils as PathTestUtils

vd = None


def initVD():
    global vd
    if vd is None:
        pts = [
            (0, 0),
            (3.5, 0),
            (3.5, 1),
            (1, 1),
            (1, 2),
            (2.5, 2),
            (2.5, 3),
            (1, 3),
            (1, 4),
            (3.5, 4),
            (3.5, 5),
            (0, 5),
        ]
        ptv = [FreeCAD.Vector(p[0], p[1]) for p in pts]
        ptv.append(ptv[0])

        vd = Path.Voronoi.Diagram()
        for i in range(len(pts)):
            vd.addSegment(ptv[i], ptv[i + 1])

        vd.construct()

        for e in vd.Edges:
            e.Color = 0 if e.isPrimary() else 1

        vd.colorExterior(2)
        vd.colorColinear(3)
        vd.colorTwins(4)


class TestPathVoronoi(PathTestUtils.PathTestBase):
    def setUp(self):
        initVD()

    def test00(self):
        """Check vertex comparison"""

        self.assertTrue(vd.Vertices[0] == vd.Vertices[0])
        self.assertTrue(vd.Vertices[1] == vd.Vertices[1])
        self.assertTrue(vd.Vertices[0] != vd.Vertices[1])
        self.assertEqual(vd.Vertices[0], vd.Vertices[0])
        self.assertEqual(vd.Vertices[1], vd.Vertices[1])
        self.assertNotEqual(vd.Vertices[0], vd.Vertices[1])
        self.assertNotEqual(vd.Vertices[1], vd.Vertices[0])

    def test10(self):
        """Check edge comparison"""

        self.assertTrue(vd.Edges[0] == vd.Edges[0])
        self.assertTrue(vd.Edges[1] == vd.Edges[1])
        self.assertTrue(vd.Edges[0] != vd.Edges[1])
        self.assertEqual(vd.Edges[0], vd.Edges[0])
        self.assertEqual(vd.Edges[1], vd.Edges[1])
        self.assertNotEqual(vd.Edges[0], vd.Edges[1])
        self.assertNotEqual(vd.Edges[1], vd.Edges[0])

    def test20(self):
        """Check cell comparison"""

        self.assertTrue(vd.Cells[0] == vd.Cells[0])
        self.assertTrue(vd.Cells[1] == vd.Cells[1])
        self.assertTrue(vd.Cells[0] != vd.Cells[1])
        self.assertEqual(vd.Cells[0], vd.Cells[0])
        self.assertEqual(vd.Cells[1], vd.Cells[1])
        self.assertNotEqual(vd.Cells[0], vd.Cells[1])
        self.assertNotEqual(vd.Cells[1], vd.Cells[0])

    def test50(self):
        """Check toShape for linear edges"""

        edges = [e for e in vd.Edges if e.Color == 0 and e.isLinear()]
        self.assertNotEqual(len(edges), 0)
        e0 = edges[0]

        e = e0.toShape()
        self.assertTrue(type(e.Curve) == Part.LineSegment or type(e.Curve) == Part.Line)
        self.assertFalse(
            Path.Geom.pointsCoincide(
                e.valueAt(e.FirstParameter), e.valueAt(e.LastParameter)
            )
        )
        self.assertRoughly(e.valueAt(e.FirstParameter).z, 0)
        self.assertRoughly(e.valueAt(e.LastParameter).z, 0)

    def test51(self):
        """Check toShape for linear edges with set z"""

        edges = [e for e in vd.Edges if e.Color == 0 and e.isLinear()]
        self.assertNotEqual(len(edges), 0)
        e0 = edges[0]

        e = e0.toShape(13.7)
        self.assertTrue(type(e.Curve) == Part.LineSegment or type(e.Curve) == Part.Line)
        self.assertFalse(
            Path.Geom.pointsCoincide(
                e.valueAt(e.FirstParameter), e.valueAt(e.LastParameter)
            )
        )
        self.assertRoughly(e.valueAt(e.FirstParameter).z, 13.7)
        self.assertRoughly(e.valueAt(e.LastParameter).z, 13.7)

    def test52(self):
        """Check toShape for linear edges with varying z"""

        edges = [e for e in vd.Edges if e.Color == 0 and e.isLinear()]
        self.assertNotEqual(len(edges), 0)
        e0 = edges[0]

        e = e0.toShape(2.37, 5.14)
        self.assertTrue(type(e.Curve) == Part.LineSegment or type(e.Curve) == Part.Line)
        self.assertFalse(
            Path.Geom.pointsCoincide(
                e.valueAt(e.FirstParameter), e.valueAt(e.LastParameter)
            )
        )
        self.assertRoughly(e.valueAt(e.FirstParameter).z, 2.37)
        self.assertRoughly(e.valueAt(e.LastParameter).z, 5.14)

    def test60(self):
        """Check toShape for curved edges"""

        edges = [e for e in vd.Edges if e.Color == 0 and e.isCurved()]
        self.assertNotEqual(len(edges), 0)
        e0 = edges[0]

        e = e0.toShape()
        self.assertTrue(
            type(e.Curve) == Part.Parabola or type(e.Curve) == Part.BSplineCurve
        )
        self.assertFalse(
            Path.Geom.pointsCoincide(
                e.valueAt(e.FirstParameter), e.valueAt(e.LastParameter)
            )
        )
        self.assertRoughly(e.valueAt(e.FirstParameter).z, 0)
        self.assertRoughly(e.valueAt(e.LastParameter).z, 0)

    def test61(self):
        """Check toShape for curved edges with set z"""

        edges = [e for e in vd.Edges if e.Color == 0 and e.isCurved()]
        self.assertNotEqual(len(edges), 0)
        e0 = edges[0]

        e = e0.toShape(13.7)
        self.assertTrue(
            type(e.Curve) == Part.Parabola or type(e.Curve) == Part.BSplineCurve
        )
        self.assertFalse(
            Path.Geom.pointsCoincide(
                e.valueAt(e.FirstParameter), e.valueAt(e.LastParameter)
            )
        )
        self.assertRoughly(e.valueAt(e.FirstParameter).z, 13.7)
        self.assertRoughly(e.valueAt(e.LastParameter).z, 13.7)

    def test62(self):
        """Check toShape for curved edges with varying z"""

        edges = [e for e in vd.Edges if e.Color == 0 and e.isCurved()]
        self.assertNotEqual(len(edges), 0)
        e0 = edges[0]

        e = e0.toShape(2.37, 5.14)
        self.assertTrue(
            type(e.Curve) == Part.Parabola or type(e.Curve) == Part.BSplineCurve
        )
        self.assertFalse(
            Path.Geom.pointsCoincide(
                e.valueAt(e.FirstParameter), e.valueAt(e.LastParameter)
            )
        )
        self.assertRoughly(e.valueAt(e.FirstParameter).z, 2.37)
        self.assertRoughly(e.valueAt(e.LastParameter).z, 5.14)
