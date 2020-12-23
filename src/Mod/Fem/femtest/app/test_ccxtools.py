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
        fcc_print("\n--------------- Start of FEM ccxtools static analysis test ---------------")

        # set up the static analysis example
        from femexamples import boxanalysis as box
        box.setup_static(self.active_doc, "ccxtools")

        analysis = self.active_doc.Analysis
        solver_object = self.active_doc.CalculiXccxTools
        fcc_print("Analysis {}".format(type(analysis)))
        fcc_print("Analysis {}".format(analysis.TypeId))

        static_analysis_dir = testtools.get_unit_test_tmp_dir(
            self.temp_dir,
            "FEM_ccx_static"
        )
        fea = ccxtools.FemToolsCcx(analysis, solver_object, test_mode=True)
        fea.update_objects()
        fcc_print("fea Analysis {}".format(type(fea.analysis)))
        fcc_print("fea Analysis {}".format(fea.analysis.TypeId))

        fcc_print("Setting up working directory {}".format(static_analysis_dir))
        fea.setup_working_dir(static_analysis_dir)
        self.assertTrue(
            True if fea.working_dir == static_analysis_dir else False,
            "Setting working directory {} failed".format(static_analysis_dir)
        )

        fcc_print("Checking FEM inp file prerequisites for static analysis...")
        error = fea.check_prerequisites()
        self.assertFalse(
            error,
            "ccxtools check_prerequisites returned error message: {}".format(error)
        )

        static_base_name = "cube_static"
        inpfile_given = join(self.test_file_dir, (static_base_name + ".inp"))
        inpfile_totest = join(static_analysis_dir, (self.mesh_name + ".inp"))
        fcc_print("Checking FEM inp file write...")
        fcc_print("Writing {} for static analysis".format(inpfile_totest))
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

        fcc_print(
            "Setting up working directory to {} in order to read simulated calculations"
            .format(self.test_file_dir)
        )
        fea.setup_working_dir(self.test_file_dir)
        fcc_print(fea.working_dir)
        fcc_print(self.test_file_dir)
        self.assertTrue(
            True if fea.working_dir == self.test_file_dir else False,
            "Setting working directory {} failed".format(self.test_file_dir)
        )

        fcc_print("Setting base name to read test {}.frd file...".format("cube_static"))
        fea.set_base_name(static_base_name)
        self.assertTrue(
            True if fea.base_name == static_base_name else False,
            "Setting base name to {} failed".format(static_base_name)
        )

        fcc_print("Setting inp file name to read test {}.frd file...".format("cube_static"))
        fea.set_inp_file_name()
        self.assertTrue(
            True if fea.inp_file_name == inpfile_given else False,
            "Setting inp file name to {} failed".format(inpfile_given)
        )

        fcc_print("Checking FEM frd file read from static analysis...")
        fea.load_results()
        self.assertTrue(
            fea.results_present,
            "Cannot read results from {}.frd frd file".format(fea.base_name)
        )

        fcc_print("Reading stats from result object for static analysis...")
        static_expected_values = join(self.test_file_dir, "cube_static_expected_values")
        ret = testtools.compare_stats(
            fea,
            static_expected_values,
            "CCX_Results"
        )
        self.assertFalse(
            ret,
            "Invalid results read from .frd file"
        )

        static_save_fc_file = static_analysis_dir + static_base_name + ".FCStd"
        fcc_print("Save FreeCAD file for static analysis to {}...".format(static_save_fc_file))
        self.active_doc.saveAs(static_save_fc_file)

        fcc_print("--------------- End of FEM ccxtools static analysis test -------------------")

    # ********************************************************************************************
    def test_2_static_multiple_material(
        self
    ):
        fcc_print("\n--------------- Start of FEM ccxtools multiple material test -------------")

        # set up the simple multimat example
        from femexamples import material_multiple_twoboxes
        material_multiple_twoboxes.setup(self.active_doc, "ccxtools")

        analysis = self.active_doc.Analysis
        solver_object = self.active_doc.CalculiXccxTools

        static_multiplemat_dir = testtools.get_unit_test_tmp_dir(
            self.temp_dir,
            "FEM_ccx_multimat/"
        )
        fea = ccxtools.FemToolsCcx(analysis, solver_object, test_mode=True)
        fea.update_objects()
        fea.setup_working_dir(static_multiplemat_dir)

        fcc_print("Checking FEM inp file prerequisites for ccxtools multimat analysis...")
        error = fea.check_prerequisites()
        self.assertFalse(
            error,
            "ccxtools check_prerequisites returned error message: {}".format(error)
        )

        static_base_name = "multimat"
        inpfile_given = join(self.test_file_dir, (static_base_name + ".inp"))
        inpfile_totest = join(static_multiplemat_dir, (self.mesh_name + ".inp"))
        fcc_print("Checking FEM inp file write...")
        fcc_print("Writing {} for static multiple material".format(inpfile_totest))
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

        static_save_fc_file = static_multiplemat_dir + static_base_name + ".FCStd"
        fcc_print("Save FreeCAD file for static analysis to {}...".format(static_save_fc_file))
        self.active_doc.saveAs(static_save_fc_file)

        fcc_print("--------------- End of FEM ccxtools multiple material test -----------------")

    # ********************************************************************************************
    def test_3_freq_analysis(
        self
    ):
        fcc_print("\n--------------- Start of FEM ccxtools frequency analysis test ------------")

        # set up the static analysis example
        from femexamples import boxanalysis as box
        box.setup_frequency(self.active_doc, "ccxtools")

        analysis = self.active_doc.Analysis
        solver_object = self.active_doc.CalculiXccxTools

        frequency_analysis_dir = testtools.get_unit_test_tmp_dir(
            self.temp_dir,
            "FEM_ccx_frequency"
        )
        fea = ccxtools.FemToolsCcx(analysis, solver_object, test_mode=True)
        fea.update_objects()

        fcc_print("Setting up working directory {}".format(frequency_analysis_dir))
        fea.setup_working_dir(frequency_analysis_dir)
        self.assertTrue(
            True if fea.working_dir == frequency_analysis_dir else False,
            "Setting working directory {} failed".format(frequency_analysis_dir)
        )

        fcc_print("Checking FEM inp file prerequisites for frequency analysis...")
        error = fea.check_prerequisites()
        self.assertFalse(
            error,
            "ccxtools check_prerequisites returned error message: {}".format(error)
        )

        frequency_base_name = "cube_frequency"
        inpfile_given = join(self.test_file_dir, (frequency_base_name + ".inp"))
        inpfile_totest = join(frequency_analysis_dir, (self.mesh_name + ".inp"))
        fcc_print("Checking FEM inp file write...")
        fcc_print("Writing {} for frequency analysis".format(inpfile_totest))
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

        fcc_print(
            "Setting up working directory to {} in order to read simulated calculations".
            format(self.test_file_dir)
        )
        fea.setup_working_dir(self.test_file_dir)
        self.assertTrue(
            True if fea.working_dir == self.test_file_dir else False,
            "Setting working directory {} failed".format(self.test_file_dir)
        )

        fcc_print("Setting base name to read test {}.frd file...".format(frequency_base_name))
        fea.set_base_name(frequency_base_name)
        self.assertTrue(
            True if fea.base_name == frequency_base_name else False,
            "Setting base name to {} failed".format(frequency_base_name)
        )

        fcc_print("Setting inp file name to read test {}.frd file...".format("cube_frequency"))
        fea.set_inp_file_name()
        self.assertTrue(
            True if fea.inp_file_name == inpfile_given else False,
            "Setting inp file name to {} failed".format(inpfile_given)
        )

        fcc_print("Checking FEM frd file read from frequency analysis...")
        fea.load_results()
        self.assertTrue(
            fea.results_present,
            "Cannot read results from {}.frd frd file".format(fea.base_name)
        )

        fcc_print("Reading stats from result object for frequency analysis...")
        frequency_expected_values = join(self.test_file_dir, "cube_frequency_expected_values")
        ret = testtools.compare_stats(
            fea,
            frequency_expected_values,
            "CCX_Mode1_Results"
        )
        self.assertFalse(
            ret,
            "Invalid results read from .frd file"
        )

        frequency_save_fc_file = frequency_analysis_dir + frequency_base_name + ".FCStd"
        fcc_print(
            "Save FreeCAD file for frequency analysis to {}..."
            .format(frequency_save_fc_file)
        )
        self.active_doc.saveAs(frequency_save_fc_file)

        fcc_print("--------------- End of FEM ccxtools frequency analysis test ----------------")

    # ********************************************************************************************
    def test_4_thermomech_analysis(
        self
    ):

        fcc_print("\n--------------- Start of FEM ccxtools thermomechanical analysis test -----")

        # set up the thermomech example
        from femexamples.thermomech_spine import setup as thermomech
        thermomech(self.active_doc, "ccxtools")

        analysis = self.active_doc.Analysis

        thermomech_analysis_dir = testtools.get_unit_test_tmp_dir(
            self.temp_dir,
            "FEM_ccx_thermomech"
        )
        fea = ccxtools.FemToolsCcx(analysis, test_mode=True)
        fea.update_objects()

        fcc_print("Setting up working directory {}".format(thermomech_analysis_dir))
        fea.setup_working_dir(thermomech_analysis_dir)
        self.assertTrue(
            True if fea.working_dir == thermomech_analysis_dir else False,
            "Setting working directory {} failed".format(thermomech_analysis_dir)
        )

        fcc_print("Checking FEM inp file prerequisites for thermo-mechanical analysis...")
        error = fea.check_prerequisites()
        self.assertFalse(
            error,
            "ccxtools check_prerequisites returned error message: {}".format(error)
        )

        thermomech_base_name = "spine_thermomech"
        inpfile_given = join(self.test_file_dir, (thermomech_base_name + ".inp"))
        inpfile_totest = join(thermomech_analysis_dir, (self.mesh_name + ".inp"))
        fcc_print("Checking FEM inp file write...")
        fcc_print("Writing {} for thermomech analysis".format(inpfile_totest))
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

        fcc_print(
            "Setting up working directory to {} in order to read simulated calculations"
            .format(self.test_file_dir)
        )
        fea.setup_working_dir(self.test_file_dir)
        self.assertTrue(
            True if fea.working_dir == self.test_file_dir else False,
            "Setting working directory {} failed".format(self.test_file_dir)
        )

        fcc_print("Setting base name to read test {}.frd file...".format("spine_thermomech"))
        fea.set_base_name(thermomech_base_name)
        self.assertTrue(
            True if fea.base_name == thermomech_base_name else False,
            "Setting base name to {} failed".format(thermomech_base_name)
        )

        fcc_print("Setting inp file name to read test {}.frd file...".format("spine_thermomech"))
        fea.set_inp_file_name()
        self.assertTrue(
            True if fea.inp_file_name == inpfile_given else False,
            "Setting inp file name to {} failed".format(inpfile_given)
        )

        fcc_print("Checking FEM frd file read from thermomech analysis...")
        fea.load_results()
        self.assertTrue(
            fea.results_present,
            "Cannot read results from {}.frd frd file".format(fea.base_name)
        )

        fcc_print("Reading stats from result object for thermomech analysis...")
        thermomech_expected_values = join(
            self.test_file_dir,
            "spine_thermomech_expected_values"
        )
        ret = testtools.compare_stats(
            fea,
            thermomech_expected_values,
            "CCX_Results"
        )
        self.assertFalse(
            ret,
            "Invalid results read from .frd file"
        )

        thermomech_save_fc_file = thermomech_analysis_dir + thermomech_base_name + ".FCStd"
        fcc_print(
            "Save FreeCAD file for thermomech analysis to {}..."
            .format(thermomech_save_fc_file)
        )
        self.active_doc.saveAs(thermomech_save_fc_file)

        fcc_print("--------------- End of FEM ccxtools thermomechanical analysis test ---------")

    # ********************************************************************************************
    def test_5_Flow1D_thermomech_analysis(
        self
    ):
        fcc_print("\n--------------- Start of FEM ccxtools Flow1D analysis test ---------------")

        # set up the thermomech flow1d example
        from femexamples.thermomech_flow1d import setup as flow1d
        flow1d(self.active_doc, "ccxtools")
        analysis = self.active_doc.Analysis

        Flow1D_thermomech_analysis_dir = testtools.get_unit_test_tmp_dir(
            self.temp_dir,
            "FEM_ccx_Flow1D_thermomech"
        )
        fea = ccxtools.FemToolsCcx(analysis, test_mode=True)
        fea.update_objects()

        fcc_print("Setting up working directory {}".format(Flow1D_thermomech_analysis_dir))
        fea.setup_working_dir(Flow1D_thermomech_analysis_dir)
        self.assertTrue(
            True if fea.working_dir == Flow1D_thermomech_analysis_dir else False,
            "Setting working directory {} failed".format(Flow1D_thermomech_analysis_dir)
        )

        fcc_print("Checking FEM inp file prerequisites for thermo-mechanical analysis...")
        error = fea.check_prerequisites()
        self.assertFalse(
            error,
            "ccxtools check_prerequisites returned error message: {}".format(error)
        )

        Flow1D_thermomech_base_name = "Flow1D_thermomech"
        inpfile_given = join(self.test_file_dir, (Flow1D_thermomech_base_name + ".inp"))
        inpfile_totest = join(Flow1D_thermomech_analysis_dir, (self.mesh_name + ".inp"))
        fcc_print("Checking FEM inp file write...")
        fcc_print("Writing {} for thermomech analysis".format(inpfile_totest))
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

        fcc_print(
            "Setting up working directory to {} in order to read simulated calculations"
            .format(self.test_file_dir)
        )
        fea.setup_working_dir(self.test_file_dir)
        self.assertTrue(
            True if fea.working_dir == self.test_file_dir else False,
            "Setting working directory {} failed".format(self.test_file_dir)
        )

        fcc_print("Setting base name to read test {}.frd file...".format("Flow1D_thermomech"))
        fea.set_base_name(Flow1D_thermomech_base_name)
        self.assertTrue(
            True if fea.base_name == Flow1D_thermomech_base_name else False,
            "Setting base name to {} failed".format(Flow1D_thermomech_base_name)
        )

        fcc_print("Setting inp file name to read test {}.frd file...".format("Flow1D_thermomech"))
        fea.set_inp_file_name()
        self.assertTrue(
            True if fea.inp_file_name == inpfile_given else False,
            "Setting inp file name to {} failed".format(inpfile_given)
        )

        fcc_print("Checking FEM frd file read from Flow1D thermomech analysis...")
        fea.load_results()
        self.assertTrue(
            fea.results_present,
            "Cannot read results from {}.frd frd file".format(fea.base_name)
        )

        fcc_print("Reading stats from result object for Flow1D thermomech analysis...")
        Flow1D_thermomech_expected_values = join(
            self.test_file_dir,
            "Flow1D_thermomech_expected_values"
        )
        ret = testtools.compare_stats(
            fea,
            Flow1D_thermomech_expected_values,
            "CCX_Time1_0_Results"
        )
        self.assertFalse(
            ret,
            "Invalid results read from .frd file"
        )

        Flow1D_thermomech_save_fc_file = join(
            Flow1D_thermomech_analysis_dir,
            (Flow1D_thermomech_base_name + ".FCStd")
        )
        fcc_print(
            "Save FreeCAD file for thermomech analysis to {}..."
            .format(Flow1D_thermomech_save_fc_file)
        )
        self.active_doc.saveAs(Flow1D_thermomech_save_fc_file)

        fcc_print("--------------- End of FEM ccxtools Flow1D analysis test -------------------")

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
