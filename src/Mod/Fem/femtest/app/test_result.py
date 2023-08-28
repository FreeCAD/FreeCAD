# ***************************************************************************
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "Results FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import unittest
from os.path import join

import FreeCAD

from . import support_utils as testtools
from .support_utils import fcc_print


class TestResult(unittest.TestCase):
    fcc_print("import TestResult")

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
        fcc_print("\n{0}\n{1} run FEM TestResult tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            64 * "*"
        ))

    # ********************************************************************************************
    def get_stress_values(
        self
    ):
        # node 5 von calculix cantilver 3D example
        # doc = FreeCAD.open(
        #     FreeCAD.ConfigGet("AppHomePath") + "data/examples/FemCalculixCantilever3D.FCStd"
        # )
        # doc.Box_Mesh.FemMesh.Nodes[5]
        # Vector (0.0, 1000.0, 0.0)
        # res = doc.CalculiX_static_results
        # stress = (
        #     res.NodeStressXX[4],
        #     res.NodeStressYY[4],
        #     res.NodeStressZZ[4],
        #     res.NodeStressXY[4],
        #     res.NodeStressXZ[4],
        #     res.NodeStressYZ[4]
        # )
        stress = (
            -4.52840E+02,  # Sxx
            -1.94075E+02,  # Syy
            -1.94075E+02,  # Szz
            6.11223E+01,  # Sxy
            -2.60754E+01,  # Sxz
            6.92759E-05  # Syz
        )
        return stress

    # ********************************************************************************************
    def test_stress_von_mises(
        self
    ):
        expected_mises = 283.2082
        from femresult.resulttools import calculate_von_mises as vm
        mises = vm(self.get_stress_values())
        # fcc_print(round(mises, 4))
        self.assertEqual(
            round(mises, 4),
            expected_mises,
            "Calculated von Mises stress is not the expected value."
        )

    # ********************************************************************************************
    def test_stress_principal_std(
        self
    ):
        expected_principal = (-178.0076, -194.0749, -468.9075, 145.4499)
        from femresult.resulttools import calculate_principal_stress_std as pr
        prin = pr(self.get_stress_values())
        rounded_prin = (
            round(prin[0], 4),
            round(prin[1], 4),
            round(prin[2], 4),
            round(prin[3], 4)
        )
        # fcc_print(rounded_prin)
        self.assertEqual(
            rounded_prin,
            expected_principal,
            "Calculated principal stresses are not the expected values."
        )

    # ********************************************************************************************
    def test_stress_principal_reinforced(
        self
    ):
        expected_principal = (-178.0076, -194.0749, -468.9075, 145.4499)
        from femresult.resulttools import calculate_principal_stress_reinforced as prrc
        prin = prrc(self.get_stress_values())
        rounded_prin = (
            round(prin[0], 4),
            round(prin[1], 4),
            round(prin[2], 4),
            round(prin[3], 4))
        # fcc_print(rounded_prin)
        self.assertEqual(
            rounded_prin,
            expected_principal,
            "Calculated principal reinforced stresses are not the expected values."
        )

    # ********************************************************************************************
    def test_rho(
        self
    ):
        data = (
            (
                # Case1: Governing Eq.14
                (2.000, -2.000, 5.000, 6.000, -4.000, 2.000),
                (0.02400, 0.00400, 0.01400)
            ),
            (
                # Case2: Governing Eq.10+
                (-3.000, -7.000, 0.000, 6.000, -4.000, 2.000),
                (0.00886, 0.00000, 0.00571),
            ),
            (
                # Case3: Governing Eq.5
                (-1.000, -7.000, 10.000, 0.000, 0.000, 5.000),
                (0.00000, 0.00000, 0.02714)
            ),
            (
                # Case4: Governing Eq.13
                (3.000, 0.000, 10.000, 0.000, 5.000, 0.000),
                (0.01600, 0.00000, 0.03000)
            ),
            (
                # Case5: Governing Eq.11-
                (10.000, 7.000, -3.000, 3.000, 1.000, -2.000),
                (0.02533, 0.02133, 0.00000)
            ),
            (
                # Case6: Governing Eq.14
                (4.000, -7.000, 3.000, 7.000, 0.000, -5.000),
                (0.02200, 0.01000, 0.01600)
            ),
            (
                # Case7: Governing Eq.14
                (8.000, -14.000, 6.000, 14.000, 0.000, -10.000),
                (0.04400, 0.02000, 0.03200)
            ),
            (
                # Case8: Governing Eq.17
                (1.000, 0.000, 3.000, 10.000, -8.000, 7.000),
                (0.02486, 0.01750, 0.01720)
            ),
            (
                # Case9: Governing Eq.13
                (0.000, 0.000, 0.000, 10.000, 8.000, 7.000),
                (0.03600, 0.03400, 0.03000)
            ),
            (
                # Case10: Governing Eq.13
                (15.000, 0.000, 0.000, 0.000, 0.000, 0.000),
                (0.03000, 0.00000, 0.00000)
            ),
            (
                # Case11: Governing Eq.13
                (0.000, 0.000, 0.000, 5.000, 0.000, 0.000),
                (0.01000, 0.01000, 0.00000)
            )
        )

        from femresult.resulttools import calculate_rho as calrho
        for i, case in enumerate(data):
            res = calrho(case[0], 500)
            rhores = (
                round(res[0], 5),
                round(res[1], 5),
                round(res[2], 5)
            )
            # fcc_print("Case{}: {}".format(i + 1 , rhores))
            self.assertEqual(
                rhores, case[1],
                "Calculated rho are not the expected Case{}."
                .format(i + 1)
            )

    # ********************************************************************************************
    def test_disp_abs(
        self
    ):
        expected_dispabs = 87.302986
        # x, y, z in node 4 of CalculiX cantilver face load
        disp_xyz = [FreeCAD.Vector(8.12900E+00, 3.38889E-02, -8.69237E+01)]
        from femresult.resulttools import calculate_disp_abs as dp
        disp_abs = round(dp(disp_xyz)[0], 6)
        # fcc_print(disp_abs)
        self.assertEqual(
            disp_abs,
            expected_dispabs,
            "Calculated displacement abs are not the expected values."
        )
