# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "Material FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import unittest
from os.path import join

import FreeCAD

from .support_utils import fcc_print


class TestMaterialUnits(unittest.TestCase):
    fcc_print("import TestMaterialUnits")

    # ********************************************************************************************
    def setUp(
        self
    ):
        # setUp is executed before every test

        # new document
        self.document = FreeCAD.newDocument(self.__class__.__name__)

    # ********************************************************************************************
    def tearDown(
        self
    ):
        # tearDown is executed after every test
        FreeCAD.closeDocument(self.document.Name)

    # ********************************************************************************************
    def test_00print(
        self
    ):
        # since method name starts with 00 this will be run first
        # this test just prints a line with stars

        fcc_print("\n{0}\n{1} run FEM TestMaterialUnits tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            57 * "*"
        ))

    # ********************************************************************************************
    def test_known_quantity_units(
        self
    ):
        from materialtools.cardutils import get_known_material_quantity_parameter as knownquant
        known_quantity_parameter = knownquant()
        from materialtools.cardutils import check_parm_unit as checkparamunit
        for param in known_quantity_parameter:
            fcc_print("{}".format(param))
            self.assertTrue(
                checkparamunit(param),
                "Unit of quantity material parameter {} "
                "is unknown to the FreeCAD unit system."
                .format(param)
            )

    # ********************************************************************************************
    def test_material_card_quantities(
        self
    ):
        # test the value and unit of known quantity parameter
        # from solid build in material cards
        # keep in mind only if FreeCAD is installed all materials are copied
        # TODO Fluid materials (are they installed?)

        # get build in materials
        builtin_solid_mat_dir = join(
            FreeCAD.getResourceDir(),
            "Mod",
            "Material",
            "StandardMaterial"
        )
        fcc_print("{}".format(builtin_solid_mat_dir))
        from materialtools.cardutils import add_cards_from_a_dir as addmats
        materials, cards, icons = addmats({}, {}, {}, builtin_solid_mat_dir, "")

        # get known material quantity parameter
        from materialtools.cardutils import get_known_material_quantity_parameter as knownquant
        known_quantities = knownquant()

        # check param, value pairs
        from materialtools.cardutils import check_value_unit as checkvalueunit
        for mat in materials:
            fcc_print("{}".format(mat))
            for param, value in materials[mat].items():
                if param in known_quantities:
                    # fcc_print("    {} --> {}".format(param, value))
                    self.assertTrue(
                        checkvalueunit(param, value),
                        "Unit of quantity {} from material parameter {} is wrong."
                        .format(value, param)
                    )
