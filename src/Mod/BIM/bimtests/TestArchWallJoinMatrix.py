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

"""Matrix-style app tests for BIM wall join relations."""

from dataclasses import dataclass

import Arch
import Draft
import FreeCAD as App
import Part

from bimtests import TestArchBase


@dataclass(frozen=True)
class JoinMatrixCase:
    baseline_kind: str
    joint_type: str
    angle_kind: str
    wall_a_start: App.Vector
    wall_a_end: App.Vector
    wall_b_start: App.Vector
    wall_b_end: App.Vector
    expected_trimmed_a: bool
    expected_trimmed_b: bool
    tee_stem: str = "Auto"


class TestArchWallJoinMatrix(TestArchBase.TestArchBase):
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
        raise ValueError(f"Unsupported baseline kind in test matrix: {baseline_kind}")

    @staticmethod
    def _is_identity_placement(placement, tol=1e-9):
        return placement.Base.Length < tol and placement.Rotation.Angle < tol

    def _assert_wall_trim_state(self, wall, initial_volume, should_be_trimmed, msg):
        self.assertTrue(wall.Shape.isValid(), f"{wall.Label} became invalid in the join matrix.")
        if should_be_trimmed:
            self.assertLess(wall.Shape.Volume, initial_volume, msg)
        else:
            self.assertAlmostEqual(wall.Shape.Volume, initial_volume, delta=1e-6, msg=msg)

    def _assert_miter_walls_closed(self, wall_a, wall_b):
        self.assertAlmostEqual(
            wall_a.Shape.distToShape(wall_b.Shape)[0],
            0.0,
            delta=1e-6,
            msg="Mitered walls should touch in the join matrix.",
        )

    def _assert_manual_endings_clear(self, *walls):
        for wall in walls:
            self.assertTrue(
                self._is_identity_placement(wall.EndingStart)
                and self._is_identity_placement(wall.EndingEnd),
                f"Manual endings should stay untouched for {wall.Label} in the relation matrix.",
            )

    def _remove_created_objects(self, existing_names):
        for obj in reversed(list(self.document.Objects)):
            if obj.Name not in existing_names:
                self.document.removeObject(obj.Name)
        self.document.recompute()

    def _make_join_matrix_case(self, baseline_kind, joint_type, angle_kind):
        if joint_type == "Tee":
            if angle_kind == "orthogonal":
                return JoinMatrixCase(
                    baseline_kind=baseline_kind,
                    joint_type=joint_type,
                    angle_kind=angle_kind,
                    wall_a_start=App.Vector(0, 0, 0),
                    wall_a_end=App.Vector(0, 1000, 0),
                    wall_b_start=App.Vector(-1000, 0, 0),
                    wall_b_end=App.Vector(1000, 0, 0),
                    expected_trimmed_a=True,
                    expected_trimmed_b=False,
                    tee_stem="WallA",
                )
            return JoinMatrixCase(
                baseline_kind=baseline_kind,
                joint_type=joint_type,
                angle_kind=angle_kind,
                wall_a_start=App.Vector(0, 0, 0),
                wall_a_end=App.Vector(400, 400, 0),
                wall_b_start=App.Vector(-800, 400, 0),
                wall_b_end=App.Vector(1200, 400, 0),
                expected_trimmed_a=True,
                expected_trimmed_b=False,
                tee_stem="WallA",
            )

        if angle_kind == "orthogonal":
            return JoinMatrixCase(
                baseline_kind=baseline_kind,
                joint_type=joint_type,
                angle_kind=angle_kind,
                wall_a_start=App.Vector(-1000, 0, 0),
                wall_a_end=App.Vector(0, 0, 0),
                wall_b_start=App.Vector(0, 0, 0),
                wall_b_end=App.Vector(0, 1000, 0),
                expected_trimmed_a=True,
                expected_trimmed_b=True,
            )

        return JoinMatrixCase(
            baseline_kind=baseline_kind,
            joint_type=joint_type,
            angle_kind=angle_kind,
            wall_a_start=App.Vector(-1000, 0, 0),
            wall_a_end=App.Vector(0, 0, 0),
            wall_b_start=App.Vector(0, 0, 0),
            wall_b_end=App.Vector(800, 600, 0),
            expected_trimmed_a=True,
            expected_trimmed_b=True,
        )

    def test_wall_join_matrix_supported_baselines(self):
        self.printTestMessage("Testing matrix coverage for supported wall joins...")

        baseline_kinds = ("baseless", "line", "sketch")
        joint_types = ("Miter", "Butt", "Tee")
        angle_kinds = ("orthogonal", "oblique")

        for baseline_kind in baseline_kinds:
            for joint_type in joint_types:
                for angle_kind in angle_kinds:
                    case = self._make_join_matrix_case(baseline_kind, joint_type, angle_kind)
                    with self.subTest(
                        baseline=baseline_kind,
                        joint_type=joint_type,
                        angle=angle_kind,
                    ):
                        existing_names = {obj.Name for obj in self.document.Objects}
                        wall_a = self._make_wall_between(
                            case.baseline_kind,
                            case.wall_a_start,
                            case.wall_a_end,
                        )
                        wall_b = self._make_wall_between(
                            case.baseline_kind,
                            case.wall_b_start,
                            case.wall_b_end,
                        )
                        initial_volume_a = wall_a.Shape.Volume
                        initial_volume_b = wall_b.Shape.Volume

                        joint = Arch.makeWallJoint(wall_a, wall_b, case.joint_type)
                        if case.joint_type == "Butt":
                            joint.ButtTrimmed = "WallB"
                        elif case.joint_type == "Tee":
                            joint.TeeStem = case.tee_stem
                        self.document.recompute()

                        self.assertEqual(joint.Status, "OK")
                        self.assertEqual(joint.JointType, case.joint_type)
                        self._assert_manual_endings_clear(wall_a, wall_b)
                        if case.joint_type == "Miter":
                            self._assert_miter_walls_closed(wall_a, wall_b)
                        else:
                            self._assert_wall_trim_state(
                                wall_a,
                                initial_volume_a,
                                case.expected_trimmed_a,
                                "Wall A trim state did not match the join matrix expectation.",
                            )
                            self._assert_wall_trim_state(
                                wall_b,
                                initial_volume_b,
                                case.expected_trimmed_b,
                                "Wall B trim state did not match the join matrix expectation.",
                            )

                        if case.joint_type == "Tee":
                            self.assertNotEqual(joint.ResolvedEndA, "None")
                            self.assertEqual(joint.ResolvedEndB, "None")
                        else:
                            self.assertNotEqual(joint.ResolvedEndA, "None")
                            self.assertNotEqual(joint.ResolvedEndB, "None")

                        self._remove_created_objects(existing_names)
