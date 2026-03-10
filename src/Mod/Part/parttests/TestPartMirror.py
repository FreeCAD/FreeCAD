"""
this test will FAIL on current main branch (with PR #26963) and PASS after the fix.
current main at: https://github.com/FreeCAD/FreeCAD/tree/24f0c8e2c321bb202410feae84eb58779ba8e8d2
"""

import unittest
import FreeCAD as App
import Part
import Draft


class TestPartMirroringRegression(unittest.TestCase):
    """Regression test for GitHub issue #27365.

    Part::Mirroring with Draft Clone produces incorrect results after PR #26963.
    The mirror position shifts on recompute when the source has non-identity Placement.
    """

    def setUp(self):
        self.doc = App.newDocument("TestMirrorRegression")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def testMirroringWithDraftCloneStability(self):
        """Test that Part::Mirroring position is stable across recomputes.

        This test reproduces issue #27365: mirroring a Draft Clone that has
        placement, and causes the mirror position to be incorrect and/or shift
        on recompute.

        This test FAILS on current main (after PR #26963) and should pass after a fix is applied.
        """
        # create sketch at origin
        sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        sketch.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0)), False)
        sketch.addGeometry(Part.LineSegment(App.Vector(10, 0, 0), App.Vector(10, 10, 0)), False)
        sketch.addGeometry(Part.LineSegment(App.Vector(10, 10, 0), App.Vector(0, 10, 0)), False)
        sketch.addGeometry(Part.LineSegment(App.Vector(0, 10, 0), App.Vector(0, 0, 0)), False)
        self.doc.recompute()

        # create draft clone at X=30 with 90° rotation around Z
        clone = Draft.make_clone(sketch)
        clone.Placement = App.Placement(App.Vector(30, 0, 0), App.Rotation(App.Vector(0, 0, 1), 90))
        self.doc.recompute()

        # verify clone is positioned correctly
        clone_bbox = clone.Shape.BoundBox
        clone_center_x = (clone_bbox.XMin + clone_bbox.XMax) / 2
        self.assertAlmostEqual(
            clone_center_x,
            30.0,
            delta=5.0,
            msg=f"clone should be centered around X=30, but is at X={clone_center_x:.2f}",
        )

        # mirror across YZ plane (X=0, normal in +X direction)
        mirror = self.doc.addObject("Part::Mirroring", "Mirror")
        mirror.Source = clone
        mirror.Base = App.Vector(0, 0, 0)
        mirror.Normal = App.Vector(1, 0, 0)
        self.doc.recompute()

        # get initial mirror position
        initial_bbox = mirror.Shape.BoundBox
        initial_center_x = (initial_bbox.XMin + initial_bbox.XMax) / 2

        # mirror should be on opposite side of plane from clone
        # clone is at X≈30, plane at X=0, so mirror should be at X≈-30
        self.assertLess(
            initial_center_x,
            -20.0,
            msg=f"mirror should be at X≈-30 (opposite side of X=0 from clone at X≈30), "
            f"but is at X={initial_center_x:.2f}",
        )

        # position must be stable across recompute
        # this is the core bug, ie. position shifts on recompute
        for i in range(5):
            self.doc.recompute()

        final_bbox = mirror.Shape.BoundBox
        final_center_x = (final_bbox.XMin + final_bbox.XMax) / 2

        # position should not change (within tolerance)
        self.assertAlmostEqual(
            initial_center_x,
            final_center_x,
            places=3,
            msg=f"mirror position shifted on recompute: "
            f"X={initial_center_x:.6f} -> X={final_center_x:.6f}",
        )


# for standalone execution
if __name__ == "__main__":
    suite = unittest.TestSuite()
    suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestPartMirroringRegression))
    runner = unittest.TextTestRunner()
    runner.run(suite)
