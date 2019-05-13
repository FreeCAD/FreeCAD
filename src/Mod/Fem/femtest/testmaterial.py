# ***************************************************************************
# *   Copyright (c) 2019 - FreeCAD Developers                               *
# *   Author: Bernd Hahnebach <bernd@bimstatik.org>                         *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/


import FreeCAD
import unittest
from .utilstest import fcc_print


class TestMaterialUnits(unittest.TestCase):
    fcc_print('import TestMaterialUnits')

    def setUp(self):
        # init, is executed before every test
        self.doc_name = "TestMaterialUnits"
        try:
            FreeCAD.setActiveDocument(self.doc_name)
        except:
            FreeCAD.newDocument(self.doc_name)
        finally:
            FreeCAD.setActiveDocument(self.doc_name)
        self.active_doc = FreeCAD.ActiveDocument

    def test_known_quantity_units(self):
        from materialtools.cardutils import get_known_material_quantity_parameter as knownquant
        known_quantity_parameter = knownquant()
        from materialtools.cardutils import check_parm_unit as checkparamunit
        for param in known_quantity_parameter:
            fcc_print('{}'.format(param))
            self.assertTrue(
                checkparamunit(param),
                'Unit of quantity material parameter {} is not known by FreeCAD unit system.'
                .format(param)
            )

    def tearDown(self):
        # clearance, is executed after every test
        FreeCAD.closeDocument(self.doc_name)
        pass
