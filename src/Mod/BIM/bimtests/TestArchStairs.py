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

import Arch
from bimtests import TestArchBase

class TestArchStairs(TestArchBase.TestArchBase):

    def test_makeStairs(self):
        """Test the makeStairs function."""
        operation = "Testing makeStairs function"
        self.printTestMessage(operation)

        stairs = Arch.makeStairs(length=5000, width=1000, height=3000, steps=10, name="TestStairs")
        self.assertIsNotNone(stairs, "makeStairs failed to create a stairs object.")
        self.assertEqual(stairs.Label, "TestStairs", "Stairs label is incorrect.")

    def test_makeRailing(self):
        """Test the makeRailing function."""
        operation = "Testing makeRailing..."
        self.printTestMessage(operation)

        stairs = Arch.makeStairs(length=5000, width=1000, height=3000, steps=10, name="TestStairs")
        self.assertIsNotNone(stairs, "makeStairs failed to create a stairs object.")

        # Pass stairs as a list to makeRailing
        obj = Arch.makeRailing([stairs])
        self.assertIsNotNone(obj, "makeRailing failed to create an object")
        self.assertEqual(obj.Label, "Railing", "Incorrect default label for Railing")

    def test_makeRailing(self):
        """Test the makeRailing function."""
        operation = "Testing makeRailing..."
        self.printTestMessage(operation)

        # Create stairs
        stairs = Arch.makeStairs(width=800, height=2500, length=3500, steps=14)
        self.document.recompute()

        # Get object names before creation
        pre_creation_names = {obj.Name for obj in self.document.Objects}

        # Create railings
        Arch.makeRailing([stairs])
        self.document.recompute()

        # Find new railings by name comparison and type checking
        new_railings = [
            obj for obj in self.document.Objects
            if obj.Name not in pre_creation_names
            and hasattr(obj, "Proxy")
            and getattr(obj.Proxy, "Type", "") == "Pipe"
        ]

        # Should create exactly 2 new railing objects
        self.assertEqual(len(new_railings), 2)

        # Verify properties exist
        for railing in new_railings:
            self.assertTrue(hasattr(railing, "Height"))
            self.assertTrue(hasattr(railing, "Diameter"))
            self.assertTrue(hasattr(railing, "Placement"))