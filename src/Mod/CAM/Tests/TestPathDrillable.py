# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
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

import FreeCAD as App
import Path
import Path.Base.Drillable as Drillable
import Tests.PathTestUtils as PathTestUtils


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TestPathDrillable(PathTestUtils.PathTestBase):
    def setUp(self):
        self.doc = App.open(App.getHomePath() + "/Mod/CAM/Tests/Drilling_1.FCStd")
        self.obj = self.doc.getObject("Pocket011")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def test00(self):
        """Test CompareVecs"""

        # Vec and origin
        v1 = App.Vector(0, 0, 10)
        v2 = App.Vector(0, 0, 0)
        self.assertTrue(Drillable.compareVecs(v1, v2))

        # two valid vectors
        v1 = App.Vector(0, 10, 0)
        v2 = App.Vector(0, 20, 0)
        self.assertTrue(Drillable.compareVecs(v1, v2))

        # two valid vectors not aligned
        v1 = App.Vector(0, 10, 0)
        v2 = App.Vector(10, 0, 0)
        self.assertFalse(Drillable.compareVecs(v1, v2))

    def test10(self):
        """Test isDrillable"""

        # Invalid types
        candidate = self.obj.getSubObject("Vertex1")
        self.assertRaises(
            TypeError, lambda: Drillable.isDrillable(self.obj.Shape, candidate)
        )

        # Test cylinder faces

        # thru-hole
        candidate = self.obj.getSubObject("Face30")

        # Typical drilling
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate))

        # Drilling with smaller bit
        self.assertTrue(
            Drillable.isDrillable(self.obj.Shape, candidate, tooldiameter=20)
        )

        # Drilling with bit too large
        self.assertFalse(
            Drillable.isDrillable(self.obj.Shape, candidate, tooldiameter=30)
        )

        # off-axis hole
        candidate = self.obj.getSubObject("Face44")

        # Typical drilling
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

        # Passing explicit vector
        self.assertTrue(
            Drillable.isDrillable(
                self.obj.Shape, candidate, vector=App.Vector(0, -1, 0)
            )
        )

        # Drilling with smaller bit
        self.assertTrue(
            Drillable.isDrillable(
                self.obj.Shape, candidate, tooldiameter=10, vector=App.Vector(0, -1, 0)
            )
        )

        # Drilling with bit too large
        self.assertFalse(
            Drillable.isDrillable(
                self.obj.Shape, candidate, tooldiameter=30, vector=App.Vector(0, -1, 0)
            )
        )

        # ellipse hole
        candidate = self.obj.getSubObject("Face29")

        # Typical drilling
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

        # raised cylinder
        candidate = self.obj.getSubObject("Face32")

        # Typical drilling
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

        # cylinder on slope
        candidate = self.obj.getSubObject("Face24")
        # Typical drilling
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

        # Circular Faces
        candidate = self.obj.getSubObject("Face54")

        # Typical drilling
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

        # Passing explicit vector
        self.assertTrue(
            Drillable.isDrillable(self.obj.Shape, candidate, vector=App.Vector(0, 0, 1))
        )

        # Drilling with smaller bit
        self.assertTrue(
            Drillable.isDrillable(self.obj.Shape, candidate, tooldiameter=10)
        )

        # Drilling with bit too large
        self.assertFalse(
            Drillable.isDrillable(self.obj.Shape, candidate, tooldiameter=30)
        )

        # off-axis circular face hole
        candidate = self.obj.getSubObject("Face58")

        # Typical drilling
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

        # Passing explicit vector
        self.assertTrue(
            Drillable.isDrillable(
                self.obj.Shape, candidate, vector=App.Vector(0, -1, 0)
            )
        )

        # raised face
        candidate = self.obj.getSubObject("Face49")
        # Typical drilling
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

        # interrupted Face
        candidate = self.obj.getSubObject("Face50")
        # Typical drilling
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

        # donut face
        candidate = self.obj.getSubObject("Face48")
        # Typical drilling
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

        # Test edges
        # circular edge
        candidate = self.obj.getSubObject("Edge55")

        # Typical drilling
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

        # Passing explicit vector
        self.assertTrue(
            Drillable.isDrillable(self.obj.Shape, candidate, vector=App.Vector(0, 0, 1))
        )

        # Drilling with smaller bit
        self.assertTrue(
            Drillable.isDrillable(self.obj.Shape, candidate, tooldiameter=10)
        )

        # Drilling with bit too large
        self.assertFalse(
            Drillable.isDrillable(
                self.obj.Shape, candidate, tooldiameter=30, vector=None
            )
        )

        # off-axis circular edge
        candidate = self.obj.getSubObject("Edge74")

        # Typical drilling
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertTrue(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

        # Passing explicit vector
        self.assertTrue(
            Drillable.isDrillable(self.obj.Shape, candidate, vector=App.Vector(0, 1, 0))
        )

        # incomplete circular edge
        candidate = self.obj.getSubObject("Edge39")
        # Typical drilling
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

        # elliptical edge
        candidate = self.obj.getSubObject("Edge56")
        # Typical drilling
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate))

        # Passing None as vector
        self.assertFalse(Drillable.isDrillable(self.obj.Shape, candidate, vector=None))

    def test20(self):
        """Test getDrillableTargets"""
        results = Drillable.getDrillableTargets(self.obj)
        self.assertEqual(len(results), 15)

        results = Drillable.getDrillableTargets(self.obj, vector=None)
        self.assertEqual(len(results), 20)

        results = Drillable.getDrillableTargets(self.obj, ToolDiameter=20, vector=None)
        self.assertEqual(len(results), 5)
