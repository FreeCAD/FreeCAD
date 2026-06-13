# SPDX-License-Identifier: LGPL-2.1-or-later
# Tests for Part boolean feature classes (Fuse, Common, Cut, MultiFuse, MultiCommon).
# Exercises the feature-level API (document objects with properties) rather than
# the raw TopoShape API, so class registration, property inheritance, and
# execute() are all exercised.

import unittest
import FreeCAD
import Part


def _make_boxes(doc):
    """Return two overlapping box features ready to use as boolean inputs."""
    b1 = doc.addObject("Part::Box", "Box1")
    b1.Length = b1.Width = b1.Height = 10.0

    b2 = doc.addObject("Part::Box", "Box2")
    b2.Length = b2.Width = b2.Height = 10.0
    b2.Placement = FreeCAD.Placement(FreeCAD.Vector(5, 0, 0), FreeCAD.Rotation())
    doc.recompute()
    return b1, b2


class TestBooleanFeatures(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("BooleanFeatureTest")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    # ------------------------------------------------------------------
    # Registration / property inheritance
    # ------------------------------------------------------------------

    def test_refinable_feature_registered(self):
        """RefinableFeature must be initialised so property inheritance works."""
        self.assertFalse(
            FreeCAD.Base.TypeId.fromName("Part::RefinableFeature").isBad(),
            "Part::RefinableFeature is not registered — missing init() call in AppPart.cpp",
        )

    def test_refine_and_check_refine_on_fuse(self):
        b1, b2 = _make_boxes(self.doc)
        fuse = self.doc.addObject("Part::Fuse", "Fuse")
        fuse.Base = b1
        fuse.Tool = b2
        self.assertTrue(hasattr(fuse, "Refine"), "Part::Fuse missing Refine property")
        self.assertTrue(hasattr(fuse, "CheckRefine"), "Part::Fuse missing CheckRefine property")

    def test_refine_and_check_refine_on_common(self):
        b1, b2 = _make_boxes(self.doc)
        common = self.doc.addObject("Part::Common", "Common")
        common.Base = b1
        common.Tool = b2
        self.assertTrue(hasattr(common, "Refine"), "Part::Common missing Refine property")
        self.assertTrue(hasattr(common, "CheckRefine"), "Part::Common missing CheckRefine property")

    def test_refine_and_check_refine_on_multifuse(self):
        b1, b2 = _make_boxes(self.doc)
        mf = self.doc.addObject("Part::MultiFuse", "MultiFuse")
        mf.Shapes = [b1, b2]
        self.assertTrue(hasattr(mf, "Refine"), "Part::MultiFuse missing Refine property")
        self.assertTrue(hasattr(mf, "CheckRefine"), "Part::MultiFuse missing CheckRefine property")

    def test_refine_and_check_refine_on_multicommon(self):
        b1, b2 = _make_boxes(self.doc)
        mc = self.doc.addObject("Part::MultiCommon", "MultiCommon")
        mc.Shapes = [b1, b2]
        self.assertTrue(hasattr(mc, "Refine"), "Part::MultiCommon missing Refine property")
        self.assertTrue(
            hasattr(mc, "CheckRefine"), "Part::MultiCommon missing CheckRefine property"
        )

    # ------------------------------------------------------------------
    # Basic execute() smoke tests
    # ------------------------------------------------------------------

    def test_fuse_two_boxes(self):
        b1, b2 = _make_boxes(self.doc)
        fuse = self.doc.addObject("Part::Fuse", "Fuse")
        fuse.Base = b1
        fuse.Tool = b2
        self.doc.recompute()
        shape = fuse.Shape
        self.assertFalse(shape.isNull(), "Part::Fuse result is null")
        self.assertGreater(shape.Volume, 0, "Part::Fuse result has no volume")

    def test_common_two_boxes(self):
        b1, b2 = _make_boxes(self.doc)
        common = self.doc.addObject("Part::Common", "Common")
        common.Base = b1
        common.Tool = b2
        self.doc.recompute()
        shape = common.Shape
        self.assertFalse(shape.isNull(), "Part::Common result is null")
        self.assertGreater(shape.Volume, 0, "Part::Common result has no volume")

    def test_cut_two_boxes(self):
        b1, b2 = _make_boxes(self.doc)
        cut = self.doc.addObject("Part::Cut", "Cut")
        cut.Base = b1
        cut.Tool = b2
        self.doc.recompute()
        shape = cut.Shape
        self.assertFalse(shape.isNull(), "Part::Cut result is null")
        self.assertGreater(shape.Volume, 0, "Part::Cut result has no volume")

    def test_multifuse_two_boxes(self):
        b1, b2 = _make_boxes(self.doc)
        mf = self.doc.addObject("Part::MultiFuse", "MultiFuse")
        mf.Shapes = [b1, b2]
        self.doc.recompute()
        shape = mf.Shape
        self.assertFalse(shape.isNull(), "Part::MultiFuse result is null")
        self.assertGreater(shape.Volume, 0, "Part::MultiFuse result has no volume")

    def test_multicommon_two_boxes(self):
        b1, b2 = _make_boxes(self.doc)
        mc = self.doc.addObject("Part::MultiCommon", "MultiCommon")
        mc.Shapes = [b1, b2]
        self.doc.recompute()
        shape = mc.Shape
        self.assertFalse(shape.isNull(), "Part::MultiCommon result is null")
        self.assertGreater(shape.Volume, 0, "Part::MultiCommon result has no volume")

    def test_fuse_with_refine(self):
        b1, b2 = _make_boxes(self.doc)
        fuse = self.doc.addObject("Part::Fuse", "Fuse")
        fuse.Base = b1
        fuse.Tool = b2
        fuse.Refine = True
        self.doc.recompute()
        shape = fuse.Shape
        self.assertFalse(shape.isNull(), "Part::Fuse with Refine=True result is null")
        self.assertGreater(shape.Volume, 0, "Part::Fuse with Refine=True result has no volume")
