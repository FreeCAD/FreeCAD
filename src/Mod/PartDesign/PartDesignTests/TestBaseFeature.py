# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD

App = FreeCAD


class TestBaseFeature(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestBaseFeature")

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
        self.Doc.recompute()

        self.assertAlmostEqual(body.Shape.BoundBox.XMin, 5)
        self.assertAlmostEqual(body.Shape.BoundBox.XMax, 15)

    def testFeatureBasePlacementControlsChangedBaseFeature(self):
        box = self.Doc.addObject("Part::Box", "Box")
        box.Length = 2
        box.Width = 10
        box.Height = 1

        rotated = self.Doc.addObject("Part::Box", "RotatedBox")
        rotated.Length = 2
        rotated.Width = 10
        rotated.Height = 1
        rotated.Placement.Rotation = App.Rotation(App.Vector(0, 0, 1), 90)

        body = self.Doc.addObject("PartDesign::Body", "Body")
        base = self.Doc.addObject("PartDesign::FeatureBase", "BaseFeature")
        body.addObject(base)
        base.BaseFeature = box
        base.Placement.Base = App.Vector(20, 0, 0)
        self.Doc.recompute()

        base.BaseFeature = rotated
        self.Doc.recompute()

        self.assertAlmostEqual(body.Shape.BoundBox.XMin, 20)
        self.assertAlmostEqual(body.Shape.BoundBox.XMax, 22)
        self.assertAlmostEqual(body.Shape.BoundBox.YMin, 0)
        self.assertAlmostEqual(body.Shape.BoundBox.YMax, 10)

    def testFeatureBasePlacementControlsChangedPartDesignBaseFeature(self):
        source_body = self.Doc.addObject("PartDesign::Body", "SourceBody")
        box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 2
        box.Width = 10
        box.Height = 1
        source_body.addObject(box)

        rotated = self.Doc.addObject("PartDesign::AdditiveBox", "RotatedBox")
        rotated.Length = 2
        rotated.Width = 10
        rotated.Height = 1
        rotated.Placement.Rotation = App.Rotation(App.Vector(0, 0, 1), 90)
        source_body.addObject(rotated)
        self.Doc.recompute()

        clone_body = self.Doc.addObject("PartDesign::Body", "CloneBody")
        base = self.Doc.addObject("PartDesign::FeatureBase", "BaseFeature")
        clone_body.addObject(base)
        base.BaseFeature = box
        base.Placement.Base = App.Vector(20, 0, 0)
        self.Doc.recompute()

        base.BaseFeature = rotated
        self.Doc.recompute()

        self.assertAlmostEqual(clone_body.Shape.BoundBox.XMin, 20)
        self.assertAlmostEqual(clone_body.Shape.BoundBox.XMax, 22)
        self.assertAlmostEqual(clone_body.Shape.BoundBox.YMin, 0)
        self.assertAlmostEqual(clone_body.Shape.BoundBox.YMax, 10)

    def tearDown(self):
        if hasattr(App, "KeepTestDoc") and App.KeepTestDoc:
            return
        FreeCAD.closeDocument("PartDesignTestBaseFeature")
