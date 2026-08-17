# #######################################################################
#
#  Copyright (c) 2023 Ondsel Inc.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
# #######################################################################

import unittest
import FreeCAD
from SheetMetalKfactor import KFactorLookupTable


class TestKFactor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        pass

    @classmethod
    def tearDownClass(cls):
        pass

    def test_00(self):

        doc = FreeCAD.newDocument()
        sheet = doc.addObject("Spreadsheet::Sheet", "material_foo")

        sheet.Label = "KFactor"

        sheet.set("A1", "Radius / Thickness")
        sheet.set("B1", "K-factor (ANSI)")
        sheet.set("A2", "1")
        sheet.set("B2", "0.38")
        sheet.set("A3", "3")
        sheet.set("B3", "0.43")
        sheet.set("A4", "99")
        sheet.set("B4", "0.5")
        sheet.recompute()

        c = KFactorLookupTable(sheet.Label)
        self.assertTrue(c.k_factor_lookup[1] == 0.38)
        self.assertTrue(c.k_factor_lookup[3] == 0.43)
        self.assertTrue(c.k_factor_lookup[99] == 0.5)
        self.assertTrue(c.k_factor_standard == "ansi")


if __name__ == "__main__":
    unittest.main()
