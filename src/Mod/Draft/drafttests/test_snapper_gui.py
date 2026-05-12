# SPDX-License-Identifier: LGPL-2.1-or-later

"""Unit tests for Draft snapping GUI behavior."""

import FreeCAD as App
import WorkingPlane

from draftguitools import gui_snapper
from drafttests import test_base


class DraftGuiSnapper(test_base.DraftTestCaseNoDoc):
    """Test Draft Snapper behavior that does not require mouse automation."""

    def make_hold_snapper(self):
        snapper = gui_snapper.Snapper()
        snapper.radius = 10
        snapper.holdPoints = [App.Vector(0, 0, 0)]
        snapper._get_wp = lambda: WorkingPlane.PlaneBase()
        return snapper

    def test_snap_to_hold_sets_y_affinity_for_vertical_extension(self):
        snapper = self.make_hold_snapper()

        snap = snapper.snapToHold(App.Vector(0.1, 5, 0), constrain=True)

        self.assertIsNotNone(snap)
        self.assertEqual("extension", snap[1])
        self.assertEqual("y", snapper.affinity)
        self.assertAlmostEqual(0, snap[2].x)
        self.assertAlmostEqual(5, snap[2].y)
        self.assertAlmostEqual(0, snap[2].z)

    def test_snap_to_hold_keeps_x_affinity_for_horizontal_extension(self):
        snapper = self.make_hold_snapper()

        snap = snapper.snapToHold(App.Vector(5, 0.1, 0), constrain=True)

        self.assertIsNotNone(snap)
        self.assertEqual("extension", snap[1])
        self.assertEqual("x", snapper.affinity)
        self.assertAlmostEqual(5, snap[2].x)
        self.assertAlmostEqual(0, snap[2].y)
        self.assertAlmostEqual(0, snap[2].z)
