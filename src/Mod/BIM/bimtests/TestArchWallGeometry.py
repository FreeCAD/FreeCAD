# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
# SPDX-FileNotice: Part of the FreeCAD project.
################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public                       #
#   License as published by the FreeCAD Project Association, either version 2 #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of              #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             #
#   GNU Lesser General Public License for more details.                        #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Direct tests for the wall path and section value objects."""

import ArchWallPath
import ArchWallSection
import FreeCAD as App
import Part

from bimtests import TestArchBase


class TestArchWallGeometry(TestArchBase.TestArchBase):
    """Exercise the standalone wall path and section geometry contracts."""

    def test_wall_path_operations_use_finite_segments(self):
        """Path queries use ordered finite endpoints and global geometry."""
        path = ArchWallPath.WallPath(
            Part.makeLine(App.Vector(0, 0, 0), App.Vector(1000, 0, 0)),
            App.Vector(0, 0, 1),
        )
        self.assertTrue(path.contains_point(App.Vector(1000.00005, 0, 0)))
        self.assertFalse(path.contains_point(App.Vector(1000.2, 0, 0)))
        self.assertFalse(path.contains_point(App.Vector(500, 0.2, 0)))
        self.assertEqual(path.nearest_end_name(App.Vector(500, 0, 0)), "End")
        self.assertTrue(path.lateral_direction().isEqual(App.Vector(0, -1, 0), 1e-6))

        crossing = ArchWallPath.WallPath(
            Part.makeLine(App.Vector(500, -500, 0), App.Vector(500, 500, 0)),
            App.Vector(0, 0, 1),
        )
        point, end_a, end_b = ArchWallPath.find_path_intersection(path, crossing)
        self.assertTrue(point.isEqual(App.Vector(500, 0, 0), 1e-6))
        self.assertEqual((end_a, end_b), ("End", "End"))

    def test_wall_path_consumes_immutable_oriented_baseline(self):
        """WallPath uses explicit endpoint orientation from WallBaseline."""
        baseline = ArchWallPath.WallBaseline(
            Part.makeLine(App.Vector(1000, 0, 0), App.Vector(0, 0, 0)),
            App.Vector(0, 0, 1),
            App.Vector(1000, 0, 0),
            App.Vector(0, 0, 0),
        )
        path = ArchWallPath.WallPath.from_baseline(baseline)
        self.assertTrue(path.start_point.isEqual(baseline.start_point, 1e-6))
        self.assertTrue(path.end_point.isEqual(baseline.end_point, 1e-6))
        with self.assertRaises(AttributeError):
            baseline.start_point = App.Vector()

    def test_wall_path_rejects_invalid_values(self):
        """WallPath rejects non-geometry, degenerate, and parallel inputs."""
        with self.assertRaises(TypeError):
            ArchWallPath.WallPath(None, App.Vector(0, 0, 1))
        with self.assertRaises((ValueError, Part.OCCError)):
            ArchWallPath.WallPath(
                Part.makeLine(App.Vector(0, 0, 0), App.Vector(0, 0, 0)),
                App.Vector(0, 0, 1),
            )
        with self.assertRaises(ValueError):
            ArchWallPath.WallPath(
                Part.makeLine(App.Vector(0, 0, 0), App.Vector(1000, 0, 0)),
                App.Vector(1, 0, 0),
            )

    def test_wall_section_is_immutable_resolved_data(self):
        """WallSection exposes resolved extents without mutable state."""
        section = ArchWallSection.WallSection(
            (
                ArchWallSection.WallSectionLayer(100, -150, -50),
                ArchWallSection.WallSectionLayer(-50, -50, 0),
                ArchWallSection.WallSectionLayer(200, 0, 200),
            )
        )

        self.assertEqual(section.y_min, -150)
        self.assertEqual(section.y_max, 200)
        self.assertEqual(len(section.visible_layers), 2)
        self.assertEqual(section.offset_towards(App.Vector(1, 0, 0), App.Vector(1, 0, 0)), 150)
        self.assertEqual(section.offset_towards(App.Vector(1, 0, 0), App.Vector(-1, 0, 0)), -200)
        with self.assertRaises(AttributeError):
            section.layers = ()
