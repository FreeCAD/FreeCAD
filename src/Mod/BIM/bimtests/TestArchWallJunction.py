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

"""App-level tests for BIM wall junction relations."""

import Arch
import FreeCAD as App

from bimtests import TestArchBase


class TestArchWallJunction(TestArchBase.TestArchBase):
    def _make_baseless_wall_between(self, p1, p2, width=200.0, height=1500.0):
        line_vector = p2.sub(p1)
        wall = Arch.makeWall(length=line_vector.Length, width=width, height=height)
        wall.Placement = App.Placement(
            (p1 + p2) * 0.5,
            App.Rotation(App.Vector(1, 0, 0), line_vector.normalize()),
        )
        self.document.recompute()
        return wall

    def test_make_wall_junction_trims_branch_walls_directly(self):
        self.printTestMessage("Testing makeWallJunction trims branch walls directly...")

        carrier_wall = self._make_baseless_wall_between(
            App.Vector(-1000, 0, 0), App.Vector(1000, 0, 0)
        )
        branch_up = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        branch_down = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, -1000, 0))
        carrier_volume = carrier_wall.Shape.Volume
        branch_up_volume = branch_up.Shape.Volume
        branch_down_volume = branch_down.Shape.Volume

        junction = Arch.makeWallJunction([carrier_wall, branch_up, branch_down])
        self.document.recompute()

        self.assertIsNotNone(junction)
        self.assertEqual(junction.Status, "OK")
        self.assertEqual(junction.ResolvedCarrierWall, carrier_wall)
        self.assertEqual(
            {wall.Name for wall in junction.ResolvedBranchWalls},
            {branch_up.Name, branch_down.Name},
        )
        self.assertAlmostEqual(junction.Intersection.x, 0.0, delta=1e-6)
        self.assertAlmostEqual(junction.Intersection.y, 0.0, delta=1e-6)
        self.assertAlmostEqual(carrier_wall.Shape.Volume, carrier_volume, delta=1e-6)
        self.assertLess(branch_up.Shape.Volume, branch_up_volume)
        self.assertLess(branch_down.Shape.Volume, branch_down_volume)
        self.assertFalse(
            any(
                getattr(getattr(obj, "Proxy", None), "Type", None) == "WallJoint"
                for obj in self.document.Objects
            ),
            "Wall junctions should trim walls directly instead of synthesizing hidden wall joints.",
        )

    def test_wall_junction_updates_when_cluster_breaks(self):
        self.printTestMessage(
            "Testing wall junction recomputes when the cluster becomes invalid..."
        )

        carrier_wall = self._make_baseless_wall_between(
            App.Vector(-1000, 0, 0), App.Vector(1000, 0, 0)
        )
        branch_up = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        branch_down = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, -1000, 0))
        branch_up_volume = branch_up.Shape.Volume
        branch_down_volume = branch_down.Shape.Volume

        junction = Arch.makeWallJunction([carrier_wall, branch_up, branch_down])
        self.document.recompute()
        self.assertEqual(junction.Status, "OK")

        branch_down.Placement.move(App.Vector(200, 0, 0))
        self.document.recompute()

        self.assertNotEqual(junction.Status, "OK")
        self.assertIn(junction.Status, ("NoIntersection", "UnsupportedTopology"))
        self.assertAlmostEqual(branch_up.Shape.Volume, branch_up_volume, delta=1e-6)
        self.assertAlmostEqual(branch_down.Shape.Volume, branch_down_volume, delta=1e-6)

    def test_wall_junction_rejects_unsupported_cross_topology(self):
        self.printTestMessage("Testing wall junction rejects unsupported cross topology...")

        horizontal = self._make_baseless_wall_between(
            App.Vector(-1000, 0, 0), App.Vector(1000, 0, 0)
        )
        vertical = self._make_baseless_wall_between(App.Vector(0, -1000, 0), App.Vector(0, 1000, 0))
        diagonal = self._make_baseless_wall_between(
            App.Vector(-1000, -1000, 0), App.Vector(1000, 1000, 0)
        )

        junction = Arch.makeWallJunction([horizontal, vertical, diagonal])
        self.document.recompute()

        self.assertEqual(junction.Status, "UnsupportedTopology")
        self.assertFalse(
            any(
                getattr(getattr(obj, "Proxy", None), "Type", None) == "WallJoint"
                for obj in self.document.Objects
            ),
            "Unsupported junction topologies should not create hidden wall joints.",
        )

    def test_wall_junction_rejects_intersection_requiring_extension(self):
        self.printTestMessage("Testing junctions that would require wall extension...")

        horizontal = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(1000, 0, 0))
        vertical = self._make_baseless_wall_between(
            App.Vector(2000, -1000, 0), App.Vector(2000, 1000, 0)
        )
        diagonal = self._make_baseless_wall_between(
            App.Vector(1500, -500, 0), App.Vector(2500, 500, 0)
        )
        horizontal_volume = horizontal.Shape.Volume
        vertical_volume = vertical.Shape.Volume
        diagonal_volume = diagonal.Shape.Volume

        junction = Arch.makeWallJunction([horizontal, vertical, diagonal])
        self.document.recompute()

        self.assertEqual(junction.Status, "RequiresExtension")
        self.assertIn("extending walls is not supported", junction.StatusMessage)
        self.assertAlmostEqual(horizontal.Shape.Volume, horizontal_volume, delta=1e-6)
        self.assertAlmostEqual(vertical.Shape.Volume, vertical_volume, delta=1e-6)
        self.assertAlmostEqual(diagonal.Shape.Volume, diagonal_volume, delta=1e-6)

    def test_deleting_wall_junction_restores_walls_without_child_relations(self):
        self.printTestMessage("Testing deleting a wall junction restores the walls directly...")

        carrier_wall = self._make_baseless_wall_between(
            App.Vector(-1000, 0, 0), App.Vector(1000, 0, 0)
        )
        branch_up = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        branch_down = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, -1000, 0))
        branch_up_volume = branch_up.Shape.Volume
        branch_down_volume = branch_down.Shape.Volume

        junction = Arch.makeWallJunction([carrier_wall, branch_up, branch_down])
        self.document.recompute()
        self.assertEqual(junction.Status, "OK")
        self.assertFalse(
            any(
                getattr(getattr(obj, "Proxy", None), "Type", None) == "WallJoint"
                for obj in self.document.Objects
            ),
            "Wall junctions should not create hidden wall joints.",
        )

        self.document.removeObject(junction.Name)
        self.document.recompute()

        self.assertAlmostEqual(branch_up.Shape.Volume, branch_up_volume, delta=1e-6)
        self.assertAlmostEqual(branch_down.Shape.Volume, branch_down_volume, delta=1e-6)

    def test_wall_junction_conflict_reports_blocking_joint(self):
        self.printTestMessage("Testing wall junction conflict reporting against a wall joint...")

        carrier_wall = self._make_baseless_wall_between(
            App.Vector(-1000, 0, 0), App.Vector(1000, 0, 0)
        )
        branch_up = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        branch_down = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, -1000, 0))
        branch_down_volume = branch_down.Shape.Volume

        blocker = Arch.makeWallJoint(branch_up, carrier_wall, "Tee")
        blocker.TeeStem = "WallA"
        self.document.recompute()
        self.assertEqual(blocker.Status, "OK")

        junction = Arch.makeWallJunction(
            [carrier_wall, branch_up, branch_down], carrier_wall=carrier_wall
        )
        self.document.recompute()

        self.assertEqual(junction.Status, "Conflict")
        self.assertEqual(junction.ResolvedCarrierWall, carrier_wall)
        self.assertIn(branch_up, junction.ConflictWalls)
        self.assertIn(blocker.Label, junction.ConflictRelationLabels)
        self.assertTrue(any("Start" in message for message in junction.ConflictMessages))
        self.assertTrue(any(blocker.Label in message for message in junction.ConflictMessages))
        self.assertAlmostEqual(
            branch_down.Shape.Volume,
            branch_down_volume,
            delta=1e-6,
            msg="A conflicted junction must not partially trim uncontested branch walls.",
        )

        self.document.removeObject(blocker.Name)
        self.document.recompute()

        self.assertEqual(junction.Status, "OK")
        self.assertEqual(junction.ConflictRelationLabels, [])
        self.assertEqual(junction.ConflictMessages, [])

    def test_wall_junction_conflict_reports_blocking_junction(self):
        self.printTestMessage(
            "Testing wall junction conflict reporting against another junction..."
        )

        carrier_a = self._make_baseless_wall_between(
            App.Vector(-1000, 0, 0), App.Vector(1000, 0, 0)
        )
        branch_up = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, 1000, 0))
        branch_down = self._make_baseless_wall_between(App.Vector(0, 0, 0), App.Vector(0, -1000, 0))

        blocker = Arch.makeWallJunction([carrier_a, branch_up, branch_down], carrier_wall=carrier_a)
        self.document.recompute()
        self.assertEqual(blocker.Status, "OK")

        carrier_b = self._make_baseless_wall_between(
            App.Vector(-1000, -200, 0), App.Vector(1000, 200, 0)
        )
        branch_diag = self._make_baseless_wall_between(
            App.Vector(0, 0, 0), App.Vector(700, -700, 0)
        )

        conflicted = Arch.makeWallJunction(
            [carrier_b, branch_up, branch_diag], carrier_wall=carrier_b
        )
        self.document.recompute()

        self.assertEqual(blocker.Status, "OK")
        self.assertEqual(conflicted.Status, "Conflict")
        self.assertIn(branch_up, conflicted.ConflictWalls)
        self.assertIn(blocker.Label, conflicted.ConflictRelationLabels)
        self.assertTrue(any(blocker.Label in message for message in conflicted.ConflictMessages))
