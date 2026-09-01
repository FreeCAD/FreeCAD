# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

import unittest

import FreeCAD
import Part


class TestThickness(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestThickness")

    def _create_case_5829(self):
        """Create the rotated wedge/box/fillet model from issue case 5829."""
        body = self.Doc.addObject("PartDesign::Body", "Body")

        wedge = self.Doc.addObject("PartDesign::AdditiveWedge", "Wedge")
        wedge.Xmin = 0.0
        wedge.X2min = 10.0
        wedge.Xmax = 96.0
        wedge.X2max = 86.0
        wedge.Zmin = 0.0
        wedge.Z2min = 10.0
        wedge.Zmax = 126.0
        wedge.Z2max = 116.0
        wedge.Ymin = 0.0
        wedge.Ymax = 25.0
        body.addObject(wedge)

        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 96.0
        box.Width = 126.0
        box.Height = 10.0
        box.Placement = FreeCAD.Placement(
            FreeCAD.Vector(),
            FreeCAD.Rotation(0.0, 0.0, 90.0),
        )
        body.addObject(box)
        self.Doc.recompute()

        fillet = self.Doc.addObject("PartDesign::Fillet", "Fillet")
        fillet.Base = (
            box,
            [
                "Edge15",
                "Edge13",
                "Edge12",
                "Edge11",
                "Edge4",
                "Edge5",
                "Edge3",
                "Edge2",
                "Face4",
                "Edge19",
                "Edge9",
                "Edge7",
            ],
        )
        fillet.Radius = 8.0
        body.addObject(fillet)
        self.Doc.recompute()
        return body, fillet

    def _find_case_5829_opening_face_name(self, shape):
        candidates = [
            (index, face)
            for index, face in enumerate(shape.Faces, 1)
            if isinstance(face.Surface, Part.Plane) and abs(face.CenterOfMass.y + 10.0) < 1.0e-7
        ]
        self.assertTrue(candidates)
        index, _face = max(candidates, key=lambda item: item[1].Area)
        return "Face" + str(index)

    def testReversedThickness(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.Box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        self.Body.addObject(self.Box)
        self.Box.Length = 10.00
        self.Box.Width = 10.00
        self.Box.Height = 10.00
        self.Doc.recompute()
        self.Thickness = self.Doc.addObject("PartDesign::Thickness", "Thickness")
        self.Thickness.Base = (self.Box, ["Face1"])
        self.Body.addObject(self.Thickness)
        self.Doc.recompute()
        self.Thickness.Value = 1.0
        self.Thickness.Reversed = 1
        self.Thickness.Mode = 0
        self.Thickness.Join = 0
        self.Thickness.Base = (self.Box, ["Face1"])
        self.Doc.recompute()
        self.assertEqual(len(self.Thickness.Shape.Faces), 11)
        # 6 faces of outer box + 4 faces of inner box + 16 edges outer, 8 inner,
        # + 8 vertexes outer, 8 inner + 1 solid = 51
        self.assertEqual(self.Thickness.Shape.ElementMapSize, 51)

    def testCase5829ThicknessOnRotatedFillet(self):
        """Verify thickness succeeds on the rotated fillet from issue 5829."""
        body, fillet = self._create_case_5829()
        self.assertTrue(fillet.Shape.isValid())
        self.assertEqual(len(fillet.Shape.Solids), 1)
        self.assertNotEqual(fillet.Placement, FreeCAD.Placement())
        opening = self._find_case_5829_opening_face_name(fillet.Shape)

        thickness = self.Doc.addObject("PartDesign::Thickness", "Thickness")
        thickness.Base = (fillet, [opening])
        thickness.Value = 1.0
        thickness.Reversed = True
        body.addObject(thickness)
        self.Doc.recompute()

        self.assertTrue(thickness.isValid())
        self.assertTrue(thickness.Shape.isValid())
        self.assertEqual(len(thickness.Shape.Solids), 1)
        self.assertEqual(thickness.Placement, fillet.Placement)

    def testCase5829RectoVersoThicknessOnRotatedFillet(self):
        """Verify recto-verso thickness processes the issue 5829 geometry locally."""
        body, fillet = self._create_case_5829()
        self.assertTrue(fillet.Shape.isValid())
        self.assertEqual(len(fillet.Shape.Solids), 1)
        self.assertNotEqual(fillet.Placement, FreeCAD.Placement())
        opening = self._find_case_5829_opening_face_name(fillet.Shape)

        thickness = self.Doc.addObject("PartDesign::Thickness", "RectoVersoThickness")
        thickness.Base = (fillet, [opening])
        thickness.Value = 1.0
        thickness.Mode = "RectoVerso"
        body.addObject(thickness)
        self.Doc.recompute()

        self.assertTrue(thickness.isValid())
        self.assertTrue(thickness.Shape.isValid())
        self.assertEqual(len(thickness.Shape.Solids), 1)
        self.assertEqual(thickness.Placement, fillet.Placement)

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument("PartDesignTestThickness")
        # print ("omit closing document for debugging")


class TestRectoVersoThickness(unittest.TestCase):
    """Regression tests for centered, two-sided Part Design thickness."""

    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestRectoVersoThickness")

    def tearDown(self):
        FreeCAD.closeDocument("PartDesignTestRectoVersoThickness")

    def makeThickness(
        self,
        shape,
        closingFaces,
        value=2.0,
        reversed=False,
        join="Intersection",
    ):
        suffix = str(len(self.Doc.Objects))
        body = self.Doc.addObject("PartDesign::Body", "Body" + suffix)
        base = self.Doc.addObject("PartDesign::Feature", "Base" + suffix)
        body.addObject(base)
        base.Shape = shape
        self.Doc.recompute()

        thickness = self.Doc.addObject("PartDesign::Thickness", "Thickness" + suffix)
        body.addObject(thickness)
        thickness.Base = (base, closingFaces)
        thickness.Value = value
        thickness.Reversed = reversed
        thickness.Mode = "RectoVerso"
        thickness.Join = join
        self.Doc.recompute()
        return thickness.Shape.copy()

    def assertValidSolid(self, shape):
        self.assertFalse(shape.isNull())
        self.assertTrue(shape.isValid())
        self.assertEqual(len(shape.Solids), 1)
        self.assertGreater(shape.ElementMapSize, 0)

    def assertShapesEquivalent(self, first, second, tolerance=1e-7):
        self.assertAlmostEqual(first.Volume, second.Volume, delta=tolerance)
        self.assertLess(first.cut(second).Volume, tolerance)
        self.assertLess(second.cut(first).Volume, tolerance)

    def testPlanarOpenShellIsCenteredOnRetainedFaces(self):
        # A 40 x 30 x 10 box opened at the top.  Value=2 means two
        # millimetres on each side of the retained shell, not two total.
        result = self.makeThickness(Part.makeBox(40, 30, 10), ["Face6"])
        self.assertValidSolid(result)
        self.assertAlmostEqual(result.Volume, 10464.0, delta=1e-7)
        bounds = result.BoundBox
        self.assertAlmostEqual(bounds.XMin, -2.0, delta=1e-7)
        self.assertAlmostEqual(bounds.XMax, 42.0, delta=1e-7)
        self.assertAlmostEqual(bounds.YMin, -2.0, delta=1e-7)
        self.assertAlmostEqual(bounds.YMax, 32.0, delta=1e-7)
        self.assertAlmostEqual(bounds.ZMin, -2.0, delta=1e-7)
        self.assertAlmostEqual(bounds.ZMax, 10.0, delta=1e-7)

    def testReversedDoesNotChangeRectoVersoResult(self):
        box = Part.makeBox(40, 30, 10)
        forward = self.makeThickness(box, ["Face6"], reversed=False)
        reversedResult = self.makeThickness(box, ["Face6"], reversed=True)
        self.assertValidSolid(forward)
        self.assertValidSolid(reversedResult)
        self.assertShapesEquivalent(forward, reversedResult)

    def testSourceSolidOrientationDoesNotChangeResult(self):
        box = Part.makeBox(40, 30, 10)
        normal = self.makeThickness(box, ["Face6"])
        reversedBox = box.copy()
        reversedBox.reverse()
        reversedSource = self.makeThickness(reversedBox, ["Face6"])
        self.assertValidSolid(normal)
        self.assertValidSolid(reversedSource)
        self.assertShapesEquivalent(normal, reversedSource)

    def testCurvedOpenShellIsCenteredRadiallyAndAxially(self):
        cylinder = Part.makeCylinder(20, 10)
        topFace = max(
            range(len(cylinder.Faces)),
            key=lambda index: cylinder.Faces[index].CenterOfMass.z,
        )
        result = self.makeThickness(cylinder, ["Face" + str(topFace + 1)])
        self.assertValidSolid(result)
        self.assertAlmostEqual(result.Volume, 3216.0 * 3.141592653589793, delta=1e-6)
        bounds = result.BoundBox
        self.assertAlmostEqual(bounds.XMin, -22.0, delta=1e-7)
        self.assertAlmostEqual(bounds.XMax, 22.0, delta=1e-7)
        self.assertAlmostEqual(bounds.ZMin, -2.0, delta=1e-7)
        self.assertAlmostEqual(bounds.ZMax, 10.0, delta=1e-7)

    def testArcJoinProducesValidCenteredCurvedWall(self):
        cylinder = Part.makeCylinder(20, 10)
        topFace = max(
            range(len(cylinder.Faces)),
            key=lambda index: cylinder.Faces[index].CenterOfMass.z,
        )
        result = self.makeThickness(
            cylinder,
            ["Face" + str(topFace + 1)],
            join="Arc",
        )
        reversedResult = self.makeThickness(
            cylinder,
            ["Face" + str(topFace + 1)],
            reversed=True,
            join="Arc",
        )
        self.assertValidSolid(result)
        self.assertValidSolid(reversedResult)
        self.assertShapesEquivalent(result, reversedResult)

    def testTwoOpeningsCreateCenteredThroughWall(self):
        # Removing the two opposite caps leaves only the four side faces.
        result = self.makeThickness(Part.makeBox(40, 30, 10), ["Face5", "Face6"])
        self.assertValidSolid(result)
        self.assertAlmostEqual(result.Volume, 5600.0, delta=1e-7)
        bounds = result.BoundBox
        self.assertAlmostEqual(bounds.XMin, -2.0, delta=1e-7)
        self.assertAlmostEqual(bounds.XMax, 42.0, delta=1e-7)
        self.assertAlmostEqual(bounds.YMin, -2.0, delta=1e-7)
        self.assertAlmostEqual(bounds.YMax, 32.0, delta=1e-7)
        self.assertAlmostEqual(bounds.ZMin, 0.0, delta=1e-7)
        self.assertAlmostEqual(bounds.ZMax, 10.0, delta=1e-7)

    def testMixedPlanarAndCylindricalFacesRemainCentered(self):
        box = Part.makeBox(36, 28, 14)
        verticalEdges = [
            edge
            for edge in box.Edges
            if abs(edge.Vertexes[-1].Point.z - edge.Vertexes[0].Point.z) > 13.9
        ]
        filletedBox = box.makeFillet(3.0, verticalEdges)
        surfaceTypes = {face.Surface.TypeId for face in filletedBox.Faces}
        self.assertIn("Part::GeomPlane", surfaceTypes)
        self.assertIn("Part::GeomCylinder", surfaceTypes)

        topFace = max(
            range(len(filletedBox.Faces)),
            key=lambda index: (
                filletedBox.Faces[index].CenterOfMass.z,
                filletedBox.Faces[index].Area,
            ),
        )
        result = self.makeThickness(
            filletedBox,
            ["Face" + str(topFace + 1)],
            value=1.25,
        )
        self.assertValidSolid(result)
        bounds = result.BoundBox
        self.assertAlmostEqual(bounds.XMin, -1.25, delta=1e-7)
        self.assertAlmostEqual(bounds.XMax, 37.25, delta=1e-7)
        self.assertAlmostEqual(bounds.YMin, -1.25, delta=1e-7)
        self.assertAlmostEqual(bounds.YMax, 29.25, delta=1e-7)
        self.assertAlmostEqual(bounds.ZMin, -1.25, delta=1e-7)
        self.assertAlmostEqual(bounds.ZMax, 14.0, delta=1e-7)
