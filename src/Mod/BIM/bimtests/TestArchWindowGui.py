# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
# SPDX-FileCopyrightText: 2025 Furgo
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

import FreeCAD as App
import Arch
import ArchSectionPlane
import Draft
from bimtests import TestArchBaseGui


class TestArchWindowGui(TestArchBaseGui.TestArchBaseGui):

    def test_change_window_opening(self):
        """Tests if changes to a window opening touches the window's chain of hosts"""

        # Create a wall, a window, a level and a section.
        points = [App.Vector(0.0, 0.0, 0.0), App.Vector(2000.0, 0.0, 0.0)]
        line = Draft.make_wire(points)
        wall = Arch.makeWall(line, height=2000)
        wpl = App.Placement(App.Vector(500, 0, 1500), App.Vector(1, 0, 0), -90)
        win = Arch.makeWindowPreset(
            "Open 1-pane",
            width=1000.0,
            height=1000.0,
            h1=50.0,
            h2=50.0,
            h3=50.0,
            w1=100.0,
            w2=50.0,
            o1=0.0,
            o2=50.0,
            placement=wpl,
        )
        win.Hosts = [wall]
        level = Arch.makeFloor()
        level.addObject(wall)
        section = Arch.makeSectionPlane(level)
        App.ActiveDocument.recompute()

        # Change opening from 0 to 50 (= 45 degrees):
        svg = ArchSectionPlane.getSVG(section)
        win.Opening = 50
        App.ActiveDocument.recompute()
        svg_new = ArchSectionPlane.getSVG(section)
        self.assertNotEqual(svg, svg_new)

        # Invert opening:
        svg = svg_new
        win.ViewObject.Proxy.invertOpening()
        App.ActiveDocument.recompute()
        svg_new = ArchSectionPlane.getSVG(section)
        self.assertNotEqual(svg, svg_new)

        # Invert hinge:
        svg = svg_new
        win.ViewObject.Proxy.invertHinge()
        App.ActiveDocument.recompute()
        svg_new = ArchSectionPlane.getSVG(section)
        self.assertNotEqual(svg, svg_new)
