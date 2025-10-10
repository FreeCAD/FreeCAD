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

"""GUI tests for the ArchWall module."""

import FreeCAD
import FreeCADGui
import Arch
import Draft
import WorkingPlane
from bimtests import TestArchBaseGui
from bimcommands.BimWall import Arch_Wall


class TestArchWallGui(TestArchBaseGui.TestArchBaseGui):

    def setUp(self):
        """Set up the test environment by activating the BIM workbench and setting preferences."""
        super().setUp()
        self.params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")
        self.original_wall_base = self.params.GetInt("WallBaseline", 1)  # Default to 1 (line)

    def tearDown(self):
        """Restore original preferences after the test."""
        self.params.SetInt("WallBaseline", self.original_wall_base)
        super().tearDown()

    def test_create_baseless_wall_interactive_mode(self):
        """
        Tests the interactive creation of a baseless wall by simulating the
        Arch_Wall command's internal logic.
        """
        from draftguitools import gui_trackers # Import the tracker module

        self.printTestMessage("Testing interactive creation of a baseless wall...")

        # 1. Arrange: Set preference to "No baseline" mode
        self.params.SetInt("WallBaseline", 0)

        # 2. Arrange: Simulate the state of the command after two clicks
        cmd = Arch_Wall()
        cmd.doc = self.document
        cmd.wp = WorkingPlane.get_working_plane()
        cmd.points = [FreeCAD.Vector(1000, 1000, 0), FreeCAD.Vector(3000, 1000, 0)]
        cmd.Align = "Center"
        cmd.Width = 200.0
        cmd.Height = 2500.0
        cmd.MultiMat = None
        cmd.existing = []
        cmd.tracker = gui_trackers.boxTracker()

        initial_object_count = len(self.document.Objects)

        # 3. Act: Call the internal method that processes the points
        cmd.create_wall()

        # 4. Assert
        self.assertEqual(len(self.document.Objects), initial_object_count + 1,
                         "Exactly one new object should have been created.")

        wall = self.document.Objects[-1]
        self.assertEqual(Draft.get_type(wall), "Wall", "The created object is not a wall.")

        self.assertIsNone(wall.Base, "A baseless wall should have its Base property set to None.")

        self.assertAlmostEqual(wall.Length.Value, 2000.0, delta=1e-6,
                               msg="Wall length is incorrect.")

        # Verify the placement is correct
        expected_center = FreeCAD.Vector(2000, 1000, 0)
        self.assertTrue(wall.Placement.Base.isEqual(expected_center, 1e-6),
                        f"Wall center {wall.Placement.Base} does not match expected {expected_center}")

        # Verify the rotation is correct (aligned with global X-axis, so no rotation)
        self.assertAlmostEqual(wall.Placement.Rotation.Angle, 0.0, delta=1e-6,
                               msg="Wall rotation should be zero for a horizontal line.")

