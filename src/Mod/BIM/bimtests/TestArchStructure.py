# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Furgo                                              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import unittest
import FreeCAD as App
from FreeCAD import Vector
import Arch
from bimtests import TestArchBase


class TestArchStructure(TestArchBase.TestArchBase):

    def testStructure(self):
        App.Console.PrintLog("Checking BIM Structure...\n")
        structure = Arch.makeStructure(length=2, width=3, height=5)
        self.assertTrue(structure, "BIM Structure failed")

    #  Dimensions
    def test_makeStructure_explicit_dimensions(self):
        """Explicit length/width/height are stored on the object."""
        self.printTestMessage("makeStructure with explicit dimensions")
        structure = Arch.makeStructure(length=200, width=300, height=5000)
        self.document.recompute()
        self.assertAlmostEqual(structure.Length.Value, 200)
        self.assertAlmostEqual(structure.Width.Value, 300)
        self.assertAlmostEqual(structure.Height.Value, 5000)

    def test_makeStructure_defaults_from_preferences(self):
        """When no dimensions are given, all three are filled from preferences."""
        self.printTestMessage("makeStructure with default dimensions")
        structure = Arch.makeStructure()
        self.document.recompute()
        self.assertGreater(structure.Length.Value, 0, "Default Length should be positive")
        self.assertGreater(structure.Width.Value, 0, "Default Width should be positive")
        self.assertGreater(structure.Height.Value, 0, "Default Height should be positive")

    #  IfcType classification
    def test_makeStructure_ifc_type_column(self):
        """Height > Length produces IfcType 'Column'."""
        self.printTestMessage("makeStructure IfcType Column")
        structure = Arch.makeStructure(length=100, width=100, height=3000)
        self.assertEqual(structure.IfcType, "Column")

    def test_makeStructure_ifc_type_beam(self):
        """Length > Height produces IfcType 'Beam'."""
        self.printTestMessage("makeStructure IfcType Beam")
        structure = Arch.makeStructure(length=3000, width=100, height=100)
        self.assertEqual(structure.IfcType, "Beam")

    def test_makeStructure_ifc_type_equal_dimensions(self):
        """Length == Height keeps the initial default IfcType ('Beam')."""
        self.printTestMessage("makeStructure IfcType with equal L/H")
        structure = Arch.makeStructure(length=500, width=100, height=500)
        # Neither Length > Height nor Height > Length triggers, so the default
        # from _Structure.__init__ ("Beam") is preserved.
        self.assertEqual(structure.IfcType, "Beam")

    #  Auto-Label
    def test_makeStructure_custom_label(self):
        """A custom name overrides the automatic label."""
        self.printTestMessage("makeStructure custom label")
        structure = Arch.makeStructure(length=100, height=3000, name="MyColumn")
        self.assertEqual(structure.Label, "MyColumn")

    def test_makeStructure_auto_label_column(self):
        """Auto-label contains 'Column' when height > length."""
        self.printTestMessage("makeStructure auto-label Column")
        structure = Arch.makeStructure(length=100, height=3000)
        self.assertIn("Column", structure.Label)

    def test_makeStructure_auto_label_beam(self):
        """Auto-label contains 'Beam' when length > height."""
        self.printTestMessage("makeStructure auto-label Beam")
        structure = Arch.makeStructure(length=3000, height=100)
        self.assertIn("Beam", structure.Label)

    #  Structural System
    def test_makeStructuralSystem_basic(self):
        """A structural system is created from a structure and an axis."""
        self.printTestMessage("makeStructuralSystem basic creation")
        structure = Arch.makeStructure(length=100, width=100, height=3000)
        axis = Arch.makeAxis(num=3, size=500)
        self.document.recompute()
        system = Arch.makeStructuralSystem([structure], [axis])
        self.assertIsNotNone(system, "makeStructuralSystem returned None")
        self.assertEqual(system.Base, structure)
        self.assertEqual(system.Axes, [axis])

    def test_makeStructuralSystem_no_axes_returns_none(self):
        """Returns None when no axes are provided."""
        self.printTestMessage("makeStructuralSystem without axes")
        result = Arch.makeStructuralSystem([], [])
        self.assertIsNone(result)

    #  placeAlongEdge — base position
    def test_placeAlongEdge_base_is_p1(self):
        """Placement base is always the first point."""
        self.printTestMessage("placeAlongEdge base == p1")
        start = Vector(100, 200, 300)
        end = Vector(100, 200, 3300)
        placement = Arch.placeAlongEdge(start, end)
        self.assertTrue(
            placement.Base.isEqual(start, 1e-6),
            f"Placement base {placement.Base} should equal start {start}",
        )

    #  placeAlongEdge — vertical column (non-horizontal mode)
    def test_placeAlongEdge_vertical_column_z_axis(self):
        """Vertical column: the placement's local Z axis should point upward."""
        self.printTestMessage("placeAlongEdge vertical column Z axis")
        start = Vector(0, 0, 0)
        end = Vector(0, 0, 1000)
        placement = Arch.placeAlongEdge(start, end)
        local_z = placement.Rotation.multVec(Vector(0, 0, 1))
        self.assertAlmostEqual(
            abs(local_z.z),
            1,
            places=5,
            msg=f"Local Z {local_z} should be aligned with global Z",
        )

    #  placeAlongEdge — horizontal (beam) mode
    def test_placeAlongEdge_horizontal_along_x(self):
        """Horizontal beam along X: local X axis points along the beam direction."""
        self.printTestMessage("placeAlongEdge horizontal along X")
        start = Vector(0, 0, 0)
        end = Vector(1000, 0, 0)
        placement = Arch.placeAlongEdge(start, end, horizontal=True)
        local_x = placement.Rotation.multVec(Vector(1, 0, 0))
        self.assertAlmostEqual(
            abs(local_x.x),
            1,
            places=5,
            msg=f"Local X {local_x} should be aligned with global X",
        )

    def test_placeAlongEdge_horizontal_along_y(self):
        """Horizontal beam along Y: local X axis points along the beam direction."""
        self.printTestMessage("placeAlongEdge horizontal along Y")
        start = Vector(0, 0, 0)
        end = Vector(0, 1000, 0)
        placement = Arch.placeAlongEdge(start, end, horizontal=True)
        local_x = placement.Rotation.multVec(Vector(1, 0, 0))
        self.assertAlmostEqual(
            abs(local_x.y),
            1,
            places=5,
            msg=f"Local X {local_x} should be aligned with global Y",
        )

    def test_placeAlongEdge_horizontal_diagonal_x_axis(self):
        """Diagonal horizontal beam: local X axis points along the beam direction."""
        self.printTestMessage("placeAlongEdge horizontal diagonal X axis")
        start = Vector(0, 0, 0)
        end = Vector(1000, 500, 300)
        placement = Arch.placeAlongEdge(start, end, horizontal=True)
        beam_dir = end - start
        beam_dir.normalize()
        local_x = placement.Rotation.multVec(Vector(1, 0, 0))
        local_x.normalize()
        dot = abs(local_x.dot(beam_dir))
        self.assertAlmostEqual(
            dot,
            1,
            places=5,
            msg=f"Local X {local_x} should be aligned with beam direction {beam_dir}",
        )

    #  placeAlongEdge — frame orthogonality
    def test_placeAlongEdge_orthogonal_frame(self):
        """The resulting rotation frame has mutually orthogonal axes."""
        self.printTestMessage("placeAlongEdge orthogonal frame")
        start = Vector(0, 0, 0)
        end = Vector(1000, 500, 300)
        placement = Arch.placeAlongEdge(start, end)
        local_x = placement.Rotation.multVec(Vector(1, 0, 0))
        local_y = placement.Rotation.multVec(Vector(0, 1, 0))
        local_z = placement.Rotation.multVec(Vector(0, 0, 1))
        self.assertAlmostEqual(
            local_x.dot(local_y), 0, places=5, msg="X and Y axes should be orthogonal"
        )
        self.assertAlmostEqual(
            local_x.dot(local_z), 0, places=5, msg="X and Z axes should be orthogonal"
        )
        self.assertAlmostEqual(
            local_y.dot(local_z), 0, places=5, msg="Y and Z axes should be orthogonal"
        )

    def test_placeAlongEdge_diagonal_orthogonal_frame(self):
        """Diagonal beam in XZ plane also has an orthogonal frame."""
        self.printTestMessage("placeAlongEdge diagonal orthogonal frame")
        start = Vector(0, 0, 0)
        end = Vector(1000, 0, 1000)
        placement = Arch.placeAlongEdge(start, end)
        local_x = placement.Rotation.multVec(Vector(1, 0, 0))
        local_y = placement.Rotation.multVec(Vector(0, 1, 0))
        local_z = placement.Rotation.multVec(Vector(0, 0, 1))
        self.assertAlmostEqual(local_x.dot(local_y), 0, places=5)
        self.assertAlmostEqual(local_x.dot(local_z), 0, places=5)
        self.assertAlmostEqual(local_y.dot(local_z), 0, places=5)

    #  placeAlongEdge — cross-section roll
    @unittest.expectedFailure
    def test_placeAlongEdge_tilted_beam_cross_section_stays_upright(self):
        """For a tilted beam, the cross-section Y axis must remain perfectly
        horizontal (zero roll).

        A beam from (0,0,0) to (1000,0,300) is tilted in the XZ plane. The
        beam's 'up' (local Y axis) should stay perfectly parallel to the
        global XY plane. Any deviation in the Z component of local Y
        indicates roll.
        """
        self.printTestMessage("placeAlongEdge tilted beam zero roll")
        start = Vector(0, 0, 0)
        end = Vector(1000, 0, 300)
        placement = Arch.placeAlongEdge(start, end)

        # Transform the local Y axis (0, 1, 0) into global coordinates
        local_y = placement.Rotation.multVec(Vector(0, 1, 0))

        # In a perfect Yaw-Pitch rotation sequence, the local Y axis stays
        # strictly in the horizontal plane (Z=0).
        self.assertAlmostEqual(
            local_y.z,
            0.0,
            places=7,
            msg=f"Cross-section Y axis {local_y} has roll: Z component is {local_y.z:.7f} (expected 0.0)",
        )

    def test_placeAlongEdge_steep_beam_cross_section_roll(self):
        """For a steeply tilted beam (45°), the cross-section should still have
        a predictable roll that keeps the Y axis roughly level."""
        self.printTestMessage("placeAlongEdge steep beam roll")
        start = Vector(0, 0, 0)
        end = Vector(1000, 500, 1000)
        placement = Arch.placeAlongEdge(start, end)
        local_y = placement.Rotation.multVec(Vector(0, 1, 0))
        # Even at 45° tilt, the cross-section Y should not plunge nearly
        # vertical. |local_y.z| should be well under 1.
        self.assertLess(
            abs(local_y.z),
            0.75,
            msg=f"Cross-section Y axis {local_y} has too much Z component "
            f"({abs(local_y.z):.3f}), indicating excessive roll",
        )

    #  placeAlongEdge — degenerate cases
    def test_placeAlongEdge_parallel_to_up_returns_identity(self):
        """When beam direction is parallel to the WP axis, the cross product
        is zero and the function returns identity rotation."""
        self.printTestMessage("placeAlongEdge parallel to WP up axis")
        start = Vector(0, 0, 0)
        end = Vector(0, 0, 1000)
        placement = Arch.placeAlongEdge(start, end, horizontal=True)
        identity = App.Rotation()
        self.assertAlmostEqual(
            placement.Rotation.Angle,
            identity.Angle,
            places=5,
            msg="Should be identity rotation when beam is parallel to WP axis",
        )

    def test_placeAlongEdge_coincident_points(self):
        """Coincident points (zero-length edge) should return identity placement."""
        self.printTestMessage("placeAlongEdge coincident points")
        start = Vector(10, 10, 10)
        end = Vector(10, 10, 10)
        placement = Arch.placeAlongEdge(start, end)
        self.assertTrue(placement.Base.isEqual(start, 1e-6))
        self.assertEqual(
            placement.Rotation.Angle, 0.0, "Zero-length edge should result in identity rotation"
        )
