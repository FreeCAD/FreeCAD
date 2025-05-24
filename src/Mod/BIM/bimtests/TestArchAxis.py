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

import FreeCAD as App
import Arch
from bimtests import TestArchBase

class TestArchAxis(TestArchBase.TestArchBase):

    def test_make_axis_default(self):
        axis = Arch.makeAxis()
        self.assertIsNotNone(axis, "Failed to create a default axis")

    def test_make_axis_custom(self):
        axis = Arch.makeAxis(num=3, size=2000)
        self.assertEqual(len(axis.Distances), 3, "Incorrect number of axes created")
        self.assertEqual(axis.Distances[1], 2000, "Axis size is incorrect")

    def test_axis_properties(self):
        axis = Arch.makeAxis()
        self.assertEqual(axis.Label, "Axes", "Default label is incorrect")

    def test_makeAxis(self):
        """Test the makeAxis function."""
        operation = "Testing makeAxis function"
        self.printTestMessage(operation)

        axis = Arch.makeAxis(num=2, size=500)
        self.assertIsNotNone(axis, "makeAxis failed to create an axis object.")
        self.assertEqual(axis.Label, "Axes", "Axis label is incorrect.")

    def test_makeAxisSystem(self):
        """Test the makeAxisSystem function."""
        operation = "Testing makeAxisSystem function"
        self.printTestMessage(operation)

        axis1 = Arch.makeAxis(num=1, size=1000)
        axis2 = Arch.makeAxis(num=1, size=2000)
        axis_system = Arch.makeAxisSystem([axis1, axis2], name="TestAxisSystem")
        self.assertIsNotNone(axis_system, "makeAxisSystem failed to create an axis system.")
        self.assertEqual(axis_system.Label, "TestAxisSystem", "Axis system label is incorrect.")