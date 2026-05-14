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

"""GUI tests for slab footprint display."""

import Arch
import Draft
from bimtests import TestArchBaseGui


class TestArchStructureGui(TestArchBaseGui.TestArchBaseGui):

    def test_structure_footprint_mode_remains_slab_only(self):
        """Structure footprints should only be exposed while the object is a slab."""

        slab = Arch.makeStructure(length=3000, width=2000, height=250, name="TestSlab")
        self.document.recompute()
        self.pump_gui_events()

        self.assertNotIn("Footprint", slab.ViewObject.listDisplayModes())

        slab.IfcType = "Slab"
        self.document.recompute()
        self.pump_gui_events()

        self.assertIn("Footprint", slab.ViewObject.listDisplayModes())
        slab.ViewObject.DisplayMode = "Footprint"
        self.pump_gui_events()

        slab.IfcType = "Beam"
        self.document.recompute()
        self.pump_gui_events()

        self.assertNotIn("Footprint", slab.ViewObject.listDisplayModes())
        self.assertNotEqual(slab.ViewObject.DisplayMode, "Footprint")

    def test_slab_populates_footprint_display_data(self):
        """Slabs should expose populated footprint data in the GUI view provider."""

        rect = Draft.makeRectangle(length=4000, height=3000)
        slab = Arch.makeStructure(rect, height=200, name="GuiSlab")
        slab.IfcType = "Slab"
        self.document.recompute()
        self.pump_gui_events()

        proxy = slab.ViewObject.Proxy
        self.assertIn("Footprint", slab.ViewObject.listDisplayModes())
        self.assertTrue(hasattr(proxy, "fcoords"))
        self.assertTrue(hasattr(proxy, "fset"))
        self.assertGreater(proxy.fcoords.point.getNum(), 0)
        self.assertGreater(proxy.fset.coordIndex.getNum(), 0)
