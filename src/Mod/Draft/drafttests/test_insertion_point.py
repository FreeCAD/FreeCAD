# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Furgo
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

"""Unit tests for the Draft Workbench, gui_insertion_point module tests."""

import FreeCAD as App
from FreeCAD import Vector
from drafttests import test_base
from draftguitools.gui_insertion_point import (
    POINT_CANDIDATES,
    InsertionPointCycler,
    insertion_point_offset,
)


class DraftInsertionPoint(test_base.DraftTestCaseNoDoc):
    """Testing the functions and classes in the file gui_insertion_point.py"""

    def test_insertion_point_offset_center(self):
        """Index 0 (Center), default axes and center: anchor is the origin."""
        rotation = App.Rotation()
        anchor = insertion_point_offset(200, 300, 0, rotation)
        self.assertTrue(anchor.isEqual(Vector(0, 0, 0), 1e-6), f"Expected (0, 0, 0), got {anchor}")

    def test_insertion_point_offset_corner(self):
        """Index 1 (Bottom-Left), default axes: anchor at (-W/2, -H/2, 0)."""
        rotation = App.Rotation()
        anchor = insertion_point_offset(200, 300, 1, rotation)
        self.assertTrue(
            anchor.isEqual(Vector(-100, -150, 0), 1e-6),
            f"Expected (-100, -150, 0), got {anchor}",
        )

    def test_insertion_point_offset_wraps_around(self):
        """Index 9 wraps to index 0 (Center)."""
        rotation = App.Rotation()
        anchor_9 = insertion_point_offset(200, 300, 9, rotation)
        anchor_0 = insertion_point_offset(200, 300, 0, rotation)
        self.assertTrue(anchor_9.isEqual(anchor_0, 1e-6), "Index 9 should wrap to 0")

    def test_insertion_point_offset_custom_axes(self):
        """A non-default cross-section plane (e.g. local YZ) is honored."""
        rotation = App.Rotation()
        anchor = insertion_point_offset(
            200, 300, 1, rotation, width_axis=Vector(0, 1, 0), height_axis=Vector(0, 0, 1)
        )
        self.assertTrue(
            anchor.isEqual(Vector(0, -100, -150), 1e-6),
            f"Expected (0, -100, -150), got {anchor}",
        )

    def test_insertion_point_offset_shape_origin(self):
        """A shape_origin shift is subtracted from the chosen candidate."""
        rotation = App.Rotation()
        origin = Vector(-100, -150, 0)
        anchor = insertion_point_offset(200, 300, 0, rotation, shape_origin=origin)
        self.assertTrue(
            anchor.isEqual(Vector(100, 150, 0), 1e-6), f"Expected (100, 150, 0), got {anchor}"
        )

    def test_insertion_point_offset_matches_rotation(self):
        """The anchor is always derivable from *rotation* alone: rotating the
        candidate table by any rotation, and rotating the result by the same
        rotation, must agree.
        """
        rotation = App.Rotation(Vector(0, 0, 1), 37)
        for index in range(len(POINT_CANDIDATES)):
            anchor = insertion_point_offset(200, 300, index, rotation)
            u, v = POINT_CANDIDATES[index]
            expected = rotation.multVec(Vector(u * 100, v * 150, 0))
            self.assertTrue(
                anchor.isEqual(expected, 1e-6), f"Index {index}: expected {expected}, got {anchor}"
            )

    def test_cycler_advances_index(self):
        """cycle() advances the index and calls the on_cycle callback."""
        calls = []
        cycler = InsertionPointCycler(on_cycle=lambda: calls.append(cycler.index))
        cycler.cycle()
        cycler.cycle()
        self.assertEqual(cycler.index, 2)
        self.assertEqual(calls, [1, 2])

    def test_cycler_reverses_index(self):
        """cycle(reverse=True) decrements the index."""
        cycler = InsertionPointCycler(on_cycle=lambda: None)
        cycler.cycle()
        cycler.cycle(reverse=True)
        self.assertEqual(cycler.index, 0)
