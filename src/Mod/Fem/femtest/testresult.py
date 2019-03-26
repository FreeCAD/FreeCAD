# ***************************************************************************
# *   Copyright (c) 2018 - FreeCAD Developers                               *
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
from . import utilstest as testtools
from .utilstest import fcc_print

from os.path import join


class TestResult(unittest.TestCase):
    fcc_print('import TestResult')

    def setUp(self):
        self.doc_name = "TestResult"
        try:
            FreeCAD.setActiveDocument(self.doc_name)
        except:
            FreeCAD.newDocument(self.doc_name)
        finally:
            FreeCAD.setActiveDocument(self.doc_name)
        self.active_doc = FreeCAD.ActiveDocument

    def test_read_frd_massflow_networkpressure(self):
        # read data from frd file
        frd_file = join(testtools.get_fem_test_home_dir(), 'ccx', 'Flow1D_thermomech.frd')
        from feminout.importCcxFrdResults import read_frd_result as read_frd
        frd_content = read_frd(frd_file)

        # do something with the read data
        frd_content_len = []
        for key in sorted(frd_content.keys()):
            frd_content_len.append(len(frd_content[key]))
        print('read data')
        print(frd_content_len)
        print(sorted(frd_content.keys()))
        # print(frd_content)
        read_mflow = frd_content['Results'][12]['mflow']
        read_npressure = frd_content['Results'][12]['npressure']
        res_len = [
            len(read_mflow),
            len(read_npressure)
        ]
        print(res_len)
        print(read_mflow)
        print(read_npressure)

        # create the expected data
        print('\nexpected data')
        efc = {}  # expected frd content
        efc['Nodes'] = {
            2: FreeCAD.Vector(0.0, 0.0, -50.0),
            3: FreeCAD.Vector(0.0, 0.0, -4300.0),
            4: FreeCAD.Vector(4950.0, 0.0, -4300.0),
            5: FreeCAD.Vector(5000.0, 0.0, -4300.0),
            6: FreeCAD.Vector(8535.53, 0.0, -7835.53),
            7: FreeCAD.Vector(8569.88, 0.0, -7870.88),
            8: FreeCAD.Vector(12105.4, 0.0, -11406.4),
            9: FreeCAD.Vector(12140.8, 0.0, -11441.8),
            10: FreeCAD.Vector(13908.5, 0.0, -13209.5),
            11: FreeCAD.Vector(13943.9, 0.0, -13244.9),
            12: FreeCAD.Vector(15047.0, 0.0, -14348.0),
            13: FreeCAD.Vector(15047.0, 0.0, -7947.97),
            15: FreeCAD.Vector(0.0, 0.0, 0.0),
            16: FreeCAD.Vector(0.0, 0.0, -2175.0),
            17: FreeCAD.Vector(2475.0, 0.0, -4300.0),
            18: FreeCAD.Vector(4975.0, 0.0, -4300.0),
            19: FreeCAD.Vector(6767.77, 0.0, -6067.77),
            20: FreeCAD.Vector(8552.71, 0.0, -7853.21),
            21: FreeCAD.Vector(10337.6, 0.0, -9638.64),
            22: FreeCAD.Vector(12123.1, 0.0, -11424.1),
            23: FreeCAD.Vector(13024.6, 0.0, -12325.6),
            24: FreeCAD.Vector(13926.2, 0.0, -13227.2),
            25: FreeCAD.Vector(14495.4, 0.0, -13796.4),
            26: FreeCAD.Vector(15047.0, 0.0, -11148.0),
            27: FreeCAD.Vector(15047.0, 0.0, -7897.97)
        }
        efc['Seg2Elem'] = {
            1: (15, 2),
            13: (13, 27)
        }
        efc['Seg3Elem'] = {}
        '''   deleted during reading because of the inout file
        efc['Seg3Elem'] = {
            2: (2, 16, 3),
            3: (3, 17, 4),
            4: (4, 18, 5),
            5: (5, 19, 6),
            6: (6, 20, 7),
            7: (7, 21, 8),
            8: (8, 22, 9),
            9: (9, 23, 10),
            10: (10, 24, 11),
            11: (11, 25, 12),
            12: (12, 26, 13)
        }
        '''
        efc['Tria3Elem'] = efc['Tria6Elem'] = efc['Quad4Elem'] = efc['Quad8Elem'] = {}  # faces
        efc['Tetra4Elem'] = efc['Tetra10Elem'] = efc['Hexa8Elem'] = efc['Hexa20Elem'] = efc['Penta6Elem'] = efc['Penta15Elem'] = {}  # volumes
        efc['Results'] = [
            {'time': 0.00390625},
            {'time': 0.0078125},
            {'time': 0.0136719},
            {'time': 0.0224609},
            {'time': 0.0356445},
            {'time': 0.0554199},
            {'time': 0.085083},
            {'time': 0.129578},
            {'time': 0.19632},
            {'time': 0.296432},
            {'time': 0.446602},
            {'time': 0.671856},
            {
                'number': 0,
                'time': 1.0,
                'mflow': {
                    1: 78.3918,  # added during reading because of the inout file
                    2: 78.3918,
                    3: 78.3918,
                    4: 78.3918,
                    5: 78.3918,
                    6: 78.3918,
                    7: 78.3918,
                    8: 78.3918,
                    9: 78.3918,
                    10: 78.3918,
                    11: 78.3918,
                    12: 78.3918,
                    13: 78.3918,
                    15: 78.3918,
                    16: 78.3918,
                    17: 78.3918,
                    18: 78.3918,
                    19: 78.3918,
                    20: 78.3918,
                    21: 78.3918,
                    22: 78.3918,
                    23: 78.3918,
                    24: 78.3918,
                    25: 78.3918,
                    26: 78.3918,
                    27: 78.3918,
                    28: 78.3918  # added during reading because of the inout file
                },
                'npressure': {
                    1: 0.1,  # added during reading because of the inout file
                    2: 0.1,
                    3: 0.134840,
                    4: 0.128261,
                    5: 0.127949,
                    6: 0.155918,
                    7: 0.157797,
                    8: 0.191647,
                    9: 0.178953,
                    10: 0.180849,
                    11: 0.161476,
                    12: 0.162658,
                    13: 0.1,
                    15: 0.1,
                    16: 0.117420,
                    17: 0.131551,
                    18: 0.128105,
                    19: 0.141934,
                    20: 0.156857,
                    21: 0.174722,
                    22: 0.185300,
                    23: 0.179901,
                    24: 0.171162,
                    25: 0.162067,
                    26: 0.131329,
                    27: 0.1,
                    28: 0.1  # added during reading because of the inout file
                }
            }
        ]
        expected_frd_content = efc

        # do something with the expected data
        expected_frd_content_len = []
        for key in sorted(expected_frd_content.keys()):
            expected_frd_content_len.append(len(expected_frd_content[key]))
        print(expected_frd_content_len)
        print(sorted(expected_frd_content.keys()))
        # expected results
        expected_mflow = expected_frd_content['Results'][12]['mflow']
        expected_npressure = expected_frd_content['Results'][12]['npressure']
        expected_res_len = [
            len(expected_mflow),
            len(expected_npressure)
        ]
        print(expected_res_len)
        print(expected_mflow)
        print(expected_npressure)

        # tests
        self.assertEqual(frd_content_len, expected_frd_content_len, "Length's of read frd data values are unexpected")
        self.assertEqual(frd_content['Nodes'], expected_frd_content['Nodes'], "Values of read node data are unexpected")
        self.assertEqual(frd_content['Seg2Elem'], expected_frd_content['Seg2Elem'], "Values of read Seg2 data are unexpected")
        self.assertEqual(frd_content['Seg3Elem'], expected_frd_content['Seg3Elem'], "Values of read Seg3 data are unexpected")
        self.assertEqual(res_len, expected_res_len, "Length's of read result data values are unexpected")
        self.assertEqual(read_mflow, expected_mflow, "Values of read mflow result data are unexpected")
        self.assertEqual(read_npressure, expected_npressure, "Values of read npressure result data are unexpected")

    def get_stress_values(self):
        # node 5 von calculix cantilver 3D example
        # doc = FreeCAD.open(FreeCAD.ConfigGet("AppHomePath") + 'data/examples/FemCalculixCantilever3D.FCStd')
        # doc.Box_Mesh.FemMesh.Nodes[5]
        # Vector (0.0, 1000.0, 0.0)
        # res = doc.CalculiX_static_results
        # stress = (res.NodeStressXX[4], res.NodeStressYY[4], res.NodeStressZZ[4], res.NodeStressXY[4], res.NodeStressXZ[4],res.NodeStressYZ[4])
        stress = (
            -4.52840E+02,  # Sxx
            -1.94075E+02,  # Syy
            -1.94075E+02,  # Szz
            6.11223E+01,  # Sxy
            -2.60754E+01,  # Sxz
            6.92759E-05  # Syz
        )
        return stress

    def test_stress_von_mises(self):
        expected_mises = 283.2082
        from femresult.resulttools import calculate_von_mises as vm
        mises = vm(self.get_stress_values())
        # fcc_print(round(mises, 4))
        self.assertEqual(round(mises, 4), expected_mises, "Calculated von Mises stress is not the expected value.")

    def test_stress_principal(self):
        expected_principal = (-178.0076, -194.0749, -468.9075, 145.4499)
        from femresult.resulttools import calculate_principal_stress as pr
        prin = pr(self.get_stress_values())
        rounded_prin = (round(prin[0], 4), round(prin[1], 4), round(prin[2], 4), round(prin[3], 4))
        # fcc_print(rounded_prin)
        self.assertEqual(rounded_prin, expected_principal, "Calculated principal stresses are not the expected values.")

    def test_disp_abs(self):
        expected_dispabs = 87.302986
        disp_xyz = [FreeCAD.Vector(8.12900E+00, 3.38889E-02, -8.69237E+01)]  # x, y, z in node 4 of CalculiX cantilver face load
        from femresult.resulttools import calculate_disp_abs as dp
        disp_abs = round(dp(disp_xyz)[0], 6)
        # fcc_print(disp_abs)
        self.assertEqual(disp_abs, expected_dispabs, "Calculated displacement abs are not the expected values.")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc_name)
        pass
