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
import Arch
import ArchAxis
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

    @unittest.skipIf(not App.GuiUp, "Requires GUI viewproviders for Axis bubble data")
    def test_axis_bubble_data_link_parity(self):
        axis = Arch.makeAxis(num=2, size=1500)
        self.document.recompute()

        link = self.document.addObject("App::Link", "AxisLink")
        link.LinkedObject = axis
        link.LinkTransform = True
        link.Placement.Base = App.Vector(1000, 2000, 0)
        self.document.recompute()

        parent_shapes, parent_texts = ArchAxis.get_axis_bubble_data(axis, axis.ViewObject)
        link_shapes, link_texts = ArchAxis.get_axis_bubble_data(link, axis.ViewObject)

        self.assertEqual(len(parent_shapes), len(link_shapes))
        self.assertEqual([t[0] for t in parent_texts], [t[0] for t in link_texts])

        delta = link.getGlobalPlacement().multiply(axis.getGlobalPlacement().inverse())
        for i, item in enumerate(parent_texts):
            expected = delta.multVec(item[1])
            actual = link_texts[i][1]
            self.assertLess((expected - actual).Length, 1e-6)
