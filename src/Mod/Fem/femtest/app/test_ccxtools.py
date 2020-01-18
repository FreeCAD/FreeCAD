# ***************************************************************************
# *   Copyright (c) 2015 - FreeCAD Developers                               *
# *   Author: Przemo Firszt <przemo@firszt.eu>                              *
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

from femtools import ccxtools
import FreeCAD
import unittest
from . import support_utils as testtools
from .support_utils import fcc_print

from os.path import join


class TestCcxTools(unittest.TestCase):
    fcc_print("import TestCcxTools")

    # ********************************************************************************************
    def setUp(
        self
    ):
        # setUp is executed before every test
        # setting up a document to hold the tests
        self.doc_name = self.__class__.__name__
        if FreeCAD.ActiveDocument:
            if FreeCAD.ActiveDocument.Name != self.doc_name:
                FreeCAD.newDocument(self.doc_name)
        else:
            FreeCAD.newDocument(self.doc_name)
        FreeCAD.setActiveDocument(self.doc_name)
        self.active_doc = FreeCAD.ActiveDocument

        # more inits
        self.mesh_name = "Mesh"
        self.temp_dir = testtools.get_fem_test_tmp_dir()
        self.test_file_dir = join(
            testtools.get_fem_test_home_dir(),
            "ccx"
        )

    def test_00print(
        self
    ):
        fcc_print("\n{0}\n{1} run FEM TestCcxTools tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            62 * "*"
        ))

    # ********************************************************************************************
    def test_1_static_analysis(
        self
    ):
        # set up
        from femexamples import boxanalysis as box
        box.setup_static(self.active_doc, "ccxtools")

        test_name = "ccxtools static analysis test"
        base_name = "cube_static"
        res_obj_name = "CCX_Results"
        analysis_dir = testtools.get_unit_test_tmp_dir(
            self.temp_dir,
            "FEM_ccx_static"
        )

        # test input file writing
        fea = self.input_file_writing_test(
            test_name=test_name,
            base_name=base_name,
            analysis_dir=analysis_dir,
            test_end=True,
        )

        # test result reading
        self.result_reading_test(
            test_name=test_name,
            base_name=base_name,
            analysis_dir=analysis_dir,
            fea=fea,
            res_obj_name=res_obj_name,
        )

    # ********************************************************************************************
    def test_2_static_multiple_material(
        self
    ):
        # set up
        from femexamples import material_multiple_twoboxes
        material_multiple_twoboxes.setup(self.active_doc, "ccxtools")

        test_name = "multiple material test"
        base_name = "multimat"
        analysis_dir = testtools.get_unit_test_tmp_dir(
            self.temp_dir,
            "FEM_ccx_multimat"
        )

        # test input file writing
        self.input_file_writing_test(
            test_name=test_name,
            base_name=base_name,
            analysis_dir=analysis_dir,
        )

    # ********************************************************************************************
    def test_3_freq_analysis(
        self
    ):
        # set up
        from femexamples import boxanalysis as box
        box.setup_frequency(self.active_doc, "ccxtools")

        test_name = "frequency analysis test"
        base_name = "cube_frequency"
        res_obj_name = "CCX_Mode1_Results"
        analysis_dir = testtools.get_unit_test_tmp_dir(
            self.temp_dir,
            "FEM_ccx_frequency"
        )

        # test input file writing
        fea = self.input_file_writing_test(
            test_name=test_name,
            base_name=base_name,
            analysis_dir=analysis_dir,
            test_end=True,
        )

        # test result reading
        self.result_reading_test(
            test_name=test_name,
            base_name=base_name,
            analysis_dir=analysis_dir,
            fea=fea,
            res_obj_name=res_obj_name,
        )

    # ********************************************************************************************
    def test_4_thermomech_analysis(
        self
    ):
        # set up
        from femexamples.thermomech_spine import setup as thermomech
        thermomech(self.active_doc, "ccxtools")

        test_name = "thermomechanical analysis test"
        base_name = "spine_thermomech"
        res_obj_name = "CCX_Results"
        analysis_dir = testtools.get_unit_test_tmp_dir(
            self.temp_dir,
            "FEM_ccx_thermomech"
        )

        # test input file writing
        fea = self.input_file_writing_test(
            test_name=test_name,
            base_name=base_name,
            analysis_dir=analysis_dir,
            test_end=True,
        )

        # test result reading
        self.result_reading_test(
            test_name=test_name,
            base_name=base_name,
            analysis_dir=analysis_dir,
            fea=fea,
            res_obj_name=res_obj_name,
        )

    # ********************************************************************************************
    def test_5_Flow1D_thermomech_analysis(
        self
    ):
        # set up
        from femexamples.thermomech_flow1d import setup as flow1d
        flow1d(self.active_doc, "ccxtools")

        test_name = "Flow1D analysis test"
        base_name = "Flow1D_thermomech"
        res_obj_name = "CCX_Time1_0_Results"
        analysis_dir = testtools.get_unit_test_tmp_dir(
            self.temp_dir,
            "FEM_ccx_Flow1D_thermomech"
        )

        # test input file writing
        fea = self.input_file_writing_test(
            test_name=test_name,
            base_name=base_name,
            analysis_dir=analysis_dir,
            test_end=True,
        )

        # test result reading
        self.result_reading_test(
            test_name=test_name,
            base_name=base_name,
            analysis_dir=analysis_dir,
            fea=fea,
            res_obj_name=res_obj_name,
        )

    # ********************************************************************************************
    def test_6_contact_shell_shell(
        self
    ):
        # set up
        from femexamples import contact_shell_shell as shellcontact
        shellcontact.setup(self.active_doc, "ccxtools")
        analysis_dir = testtools.get_unit_test_tmp_dir(
            self.temp_dir,
            "FEM_ccx_contact_shell_shell",
        )

        # test input file writing
        self.input_file_writing_test(
            test_name="contact shell shell analysis test",
            base_name="contact_shell_shell",
            analysis_dir=analysis_dir,
        )

    # ********************************************************************************************
    def input_file_writing_test(
        self,
        test_name,
        base_name,
        analysis_dir,
        test_end=False,
    ):
        fcc_print(
            "\n--------------- "
            "Start of FEM ccxtools {}"
            "---------------"
            .format(test_name)
        )

        analysis = self.active_doc.Analysis
        solver_object = self.active_doc.CalculiXccxTools
        fea = ccxtools.FemToolsCcx(analysis, solver_object, test_mode=True)
        fea.update_objects()

        fcc_print("Setting up working directory {}".format(analysis_dir))
        fea.setup_working_dir(analysis_dir)
        self.assertTrue(
            True if fea.working_dir == analysis_dir else False,
            "Setting working directory {} failed".format(analysis_dir)
        )

        fcc_print("Checking FEM inp file prerequisites for {} ...".format(test_name))
        error = fea.check_prerequisites()
        self.assertFalse(
            error,
            "ccxtools check_prerequisites returned error message: {}".format(error)
        )

        inpfile_given = join(self.test_file_dir, (base_name + ".inp"))
        inpfile_totest = join(analysis_dir, (self.mesh_name + ".inp"))
        fcc_print("Checking FEM inp file write...")
        fcc_print("Writing {} for {}".format(inpfile_totest, test_name))
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

        save_fc_file = analysis_dir + base_name + ".FCStd"
        fcc_print(
            "Save FreeCAD file for {} to {}..."
            .format(test_name, save_fc_file)
        )
        self.active_doc.saveAs(save_fc_file)

        fcc_print(
            "\n--------------- "
            "End of FEM ccxtools {}"
            "---------------"
            .format(test_name)
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

        fcc_print("Checking FEM frd file read from {}...".format(test_name))
        fea.load_results()
        self.assertTrue(
            fea.results_present,
            "Cannot read results from {}.frd frd file".format(fea.base_name)
        )

        fcc_print("Reading stats from result object for {}...".format(test_name))
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

        save_fc_file = join(
            analysis_dir,
            (base_name + ".FCStd")
        )
        fcc_print(
            "Save FreeCAD file for {} to {}..."
            .format(test_name, save_fc_file)
        )
        self.active_doc.saveAs(save_fc_file)

        fcc_print("--------------- End of {} -------------------".format(test_name))

    # ********************************************************************************************
    def tearDown(
        self
    ):
        # clearance, is executed after every test
        FreeCAD.closeDocument(self.doc_name)


# ************************************************************************************************
def create_test_results():

    import shutil
    import os
    import FemGui
    import femresult.resulttools as resulttools
    from femtools import ccxtools

    stat_types = [
        "U1", "U2", "U3", "Uabs",
        "Sabs",
        "MaxPrin", "MidPrin", "MinPrin", "MaxShear",
        "Peeq", "Temp", "MFlow", "NPress"
    ]
    temp_dir = testtools.get_fem_test_tmp_dir()
    static_analysis_dir = temp_dir + "FEM_ccx_static/"
    frequency_analysis_dir = temp_dir + "FEM_ccx_frequency/"
    thermomech_analysis_dir = temp_dir + "FEM_ccx_thermomech/"
    Flow1D_thermomech_analysis_dir = temp_dir + "FEM_ccx_Flow1D_thermomech/"

    # run all unit tests from this module
    import Test
    import sys
    current_module = sys.modules[__name__]
    Test.runTestsFromModule(current_module)

    # static cube
    FreeCAD.open(static_analysis_dir + "cube_static.FCStd")
    FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.Analysis)
    fea = ccxtools.FemToolsCcx()
    fea.update_objects()

    print("create static result files")
    fea.reset_all()
    fea.run()
    fea.load_results()
    stats_static = []
    for s in stat_types:
        statval = resulttools.get_stats(
            FreeCAD.ActiveDocument.getObject("CalculiX_static_results"),
            s
        )
        stats_static.append(
            "{0}: ({1:.14g}, {2:.14g}, {3:.14g})\n"
            .format(s, statval[0], statval[1], statval[2])
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
    frd_static_test_result_file = static_analysis_dir + "cube_static.frd"
    dat_static_test_result_file = static_analysis_dir + "cube_static.dat"
    shutil.copyfile(frd_result_file, frd_static_test_result_file)
    shutil.copyfile(dat_result_file, dat_static_test_result_file)

    # frequency cube
    FreeCAD.open(frequency_analysis_dir + "cube_frequency.FCStd")
    FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.Analysis)
    fea = ccxtools.FemToolsCcx()
    fea.update_objects()

    print("create frequency result files")
    fea.reset_all()
    fea.solver.EigenmodesCount = 1  # we should only have one result object
    fea.run()
    fea.load_results()
    stats_frequency = []
    for s in stat_types:
        statval = resulttools.get_stats(
            FreeCAD.ActiveDocument.getObject("CalculiX_frequency_mode_1_results"),
            s
        )
        stats_frequency.append(
            "{0}: ({1:.14g}, {2:.14g}, {3:.14g})\n"
            .format(s, statval[0], statval[1], statval[2])
        )
    frequency_expected_values_file = join(
        frequency_analysis_dir,
        "cube_frequency_expected_values"
    )
    f = open(frequency_expected_values_file, "w")
    for s in stats_frequency:
        f.write(s)
    f.close()
    frd_frequency_test_result_file = frequency_analysis_dir + "cube_frequency.frd"
    dat_frequency_test_result_file = frequency_analysis_dir + "cube_frequency.dat"
    shutil.copyfile(frd_result_file, frd_frequency_test_result_file)
    shutil.copyfile(dat_result_file, dat_frequency_test_result_file)

    # thermomech
    print("create thermomech result files")
    FreeCAD.open(thermomech_analysis_dir + "spine_thermomech.FCStd")
    FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.Analysis)
    fea = ccxtools.FemToolsCcx()
    fea.reset_all()
    fea.run()
    fea.load_results()
    stats_thermomech = []
    for s in stat_types:
        statval = resulttools.get_stats(
            FreeCAD.ActiveDocument.getObject("CalculiX_thermomech_results"),
            s
        )
        stats_thermomech.append(
            "{0}: ({1:.14g}, {2:.14g}, {3:.14g})\n"
            .format(s, statval[0], statval[1], statval[2])
        )
    thermomech_expected_values_file = join(
        thermomech_analysis_dir,
        "spine_thermomech_expected_values"
    )
    f = open(thermomech_expected_values_file, "w")
    for s in stats_thermomech:
        f.write(s)
    f.close()
    frd_result_file = os.path.splitext(fea.inp_file_name)[0] + ".frd"
    dat_result_file = os.path.splitext(fea.inp_file_name)[0] + ".dat"
    frd_thermomech_test_result_file = thermomech_analysis_dir + "spine_thermomech.frd"
    dat_thermomech_test_result_file = thermomech_analysis_dir + "spine_thermomech.dat"
    shutil.copyfile(frd_result_file, frd_thermomech_test_result_file)
    shutil.copyfile(dat_result_file, dat_thermomech_test_result_file)
    print("Results copied to the appropriate FEM test dirs in: " + temp_dir)

    # Flow1D
    print("create Flow1D result files")
    FreeCAD.open(Flow1D_thermomech_analysis_dir + "Flow1D_thermomech.FCStd")
    FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.Analysis)
    fea = ccxtools.FemToolsCcx()
    fea.reset_all()
    fea.run()
    fea.load_results()
    stats_flow1D = []
    for s in stat_types:
        statval = resulttools.get_stats(
            FreeCAD.ActiveDocument.getObject("CalculiX_thermomech_time_1_0_results"),
            s
        )
        stats_flow1D.append(
            "{0}: ({1:.14g}, {2:.14g}, {3:.14g})\n"
            .format(s, statval[0], statval[1], statval[2])
        )
    Flow1D_thermomech_expected_values_file = join(
        Flow1D_thermomech_analysis_dir,
        "Flow1D_thermomech_expected_values"
    )
    f = open(Flow1D_thermomech_expected_values_file, "w")
    for s in stats_flow1D:
        f.write(s)
    f.close()
    frd_result_file = os.path.splitext(fea.inp_file_name)[0] + ".frd"
    dat_result_file = os.path.splitext(fea.inp_file_name)[0] + ".dat"
    frd_Flow1D_thermomech_test_result_file = join(
        Flow1D_thermomech_analysis_dir,
        "Flow1D_thermomech.frd"
    )
    dat_Flow1D_thermomech_test_result_file = join(
        Flow1D_thermomech_analysis_dir,
        "Flow1D_thermomech.dat"
    )
    shutil.copyfile(frd_result_file, frd_Flow1D_thermomech_test_result_file)
    shutil.copyfile(dat_result_file, dat_Flow1D_thermomech_test_result_file)
    print("Flow1D thermomech results copied to the appropriate FEM test dirs in: " + temp_dir)


"""
update the results of FEM ccxtools unit tests:

from femtest.testccxtools import create_test_results
create_test_results()

copy result files from your_temp_directory/FEM_unittests/   test directories into the src directory
compare the results with git difftool
run make
start FreeCAD and run FEM unit test
if FEM unit test is fine --> commit new FEM unit test results

TODO compare the inp file of the helper with the inp file of FEM unit tests
TODO the better way: move the result creation inside the TestFem
and add some preference to deactivate this because it needs ccx
"""
