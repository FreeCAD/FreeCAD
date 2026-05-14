# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors
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

"""GUI regressions for footprint display data."""

import Arch
import FreeCAD
from bimtests import TestArchBaseGui


class TestArchFootprintGui(TestArchBaseGui.TestArchBaseGui):

    def test_new_wall_populates_footprint_display_data(self):
        """New walls should populate their footprint nodes on shape update."""

        wall = Arch.makeWall(length=3000, width=200, height=2500)
        self.document.recompute()
        self.pump_gui_events()

        proxy = wall.ViewObject.Proxy
        self.assertIn("Footprint", wall.ViewObject.listDisplayModes())
        self.assertTrue(hasattr(proxy, "fcoords"))
        self.assertTrue(hasattr(proxy, "fset"))
        self.assertGreater(proxy.fcoords.point.getNum(), 0)
        self.assertGreater(proxy.fset.coordIndex.getNum(), 0)

    def test_wall_footprint_display_data_is_local_to_placement(self):
        """Footprint display data should be stored in object-local coordinates."""

        wall = Arch.makeWall(length=3000, width=200, height=2500)
        wall.Placement.Base = FreeCAD.Vector(1234, 5678, 0)
        self.document.recompute()
        self.pump_gui_events()

        points = wall.ViewObject.Proxy.fcoords.point
        xs = []
        ys = []
        for idx in range(points.getNum()):
            point = points[idx]
            xs.append(point[0])
            ys.append(point[1])

        self.assertLess(min(xs), -1000)
        self.assertGreater(max(xs), 1000)
        self.assertLess(min(ys), 0)
        self.assertGreater(max(ys), 0)
        self.assertLess(max(abs(value) for value in xs), 5000)
        self.assertLess(max(abs(value) for value in ys), 500)
