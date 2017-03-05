# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
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
from __future__ import print_function

__title__ = "FemToolsZ88"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import FreeCAD
import FemTools
from PySide import QtCore
from PySide.QtGui import QApplication


class FemToolsZ88(FemTools.FemTools):

    known_analysis_types = ["static"]

    ## The constructor
    #  @param analysis - analysis object to be used as the core object.
    #  "__init__" tries to use current active analysis in analysis is left empty.
    #  Rises exception if analysis is not set and there is no active analysis
    def __init__(self, analysis=None, solver=None, test_mode=False):
        if analysis:
            ## @var analysis
            #  FEM analysis - the core object. Has to be present.
            #  It's set to analysis passed in "__init__" or set to current active analysis by default if nothing has been passed to "__init__".
            self.analysis = analysis
        else:
            import FemGui
            self.analysis = FemGui.getActiveAnalysis()
        if solver:
            ## @var solver
            #  solver of the analysis. Used to store the active solver and analysis parameters
            self.solver = solver
        else:
            self.solver = None
        if self.analysis:
            self.update_objects()
            self.base_name = ""
            if self.solver:
                self.set_analysis_type()
                self.setup_working_dir()
                self.setup_z88()
            else:
                raise Exception('FEM: No solver found!')
        else:
            raise Exception('FEM: No active analysis found!')

        self.z88_is_running = False
        self.z88_testrun = QtCore.QProcess()
        self.z88_solverun = QtCore.QProcess()
        QtCore.QObject.connect(self.z88_testrun, QtCore.SIGNAL("started()"), self.z88_testrun_started)
        QtCore.QObject.connect(self.z88_testrun, QtCore.SIGNAL("finished(int)"), self.z88_testrun_finished)
        QtCore.QObject.connect(self.z88_solverun, QtCore.SIGNAL("started()"), self.z88_solverun_started)
        QtCore.QObject.connect(self.z88_solverun, QtCore.SIGNAL("finished(int)"), self.z88_solverun_finished)

    def write_inp_file(self):
        import FemInputWriterZ88 as iw
        import sys
        self.inp_file_name = ""
        try:
            inp_writer = iw.FemInputWriterZ88(
                self.analysis, self.solver,
                self.mesh, self.materials_linear, self.materials_nonlinear,
                self.fixed_constraints, self.displacement_constraints,
                self.contact_constraints, self.planerotation_constraints, self.transform_constraints,
                self.selfweight_constraints, self.force_constraints, self.pressure_constraints,
                self.temperature_constraints, self.heatflux_constraints, self.initialtemperature_constraints,
                self.beam_sections, self.shell_thicknesses, self.fluid_sections,
                self.analysis_type, self.working_dir)
            self.inp_file_name = inp_writer.write_z88_input()
        except:
            print("Unexpected error when writing Z88 input files:", sys.exc_info()[0])
            raise

    ## Sets Z88 solver z88r binary path
    #  @param self The python object self
    #  @z88_binary path to z88r binary, default is guessed: "bin/z88r" windows, "z88r" for other systems
    def setup_z88(self, z88_binary=None):
        from platform import system
        z88_std_location = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Z88").GetBool("UseStandardZ88Location")
        print(z88_std_location)
        if z88_std_location:
            if system() == "Windows":
                z88_path = FreeCAD.getHomePath() + "bin/z88r.exe"
                FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Z88").SetString("z88BinaryPath", z88_path)
                self.z88_binary = z88_path
        else:
            if not z88_binary:
                z88_binary = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Z88").GetString("z88BinaryPath", "")
            if not z88_binary:
                if system() == "Linux":
                    z88_binary = "z88r"
                elif system() == "Windows":
                    z88_binary = FreeCAD.getHomePath() + "bin/z88r.exe"
                else:
                    z88_binary = "z88r"
            self.z88_binary = z88_binary

    def run(self):
        # TODO: reimplement the process handling for z88 binary
        message = self.check_prerequisites()
        if not message:
            self.write_inp_file()
            self.cwd = QtCore.QDir.currentPath()
            self.calc_path = QtCore.QFileInfo(self.working_dir + '/pseudofile.txt')
            print(self.cwd)
            print(self.inp_file_name)
            print(self.calc_path.path())
            self.z88_test_run()
        else:
            print("Running analysis failed! {}".format(message))

    def z88_test_run(self):
        # z88_testrun for memory
        QtCore.QDir.setCurrent(self.calc_path.path())
        if not self.z88_is_running:
            z88_testrun_binary = self.z88_binary + "  -t  -choly"
            print("Testrun Z88")
            print(z88_testrun_binary)
            self.z88_is_running = True
            self.z88_testrun.start(z88_testrun_binary)
        else:
            print("Unable to start Z88, because it runs (testrun)!")
        QtCore.QDir.setCurrent(self.cwd)
        QApplication.restoreOverrideCursor()

    def z88_solve_run(self):
        # z88_solve run
        QtCore.QDir.setCurrent(self.calc_path.path())
        if not self.z88_is_running:
            z88_solverun_binary = self.z88_binary + "  -c  -choly"
            print("Solverun Z88")
            print(z88_solverun_binary)
            self.z88_is_running = True
            self.z88_solverun.start(z88_solverun_binary)
        else:
            print("Unable to start Z88, because it runs (solverun)!")
        QtCore.QDir.setCurrent(self.cwd)
        QApplication.restoreOverrideCursor()

    def z88_testrun_started(self):
        print("  z88_testrun_started()")
        # print(self.z88_testrun.state())

    def z88_testrun_finished(self, exitCode):
        self.z88_is_running = False
        print("  z88_testrun_finished() --> " + str(exitCode))
        # print(self.z88_testrun.state())
        # out = self.z88_testrun.readAllStandardOutput()
        # print(out + '\n')  # in some cases output will be cutted, see gmsh macro --> same problem
        # TODO search out for "Vektor GS" and "Vektor KOI" and print values, may be compare with z88_params value
        self.z88_solve_run()

    def z88_solverun_started(self):
        print("  z88_solverun_started()")
        # print(self.z88_solverun.state())

    def z88_solverun_finished(self, exitCode):
        self.z88_is_running = False
        print("  z88_solverun_finished() --> " + str(exitCode))
        # print(self.z88_solverun.state())
        self.load_results()

    def load_results(self):
        self.results_present = False
        self.load_results_o2()

    def load_results_o2(self):
        import os
        import importZ88O2Results
        disp_result_file = self.working_dir + '/z88o2.txt'
        if os.path.isfile(disp_result_file):
            result_name_prefix = 'Z88_' + self.solver.AnalysisType + '_'
            importZ88O2Results.import_z88_disp(disp_result_file, self.analysis, result_name_prefix)
            for m in self.analysis.Member:
                if m.isDerivedFrom("Fem::FemResultObject"):
                    self.results_present = True
        else:
            raise Exception('FEM: No results found at {}!'.format(disp_result_file))

##  @}
