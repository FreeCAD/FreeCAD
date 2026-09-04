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

"""App-level tests for BIM wall joint relations."""

import os
import tempfile

import Arch
import ArchWallGeometry
import ArchWallRelation
import ArchWallRelationResolver
import ArchWallTrimming
import Draft
import FreeCAD as App
import Part

from bimtests import TestArchBase


class TestArchWallJoint(TestArchBase.TestArchBase):
    def _make_baseless_wall_between(self, p1, p2, width=200.0, height=1500.0):
        line_vector = p2.sub(p1)
        wall = Arch.makeWall(length=line_vector.Length, width=width, height=height)
        wall.Placement = App.Placement(
            (p1 + p2) * 0.5,
            App.Rotation(App.Vector(1, 0, 0), line_vector.normalize()),
        )
        self.document.recompute()
        return wall

    @staticmethod
    def _is_identity_placement(placement, tol=1e-9):
        return placement.Base.Length < tol and placement.Rotation.Angle < tol

    @staticmethod
    def _path_endpoints(wall):
        path = ArchWallRelation.get_join_path(wall)
        return path.start_point, path.end_point

    def _assert_wall_trimmed(self, wall, initial_volume, msg):
        self.assertTrue(wall.Shape.isValid(), f"{wall.Label} became invalid after the joint.")
        self.assertLess(wall.Shape.Volume, initial_volume, msg)

    def _assert_wall_unchanged(self, wall, initial_volume, msg):
        self.assertTrue(wall.Shape.isValid(), f"{wall.Label} became invalid after the joint.")
        self.assertAlmostEqual(wall.Shape.Volume, initial_volume, delta=1e-6, msg=msg)

    def _assert_miter_walls_closed(self, wall1, wall2):
        self.assertTrue(wall1.Shape.isValid(), "First wall became invalid after the miter.")
        self.assertTrue(wall2.Shape.isValid(), "Second wall became invalid after the miter.")
        self.assertAlmostEqual(
            wall1.Shape.distToShape(wall2.Shape)[0],
            0.0,
            delta=1e-6,
            msg="Mitered walls should touch.",
        )

    def test_make_wall_joint_miter_trims_wall_shapes(self):
        self.printTestMessage("Testing makeWallJoint creates a usable miter relation...")

        wall1 = self._make_baseless_wall_between(App.Vector(-1000, 0, 0), App.Vector(0, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))

        joint = Arch.makeWallJoint(wall1, wall2, "Miter")
        self.document.recompute()

        self.assertIsNotNone(joint, "makeWallJoint failed to create a relation object.")
        self.assertEqual(joint.Status, "OK")
        self.assertEqual(joint.ResolvedEndA, "End")
        self.assertEqual(joint.ResolvedEndB, "Start")
        self._assert_miter_walls_closed(wall1, wall2)
        self.assertTrue(
            self._is_identity_placement(wall1.EndingStart)
            and self._is_identity_placement(wall1.EndingEnd),
            "Manual wall endings should stay untouched when a relation drives the trim.",
        )
        self.assertTrue(
            wall1.Proxy.requires_brep_export(wall1) and wall2.Proxy.requires_brep_export(wall2),
            "Relation-trimmed walls must export their processed shapes.",
        )
        self.assertFalse(
            wall1.Proxy.isStandardCase(wall1) or wall2.Proxy.isStandardCase(wall2),
            "Relation-trimmed walls must not export as IfcWallStandardCase.",
        )

    def test_miter_preserves_subtraction_geometry(self):
        """Miter construction extension must not refill an existing opening."""
        self.printTestMessage("Testing miter extension preserves wall subtractions...")

        wall1 = self._make_baseless_wall_between(
            App.Vector(-1000, 0, 0), App.Vector(0, 0, 0), height=1000
        )
        wall2 = self._make_baseless_wall_between(
            App.Vector(0, 0, 0), App.Vector(0, 1000, 0), height=1000
        )
        window_base = Draft.makeRectangle(length=200, height=400)
        window_base.Placement = App.Placement(
            App.Vector(-600, -100, 300),
            App.Rotation(App.Vector(1, 0, 0), 90),
        )
        window = Arch.makeWindow(window_base)
        window.Hosts = [wall1]
        self.document.recompute()
        volume_with_opening = wall1.Shape.Volume
        self.assertLess(volume_with_opening, 1000 * 200 * 1000)

        Arch.makeWallJoint(wall1, wall2, "Miter")
        self.document.recompute()

        self.assertLess(
            wall1.Shape.Volume,
            volume_with_opening,
            "The miter should trim the wall without refilling its opening.",
        )
        self.assertTrue(wall1.Shape.isValid())

    def test_cutting_plane_failure_preserves_current_solid(self):
        """A failed Part operation must return the solid built this recompute."""
        wall = Arch.makeWall(length=1000, width=200, height=1000)
        self.document.recompute()
        original_volume = wall.Shape.Volume
        original_creator = ArchWallTrimming._create_cutting_tool_from_plane

        def fail_cutting_tool(*_args, **_kwargs):
            raise Part.OCCError("forced cutting failure")

        ArchWallTrimming._create_cutting_tool_from_plane = fail_cutting_tool
        try:
            result = ArchWallTrimming.apply_cutting_plane(
                wall,
                wall.Shape,
                wall.Placement,
                App.Placement(),
                App.Vector(0, 0, 0),
                wall.Shape.BoundBox.DiagonalLength,
                is_global=True,
            )
        finally:
            ArchWallTrimming._create_cutting_tool_from_plane = original_creator

        self.assertAlmostEqual(result.Volume, original_volume, delta=1e-6)

    def test_wall_extension_stays_continuous_beyond_baseline_length(self):
        """Long end extensions must not create disconnected translated solids."""
        baseline = ArchWallGeometry.WallBaseline(
            Part.makeLine(App.Vector(0, 0, 0), App.Vector(50, 0, 0)),
            App.Vector(0, 0, 1),
            App.Vector(0, 0, 0),
            App.Vector(50, 0, 0),
        )
        solid = Part.makeBox(50, 20, 20)

        for end_name in ("Start", "End"):
            with self.subTest(end=end_name):
                extended = ArchWallTrimming.extend_solid_along_baseline(
                    solid,
                    baseline,
                    App.Placement(),
                    end_name,
                    150,
                )

                self.assertEqual(len(extended.Solids), 1)
                self.assertAlmostEqual(extended.Volume, 200 * 20 * 20, delta=1e-6)
                if end_name == "Start":
                    self.assertAlmostEqual(extended.BoundBox.XMin, -150, delta=1e-6)
                    self.assertAlmostEqual(extended.BoundBox.XMax, 50, delta=1e-6)
                else:
                    self.assertAlmostEqual(extended.BoundBox.XMin, 0, delta=1e-6)
                    self.assertAlmostEqual(extended.BoundBox.XMax, 200, delta=1e-6)

    def test_solve_wall_joint_inputs_returns_typed_solution(self):
        self.printTestMessage("Testing typed wall joint solver output...")

        wall1 = self._make_baseless_wall_between(App.Vector(-1000, 0, 0), App.Vector(0, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))

        solution = ArchWallRelation.solve_wall_joint_inputs(wall1, wall2, "Miter")

        self.assertIsInstance(solution, ArchWallRelation.WallJointSolution)
        self.assertTrue(solution.is_ok())
        self.assertEqual(solution.status, "OK")
        claim = solution.trim_for_wall(wall1)
        self.assertEqual(claim.end_name, "End")
        self.assertIsNotNone(claim.plane)
        self.assertGreater(claim.extension, 0.0)

    def test_wall_joint_publishes_unsupported_topology_status(self):
        self.printTestMessage("Testing wall joint status for incompatible normals...")

        wall1 = self._make_baseless_wall_between(App.Vector(-1000, 0, 0), App.Vector(0, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))

        original_get_join_path = ArchWallRelation.get_join_path

        def get_incompatible_path(wall):
            path = original_get_join_path(wall)
            if wall == wall2:
                return ArchWallGeometry.WallPath(path.edge, App.Vector(1, 0, 0))
            return path

        ArchWallRelation.get_join_path = get_incompatible_path
        try:
            joint = Arch.makeWallJoint(wall1, wall2, "Miter")
            self.document.recompute()
        finally:
            ArchWallRelation.get_join_path = original_get_join_path

        self.assertEqual(joint.Status, "UnsupportedTopology")
        self.assertIn("shared section normal", joint.StatusMessage)

    def test_make_wall_joint_rejects_invalid_type_without_creating_object(self):
        self.printTestMessage("Testing invalid wall joint types do not create objects...")

        wall1 = self._make_baseless_wall_between(App.Vector(-1000, 0, 0), App.Vector(0, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        object_names = {obj.Name for obj in self.document.Objects}

        with self.assertRaisesRegex(ValueError, "Unsupported wall joint type"):
            Arch.makeWallJoint(wall1, wall2, "Mitre")

        self.assertEqual({obj.Name for obj in self.document.Objects}, object_names)

    def test_wall_joint_reports_non_wall_operand_without_recompute_error(self):
        self.printTestMessage("Testing wall joint validation for non-wall links...")

        not_a_wall = self.document.addObject("Part::Feature", "NotAWall")
        wall = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        joint = Arch.makeWallJoint(not_a_wall, wall, "Miter")
        self.document.recompute()

        self.assertEqual(joint.Status, "UnsupportedBaseline")
        self.assertIn("single straight baseline", joint.StatusMessage)

    def test_make_wall_joint_butt_trims_wall_shapes(self):
        self.printTestMessage("Testing makeWallJoint creates a usable butt relation...")

        wall1 = self._make_baseless_wall_between(App.Vector(-1000, 0, 0), App.Vector(0, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))

        joint = Arch.makeWallJoint(wall1, wall2, "Butt")
        joint.ButtTrimmed = "WallB"
        self.document.recompute()

        self.assertEqual(joint.Status, "OK")
        self.assertEqual(joint.ButtTrimmed, "WallB")
        self.assertEqual(joint.ResolvedEndA, "End")
        self.assertEqual(joint.ResolvedEndB, "Start")
        self.assertFalse(
            self._is_identity_placement(joint.ResolvedPlaneA),
            "The butt relation should solve a cutting plane for the first wall.",
        )
        self.assertFalse(
            self._is_identity_placement(joint.ResolvedPlaneB),
            "The butt relation should solve a cutting plane for the second wall.",
        )
        self.assertTrue(wall1.Shape.isValid(), "First wall became invalid after the butt relation.")
        self.assertTrue(
            wall2.Shape.isValid(), "Second wall became invalid after the butt relation."
        )

    def test_make_wall_joint_butt_uses_support_section_side_extent(self):
        self.printTestMessage("Testing butt joints use support wall section extents...")

        expectations = {
            "Center": 100.0,
            "Left": 0.0,
            "Right": 200.0,
        }
        for align, expected_y_min in expectations.items():
            with self.subTest(align=align):
                support_wall = self._make_baseless_wall_between(
                    App.Vector(-1000, 0, 0), App.Vector(0, 0, 0), width=200
                )
                support_wall.Align = align
                trimmed_wall = self._make_baseless_wall_between(
                    App.Vector(0, 0, 0), App.Vector(0, 1000, 0), width=300
                )
                self.document.recompute()

                joint = Arch.makeWallJoint(support_wall, trimmed_wall, "Butt")
                joint.ButtTrimmed = "WallB"
                self.document.recompute()

                self.assertEqual(joint.Status, "OK")
                self.assertAlmostEqual(
                    trimmed_wall.Shape.BoundBox.YMin,
                    expected_y_min,
                    delta=1e-4,
                    msg="The trimmed wall should start at the support wall face that it butts into.",
                )

                self.document.removeObject(joint.Name)
                self.document.removeObject(trimmed_wall.Name)
                self.document.removeObject(support_wall.Name)
                self.document.recompute()

    def test_make_wall_joint_butt_uses_trimmed_section_side_extent(self):
        self.printTestMessage("Testing butt joints use trimmed wall section extents...")

        expectations = {
            "Center": -150.0,
            "Left": 0.0,
            "Right": -300.0,
        }
        for align, expected_x_max in expectations.items():
            with self.subTest(align=align):
                support_wall = self._make_baseless_wall_between(
                    App.Vector(-1000, 0, 0), App.Vector(0, 0, 0), width=200
                )
                trimmed_wall = self._make_baseless_wall_between(
                    App.Vector(0, 0, 0), App.Vector(0, 1000, 0), width=300
                )
                trimmed_wall.Align = align
                self.document.recompute()

                joint = Arch.makeWallJoint(support_wall, trimmed_wall, "Butt")
                joint.ButtTrimmed = "WallB"
                self.document.recompute()

                self.assertEqual(joint.Status, "OK")
                self.assertAlmostEqual(
                    support_wall.Shape.BoundBox.XMax,
                    expected_x_max,
                    delta=1e-4,
                    msg="The supporting wall should end at the face of the trimmed wall.",
                )

                self.document.removeObject(joint.Name)
                self.document.removeObject(trimmed_wall.Name)
                self.document.removeObject(support_wall.Name)
                self.document.recompute()

    def test_make_wall_joint_tee_uses_top_section_side_extent(self):
        self.printTestMessage("Testing tee joints use top wall section extents...")

        expectations = {
            "Center": 150.0,
            "Left": 0.0,
            "Right": 300.0,
        }
        for align, expected_y_min in expectations.items():
            with self.subTest(align=align):
                stem_wall = self._make_baseless_wall_between(
                    App.Vector(0, 0, 0), App.Vector(0, 1000, 0), width=200
                )
                top_wall = self._make_baseless_wall_between(
                    App.Vector(-1000, 0, 0), App.Vector(1000, 0, 0), width=300
                )
                top_wall.Align = align
                self.document.recompute()

                joint = Arch.makeWallJoint(stem_wall, top_wall, "Tee")
                joint.TeeStem = "WallA"
                self.document.recompute()

                self.assertEqual(joint.Status, "OK")
                self.assertAlmostEqual(
                    stem_wall.Shape.BoundBox.YMin,
                    expected_y_min,
                    delta=1e-4,
                    msg="The tee stem should trim to the actual face of the top wall section.",
                )

                self.document.removeObject(joint.Name)
                self.document.removeObject(top_wall.Name)
                self.document.removeObject(stem_wall.Name)
                self.document.recompute()

    def test_wall_joint_updates_after_wall_move(self):
        self.printTestMessage("Testing wall joint recomputes from wall movement...")

        wall1 = self._make_baseless_wall_between(App.Vector(-1000, 0, 0), App.Vector(1000, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))

        joint = Arch.makeWallJoint(wall1, wall2, "Miter")
        self.document.recompute()
        initial_intersection = App.Vector(joint.Intersection)

        wall2.Placement.move(App.Vector(200, 0, 0))
        self.document.recompute()

        self.assertNotAlmostEqual(
            joint.Intersection.x,
            initial_intersection.x,
            delta=1e-6,
            msg="The joint intersection should update when a joined wall moves.",
        )
        self.assertAlmostEqual(joint.Intersection.x, 200.0, delta=1e-6)
        self.assertEqual(joint.Status, "OK")

    def test_wall_joint_updates_when_linked_base_line_moves(self):
        """A linked base edit invalidates both walls in the relation."""
        line_a = Draft.makeLine(App.Vector(-1000, 0, 0), App.Vector(0, 0, 0))
        line_b = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        wall_a = Arch.makeWall(line_a, width=200, height=1000)
        wall_b = Arch.makeWall(line_b, width=200, height=1000)
        self.document.recompute()
        Arch.makeWallJoint(wall_a, wall_b, "Miter")
        self.document.recompute()
        initial_volume = wall_b.Shape.Volume

        line_a.Start = App.Vector(-1000, 200, 0)
        line_a.End = App.Vector(0, 200, 0)
        self.document.recompute()

        self.assertNotAlmostEqual(
            wall_b.Shape.Volume,
            initial_volume,
            delta=1e-6,
            msg="The sibling wall kept stale geometry after its linked base moved.",
        )

    def test_disabling_wall_joint_restores_original_wall_shapes(self):
        self.printTestMessage("Testing disabling a wall joint restores the walls...")

        wall1 = self._make_baseless_wall_between(App.Vector(-1000, 0, 0), App.Vector(0, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        original_volume1 = wall1.Shape.Volume
        original_volume2 = wall2.Shape.Volume

        joint = Arch.makeWallJoint(wall1, wall2, "Miter")
        self.document.recompute()
        self.assertEqual(joint.Status, "OK")

        joint.Enabled = False
        self.document.recompute()

        self.assertEqual(joint.Status, "Disabled")
        self.assertAlmostEqual(wall1.Shape.Volume, original_volume1, delta=1e-6)
        self.assertAlmostEqual(wall2.Shape.Volume, original_volume2, delta=1e-6)

    def test_deleting_wall_joint_restores_original_wall_shapes(self):
        self.printTestMessage("Testing deleting a wall joint restores the walls...")

        wall1 = self._make_baseless_wall_between(App.Vector(-1000, 0, 0), App.Vector(0, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        original_volume1 = wall1.Shape.Volume
        original_volume2 = wall2.Shape.Volume

        joint = Arch.makeWallJoint(wall1, wall2, "Miter")
        self.document.recompute()
        self.assertEqual(joint.Status, "OK")

        self.document.removeObject(joint.Name)
        self.document.recompute()

        self.assertAlmostEqual(wall1.Shape.Volume, original_volume1, delta=1e-6)
        self.assertAlmostEqual(wall2.Shape.Volume, original_volume2, delta=1e-6)

    def test_make_wall_joint_reports_unsupported_baselines(self):
        self.printTestMessage("Testing wall joint status on unsupported baselines...")

        wire = Draft.makeWire(
            [App.Vector(0, 0, 0), App.Vector(1000, 0, 0), App.Vector(1000, 1000, 0)]
        )
        self.document.recompute()
        unsupported_wall = Arch.makeWall(wire, width=200, height=1500)
        supported_wall = self._make_baseless_wall_between(
            App.Vector(1000, 0, 0), App.Vector(1000, -1000, 0)
        )

        joint = Arch.makeWallJoint(unsupported_wall, supported_wall, "Miter")
        self.document.recompute()

        self.assertEqual(joint.Status, "UnsupportedBaseline")
        self.assertIn("single straight baseline", joint.StatusMessage)

    def test_make_wall_joint_auto_label_and_custom_name(self):
        self.printTestMessage("Testing wall joint labels...")

        wall1 = self._make_baseless_wall_between(App.Vector(-1000, 0, 0), App.Vector(0, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))

        joint = Arch.makeWallJoint(wall1, wall2, "Miter")
        self.document.recompute()

        self.assertTrue(joint.AutoLabel)
        self.assertEqual(joint.Label, f"Miter: {wall1.Label} <-> {wall2.Label}")

        joint.JointType = "Butt"
        self.document.recompute()
        self.assertEqual(joint.Label, f"Butt: {wall1.Label} <-> {wall2.Label}")

        named_joint = Arch.makeWallJoint(wall1, wall2, "Tee", name="Custom Joint")
        self.document.recompute()

        self.assertFalse(named_joint.AutoLabel)
        self.assertEqual(named_joint.Label, "Custom Joint")

        named_joint.JointType = "Miter"
        self.document.recompute()
        self.assertEqual(named_joint.Label, "Custom Joint")

    def test_wall_joint_conflict_reports_blocking_relation(self):
        self.printTestMessage("Testing wall joint conflict reporting...")

        wall1 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(2000, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        wall3 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, -1000, 0))

        blocker = Arch.makeWallJoint(wall1, wall2, "Miter")
        self.document.recompute()
        self.assertEqual(blocker.Status, "OK")
        winning_volume = wall1.Shape.Volume
        losing_branch_volume = wall3.Shape.Volume

        conflicted = Arch.makeWallJoint(wall1, wall3, "Miter")
        self.document.recompute()

        self.assertEqual(conflicted.Status, "Conflict")
        self.assertEqual(conflicted.ResolvedEndA, "Start")
        self.assertEqual(conflicted.ConflictJointLabelA, blocker.Label)
        self.assertEqual(conflicted.ConflictJointLabelB, "")
        self.assertIn("Start", conflicted.ConflictMessageA)
        self.assertIn(blocker.Label, conflicted.ConflictMessageA)
        self.assertEqual(conflicted.ConflictMessageB, "")
        self.assertIn("Conflict:", conflicted.StatusMessage)
        self.assertIn(blocker.Label, conflicted.StatusMessage)
        self.assertAlmostEqual(
            wall1.Shape.Volume,
            winning_volume,
            delta=1e-6,
            msg="A losing relation must not suppress the winning relation's trim.",
        )
        self.assertAlmostEqual(
            wall3.Shape.Volume,
            losing_branch_volume,
            delta=1e-6,
            msg="A conflicted relation must not partially trim its uncontested wall.",
        )

        conflicts = ArchWallRelationResolver.get_joint_conflicts(
            conflicted,
            ArchWallRelation.solve_wall_joint(conflicted),
        )
        self.assertTrue(conflicts)
        self.assertIsInstance(conflicts[0], ArchWallRelation.WallJointConflict)
        self.assertEqual(conflicts[0].other_joint_label, blocker.Label)

    def test_wall_joint_priority_change_reassigns_conflict_owner(self):
        self.printTestMessage("Testing priority changes reassign wall-end ownership...")

        wall1 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(2000, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        wall3 = self._make_baseless_wall_between(App.Vector(100, 0, 0), App.Vector(100, 1000, 0))

        first = Arch.makeWallJoint(wall1, wall2, "Miter")
        second = Arch.makeWallJoint(wall1, wall3, "Miter")
        self.document.recompute()
        self.assertEqual(first.Priority, 0)
        self.assertEqual(second.Priority, 1)
        self.assertEqual(first.Status, "OK")
        self.assertEqual(second.Status, "Conflict")

        first.Priority = second.Priority + 1
        self.document.recompute()

        self.assertEqual(second.Status, "OK")
        self.assertEqual(first.Status, "Conflict")

        def shape_extent(wall):
            bounds = wall.Shape.BoundBox
            return (
                wall.Shape.Volume,
                bounds.XMin,
                bounds.XMax,
                bounds.YMin,
                bounds.YMax,
                bounds.ZMin,
                bounds.ZMax,
            )

        extents_before_refresh = {wall.Name: shape_extent(wall) for wall in (wall1, wall2, wall3)}
        for wall in (wall1, wall2, wall3):
            wall.touch()
        self.document.recompute()

        for wall in (wall1, wall2, wall3):
            for expected, actual in zip(extents_before_refresh[wall.Name], shape_extent(wall)):
                self.assertAlmostEqual(
                    actual,
                    expected,
                    delta=1e-6,
                    msg=f"Priority change left {wall.Label} geometry stale.",
                )

    def test_wall_joint_rejects_intersection_requiring_extension(self):
        self.printTestMessage("Testing joins that would require wall extension...")

        wall1 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(1000, 0, 0))
        wall2 = self._make_baseless_wall_between(
            App.Vector(2000, 1000, 0), App.Vector(2000, 2000, 0)
        )
        wall1_endpoints = self._path_endpoints(wall1)
        wall2_endpoints = self._path_endpoints(wall2)
        wall1_volume = wall1.Shape.Volume
        wall2_volume = wall2.Shape.Volume

        joint = Arch.makeWallJoint(wall1, wall2, "Miter")
        self.document.recompute()

        self.assertEqual(joint.Status, "RequiresExtension")
        self.assertIn("extending walls is not supported", joint.StatusMessage)
        self.assertEqual(self._path_endpoints(wall1), wall1_endpoints)
        self.assertEqual(self._path_endpoints(wall2), wall2_endpoints)
        self.assertAlmostEqual(wall1.Shape.Volume, wall1_volume, delta=1e-6)
        self.assertAlmostEqual(wall2.Shape.Volume, wall2_volume, delta=1e-6)

    def test_wall_relation_priority_ignores_unrelated_objects(self):
        self.printTestMessage("Testing wall relation priority allocation scope...")

        unrelated = self.document.addObject("App::FeaturePython", "UnrelatedPriority")
        unrelated.addProperty("App::PropertyInteger", "Priority")
        unrelated.Priority = 1000
        wall1 = self._make_baseless_wall_between(App.Vector(-1000, 0, 0), App.Vector(0, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))

        joint = Arch.makeWallJoint(wall1, wall2, "Miter")

        self.assertEqual(joint.Priority, 0)

    def test_wall_joint_negative_priority_allocates_next_priority(self):
        self.printTestMessage("Testing negative priority is treated as unset...")

        wall1 = self._make_baseless_wall_between(App.Vector(-1000, 0, 0), App.Vector(0, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        wall3 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, -1000, 0))

        first = Arch.makeWallJoint(wall1, wall2, "Miter")
        second = Arch.makeWallJoint(wall1, wall3, "Miter")
        self.document.recompute()

        second.Priority = -1
        self.document.recompute()

        self.assertEqual(first.Priority, 0)
        self.assertEqual(second.Priority, 1)
        self.assertEqual(first.Status, "OK")
        self.assertEqual(second.Status, "Conflict")

    def test_wall_joint_conflict_resolves_after_blocker_removed(self):
        self.printTestMessage("Testing wall joint conflict clears after blocker removal...")

        wall1 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(2000, 0, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        wall3 = self._make_baseless_wall_between(App.Vector(100, 0, 0), App.Vector(100, 1000, 0))

        blocker = Arch.makeWallJoint(wall1, wall2, "Miter")
        conflicted = Arch.makeWallJoint(wall1, wall3, "Miter")
        self.document.recompute()
        self.assertEqual(conflicted.Status, "Conflict")

        self.document.removeObject(blocker.Name)
        self.document.recompute()

        self.assertEqual(conflicted.Status, "OK")
        self.assertEqual(conflicted.ConflictJointLabelA, "")
        self.assertEqual(conflicted.ConflictJointLabelB, "")
        self.assertEqual(conflicted.ConflictMessageA, "")
        self.assertEqual(conflicted.ConflictMessageB, "")

    def test_wall_relations_restore_after_document_round_trip(self):
        """Persisted wall relations restore their links, settings, and outputs."""
        self.printTestMessage("Testing wall relation document round trip...")

        joint_wall_a = self._make_baseless_wall_between(
            App.Vector(-1000, 0, 0), App.Vector(0, 0, 0)
        )
        joint_wall_b = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        joint = Arch.makeWallJoint(
            joint_wall_a,
            joint_wall_b,
            "Miter",
            name="Persisted WallJoint",
        )

        junction_carrier = self._make_baseless_wall_between(
            App.Vector(3000, 0, 0), App.Vector(5000, 0, 0)
        )
        junction_branch_up = self._make_baseless_wall_between(
            App.Vector(4000, 0, 0), App.Vector(4000, 1000, 0)
        )
        junction_branch_down = self._make_baseless_wall_between(
            App.Vector(4000, 0, 0), App.Vector(4000, -1000, 0)
        )
        junction = Arch.makeWallJunction(
            [junction_carrier, junction_branch_up, junction_branch_down],
            carrier_wall=junction_carrier,
            name="Persisted WallJunction",
        )
        self.document.recompute()
        self.assertEqual(joint.Status, "OK")
        self.assertEqual(junction.Status, "OK")
        joint_name = joint.Name
        joint_wall_a_name = joint_wall_a.Name
        joint_wall_b_name = joint_wall_b.Name
        junction_name = junction.Name
        junction_carrier_name = junction_carrier.Name
        junction_branch_up_name = junction_branch_up.Name
        junction_branch_down_name = junction_branch_down.Name

        fd, path = tempfile.mkstemp(suffix=".FCStd")
        os.close(fd)
        try:
            original_document_name = self.document.Name
            self.document.saveAs(path)
            App.closeDocument(original_document_name)
            self.document = None

            reopened = App.openDocument(path)
            self.document = reopened
            reopened.recompute()

            restored_joint = reopened.getObject(joint_name)
            self.assertIsNotNone(restored_joint)
            self.assertEqual(restored_joint.Proxy.Type, "WallJoint")
            self.assertEqual(restored_joint.Label, "Persisted WallJoint")
            self.assertEqual(restored_joint.JointType, "Miter")
            self.assertEqual(restored_joint.Status, "OK")
            self.assertEqual(restored_joint.WallA.Name, joint_wall_a_name)
            self.assertEqual(restored_joint.WallB.Name, joint_wall_b_name)
            self.assertFalse(self._is_identity_placement(restored_joint.ResolvedPlaneA))

            restored_junction = reopened.getObject(junction_name)
            self.assertIsNotNone(restored_junction)
            self.assertEqual(restored_junction.Proxy.Type, "WallJunction")
            self.assertEqual(restored_junction.Label, "Persisted WallJunction")
            self.assertEqual(restored_junction.Status, "OK")
            self.assertEqual(
                restored_junction.ResolvedCarrierWall.Name,
                junction_carrier_name,
            )
            self.assertEqual(
                {wall.Name for wall in restored_junction.Walls},
                {junction_carrier_name, junction_branch_up_name, junction_branch_down_name},
            )
        finally:
            if os.path.exists(path):
                os.remove(path)

    def test_wall_joint_tee_stem_edit_updates_trimmed_wall_geometry(self):
        self.printTestMessage("Testing tee stem edits update the trimmed wall geometry...")

        wall1 = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        wall2 = self._make_baseless_wall_between(App.Vector(-1000, 0, 0), App.Vector(1000, 0, 0))
        original_volume1 = wall1.Shape.Volume
        original_volume2 = wall2.Shape.Volume

        joint = Arch.makeWallJoint(wall1, wall2, "Tee")
        joint.TeeStem = "WallA"
        self.document.recompute()

        self.assertEqual(joint.Status, "OK")
        self._assert_wall_trimmed(
            wall1,
            original_volume1,
            "The initial tee stem wall should be trimmed.",
        )
        self._assert_wall_unchanged(
            wall2,
            original_volume2,
            "The top wall should remain untrimmed before flipping the tee stem.",
        )

        joint.TeeStem = "WallB"
        self.document.recompute()

        self.assertEqual(joint.Status, "OK")
        self.assertEqual(joint.ResolvedEndA, "None")
        self.assertNotEqual(joint.ResolvedEndB, "None")
        self._assert_wall_unchanged(
            wall1,
            original_volume1,
            "Changing TeeStem should restore the original wall when it stops being the stem.",
        )
        self._assert_wall_trimmed(
            wall2,
            original_volume2,
            "Changing TeeStem should trim the newly selected stem wall.",
        )
