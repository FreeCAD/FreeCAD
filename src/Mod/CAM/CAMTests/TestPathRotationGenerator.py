# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Path
import Path.Base.Generator.rotation as orientation
import CAMTests.PathTestUtils as PathTestUtils
import numpy as np
from Machine.models.machine import Machine, RotaryAxis, AxisRole

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestPathRotationGenerator(PathTestUtils.PathTestBase):

    def _create_table_table_machine(self):
        """Create a standard C-A table-table machine for regression testing."""
        machine = Machine(name="Test CA Machine")

        # Add C axis (rotates about Z)
        machine.rotary_axes["C"] = RotaryAxis(
            name="C",
            rotation_vector=FreeCAD.Vector(0, 0, 1),
            min_limit=-360,
            max_limit=360,
            role=AxisRole.TABLE_ROTARY,
            parent=None,
            sequence=0,
        )

        # Add A axis (rotates about X)
        machine.rotary_axes["A"] = RotaryAxis(
            name="A",
            rotation_vector=FreeCAD.Vector(1, 0, 0),
            min_limit=-120,
            max_limit=120,
            role=AxisRole.TABLE_ROTARY,
            parent="C",
            sequence=1,
        )

        return machine

    def _create_head_head_machine(self):
        """Create a B-C head-head machine."""
        machine = Machine(name="Test BC Machine")

        # Add B axis (rotates about Y)
        machine.rotary_axes["B"] = RotaryAxis(
            name="B",
            rotation_vector=FreeCAD.Vector(0, 1, 0),
            min_limit=-90,
            max_limit=90,
            role=AxisRole.HEAD_ROTARY,
            parent=None,
            sequence=0,
        )

        # Add C axis (rotates about Z)
        machine.rotary_axes["C"] = RotaryAxis(
            name="C",
            rotation_vector=FreeCAD.Vector(0, 0, 1),
            min_limit=-360,
            max_limit=360,
            role=AxisRole.HEAD_ROTARY,
            parent="B",
            sequence=1,
        )

        return machine

    def _create_mixed_machine(self):
        """Create a mixed table-head machine."""
        machine = Machine(name="Test Mixed Machine")

        # Add C axis (table rotary about Z)
        machine.rotary_axes["C"] = RotaryAxis(
            name="C",
            rotation_vector=FreeCAD.Vector(0, 0, 1),
            min_limit=-360,
            max_limit=360,
            role=AxisRole.TABLE_ROTARY,
            parent=None,
            sequence=0,
        )

        # Add B axis (head rotary about Y)
        machine.rotary_axes["B"] = RotaryAxis(
            name="B",
            rotation_vector=FreeCAD.Vector(0, 1, 0),
            min_limit=-90,
            max_limit=90,
            role=AxisRole.HEAD_ROTARY,
            parent=None,
            sequence=1,
        )

        return machine

    def _create_single_axis_machine(self):
        """Create a 3+1 single rotary axis machine."""
        machine = Machine(name="Test Single Axis Machine")

        # Add A axis only
        machine.rotary_axes["A"] = RotaryAxis(
            name="A",
            rotation_vector=FreeCAD.Vector(1, 0, 0),
            min_limit=-360,
            max_limit=360,
            role=AxisRole.TABLE_ROTARY,
            parent=None,
            sequence=0,
        )

        return machine

    def test00_identity_orientation(self):
        """
        Test solving for identity orientation (Z-up) on table-table machine.

        Expected behavior:
            Should return A=0, C=0 for Z-up orientation.
        """
        machine = self._create_table_table_machine()
        desired_axis = FreeCAD.Vector(0, 0, 1)  # Z-up

        result = orientation.solve_orientation(machine, desired_axis)

        self.assertTrue(result.success, "Should successfully solve identity orientation")
        self.assertAlmostEqual(result.angles.get("A", 0), 0, places=3)
        self.assertAlmostEqual(result.angles.get("C", 0), 0, places=3)
        self.assertLess(result.error_norm, 1e-6)

    def test10_forty_five_degree_tilt(self):
        """
        Test solving for 45-degree tilt on table-table machine.

        Expected behavior:
            Should find valid A and C angles to achieve 45-degree orientation.
        """
        machine = self._create_table_table_machine()
        # 45-degree tilt in XZ plane
        desired_axis = FreeCAD.Vector(0.7071, 0, 0.7071).normalize()

        result = orientation.solve_orientation(machine, desired_axis)

        self.assertTrue(result.success, "Should successfully solve 45-degree tilt")
        self.assertIn("A", result.angles, "Should return A angle")
        self.assertIn("C", result.angles, "Should return C angle")
        self.assertLess(result.error_norm, 1e-6)

    def test20_axis_limits_active(self):
        """
        Test solving with axis limits that constrain solution.

        Expected behavior:
            Should fail when desired orientation requires angles outside limits.
        """
        machine = self._create_table_table_machine()
        # Constrain A axis to small range
        machine.rotary_axes["A"].min_limit = -10
        machine.rotary_axes["A"].max_limit = 10

        # Request orientation that requires large A angle
        desired_axis = FreeCAD.Vector(0, 0.7071, 0.7071).normalize()

        result = orientation.solve_orientation(machine, desired_axis)

        self.assertFalse(result.success, "Should fail when limits prevent solution")
        self.assertNotEqual(result.reason, "", "Should provide failure reason")

    def test30_head_head_machine(self):
        """
        Test solving on head-head machine configuration.

        Expected behavior:
            Should correctly handle head rotary axes (direct tool rotation).
        """
        machine = self._create_head_head_machine()
        desired_axis = FreeCAD.Vector(0.5, 0.5, 0.7071).normalize()

        result = orientation.solve_orientation(machine, desired_axis)

        self.assertTrue(result.success, "Should successfully solve head-head configuration")
        self.assertIn("B", result.angles, "Should return B angle")
        self.assertIn("C", result.angles, "Should return C angle")
        self.assertLess(result.error_norm, 1e-6)

    def test40_mixed_machine(self):
        """
        Test solving on mixed table-head machine.

        Expected behavior:
            Should correctly handle combination of table and head rotaries.
        """
        machine = self._create_mixed_machine()
        desired_axis = FreeCAD.Vector(0.3, 0.4, 0.866).normalize()

        result = orientation.solve_orientation(machine, desired_axis)

        self.assertTrue(result.success, "Should successfully solve mixed configuration")
        self.assertIn("C", result.angles, "Should return C angle (table)")
        self.assertIn("B", result.angles, "Should return B angle (head)")
        self.assertLess(result.error_norm, 1e-6)

    def test50_single_axis_solve(self):
        """
        Test solving with single rotary axis (3+1 indexing).

        Expected behavior:
            Should solve with only one rotary axis.
        """
        machine = self._create_single_axis_machine()
        desired_axis = FreeCAD.Vector(0.866, 0, 0.5).normalize()

        result = orientation.solve_orientation(machine, desired_axis)

        self.assertTrue(result.success, "Should successfully solve single axis")
        self.assertIn("A", result.angles, "Should return A angle")
        self.assertEqual(len(result.angles), 1, "Should only return one axis")
        self.assertLess(result.error_norm, 1e-6)

    def test60_restrict_axes(self):
        """
        Test solving with restricted axis subset.

        Expected behavior:
            Should only solve for specified axes, ignoring others.
        """
        machine = self._create_table_table_machine()
        desired_axis = FreeCAD.Vector(0, 0, 1)  # Z-up

        # Restrict to only C axis
        result = orientation.solve_orientation(machine, desired_axis, restrict_axes=["C"])

        self.assertTrue(result.success, "Should successfully solve with restricted axes")
        self.assertIn("C", result.angles, "Should return C angle")
        self.assertNotIn("A", result.angles, "Should not return A axis when restricted")

    def test70_current_state_awareness(self):
        """
        Test that solver considers current state for minimal motion.

        Expected behavior:
            When multiple solutions exist, the solver should pick the one
            closest to the current state. Solving from two different starting
            states should yield different chosen solutions.
        """
        machine = self._create_table_table_machine()
        desired_axis = FreeCAD.Vector(0.5, 0.5, 0.5).normalize()

        # Solve from two different current states
        current_state_1 = {"A": 0, "C": 0}
        current_state_2 = {"A": -50, "C": -130}

        result_1 = orientation.solve_orientation(machine, desired_axis, current_state_1)
        result_2 = orientation.solve_orientation(machine, desired_axis, current_state_2)

        self.assertTrue(result_1.success, "Should succeed from state 1")
        self.assertTrue(result_2.success, "Should succeed from state 2")

        # Each result's cost should reflect distance from its own starting state
        # Verify deltas are populated
        self.assertIn("A", result_1.deltas, "Should have A delta")
        self.assertIn("C", result_1.deltas, "Should have C delta")

        # The two solutions should differ (different starting states → different best)
        angles_differ = (
            abs(result_1.angles.get("A", 0) - result_2.angles.get("A", 0)) > 1
            or abs(result_1.angles.get("C", 0) - result_2.angles.get("C", 0)) > 1
        )
        self.assertTrue(angles_differ, "Different starting states should yield different solutions")

    def test80_solution_preference_shortest(self):
        """
        Test shortest solution preference.

        Expected behavior:
            Should select solution with minimal total angular travel.
        """
        machine = self._create_table_table_machine()
        desired_axis = FreeCAD.Vector(0.7071, 0, 0.7071).normalize()

        # Set both axes to prefer shortest
        machine.rotary_axes["A"].solution_preference = "shortest"
        machine.rotary_axes["C"].solution_preference = "shortest"

        result = orientation.solve_orientation(machine, desired_axis)

        self.assertTrue(result.success, "Should successfully solve with shortest preference")
        # Verify solution is within reasonable range
        for axis_name, angle in result.angles.items():
            axis = machine.rotary_axes[axis_name]
            self.assertGreaterEqual(angle, axis.min_limit)
            self.assertLessEqual(angle, axis.max_limit)

    def test90_allow_flip_enabled(self):
        """
        Test flip solutions when allowed.

        Expected behavior:
            Should consider 180-degree equivalent solutions.
        """
        machine = self._create_table_table_machine()
        desired_axis = FreeCAD.Vector(0.7071, 0, 0.7071).normalize()

        # Enable flip on both axes
        machine.rotary_axes["A"].allow_flip = True
        machine.rotary_axes["C"].allow_flip = True

        result = orientation.solve_orientation(machine, desired_axis)

        self.assertTrue(result.success, "Should successfully solve with flip allowed")
        self.assertLess(result.error_norm, 1e-6)

    def test100_no_rotary_axes(self):
        """
        Test behavior when machine has no rotary axes.

        Expected behavior:
            Should fail gracefully with appropriate error message.
        """
        machine = Machine(name="No Rotary Machine")
        desired_axis = FreeCAD.Vector(0, 0, 1)

        result = orientation.solve_orientation(machine, desired_axis)

        self.assertFalse(result.success, "Should fail when no rotary axes available")
        self.assertIn("No rotary axes", result.reason)

    def test110_sequential_solves(self):
        """
        Test sequential solves to verify minimal-motion selection.

        Expected behavior:
            Series of solves should prefer minimal motion between solutions.
        """
        machine = self._create_table_table_machine()

        # Sequence of desired orientations
        orientations = [
            FreeCAD.Vector(0, 0, 1),  # Start with Z-up
            FreeCAD.Vector(0.7071, 0, 0.7071),  # Tilt in X
            FreeCAD.Vector(0, 0.7071, 0.7071),  # Tilt in Y
            FreeCAD.Vector(0, 0, 1),  # Back to Z-up
        ]

        current_state = {}
        total_cost = 0

        for i, desired_axis in enumerate(orientations):
            result = orientation.solve_orientation(machine, desired_axis, current_state)

            self.assertTrue(result.success, f"Sequential solve {i} should succeed")

            # Accumulate total motion cost
            total_cost += result.cost

            # Update current state for next iteration
            current_state = result.angles.copy()

        # Verify total cost is reasonable (should be minimal path)
        self.assertGreater(total_cost, 0, "Should have some motion cost")
        self.assertLess(total_cost, 1000, "Total cost should be reasonable")

    def _create_ac_table_table_machine(self):
        """Create an A-C table-table machine where C is mounted on A.

        Topology: A (parent=None, rot about X) → C (parent=A, rot about Z)
        This matches the user's 'rotator' machine config with limits ±180.
        """
        machine = Machine(name="Test AC Machine")

        machine.rotary_axes["A"] = RotaryAxis(
            name="A",
            rotation_vector=FreeCAD.Vector(1, 0, 0),
            min_limit=-180,
            max_limit=180,
            role=AxisRole.TABLE_ROTARY,
            parent=None,
            sequence=0,
            solution_preference="shortest",
            allow_flip=True,
        )

        machine.rotary_axes["C"] = RotaryAxis(
            name="C",
            rotation_vector=FreeCAD.Vector(0, 0, 1),
            min_limit=-180,
            max_limit=180,
            role=AxisRole.TABLE_ROTARY,
            parent="A",
            sequence=1,
            solution_preference="shortest",
            allow_flip=True,
        )

        return machine

    def test120_geometry_rotation_y_workplane(self):
        """
        Test that compute_rotation_matrix produces a rotation consistent with
        the solved angles for workplane Vector(0, 1, 0).

        The geometry rotation R must satisfy R.multVec(workplane) ≈ Z,
        i.e. it maps the workplane normal into the spindle axis.
        """
        machine = self._create_ac_table_table_machine()
        desired = FreeCAD.Vector(0, 1, 0)

        result = orientation.solve_orientation(machine, desired)
        self.assertTrue(result.success, f"Should solve for Y workplane: {result.reason}")

        chain = orientation.build_kinematic_chain(machine)
        rot = orientation.compute_rotation_matrix(chain, result.angles)
        # Table rotaries: R · desired = Z, so use direct rotation
        achieved = rot.multVec(desired)

        self.assertAlmostEqual(
            achieved.x, 0.0, places=6, msg=f"X component should be 0, got {achieved.x}"
        )
        self.assertAlmostEqual(
            achieved.y, 0.0, places=6, msg=f"Y component should be 0, got {achieved.y}"
        )
        self.assertAlmostEqual(
            achieved.z, 1.0, places=6, msg=f"Z component should be 1, got {achieved.z}"
        )

    def test130_geometry_rotation_neg_x_workplane(self):
        """
        Test geometry rotation for workplane Vector(-1, 0, 0).

        R.multVec((-1,0,0)) should ≈ (0,0,1).
        """
        machine = self._create_ac_table_table_machine()
        desired = FreeCAD.Vector(-1, 0, 0)

        result = orientation.solve_orientation(machine, desired)
        self.assertTrue(result.success, f"Should solve for -X workplane: {result.reason}")

        chain = orientation.build_kinematic_chain(machine)
        rot = orientation.compute_rotation_matrix(chain, result.angles)
        # Table rotaries: R · desired = Z, so use direct rotation
        achieved = rot.multVec(desired)

        self.assertAlmostEqual(achieved.x, 0.0, places=6)
        self.assertAlmostEqual(achieved.y, 0.0, places=6)
        self.assertAlmostEqual(achieved.z, 1.0, places=6)

    def test140_geometry_rotation_diagonal_workplane(self):
        """
        Test geometry rotation for an arbitrary diagonal workplane.

        R.multVec(desired) should ≈ (0,0,1).
        """
        machine = self._create_ac_table_table_machine()
        desired = FreeCAD.Vector(0.5, 0.5, 0.5).normalize()

        result = orientation.solve_orientation(machine, desired)
        self.assertTrue(result.success, f"Should solve for diagonal workplane: {result.reason}")

        chain = orientation.build_kinematic_chain(machine)
        rot = orientation.compute_rotation_matrix(chain, result.angles)
        # Table rotaries: R · desired = Z, so use direct rotation
        achieved = rot.multVec(desired)

        self.assertAlmostEqual(achieved.x, 0.0, places=5)
        self.assertAlmostEqual(achieved.y, 0.0, places=5)
        self.assertAlmostEqual(achieved.z, 1.0, places=5)
