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
import ArchAxis
from bimtests import TestArchBaseGui


class TestArchAxisGui(TestArchBaseGui.TestArchBaseGui):

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

        delta = link.Placement.multiply(axis.Placement.inverse())
        for i, item in enumerate(parent_texts):
            expected = delta.multVec(item[1])
            actual = link_texts[i][1]
            self.assertLess((expected - actual).Length, 1e-6)
