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


class TestArchSchedule(TestArchBase.TestArchBase):

    def test_makeSchedule(self):
        """Test the makeSchedule function."""
        operation = "Testing makeSchedule..."
        self.printTestMessage(operation)

        obj = Arch.makeSchedule()
        self.assertIsNotNone(obj, "makeSchedule failed to create an object")
        self.assertEqual(obj.Label, "Schedule", "Incorrect default label for Schedule")

    def test_wall_schedule_includes_added_wall_components(self):
        """Schedules should count walls merged through Additions."""
        operation = "Testing Schedule wall Additions..."
        self.printTestMessage(operation)

        base_wall = Arch.makeWall(length=4000, name="Base Wall")
        added_wall = Arch.makeWall(length=2000, name="Added Wall")
        base_wall.IfcType = "Wall"
        added_wall.IfcType = "Wall"
        Arch.addComponents(added_wall, host=base_wall)
        self.document.recompute()

        schedule = Arch.makeSchedule()
        schedule.Operation = ["Wall length"]
        schedule.Value = ["Length"]
        schedule.Unit = [""]
        schedule.Objects = [""]
        schedule.Filter = ["Type:Wall"]

        schedule.Proxy.execute(schedule)

        self.assertIn("B2", schedule.Proxy.data)
        self.assertAlmostEqual(float(schedule.Proxy.data["B2"]), 6000.0)
