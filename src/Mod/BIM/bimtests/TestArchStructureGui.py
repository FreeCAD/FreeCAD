# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Furgo
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""GUI tests for the interactive Structure command's preview tracker.

These tests drive _CommandStructure.update() itself (unlike TestArchStructure's
placement-config tests, which recompute the anchor independently), so they
exercise the same code path that produced two real preview/placement mismatches
found through manual testing: a precast beam's preview was offset by half its
cross-section, and a column's preview was centered on the cursor instead of
based there.
"""

import FreeCAD as App
import FreeCADGui
from FreeCAD import Vector
import WorkingPlane
from draftguitools import gui_trackers as DraftTrackers
from ArchStructure import _CommandStructure, StructureMode, ShapeKind, InsertionPointCycler
from bimtests import TestArchBaseGui


class _FakeControl:
    """Stands in for FreeCADGui.Control so update()'s activeDialog() guard
    passes. FreeCADGui.Control is a C++ singleton that does not support being
    patched in place (mock.patch's introspection of it raises), so the whole
    module-level name is swapped out instead."""

    def activeDialog(self):
        return True


class _FakeLengthField:
    """Stands in for the taskbox's vLength widget, which update() writes the
    computed beam length to."""

    def blockSignals(self, blocked):
        pass

    def setText(self, text):
        pass


class TestArchStructureGui(TestArchBaseGui.TestArchBaseGui):

    def setUp(self):
        super().setUp()
        self._real_control = FreeCADGui.Control
        FreeCADGui.Control = _FakeControl()

    def tearDown(self):
        FreeCADGui.Control = self._real_control
        super().tearDown()

    def _make_command(self, mode, shape_kind, width, height, length):
        cmd = _CommandStructure()
        cmd.mode = mode
        cmd.shape_kind = shape_kind
        cmd.Width = width
        cmd.Height = height
        cmd.Length = length
        cmd.precastvalues = None
        cmd.bpoint = None
        wp = WorkingPlane.get_working_plane()
        wp.align_to_rotation(App.Rotation())
        cmd.wp = wp
        cmd.tracker = DraftTrackers.boxTracker()
        cmd.tracker.width(width)
        cmd.tracker.height(height)
        cmd.tracker.length(length)
        cmd.tracker.on()
        cmd.cycler = InsertionPointCycler(on_cycle=lambda: None)
        cmd.vLength = _FakeLengthField()
        return cmd

    def _tracker_position(self, cmd):
        pos = cmd.tracker.trans.translation.getValue()
        return Vector(pos[0], pos[1], pos[2])

    def test_precast_beam_preview_centers_on_cross_section(self):
        """A precast beam's preview must be centered on the picked line, not
        shifted by half its cross-section (regression for a real preview bug
        found by manual testing: the preview rendered top/corner-aligned)."""
        self.printTestMessage("Precast beam preview centers on cross-section")
        cmd = self._make_command(StructureMode.BEAM, ShapeKind.PRECAST, 970.0, 300.0, 1000.0)

        first_point = Vector(1000, 2000, 500)
        second_point = Vector(2000, 2000, 500)
        cmd.bpoint = first_point
        cmd.update(second_point, None)

        expected_center = first_point.add(second_point).multiply(0.5)
        self.assertTrue(
            self._tracker_position(cmd).isEqual(expected_center, 1e-6),
            f"Tracker at {self._tracker_position(cmd)}, expected {expected_center}",
        )

    def test_precast_beam_preview_cycle_round_trip(self):
        """Cycling through every insertion point and back to index 0 must
        return the preview to its starting position (regression for a real
        cycling-drift bug found by manual testing)."""
        self.printTestMessage("Precast beam preview cycle round trip")
        cmd = self._make_command(StructureMode.BEAM, ShapeKind.PRECAST, 970.0, 300.0, 1000.0)

        first_point = Vector(1000, 2000, 500)
        second_point = Vector(2000, 2000, 500)
        cmd.bpoint = first_point
        cmd.update(second_point, None)
        start_position = self._tracker_position(cmd)

        for _ in range(9):
            cmd.cycler.cycle()
            cmd.update(second_point, None)
        # The index itself grows unbounded; only insertion_point_offset wraps it
        # modulo len(POINT_CANDIDATES) (9), so index 9 selects the same candidate as 0.
        self.assertEqual(cmd.cycler.index, 9)

        self.assertTrue(
            self._tracker_position(cmd).isEqual(start_position, 1e-6),
            f"Tracker at {self._tracker_position(cmd)} after a full cycle, "
            f"expected to return to {start_position}",
        )

    def test_column_preview_is_based_at_click_point(self):
        """A column's preview must sit with its base at the picked point, like
        the column it creates, not centered on it vertically (regression for
        a real preview bug found by manual testing: the preview showed the
        cursor at the column's center of mass instead of its base)."""
        self.printTestMessage("Column preview is based at click point")
        cmd = self._make_command(StructureMode.COLUMN, ShapeKind.PLAIN, 200.0, 300.0, 3000.0)

        click_point = Vector(1000, 2000, 0)
        cmd.update(click_point, None)

        expected_center = Vector(click_point.x, click_point.y, click_point.z + cmd.Height / 2)
        self.assertTrue(
            self._tracker_position(cmd).isEqual(expected_center, 1e-6),
            f"Tracker at {self._tracker_position(cmd)}, expected {expected_center}",
        )
