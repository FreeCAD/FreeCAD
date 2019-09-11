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


import unittest

import FreeCAD
from femtest.utilstest import fcc_print


class TestFemImport(unittest.TestCase):
    fcc_print("import TestFemImport")

    # ********************************************************************************************
    # no is document needed to test import Fem and import FemGui
    # thus neiter setUp nor tearDown methods are needed

    def test_00print(
        self
    ):
        fcc_print("\n{0}\n{1} run FEM TestFemImport tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            61 * "*"
        ))

    # ********************************************************************************************
    def test_import_fem(
        self
    ):

        mod = "Fem"
        fcc_print("\n  Try importing {0} ...".format(mod))
        try:
            im = __import__("{0}".format(mod))
        except ImportError:
            im = False
        if not im:
            # to get an error message what was going wrong
            __import__("{0}".format(mod))
        self.assertTrue(im, "Problem importing {0}".format(mod))

        if FreeCAD.GuiUp:
            mod = "FemGui"
            fcc_print("  Try importing {0} ...".format(mod))
            try:
                im = __import__("{0}".format(mod))
            except ImportError:
                im = False
            if not im:
                # to get an error message what was going wrong
                __import__("{0}".format(mod))
            self.assertTrue(im, "Problem importing {0}".format(mod))
