# -*- coding: utf-8 -*-
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
import FreeCAD as App
import Part
from SheetMetalBendSolid import (
    get_point_on_cylinder,
    wrap_bspline,
    wrap_face,
    bend_solid,
)
from FreeCAD import Vector


class TestFolder(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Setup code that runs before all tests start.
        # This could include creating a FreeCAD document.
        cls.doc = App.newDocument()

    def test_get_point_on_cylinder(self):
        # test data generated from a known working model.  Not otherwise
        # validated

        data = [
            {
                "zero_vert": Vector(631.5394914952352, 377.0, 3.17),
                "point": Vector(631.5394914952352, -123.00000000000028, 3.17),
                "radius": 2.585,
                "center": Vector(631.5394914952352, 377.0, 4.17),
                "axis": Vector(0.0, -1.0, 0.0),
                "zero_vert_normal": Vector(-1.0, -0.0, 0.0),
                "expected": Vector(
                    631.875387629876, -123.00000000000028, 3.2281009678668093
                ),
            },
            {
                "zero_vert": Vector(631.5394914952352, 377.0, 3.17),
                "point": Vector(631.5394914952352, -123.00000000000028, 3.17),
                "radius": 2.585,
                "center": Vector(631.5394914952352, 377.0, 4.17),
                "axis": Vector(0.0, -1.0, 0.0),
                "zero_vert_normal": Vector(-1.0, -0.0, 0.0),
                "expected": Vector(
                    632.3567723938988, -123.00000000000034, 3.5937605248860045
                ),
            },
            {
                "zero_vert": Vector(631.5394914952352, 377.0, 3.17),
                "point": Vector(631.5394914952352, -123.00000000000028, 3.17),
                "radius": 2.585,
                "center": Vector(631.5394914952352, 377.0, 4.17),
                "axis": Vector(0.0, -1.0, 0.0),
                "zero_vert_normal": Vector(-1.0, -0.0, 0.0),
                "expected": Vector(
                    632.5394914952352, -123.00000000000034, 4.170000000000002
                ),
            },
            {
                "zero_vert": Vector(631.5394914952352, 377.0, 3.17),
                "point": Vector(631.5394914952352, -123.0, 3.17),
                "radius": 2.585,
                "center": Vector(631.5394914952352, 377.0, 4.17),
                "axis": Vector(0.0, -1.0, 0.0),
                "zero_vert_normal": Vector(-1.0, -0.0, 0.0),
                "expected": Vector(631.8753876298758, -123.0, 3.2281009678667942),
            },
            {
                "zero_vert": Vector(631.5394914952352, 377.0, 3.17),
                "point": Vector(631.5394914952352, -123.0, 3.17),
                "radius": 2.585,
                "center": Vector(631.5394914952352, 377.0, 4.17),
                "axis": Vector(0.0, -1.0, 0.0),
                "zero_vert_normal": Vector(-1.0, -0.0, 0.0),
                "expected": Vector(631.709933520374, -123.0, 3.184632293980257),
            },
            {
                "zero_vert": Vector(631.5394914952352, 377.0, 3.17),
                "point": Vector(631.5394914952352, -123.0, 3.17),
                "radius": 2.585,
                "center": Vector(631.5394914952352, 377.0, 4.17),
                "axis": Vector(0.0, -1.0, 0.0),
                "zero_vert_normal": Vector(-1.0, -0.0, 0.0),
                "expected": Vector(631.5394914952352, -123.0, 3.17),
            },
            {
                "zero_vert": Vector(631.5394914952352, 377.0, 3.17),
                "point": Vector(631.5394914952352, 377.0, 3.17),
                "radius": 2.585,
                "center": Vector(631.5394914952352, 377.0, 4.17),
                "axis": Vector(0.0, -1.0, 0.0),
                "zero_vert_normal": Vector(-1.0, -0.0, 0.0),
                "expected": Vector(631.5394914952352, 377.0, 3.17),
            },
        ]

        for d in data:
            result = get_point_on_cylinder(
                d["zero_vert"],
                d["point"],
                d["radius"],
                d["center"],
                d["axis"],
                d["zero_vert_normal"],
            )
            d["expected"] = result

            self.assertEqual(result, d["expected"])

    @classmethod
    def tearDownClass(cls):
        # Cleanup code that runs after all tests are complete.
        # This could include closing the FreeCAD document.
        App.closeDocument(cls.doc.Name)


if __name__ == "__main__":
    unittest.main()
