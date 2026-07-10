# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD
import TestSketcherApp

App = FreeCAD


class TestBaseFeature(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestBaseFeature")

    def testBodyBaseFeatureInitializesPlacement(self):
        box = self.Doc.addObject("Part::Box", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        box.Placement.Base = App.Vector(120, 0, 0)

        body = self.Doc.addObject("PartDesign::Body", "Body")
        # Body.BaseFeature initializes the internal FeatureBase from current global placements.
        body.Placement.Base = App.Vector(100, 0, 0)
        body.BaseFeature = box
        self.Doc.recompute()

        base = body.Group[0]
        self.assertEqual(base.TypeId, "PartDesign::FeatureBase")
        self.assertFalse(base.UseLegacyBaseFeaturePlacement)
        self.assertAlmostEqual(base.Placement.Base.x, 20)
        self.assertAlmostEqual(body.Shape.BoundBox.XMin, 120)
        self.assertAlmostEqual(body.Shape.BoundBox.XMax, 130)

    def testBodyBaseFeatureInitializesGlobalPlacement(self):
        box = self.Doc.addObject("Part::Box", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        box.Placement.Base = App.Vector(125, 0, 0)

        container = self.Doc.addObject("App::Part", "Part")
        container.Placement.Base = App.Vector(100, 0, 0)
        body = self.Doc.addObject("PartDesign::Body", "Body")
        container.addObject(body)
        body.Placement.Base = App.Vector(20, 0, 0)

        body.BaseFeature = box
        self.Doc.recompute()

        base = body.Group[0]
        self.assertEqual(base.TypeId, "PartDesign::FeatureBase")
        self.assertFalse(base.UseLegacyBaseFeaturePlacement)
        self.assertAlmostEqual(base.Placement.Base.x, 5)
        self.assertAlmostEqual(body.Shape.BoundBox.XMin, 25)
        self.assertAlmostEqual(body.Shape.BoundBox.XMax, 35)

    def testBodyBaseFeaturePreservesPlacementInsideSamePart(self):
        container = self.Doc.addObject("App::Part", "Part")
        container.Placement.Base = App.Vector(100, 0, 0)

        box = self.Doc.addObject("Part::Box", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        container.addObject(box)
        box.Placement.Base = App.Vector(25, 0, 0)

        body = self.Doc.addObject("PartDesign::Body", "Body")
        container.addObject(body)
        body.Placement.Base = App.Vector(20, 0, 0)

        body.BaseFeature = box
        self.Doc.recompute()

        base = body.Group[0]
        self.assertEqual(base.TypeId, "PartDesign::FeatureBase")
        self.assertFalse(base.UseLegacyBaseFeaturePlacement)
        self.assertAlmostEqual(base.Placement.Base.x, 5)
        self.assertAlmostEqual(body.Shape.BoundBox.XMin, 25)
        self.assertAlmostEqual(body.Shape.BoundBox.XMax, 35)

    def testFeatureBasePlacementControlsExternalBaseFeature(self):
        box = self.Doc.addObject("Part::Box", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        box.Placement.Base = App.Vector(20, 0, 0)

        body = self.Doc.addObject("PartDesign::Body", "Body")
        base = self.Doc.addObject("PartDesign::FeatureBase", "BaseFeature")
        body.addObject(base)
        base.BaseFeature = box
        base.Placement.Base = App.Vector(5, 0, 0)
        self.assertFalse(base.UseLegacyBaseFeaturePlacement)
        self.Doc.recompute()

        self.assertAlmostEqual(body.Shape.BoundBox.XMin, 5)
        self.assertAlmostEqual(body.Shape.BoundBox.XMax, 15)

    def testFeatureBaseLegacyPlacementKeepsOldBehavior(self):
        box = self.Doc.addObject("Part::Box", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        box.Placement.Base = App.Vector(20, 0, 0)

        body = self.Doc.addObject("PartDesign::Body", "Body")
        base = self.Doc.addObject("PartDesign::FeatureBase", "BaseFeature")
        body.addObject(base)
        base.UseLegacyBaseFeaturePlacement = True
        base.BaseFeature = box
        self.Doc.recompute()

        self.assertAlmostEqual(body.Shape.BoundBox.XMin, 0)
        self.assertAlmostEqual(body.Shape.BoundBox.XMax, 10)

    def testFeatureBaseDoesNotBakeSourceBodyPlacement(self):
        source_body = self.Doc.addObject("PartDesign::Body", "SourceBody")
        source_body.Placement.Base = App.Vector(50, 0, 0)
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 1
        source_body.addObject(box)
        self.Doc.recompute()

        clone_body = self.Doc.addObject("PartDesign::Body", "CloneBody")
        base = self.Doc.addObject("PartDesign::FeatureBase", "BaseFeature")
        clone_body.addObject(base)
        base.BaseFeature = source_body
        base.Placement.Base = App.Vector(5, 0, 0)
        self.Doc.recompute()

        self.assertAlmostEqual(clone_body.Shape.BoundBox.XMin, 5)
        self.assertAlmostEqual(clone_body.Shape.BoundBox.XMax, 15)

    def testBodyCloneCanSwitchToTipFeatureWithoutTransform(self):
        source_body = self.Doc.addObject("PartDesign::Body", "SourceBody")
        sketch = source_body.newObject("Sketcher::SketchObject", "RevolutionSketch")
        TestSketcherApp.CreateRectangleSketch(sketch, (9, 0), (10, 5))
        self.Doc.recompute()

        revolution = source_body.newObject("PartDesign::Revolution", "Revolution")
        revolution.Profile = sketch
        revolution.ReferenceAxis = (sketch, ["V_Axis"])
        revolution.Angle = 180
        self.Doc.recompute()

        clone_body = self.Doc.addObject("PartDesign::Body", "CloneBody")
        base = self.Doc.addObject("PartDesign::FeatureBase", "BaseFeature")
        clone_body.addObject(base)
        base.BaseFeature = source_body
        base.Placement.Base = App.Vector(20, 0, 0)
        self.Doc.recompute()

        initial_box = clone_body.Shape.BoundBox
        initial_bounds = (
            initial_box.XMin,
            initial_box.XMax,
            initial_box.YMin,
            initial_box.YMax,
            initial_box.ZMin,
            initial_box.ZMax,
        )

        base.BaseFeature = revolution
        self.Doc.recompute()

        changed_box = clone_body.Shape.BoundBox
        changed_bounds = (
            changed_box.XMin,
            changed_box.XMax,
            changed_box.YMin,
            changed_box.YMax,
            changed_box.ZMin,
            changed_box.ZMax,
        )
        for changed, initial in zip(changed_bounds, initial_bounds):
            self.assertAlmostEqual(changed, initial)

    def testFeatureBasePlacementControlsChangedPartDesignBaseFeature(self):
        def make_pad(body, name, x):
            sketch = body.newObject("Sketcher::SketchObject", f"{name}Sketch")
            TestSketcherApp.CreateRectangleSketch(sketch, (x, 0), (2, 10))
            self.Doc.recompute()

            pad = body.newObject("PartDesign::Pad", name)
            pad.Profile = sketch
            pad.Length = 1
            self.Doc.recompute()
            return pad

        source_body = self.Doc.addObject("PartDesign::Body", "SourceBody")
        box = make_pad(source_body, "Box", 0)

        moved_body = self.Doc.addObject("PartDesign::Body", "MovedBody")
        moved = make_pad(moved_body, "MovedBox", 5)

        clone_body = self.Doc.addObject("PartDesign::Body", "CloneBody")
        base = self.Doc.addObject("PartDesign::FeatureBase", "BaseFeature")
        clone_body.addObject(base)
        base.BaseFeature = box
        base.Placement.Base = App.Vector(20, 0, 0)
        self.Doc.recompute()

        self.assertAlmostEqual(clone_body.Shape.BoundBox.XMin, 20)
        self.assertAlmostEqual(clone_body.Shape.BoundBox.XMax, 22)

        base.BaseFeature = moved
        self.Doc.recompute()

        self.assertAlmostEqual(base.Placement.Base.x, 20)
        self.assertAlmostEqual(clone_body.Shape.BoundBox.XMin, 25)
        self.assertAlmostEqual(clone_body.Shape.BoundBox.XMax, 27)
        self.assertAlmostEqual(clone_body.Shape.BoundBox.YMin, 0)
        self.assertAlmostEqual(clone_body.Shape.BoundBox.YMax, 10)

    def tearDown(self):
        if hasattr(App, "KeepTestDoc") and App.KeepTestDoc:
            return
        FreeCAD.closeDocument("PartDesignTestBaseFeature")
