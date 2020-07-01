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

__title__ = "Solver calculix FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import unittest
from os.path import join

import FreeCAD

import femsolver.run
from . import support_utils as testtools
from .support_utils import fcc_print


class TestSolverCalculix(unittest.TestCase):
    fcc_print("import TestSolverCalculix")

    # ********************************************************************************************
    def setUp(
        self
    ):
        # setUp is executed before every test

        # new document
        self.document = FreeCAD.newDocument(self.__class__.__name__)

        # more inits
        self.mesh_name = "Mesh"
        self.temp_dir = testtools.get_unit_test_tmp_dir(
            testtools.get_fem_test_tmp_dir(),
            "FEM_solver_calculix"
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

        fcc_print("\n{0}\n{1} run FEM TestSolverFrameWork tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            55 * "*"
        ))

    # ********************************************************************************************
    def test_solver_calculix(
        self
    ):
        fcc_print("\n--------------- Start of FEM tests solver framework solver CalculiX ------")

        # set up the CalculiX static analysis example
        from femexamples.boxanalysis_static import setup
        setup(self.document, "calculix")

        solver_obj = self.document.SolverCalculiX

        base_name = "cube_static"
        analysis_dir = testtools.get_unit_test_tmp_dir(self.temp_dir, solver_obj.Name)

        # save the file
        save_fc_file = join(analysis_dir, solver_obj.Name + "_" + base_name + ".FCStd")
        fcc_print("Save FreeCAD file to {}...".format(save_fc_file))
        self.document.saveAs(save_fc_file)

        # write input file
        fcc_print("Checking FEM input file writing for CalculiX solver framework solver ...")
        machine_ccx = solver_obj.Proxy.createMachine(
            solver_obj,
            analysis_dir
        )
        machine_ccx.target = femsolver.run.PREPARE
        machine_ccx.start()
        machine_ccx.join()  # wait for the machine to finish.

        infile_given = join(
            testtools.get_fem_test_home_dir(),
            "ccx",
            (base_name + ".inp")
        )
        inpfile_totest = join(analysis_dir, (self.mesh_name + ".inp"))
        fcc_print("Comparing {} to {}".format(infile_given, inpfile_totest))
        ret = testtools.compare_inp_files(infile_given, inpfile_totest)
        self.assertFalse(ret, "ccxtools write_inp_file test failed.\n{}".format(ret))

        fcc_print("--------------- End of FEM tests solver framework solver CalculiX --------")
