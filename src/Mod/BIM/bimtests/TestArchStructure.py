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
import Part
from bimtests import TestArchBase


class TestArchStructure(TestArchBase.TestArchBase):

    def testStructure(self):
        App.Console.PrintLog("Checking BIM Structure...\n")
        structure = Arch.makeStructure(length=2, width=3, height=5)
        self.assertTrue(structure, "BIM Structure failed")

    # Dimensions
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

    # IfcType classification
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

    # Auto-Label
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

    # Structural System
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

    # placeAlongEdge: base position
    def test_placeAlongEdge_base_is_p1(self):
        """Placement base is always the first point."""
        self.printTestMessage("placeAlongEdge base == p1")
        start = Vector(100, 200, 300)
        end = Vector(100, 200, 3300)
        placement = Arch.placeAlongEdge(start, end)
        self.assertTrue(
            placement.Base.isEqual(start, Part.Precision.confusion()),
            f"Placement base {placement.Base} should equal start {start}",
        )

    # placeAlongEdge: degenerate cases
    def test_placeAlongEdge_coincident_points(self):
        """Coincident points (zero-length edge) should return identity placement."""
        self.printTestMessage("placeAlongEdge coincident points")
        start = Vector(10, 10, 10)
        end = Vector(10, 10, 10)
        placement = Arch.placeAlongEdge(start, end)
        self.assertTrue(placement.Base.isEqual(start, Part.Precision.confusion()))
        self.assertEqual(
            placement.Rotation.Angle, 0.0, "Zero-length edge should result in identity rotation"
        )

    # placeAlongEdge: orientation permutations
    _WP_ALIGNMENTS = [
        ("top", App.Rotation()),
        ("front", App.Rotation(Vector(1, 0, 0), 90)),
        ("right", App.Rotation(Vector(0, 1, 0), 90)),
        ("45 degrees", App.Rotation(Vector(0, 0, 1), 45)),
    ]

    _BEAM_DIRECTIONS = [
        ("X axis", Vector(0, 0, 0), Vector(1000, 0, 0)),
        ("Y axis", Vector(0, 0, 0), Vector(0, 1000, 0)),
        ("Z axis", Vector(0, 0, 0), Vector(0, 0, 1000)),
        ("XZ tilt", Vector(0, 0, 0), Vector(1000, 0, 300)),
        ("XY diagonal", Vector(0, 0, 0), Vector(1000, 1000, 0)),
        ("3D diagonal", Vector(0, 0, 0), Vector(2000, 2000, 2000)),
    ]

    def _check_beam_orientation(self, wp_rotation, start, end, horizontal, label):
        """Verify axis alignment and absence of roll for non-degenerate edges."""
        import WorkingPlane

        wp = WorkingPlane.PlaneBase()
        wp.align_to_rotation(wp_rotation)
        wp_normal = wp.axis

        placement = Arch.placeAlongEdge(start, end, horizontal, wp=wp)
        rotation = placement.Rotation
        edge_direction = end.sub(start)
        edge_direction.normalize()

        local_x = rotation.multVec(Vector(1, 0, 0))
        local_y = rotation.multVec(Vector(0, 1, 0))
        local_z = rotation.multVec(Vector(0, 0, 1))

        lateral = wp_normal.cross(edge_direction)
        lateral.normalize()

        if horizontal:
            self.assertAlmostEqual(
                abs(local_x.dot(edge_direction)),
                1,
                places=5,
                msg=f"{label}: local X should align with edge direction",
            )
            self.assertAlmostEqual(
                local_z.dot(lateral),
                0,
                places=5,
                msg=f"{label}: local Z should lie in the edge/WP-normal plane (no roll)",
            )
        else:
            self.assertAlmostEqual(
                abs(local_z.dot(edge_direction)),
                1,
                places=5,
                msg=f"{label}: local Z should align with edge direction",
            )
            self.assertAlmostEqual(
                local_y.dot(lateral),
                0,
                places=5,
                msg=f"{label}: local Y should lie in the edge/WP-normal plane (no roll)",
            )

    def test_placeAlongEdge_tilted_beam_cross_section_stays_upright(self):
        """For a tilted beam, the cross-section Y axis must remain perfectly
        horizontal (zero roll).

        A beam from (0,0,0) to (1000,0,300) is tilted in the XZ plane. The
        beam's 'up' (local Y axis) should stay perfectly parallel to the
        global XY plane. Any deviation in the Z component of local Y
        indicates roll.
        """
        import WorkingPlane

        self.printTestMessage("placeAlongEdge tilted beam zero roll")
        start = Vector(0, 0, 0)
        end = Vector(1000, 0, 300)
        wp = WorkingPlane.PlaneBase()
        placement = Arch.placeAlongEdge(start, end, horizontal=False, wp=wp)

        # Transform the local Y axis (0, 1, 0) into global coordinates
        local_y = placement.Rotation.multVec(Vector(0, 1, 0))

        edge_direction = end.sub(start)
        edge_direction.normalize()
        lateral = wp.axis.cross(edge_direction)
        lateral.normalize()

        # The cross-section Y axis must lie in the span/WP-normal plane (no roll).
        self.assertAlmostEqual(
            local_y.dot(lateral),
            0,
            places=5,
            msg=f"Cross-section Y axis {local_y} has roll around the span axis",
        )

    def test_placeAlongEdge_orientation(self):
        """Axis alignment and roll across WP alignments, beam directions, and modes.

        Skips combinations where the edge is parallel to the WP normal; the
        identity-rotation behaviour for that degenerate case is covered by
        ``test_placeAlongEdge_parallel_to_up_returns_identity``.
        """
        import WorkingPlane

        self.printTestMessage("placeAlongEdge orientation permutations")
        for wp_name, wp_rotation in self._WP_ALIGNMENTS:
            wp = WorkingPlane.PlaneBase()
            wp.align_to_rotation(wp_rotation)
            for direction_name, start, end in self._BEAM_DIRECTIONS:
                edge = end.sub(start)
                edge.normalize()
                if wp.axis.cross(edge).Length <= Part.Precision.confusion():
                    continue
                for horizontal in (True, False):
                    mode = "beam" if horizontal else "profile"
                    label = f"{wp_name} WP, {direction_name}, {mode}"
                    with self.subTest(label):
                        self._check_beam_orientation(wp_rotation, start, end, horizontal, label)
