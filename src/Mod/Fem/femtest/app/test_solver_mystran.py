# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "Solver mystran FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import unittest
from os.path import join

import FreeCAD

import femsolver.run
from . import support_utils as testtools
from .support_utils import fcc_print
from .support_utils import get_namefromdef


class TestSolverMystran(unittest.TestCase):
    fcc_print("import TestSolverMystran")

    # ********************************************************************************************
    def setUp(
        self
    ):
        # setUp is executed before every test

        # new document
        self.document = FreeCAD.newDocument(self.__class__.__name__)

        # more inits
        self.pre_dir_name = "solver_mystran_"
        self.ending = ".bdf"
        self.infilename = "Mesh"
        self.test_file_dir = join(
            testtools.get_fem_test_home_dir(),
            "mystran"
        )

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

        fcc_print("\n{0}\n{1} run FEM TestSolverMystran tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            55 * "*"
        ))

    # ********************************************************************************************
    def test_ccx_cantilever_ele_quad4(
        self
    ):
        fcc_print("")
        from femexamples.ccx_cantilever_ele_quad4 import setup
        setup(self.document, "mystran")
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_ccx_cantilever_ele_seg2(
        self
    ):
        fcc_print("")
        from femexamples.ccx_cantilever_ele_seg2 import setup
        setup(self.document, "mystran")
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_ccx_cantilever_ele_tria3(
        self
    ):
        fcc_print("")
        from femexamples.ccx_cantilever_ele_tria3 import setup
        setup(self.document, "mystran")
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_ccx_cantilever_faceload(
        self
    ):
        fcc_print("")
        from femexamples.ccx_cantilever_faceload import setup
        setup(self.document, "mystran")
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_ccx_cantilever_nodeload(
        self
    ):
        fcc_print("")
        from femexamples.ccx_cantilever_nodeload import setup
        setup(self.document, "mystran")
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_mystran_plate(
        self
    ):
        fcc_print("")
        from femexamples.mystran_plate import setup
        setup(self.document, "mystran")
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def input_file_writing_test(
        self,
        base_name
    ):
        self.document.recompute()

        # get analysis working directory and save FreeCAD file
        working_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + base_name)
        save_fc_file = join(working_dir, base_name + ".FCStd")
        # fcc_print("Save FreeCAD file to {} ...".format(save_fc_file))
        self.document.saveAs(save_fc_file)

        # write input file
        machine = self.document.SolverMystran.Proxy.createMachine(
            self.document.SolverMystran,
            working_dir,
            True  # set testmode to True
        )
        machine.target = femsolver.run.PREPARE
        machine.start()
        machine.join()  # wait for the machine to finish

        # compare input file with the given one
        inpfile_given = join(
            self.test_file_dir,
            base_name + self.ending
        )
        inpfile_totest = join(
            working_dir,
            self.infilename + self.ending
        )
        # fcc_print("Comparing {}  to  {}".format(inpfile_given, inpfile_totest))
        ret = testtools.compare_inp_files(
            inpfile_given,
            inpfile_totest
        )
        self.assertFalse(
            ret,
            "Mystran write_solver_input for {0} test failed.\n{1}".format(base_name, ret)
        )
