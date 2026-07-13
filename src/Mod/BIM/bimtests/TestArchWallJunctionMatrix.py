# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
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

"""Matrix-style app tests for BIM wall junction relations."""

import Arch
import Draft
import FreeCAD as App
import Part

from bimtests import TestArchBase


class TestArchWallJunctionMatrix(TestArchBase.TestArchBase):
    def _make_baseless_wall_between(self, p1, p2, width=200.0, height=1500.0):
        line_vector = p2.sub(p1)
        wall = Arch.makeWall(length=line_vector.Length, width=width, height=height)
        wall.Placement = App.Placement(
            (p1 + p2) * 0.5,
            App.Rotation(App.Vector(1, 0, 0), line_vector.normalize()),
        )
        self.document.recompute()
        return wall

    def _make_line_based_wall_between(self, p1, p2, width=200.0, height=1500.0):
        line = Draft.makeLine(p1, p2)
        self.document.recompute()
        wall = Arch.makeWall(line, width=width, height=height)
        self.document.recompute()
        return wall

    def _make_sketch_based_wall_between(self, p1, p2, width=200.0, height=1500.0):
        sketch = self.document.addObject("Sketcher::SketchObject", "WallSketch")
        sketch.addGeometry(Part.LineSegment(p1, p2))
        wall = Arch.makeWall(sketch, width=width, height=height)
        self.document.recompute()
        return wall

    def _make_wall_between(self, baseline_kind, p1, p2, width=200.0, height=1500.0):
        if baseline_kind == "baseless":
            return self._make_baseless_wall_between(p1, p2, width=width, height=height)
        if baseline_kind == "line":
            return self._make_line_based_wall_between(p1, p2, width=width, height=height)
        if baseline_kind == "sketch":
            return self._make_sketch_based_wall_between(p1, p2, width=width, height=height)
        raise ValueError(f"Unsupported baseline kind in wall junction matrix: {baseline_kind}")

    @staticmethod
    def _is_identity_placement(placement, tol=1e-9):
        return placement.Base.Length < tol and placement.Rotation.Angle < tol

    def _assert_manual_endings_clear(self, *walls):
        for wall in walls:
            self.assertTrue(
                self._is_identity_placement(wall.EndingStart)
                and self._is_identity_placement(wall.EndingEnd),
                f"Manual endings should stay untouched for {wall.Label} in the junction matrix.",
            )

    def _assert_no_hidden_wall_joints(self):
        self.assertFalse(
            any(
                getattr(getattr(obj, "Proxy", None), "Type", None) == "WallJoint"
                for obj in self.document.Objects
            ),
            "Wall junctions should solve trims directly without synthesizing hidden wall joints.",
        )

    def _remove_created_objects(self, existing_names):
        for obj in reversed(list(self.document.Objects)):
            if obj.Name not in existing_names:
                self.document.removeObject(obj.Name)
        self.document.recompute()

    def _make_junction_geometry(self, angle_kind, branch_count):
        if angle_kind == "orthogonal":
            carrier_points = (App.Vector(-1000, 0, 0), App.Vector(1000, 0, 0))
        else:
            carrier_points = (App.Vector(-1000, -400, 0), App.Vector(1000, 400, 0))

        branch_points = [
            (App.Vector(0, 0, 0), App.Vector(0, 1000, 0)),
            (App.Vector(0, 0, 0), App.Vector(0, -1000, 0)),
            (App.Vector(0, 0, 0), App.Vector(700, 700, 0)),
        ]
        return carrier_points, branch_points[:branch_count]

    def test_wall_junction_matrix_supported_configurations(self):
        self.printTestMessage("Testing matrix coverage for supported wall junctions...")

        cases = (
            ("baseless", "orthogonal", 2, "Center"),
            ("line", "oblique", 2, "Left"),
            ("sketch", "orthogonal", 3, "Right"),
            ("baseless", "oblique", 3, "Left"),
            ("line", "orthogonal", 3, "Center"),
            ("sketch", "oblique", 2, "Right"),
        )
        branch_alignments = ("Center", "Left", "Right")
        for baseline_kind, angle_kind, branch_count, carrier_align in cases:
            with self.subTest(
                baseline=baseline_kind,
                angle=angle_kind,
                branch_count=branch_count,
                carrier_align=carrier_align,
            ):
                existing_names = {obj.Name for obj in self.document.Objects}
                carrier_points, branch_point_pairs = self._make_junction_geometry(
                    angle_kind, branch_count
                )
                carrier_wall = self._make_wall_between(
                    baseline_kind,
                    carrier_points[0],
                    carrier_points[1],
                    width=200,
                )
                carrier_wall.Align = carrier_align
                branch_walls = []
                branch_volumes = {}
                for index, (start, end) in enumerate(branch_point_pairs):
                    branch_wall = self._make_wall_between(
                        baseline_kind,
                        start,
                        end,
                        width=200 + 20 * index,
                    )
                    branch_wall.Align = branch_alignments[index % len(branch_alignments)]
                    branch_walls.append(branch_wall)
                    branch_volumes[branch_wall.Name] = branch_wall.Shape.Volume
                self.document.recompute()
                carrier_volume = carrier_wall.Shape.Volume

                junction = Arch.makeWallJunction(
                    [carrier_wall] + branch_walls,
                    carrier_wall=carrier_wall,
                )
                self.document.recompute()

                self.assertEqual(junction.Status, "OK")
                self.assertEqual(junction.ResolvedCarrierWall, carrier_wall)
                self.assertEqual(
                    {wall.Name for wall in junction.ResolvedBranchWalls},
                    {wall.Name for wall in branch_walls},
                )
                self.assertAlmostEqual(junction.Intersection.x, 0.0, delta=1e-6)
                self.assertAlmostEqual(junction.Intersection.y, 0.0, delta=1e-6)
                self.assertAlmostEqual(
                    carrier_wall.Shape.Volume,
                    carrier_volume,
                    delta=1e-6,
                )
                for branch_wall in branch_walls:
                    self.assertLess(
                        branch_wall.Shape.Volume,
                        branch_volumes[branch_wall.Name],
                        "Branch walls should be trimmed by supported wall junctions.",
                    )

                self._assert_manual_endings_clear(carrier_wall, *branch_walls)
                self._assert_no_hidden_wall_joints()
                self._remove_created_objects(existing_names)

    def test_wall_junction_matrix_auto_carrier_is_order_independent(self):
        self.printTestMessage(
            "Testing auto-carrier wall junction resolution is order-independent..."
        )

        for baseline_kind in ("baseless",):
            with self.subTest(baseline=baseline_kind):
                existing_names = {obj.Name for obj in self.document.Objects}
                carrier_wall = self._make_wall_between(
                    baseline_kind,
                    App.Vector(-1000, 0, 0),
                    App.Vector(1000, 0, 0),
                )
                branch_up = self._make_wall_between(
                    baseline_kind,
                    App.Vector(0, 0, 0),
                    App.Vector(0, 1000, 0),
                )
                branch_down = self._make_wall_between(
                    baseline_kind,
                    App.Vector(0, 0, 0),
                    App.Vector(0, -1000, 0),
                )
                branch_diag = self._make_wall_between(
                    baseline_kind,
                    App.Vector(0, 0, 0),
                    App.Vector(700, 700, 0),
                )

                junction = Arch.makeWallJunction(
                    [branch_diag, branch_up, carrier_wall, branch_down]
                )
                self.document.recompute()

                self.assertEqual(junction.Status, "OK")
                self.assertEqual(junction.ResolvedCarrierWall, carrier_wall)
                self.assertEqual(
                    {wall.Name for wall in junction.ResolvedBranchWalls},
                    {branch_up.Name, branch_down.Name, branch_diag.Name},
                )
                self._assert_no_hidden_wall_joints()
                self._remove_created_objects(existing_names)
