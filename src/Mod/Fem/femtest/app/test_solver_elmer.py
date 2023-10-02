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

__title__ = "Solver elmer FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import unittest
from os.path import join

import FreeCAD

import femsolver.run
from . import support_utils as testtools
from .support_utils import fcc_print
from .support_utils import get_namefromdef


class TestSolverElmer(unittest.TestCase):
    fcc_print("import TestSolverElmer")

    # ********************************************************************************************
    def setUp(
        self
    ):
        # setUp is executed before every test

        # new document
        self.document = FreeCAD.newDocument(self.__class__.__name__)

        # more inits
        self.pre_dir_name = "solver_elmer_"
        self.ending = ".sif"
        self.infilename = "case"
        self.test_file_dir = join(
            testtools.get_fem_test_home_dir(),
            "elmer"
        )
        # set Units
        # since in Elmer writer the FreeCAD pref is used, here we need to set the FreeCAD pref
        # the use of FreeCAD.Units.setScheme would not take affect because the pref is not changed
        # see https://forum.freecad.org/viewtopic.php?t=48451
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
        self.saved_unit_schema = param.GetInt("UserSchema")

    # ********************************************************************************************
    def tearDown(
        self
    ):
        # set back unit unit schema
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
        unit_schema = param.GetInt("UserSchema")
        if unit_schema != self.saved_unit_schema:
            fcc_print("Reset unit schema back to {}".format(self.saved_unit_schema))
            param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
            param.SetInt("UserSchema", self.saved_unit_schema)

        # tearDown is executed after every test
        FreeCAD.closeDocument(self.document.Name)
        fcc_print("")

    # ********************************************************************************************
    def test_00print(
        self
    ):
        # since method name starts with 00 this will be run first
        # this test just prints a line with stars

        fcc_print("\n{0}\n{1} run FEM TestSolverElmer tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            55 * "*"
        ))

    # ********************************************************************************************
    def set_unit_schema(
        self,
        new_unit_schema=0
    ):
        fcc_print(
            "\nSaved unit schema: {}. Set unit schema to {}."
            .format(self.saved_unit_schema, new_unit_schema)
        )
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
        param.SetInt("UserSchema", new_unit_schema)

    # ********************************************************************************************
    def test_box_static_0_mm(
        self
    ):
        fcc_print("")
        self.set_unit_schema(0)  # mm/kg/s

        # set up the Elmer static analysis example
        from femexamples.boxanalysis_static import setup
        setup(self.document, "elmer")

        # for information:
        # elmer needs gmsh mesho object
        # FIXME error message on Python solver run
        # the examples do use a gmsh mesh object thus ok

        base_name = get_namefromdef("test_")
        analysis_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + base_name)

        # save the file
        save_fc_file = join(analysis_dir, base_name + ".FCStd")
        # fcc_print("Save FreeCAD file to {}...".format(save_fc_file))
        self.document.saveAs(save_fc_file)

        # write input files
        # fcc_print("Checking FEM input file writing for Elmer solver framework solver ...")
        machine_elmer = self.document.SolverElmer.Proxy.createMachine(
            self.document.SolverElmer,
            analysis_dir,
            True
        )
        machine_elmer.target = femsolver.run.PREPARE
        machine_elmer.start()
        machine_elmer.join()  # wait for the machine to finish.

        fcc_print("Test writing STARTINFO file")
        startinfo_given = join(self.test_file_dir, "ELMERSOLVER_STARTINFO")
        startinfo_totest = join(analysis_dir, "ELMERSOLVER_STARTINFO")
        # fcc_print("Comparing {} to {}".format(startinfo_given, startinfo_totest))
        ret = testtools.compare_files(startinfo_given, startinfo_totest)
        self.assertFalse(ret, "STARTINFO write file test failed.\n{}".format(ret))

        fcc_print("Test writing case file")
        casefile_given = join(self.test_file_dir, base_name + self.ending)
        casefile_totest = join(analysis_dir, self.infilename + self.ending)
        # fcc_print("Comparing {} to {}".format(casefile_given, casefile_totest))
        ret = testtools.compare_files(casefile_given, casefile_totest)
        self.assertFalse(ret, "case write file test failed.\n{}".format(ret))

        fcc_print("Test writing GMSH geo file")
        gmshgeofile_given = join(self.test_file_dir, "group_mesh.geo")
        gmshgeofile_totest = join(analysis_dir, "group_mesh.geo")
        # fcc_print("Comparing {} to {}".format(gmshgeofile_given, gmshgeofile_totest))
        ret = testtools.compare_files(gmshgeofile_given, gmshgeofile_totest)
        self.assertFalse(ret, "GMSH geo write file test failed.\n{}".format(ret))

    # ********************************************************************************************
    def test_ccxcantilever_faceload_0_mm(
        self
    ):
        fcc_print("")
        self.set_unit_schema(0)  # mm/kg/s
        from femexamples.ccx_cantilever_faceload import setup
        setup(self.document, "elmer")
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_ccxcantilever_faceload_1_si(
        self
    ):
        fcc_print("")
        self.set_unit_schema(1)  # SI-units m/kg/s
        from femexamples.ccx_cantilever_faceload import setup
        setup(self.document, "elmer")
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_ccxcantilever_nodeload_0_mm(
        self
    ):
        fcc_print("")
        self.set_unit_schema(0)  # mm/kg/s
        from femexamples.ccx_cantilever_nodeload import setup
        setup(self.document, "elmer")
        self.input_file_writing_test(get_namefromdef("test_"))

    # ********************************************************************************************
    def test_ccxcantilever_prescribeddisplacement_0_mm(
        self
    ):
        fcc_print("")
        self.set_unit_schema(0)  # mm/kg/s
        from femexamples.ccx_cantilever_prescribeddisplacement import setup
        setup(self.document, "elmer")
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
        machine = self.document.SolverElmer.Proxy.createMachine(
            self.document.SolverElmer,
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
            "Elmer write_inp_file for {0} test failed.\n{1}".format(base_name, ret)
        )
