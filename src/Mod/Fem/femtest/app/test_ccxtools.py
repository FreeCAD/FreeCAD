# ***************************************************************************
# *   Copyright (c) 2015 Przemo Firszt <przemo@firszt.eu>                   *
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "Ccxtools FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import unittest
from os.path import join

import FreeCAD

from . import support_utils as testtools
from .support_utils import fcc_print
from .support_utils import get_namefromdef
from femtools import ccxtools


class TestCcxTools(unittest.TestCase):
    fcc_print("import TestCcxTools")

    # ********************************************************************************************
    def setUp(
        self
    ):
        # setUp is executed before every test

        # new document
        self.document = FreeCAD.newDocument(self.__class__.__name__)

        # directory pre face in name
        self.pre_dir_name = "ccxtools_"

        # more inits
        self.mesh_name = "Mesh"
        self.test_file_dir = join(
            testtools.get_fem_test_home_dir(),
            "calculix"
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

        fcc_print("\n{0}\n{1} run FEM TestCcxTools tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            62 * "*"
        ))

    # ********************************************************************************************
    def test_box_frequency(
        self
    ):
        # set up
        from femexamples.boxanalysis_frequency import setup
        setup(self.document, "ccxtools")
        base_name = get_namefromdef("test_")
        res_obj_name = "CCX_EigenMode_1_Results"
        analysis_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + base_name)

        # test input file writing
        fea = self.input_file_writing_test(
            None,
            base_name,
            analysis_dir=analysis_dir,
            test_end=True,
        )

        # test result reading
        self.result_reading_test(
            None,
            base_name,
            analysis_dir=analysis_dir,
            fea=fea,
            res_obj_name=res_obj_name,
        )

    # ********************************************************************************************
    def test_box_static(
        self
    ):
        # set up
        from femexamples.boxanalysis_static import setup
        setup(self.document, "ccxtools")
        base_name = get_namefromdef("test_")
        res_obj_name = "CCX_Results"
        analysis_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + base_name)

        # test input file writing
        fea = self.input_file_writing_test(
            None,
            base_name,
            analysis_dir=analysis_dir,
            test_end=True,
        )

        # test result reading
        self.result_reading_test(
            None,
            base_name,
            analysis_dir=analysis_dir,
            fea=fea,
            res_obj_name=res_obj_name,
        )

    # ********************************************************************************************
    def input_file_writing_test(
        self,
        test_name,
        base_name,
        analysis_dir=None,
        test_end=False,
    ):
        fcc_print(
            "\n--------------- "
            "Start of FEM ccxtools {} test"
            "---------------"
            .format(base_name)
        )

        if analysis_dir is None:
            analysis_dir = testtools.get_fem_test_tmp_dir(self.pre_dir_name + base_name)
        analysis = self.document.Analysis
        solver_object = self.document.CalculiXccxTools
        fea = ccxtools.FemToolsCcx(analysis, solver_object, test_mode=True)
        fea.update_objects()

        fcc_print("Setting up working directory {}".format(analysis_dir))
        fea.setup_working_dir(analysis_dir)
        self.assertTrue(
            True if fea.working_dir == analysis_dir else False,
            "Setting working directory {} failed".format(analysis_dir)
        )

        fcc_print("Checking FEM inp file prerequisites for {} ...".format(base_name))
        error = fea.check_prerequisites()
        self.assertFalse(
            error,
            "ccxtools check_prerequisites returned error message: {}".format(error)
        )

        inpfile_given = join(self.test_file_dir, (base_name + ".inp"))
        inpfile_totest = join(analysis_dir, (self.mesh_name + ".inp"))
        fcc_print("Checking FEM inp file write...")
        fcc_print("Writing {} for {}".format(inpfile_totest, base_name))
        error = fea.write_inp_file()
        self.assertFalse(
            error,
            "Writing failed"
        )

        fcc_print("Comparing {} to {}".format(inpfile_given, inpfile_totest))
        ret = testtools.compare_inp_files(inpfile_given, inpfile_totest)
        self.assertFalse(
            ret,
            "ccxtools write_inp_file test failed.\n{}".format(ret)
        )

        if test_end is True:
            # do not save and print End of tests
            return fea

        save_fc_file = join(analysis_dir, base_name + ".FCStd")
        fcc_print(
            "Save FreeCAD file for {} to {}..."
            .format(base_name, save_fc_file)
        )
        self.document.saveAs(save_fc_file)

        fcc_print(
            "\n--------------- "
            "End of FEM ccxtools {}"
            "---------------"
            .format(base_name)
        )

    # ********************************************************************************************
    def result_reading_test(
        self,
        test_name,
        base_name,
        analysis_dir,
        fea,
        res_obj_name,
    ):
        inpfile_given = join(self.test_file_dir, (base_name + ".inp"))

        fcc_print(
            "Setting up working directory to {} in order to read simulated calculations"
            .format(self.test_file_dir)
        )
        fea.setup_working_dir(self.test_file_dir)
        self.assertTrue(
            True if fea.working_dir == self.test_file_dir else False,
            "Setting working directory {} failed".format(self.test_file_dir)
        )

        fcc_print(
            "Setting base name to read test {}.frd file..."
            .format(base_name)
        )
        fea.set_base_name(base_name)
        self.assertTrue(
            True if fea.base_name == base_name else False,
            "Setting base name to {} failed".format(base_name)
        )

        fcc_print(
            "Setting inp file name to read test {}.frd file..."
            .format(base_name)
        )
        fea.set_inp_file_name()
        self.assertTrue(
            True if fea.inp_file_name == inpfile_given else False,
            "Setting inp file name to {} failed".format(inpfile_given)
        )

        fcc_print("Checking FEM frd file read from {}...".format(base_name))
        fea.load_results()
        self.assertTrue(
            fea.results_present,
            "Cannot read results from {}.frd frd file".format(fea.base_name)
        )

        fcc_print("Reading stats from result object for {}...".format(base_name))
        expected_values = join(
            self.test_file_dir,
            base_name + "_expected_values"
        )
        ret = testtools.compare_stats(
            fea,
            expected_values,
            res_obj_name
        )
        self.assertFalse(
            ret,
            "Invalid results read from .frd file"
        )

        save_fc_file = join(analysis_dir, base_name + ".FCStd")
        fcc_print(
            "Save FreeCAD file for {} to {}..."
            .format(base_name, save_fc_file)
        )
        self.document.saveAs(save_fc_file)

        fcc_print("--------------- End of {} -------------------".format(base_name))


