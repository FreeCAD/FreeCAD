# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tests for Part per-face color persistence and topology updates."""

from collections import Counter
import FreeCAD as App
import Part
import Sketcher
import os
import tempfile
import unittest
from BOPTools import BOPFeatures
from pivy import coin


class ColorPerFaceTest(unittest.TestCase):
    """Exercise per-face color behavior across recompute and restore."""

    def setUp(self):
        """Create a temporary document for each test."""
        TempPath = tempfile.gettempdir()
        self.fileName = TempPath + os.sep + "ColorPerFaceTest.FCStd"
        self.doc = App.newDocument()

    def tearDown(self):
        """Close the temporary document after each test."""
        App.closeDocument(self.doc.Name)

    def makeClosedSketch(self, name, points):
        """Create a closed sketch from ordered 2D point tuples."""
        sketch = self.doc.addObject("Sketcher::SketchObject", name)
        geometry = []
        for start, end in zip(points, points[1:] + points[:1]):
            geometry.append(
                Part.LineSegment(
                    App.Vector(start[0], start[1], 0),
                    App.Vector(end[0], end[1], 0),
                )
            )
        sketch.addGeometry(geometry, False)
        constraints = []
        for index in range(len(points)):
            constraints.append(
                Sketcher.Constraint("Coincident", index, 2, (index + 1) % len(points), 1)
            )
        sketch.addConstraint(constraints)
        return sketch

    def assertFaceMaterialCount(self, obj, expected):
        """Assert that the scene graph has one material per rendered face."""
        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterialBinding.getClassTypeId())
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(obj.ViewObject.RootNode)
        paths = sa.getPaths()

        bind = paths.get(1).getTail()
        self.assertEqual(bind.value.getValue(), bind.PER_PART)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(obj.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(1).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), expected)

    def colorTuple(self, color):
        """Convert a color sequence into a rounded tuple."""
        return tuple(round(float(value), 4) for value in color)

    def faceColorCounts(self, obj):
        """Return rounded DiffuseColor counts for an object."""
        return Counter(self.colorTuple(color) for color in obj.ViewObject.DiffuseColor)

    def faceColorsMatching(self, obj, predicate):
        """Return colors of faces whose center normal satisfies predicate."""
        matches = []
        for face, color in zip(obj.Shape.Faces, obj.ViewObject.DiffuseColor):
            u0, u1, v0, v1 = face.ParameterRange
            normal = face.normalAt((u0 + u1) / 2, (v0 + v1) / 2)
            if predicate(normal):
                matches.append(self.colorTuple(color))
        return matches

    def assertSingleMatchingFaceColor(self, obj, predicate, expected):
        """Assert that exactly one matching face has the expected color."""
        matches = self.faceColorsMatching(obj, predicate)
        self.assertEqual(len(matches), 1)
        self.assertEqual(matches[0], self.colorTuple(expected))

    def assertMatchingFaceColors(self, obj, predicate, expected):
        """Assert that matching faces have the expected color multiset."""
        expectedColors = Counter(self.colorTuple(color) for color in expected)
        self.assertEqual(Counter(self.faceColorsMatching(obj, predicate)), expectedColors)

    def assertHasBlueFace(self, obj):
        """Assert that an object still has at least one blue face color."""
        colors = obj.ViewObject.getElementColors("Face*")
        self.assertIn((0.0, 0.0, 1.0, 1.0), [self.colorTuple(color) for color in colors.values()])

    def testBox(self):
        """Restore a box with a complete per-face color list."""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        box.ViewObject.DiffuseColor = [
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 1.0, 0.0, 1.0),
            (1.0, 1.0, 0.0, 1.0),
        ]

        box.Visibility = False
        self.doc.recompute()

        self.doc.saveAs(self.fileName)
        App.closeDocument(self.doc.Name)

        self.doc = App.openDocument(self.fileName)
        box = self.doc.Box
        box.Visibility = True
        self.assertEqual(len(box.ViewObject.DiffuseColor), 6)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(box.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(1).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 6)

    def testBoxAndLink(self):
        """Restore linked box colors without losing per-face materials."""
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        box.ViewObject.DiffuseColor = [
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 1.0, 0.0, 1.0),
            (1.0, 1.0, 0.0, 1.0),
        ]

        link = self.doc.addObject("App::Link", "Link")
        link.setLink(box)
        box.Visibility = False
        self.doc.recompute()

        self.doc.saveAs(self.fileName)
        App.closeDocument(self.doc.Name)

        self.doc = App.openDocument(self.fileName)
        box = self.doc.Box
        box.Visibility = True
        self.assertEqual(len(box.ViewObject.DiffuseColor), 6)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(box.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(1).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 6)

    def testTransparency(self):
        """
        If color per face is set then changing the transparency must not revert it
        """
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        box.ViewObject.DiffuseColor = [
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 1.0, 0.0, 1.0),
            (1.0, 1.0, 0.0, 1.0),
        ]

        box.ViewObject.Transparency = 35
        self.assertEqual(box.ViewObject.Transparency, 35)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterialBinding.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(box.ViewObject.RootNode)
        paths = sa.getPaths()

        bind = paths.get(1).getTail()
        self.assertEqual(bind.value.getValue(), bind.PER_PART)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(box.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(1).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 6)

    def testTooFewFaceColorsSaveRestore(self):
        """
        If the shape has more faces than stored colors then restore must use
        the first stored color, matching the live recompute fallback.
        """
        box = self.doc.addObject("Part::Box", "Box")
        self.doc.recompute()

        box.ViewObject.DiffuseColor = [
            (1.0, 0.0, 0.0, 1.0),
            (0.0, 0.0, 1.0, 1.0),
        ]

        self.doc.saveAs(self.fileName)
        App.closeDocument(self.doc.Name)

        self.doc = App.openDocument(self.fileName)
        box = self.doc.Box
        self.assertEqual(len(box.Shape.Faces), 6)
        self.assertEqual(len(box.ViewObject.DiffuseColor), 2)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterialBinding.getClassTypeId())
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(box.ViewObject.RootNode)
        paths = sa.getPaths()

        bind = paths.get(1).getTail()
        self.assertEqual(bind.value.getValue(), bind.OVERALL)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(box.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(1).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 1)
        self.assertEqual(mat.diffuseColor[0].getValue(), (1.0, 0.0, 0.0))

    def testFaceColorsTrackStableFacesAfterTopologyChange(self):
        """Keep stable face colors after a topology-changing sketch edit."""
        sketch = self.makeClosedSketch("Sketch", [(0, 0), (10, 0), (10, 10), (0, 10)])
        sketch001 = self.doc.copyObject(sketch, False)
        sketch001.Label = "Sketch001"
        sketch001.delGeometry(0)
        first = sketch001.addGeometry(
            Part.LineSegment(App.Vector(0, 0, 0), App.Vector(5, 0, 0)),
            False,
        )
        second = sketch001.addGeometry(
            Part.LineSegment(App.Vector(5, 0, 0), App.Vector(10, 0, 0)),
            False,
        )
        sketch001.addConstraint(Sketcher.Constraint("Coincident", first, 2, second, 1))

        extrude = self.doc.addObject("Part::Extrusion", "Extrude")
        extrude.Base = sketch
        extrude.Dir = App.Vector(0, 0, 10)
        extrude.Solid = True
        self.doc.recompute()

        defaultFaceColor = (0.0, 1.0, 0.0, 1.0)
        red = (1.0, 0.0, 0.0, 1.0)
        blue = (0.0, 0.0, 1.0, 1.0)
        yellow = (1.0, 1.0, 0.0, 1.0)

        extrude.ViewObject.ShapeColor = defaultFaceColor
        extrude.ViewObject.DiffuseColor = [
            red,
            blue,
            red,
            blue,
            yellow,
            yellow,
        ]
        self.assertEqual(len(extrude.Shape.Faces), 6)
        self.assertFaceMaterialCount(extrude, 6)

        extrude.Base = sketch001
        self.doc.recompute()

        self.assertEqual(len(extrude.Shape.Faces), 7)
        self.assertFaceMaterialCount(extrude, 7)
        self.assertHasBlueFace(extrude)
        self.assertEqual(len(extrude.ViewObject.DiffuseColor), 7)
        counts = self.faceColorCounts(extrude)
        self.assertEqual(counts[red], 1, counts)
        self.assertEqual(counts[blue], 2, counts)
        self.assertEqual(counts[yellow], 2, counts)
        self.assertEqual(counts[defaultFaceColor], 2, counts)
        self.assertSingleMatchingFaceColor(extrude, lambda normal: normal.z > 0.9, yellow)
        self.assertSingleMatchingFaceColor(extrude, lambda normal: normal.z < -0.9, yellow)
        self.assertMatchingFaceColors(
            extrude, lambda normal: normal.y < -0.9, [defaultFaceColor, defaultFaceColor]
        )

        self.doc.saveAs(self.fileName)
        App.closeDocument(self.doc.Name)

        self.doc = App.openDocument(self.fileName)
        extrude = self.doc.Extrude
        self.assertEqual(len(extrude.Shape.Faces), 7)
        self.assertFaceMaterialCount(extrude, 7)
        self.assertHasBlueFace(extrude)
        self.assertEqual(len(extrude.ViewObject.DiffuseColor), 7)
        counts = self.faceColorCounts(extrude)
        self.assertEqual(counts[red], 1, counts)
        self.assertEqual(counts[blue], 2, counts)
        self.assertEqual(counts[yellow], 2, counts)
        self.assertEqual(counts[defaultFaceColor], 2, counts)
        self.assertSingleMatchingFaceColor(extrude, lambda normal: normal.z > 0.9, yellow)
        self.assertSingleMatchingFaceColor(extrude, lambda normal: normal.z < -0.9, yellow)
        self.assertMatchingFaceColors(
            extrude, lambda normal: normal.y < -0.9, [defaultFaceColor, defaultFaceColor]
        )

    def testMultiFuse(self):
        """
        Both input objects are red. So, it's expected that the output object is red, too.
        """
        box = self.doc.addObject("Part::Box", "Box")
        cyl = self.doc.addObject("Part::Cylinder", "Cylinder")
        box.ViewObject.ShapeColor = (1.0, 0.0, 0.0, 1.0)
        cyl.ViewObject.ShapeColor = (1.0, 0.0, 0.0, 1.0)
        self.doc.recompute()

        bp = BOPFeatures.BOPFeatures(self.doc)
        fuse = bp.make_multi_fuse([box.Name, cyl.Name])
        self.assertEqual(fuse.TypeId, "Part::MultiFuse")
        fuse.Refine = False
        self.doc.recompute()

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterialBinding.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(fuse.ViewObject.RootNode)
        paths = sa.getPaths()

        bind = paths.get(1).getTail()
        self.assertEqual(bind.value.getValue(), bind.PER_PART)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(fuse.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(1).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 11)

        self.assertEqual(len(fuse.Shape.Faces), 11)
        self.assertEqual(len(fuse.ViewObject.DiffuseColor), 11)
        self.assertEqual(fuse.ViewObject.DiffuseColor[0], (1.0, 0.0, 0.0, 1.0))

    def testMultiFuseSaveRestore(self):
        """Restore generated multi-fuse per-face colors."""
        box = self.doc.addObject("Part::Box", "Box")
        cyl = self.doc.addObject("Part::Cylinder", "Cylinder")
        box.ViewObject.ShapeColor = (1.0, 0.0, 0.0, 1.0)
        cyl.ViewObject.ShapeColor = (1.0, 0.0, 0.0, 1.0)
        self.doc.recompute()

        bp = BOPFeatures.BOPFeatures(self.doc)
        fuse = bp.make_multi_fuse([box.Name, cyl.Name])
        self.assertEqual(fuse.TypeId, "Part::MultiFuse")
        fuse.Refine = False
        self.doc.recompute()

        fuse.ViewObject.DiffuseColor = [(1.0, 0.0, 0.0, 1.0)] * 11

        self.doc.saveAs(self.fileName)
        App.closeDocument(self.doc.Name)

        self.doc = App.openDocument(self.fileName)

        fuse = self.doc.ActiveObject
        self.assertEqual(len(fuse.Shape.Faces), 11)
        self.assertEqual(len(fuse.ViewObject.DiffuseColor), 11)
        self.assertEqual(fuse.ViewObject.DiffuseColor[0], (1.0, 0.0, 0.0, 1.0))

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterialBinding.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(fuse.ViewObject.RootNode)
        paths = sa.getPaths()

        bind = paths.get(1).getTail()
        self.assertEqual(bind.value.getValue(), bind.PER_PART)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(fuse.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(1).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 11)
