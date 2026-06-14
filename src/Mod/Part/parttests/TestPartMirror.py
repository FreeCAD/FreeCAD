# SPDX-License-Identifier: LGPL-2.1-or-later
# --------------------------------------------------------------------------
#                                                                          *
#    Copyright (c) 2026 Chris Jones github.com/ipatch                      *
#                                                                          *
#    This file is part of FreeCAD.                                         *
#                                                                          *
#    FreeCAD is free software: you can redistribute it and/or modify it    *
#    under the terms of the GNU Lesser General Public License as           *
#    published by the Free Software Foundation, either version 2.1 of the  *
#    License, or (at your option) any later version.                       *
#                                                                          *
#    FreeCAD is distributed in the hope that it will be useful, but        *
#    WITHOUT ANY WARRANTY; without even the implied warranty of            *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
#    Lesser General Public License for more details.                       *
#                                                                          *
#    You should have received a copy of the GNU Lesser General Public      *
#    License along with FreeCAD. If not, see                               *
#    <https://www.gnu.org/licenses/>.                                      *
#                                                                          *
# --------------------------------------------------------------------------

"""
this test will FAIL on current main branch, and will PASS with PR #26963 ie. the fix.
current main at: https://github.com/FreeCAD/FreeCAD/tree/24f0c8e2c321bb202410feae84eb58779ba8e8d2

add test to validate App:Link placement
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

    def testMirroringWithAppLinkPlacement(self):
        """Test that Part::Mirroring respects App::Link placement.

        App::Link provides its Placement via extension rather than GeoFeature
        inheritance, so mirror code must not rely on isDerivedFrom(GeoFeature).
        """
        # create a simple body with a box
        body = self.doc.addObject("PartDesign::Body", "Body")
        box = self.doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        body.addObject(box)
        self.doc.recompute()

        # body shape should be at origin
        body_bbox = body.Shape.BoundBox
        self.assertAlmostEqual(body_bbox.XMin, 0.0, places=2)
        self.assertAlmostEqual(body_bbox.XMax, 10.0, places=2)

        # create a link to the body and move it to X=20
        link = self.doc.addObject("App::Link", "Link")
        link.LinkedObject = body
        link.Placement = App.Placement(
            App.Vector(20, 0, 0),
            App.Rotation(),
        )
        self.doc.recompute()

        # mirror across YZ plane (X=0)
        mirror = self.doc.addObject("Part::Mirroring", "Mirror")
        mirror.Source = link
        mirror.Base = App.Vector(0, 0, 0)
        mirror.Normal = App.Vector(1, 0, 0)
        self.doc.recompute()

        # link is at X=[20,30], so mirror across X=0 should be at X=[-30,-20]
        mirror_bbox = mirror.Shape.BoundBox
        mirror_center_x = (mirror_bbox.XMin + mirror_bbox.XMax) / 2

        self.assertAlmostEqual(
            mirror_center_x,
            -25.0,
            delta=2.0,
            msg=f"mirror should be centered at X=-25 (opposite of link at X=25), "
            f"but is at X={mirror_center_x:.2f}",
        )

        # verify stability across recomputes
        initial_center_x = mirror_center_x
        for i in range(5):
            self.doc.recompute()

        final_bbox = mirror.Shape.BoundBox
        final_center_x = (final_bbox.XMin + final_bbox.XMax) / 2

        self.assertAlmostEqual(
            initial_center_x,
            final_center_x,
            places=3,
            msg=f"mirror position shifted on recompute: "
            f"X={initial_center_x:.6f} -> X={final_center_x:.6f}",
        )

    def testMirroringWithAppLinkRotation(self):
        """Test that Part::Mirroring respects App::Link rotation + translation."""
        body = self.doc.addObject("PartDesign::Body", "Body")
        box = self.doc.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 10
        box.Width = 20
        box.Height = 5
        body.addObject(box)
        self.doc.recompute()

        # link with translation and 90° rotation around Z
        link = self.doc.addObject("App::Link", "Link")
        link.LinkedObject = body
        link.Placement = App.Placement(
            App.Vector(30, 0, 0),
            App.Rotation(App.Vector(0, 0, 1), 90),
        )
        self.doc.recompute()

        # capture the link's transformed bbox for reference
        link_bbox = link.Shape.BoundBox

        # mirror across YZ plane (X=0)
        mirror = self.doc.addObject("Part::Mirroring", "Mirror")
        mirror.Source = link
        mirror.Base = App.Vector(0, 0, 0)
        mirror.Normal = App.Vector(1, 0, 0)
        self.doc.recompute()

        mirror_bbox = mirror.Shape.BoundBox

        # mirroring across X=0 should negate X coordinates
        # link XMin/XMax should become -XMax/-XMin on the mirror
        self.assertAlmostEqual(
            mirror_bbox.XMin,
            -link_bbox.XMax,
            delta=1.0,
            msg=f"mirror XMin should be -{link_bbox.XMax:.2f}, got {mirror_bbox.XMin:.2f}",
        )
        self.assertAlmostEqual(
            mirror_bbox.XMax,
            -link_bbox.XMin,
            delta=1.0,
            msg=f"mirror XMax should be -{link_bbox.XMin:.2f}, got {mirror_bbox.XMax:.2f}",
        )

        # Y and Z extents should be unchanged by X-axis mirror
        self.assertAlmostEqual(mirror_bbox.YMin, link_bbox.YMin, delta=1.0)
        self.assertAlmostEqual(mirror_bbox.YMax, link_bbox.YMax, delta=1.0)
        self.assertAlmostEqual(mirror_bbox.ZMin, link_bbox.ZMin, delta=1.0)
        self.assertAlmostEqual(mirror_bbox.ZMax, link_bbox.ZMax, delta=1.0)


# for standalone execution
if __name__ == "__main__":
    suite = unittest.TestSuite()
    suite.addTest(unittest.TestLoader().loadTestsFromTestCase(TestPartMirroringRegression))
    runner = unittest.TextTestRunner()
    runner.run(suite)