# ************************************************************************************************
def create_test_results():

    import os
    import shutil
    import unittest

    import FemGui
    from femresult import resulttools
    from femtools import ccxtools

    temp_dir = testtools.get_fem_test_tmp_dir()
    test_class = "femtest.app.test_ccxtools.TestCcxTools"  # unit test class
    stat_types = [
        "U1", "U2", "U3", "Uabs", "Sabs",
        "MaxPrin", "MidPrin", "MinPrin", "MaxShear",
        "Peeq", "Temp", "MFlow", "NPress"
    ]

    # ****************************************************************************
    # static cube
    print("create static result files")
    unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
        test_class + ".test_static_analysis")
    )
    static_analysis_dir = join(temp_dir, "FEM_ccx_static")
    doc_static_cube = FreeCAD.open(join(static_analysis_dir, "cube_static.FCStd"))
    FemGui.setActiveAnalysis(doc_static_cube.Analysis)
    fea = ccxtools.FemToolsCcx()
    fea.update_objects()

    fea.reset_all()
    fea.run()
    fea.load_results()
    stats_static = []
    res_obj_static = doc_static_cube.getObject("CCX_Results")
    for s in stat_types:
        statval = resulttools.get_stats(res_obj_static, s)
        stats_static.append(
            "{0}: ({1:.14g}, {2:.14g}, )\n"
            .format(s, statval[0], statval[1])
        )
    static_expected_values_file = join(
        static_analysis_dir,
        "cube_static_expected_values"
    )
    f = open(static_expected_values_file, "w")
    for s in stats_static:
        f.write(s)
    f.close()
    frd_result_file = os.path.splitext(fea.inp_file_name)[0] + ".frd"
    dat_result_file = os.path.splitext(fea.inp_file_name)[0] + ".dat"
    frd_static_test_result_file = join(static_analysis_dir, "cube_static.frd")
    dat_static_test_result_file = join(static_analysis_dir, "cube_static.dat")
    shutil.copyfile(frd_result_file, frd_static_test_result_file)
    shutil.copyfile(dat_result_file, dat_static_test_result_file)
    print("Results copied to the appropriate FEM test dirs in: " + temp_dir)

    # ****************************************************************************
    # frequency cube
    print("create frequency result files")
    unittest.TextTestRunner().run(unittest.TestLoader().loadTestsFromName(
        test_class + ".test_freq_analysis")
    )
    frequency_analysis_dir = join(temp_dir, "FEM_ccx_frequency")
    doc_frequency_cube = FreeCAD.open(join(frequency_analysis_dir, "cube_frequency.FCStd"))
    FemGui.setActiveAnalysis(doc_frequency_cube.Analysis)
    fea = ccxtools.FemToolsCcx()
    fea.update_objects()
    fea.reset_all()
    # we should only have one result object 1 to 6 will be less than 0.01 and ignored
    fea.solver.EigenmodesCount = 7
    doc_frequency_cube.recompute()
    fea.run()
    fea.load_results()
    stats_frequency = []
    res_obj_freq = doc_frequency_cube.getObject("CCX_Mode7_Results")
    for s in stat_types:
        statval = resulttools.get_stats(res_obj_freq, s)
        stats_frequency.append(
            "{0}: ({1:.14g}, {2:.14g})\n"
            .format(s, statval[0], statval[1])
        )
    frequency_expected_values_file = join(
        frequency_analysis_dir,
        "cube_frequency_expected_values"
    )
    f = open(frequency_expected_values_file, "w")
    for s in stats_frequency:
        f.write(s)
    f.close()
    frd_result_file = os.path.splitext(fea.inp_file_name)[0] + ".frd"
    dat_result_file = os.path.splitext(fea.inp_file_name)[0] + ".dat"
    frd_frequency_test_result_file = join(frequency_analysis_dir, "cube_frequency.frd")
    dat_frequency_test_result_file = join(frequency_analysis_dir, "cube_frequency.dat")
    shutil.copyfile(frd_result_file, frd_frequency_test_result_file)
    shutil.copyfile(dat_result_file, dat_frequency_test_result_file)
    print("Results copied to the appropriate FEM test dirs in: " + temp_dir)


"""
update the results of FEM ccxtools unit tests:

from femtest.app.test_ccxtools import create_test_results
create_test_results()

copy result files
from unit_test_temp_directory/FEM_unittests/specific_test into the src directory
compare the results with git difftool
run make
start FreeCAD and run FEM unit test
if FEM unit test is fine --> commit new FEM unit test results

TODO compare the inp file of the helper with the inp file of FEM unit tests
TODO the better way: move the result creation inside the TestFem
and add some preference to deactivate this because it needs ccx
"""
