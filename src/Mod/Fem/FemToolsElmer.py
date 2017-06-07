# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Markus Hovorka <m.hovorka@live.de>               *
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

__title__ = "FemToolsElmer"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import os
import os.path
import subprocess
import shutil
import tempfile
from PySide import QtCore

import FreeCAD as App
from FreeCAD import Console
import FemTools
import FemInputWriterElmer


err_lookup = {
    "cd_non_existent": "Case directory {} doesn't exist.",
    "cd_not_dir": "Case directory {} is not a directory.",
    "create_inp_failed": "Failed to create input files: {}",
    "exec_solver_failed": "Solver execution failed with exit code: {}",
    "mesh_missing": "Mesh object missing.",
    "no_freetext": "Analysis without FreeText not jet supported!",
    "freetext_empty": "FreeText must not be empty.",
}


def runSolver(analysis, solver, caseDir=None):
    isTemp = False
    if not caseDir:
        isTemp, caseDir = _getCaseDir(analysis)
    Console.PrintMessage("Case directory path: {}\n".format(caseDir))
    fea = FemToolsElmer(analysis, solver, caseDir)
    fea.finished.connect(_solverFinished)
    QtCore.QThreadPool.globalInstance().start(fea)
    if isTemp:
        shutil.rmtree(caseDir)


def _getCaseDir(analysis):
    case_dir = _createCaseDirFromSettings(analysis.Label)
    if case_dir:
        Console.PrintMessage("Use working directory from settings.\n")
        return False, case_dir
    Console.PrintMessage("Use temporary case directory.\n")
    return True, tempfile.mkdtemp()


def _createCaseDirFromSettings(name):
    workingDir = (App.ParamGet(
        "User parameter:BaseApp/Preferences/Mod/Fem/General")
        .GetString("WorkingDir"))
    if not os.path.exists(workingDir):
        Console.PrintWarning(
                "Working directory {} doesn't exist.\n"
                .format(workingDir))
        return ""
    if not os.path.isdir(workingDir):
        Console.PrintWarning(
                "Working directory {} not a directory.\n"
                .format(workingDir))
        return ""
    caseDir = os.path.join(workingDir, name)
    if os.path.exists(caseDir) and not os.path.isdir(caseDir):
        Console.PrintWarning(
                "Can't use {}. Not a directory.\n"
                .format(caseDir))
        return ""
    if not os.path.exists(caseDir):
        try:
            os.mkdir(caseDir)
        except OSError as e:
            Console.PrintWarning(
                    "Couldn't create directory {}: {}\n"
                    .format(caseDir, e.strerror))
            return ""
    return caseDir


def _solverFinished(status):
    if status.meshOut is not None:
        Console.PrintLog("=== STDOUT of ElmerGrid ==========\n")
        Console.PrintLog(str(status.meshOut))
    if status.meshErr is not None:
        Console.PrintLog("=== STDERR of ElmerGrid ==========\n")
        Console.PrintLog(str(status.meshErr))
    if status.solverOut is not None:
        Console.PrintLog("=== STDOUT of ElmerSolver ==========\n")
        Console.PrintLog(str(status.solverOut))
    if status.solverErr is not None:
        Console.PrintLog("=== STDERR of ElmerSolver ==========\n")
        Console.PrintLog(str(status.solverErr))
    if not status.success:
        Console.PrintMessage("Solver execution failed.\n")
        status.printErrorList(err_lookup)


class _Status(object):

    def __init__(self):
        self._success = True
        self.meshOut = None
        self.meshErr = None
        self.solverOut = None
        self.solverErr = None
        self.errorList = []

    @property
    def success(self):
        return self._success

    def error(self, err, *args):
        self._success = False
        self.errorList.append((err, args))

    def printErrorList(self, lookup):
        for err_id, args in self.errorList:
            Console.PrintError(lookup[err_id].format(*args) + "\n")


class FemToolsElmer(FemTools.FemTools):

    finished = QtCore.Signal(object)
    SIF_NAME = "case.sif"

    def __init__(self, analysis, solver, case_dir):
        if analysis is None:
            raise Exception("Analysis must not be None.")
        if solver is None:
            raise Exception("Solver must not be None.")
        QtCore.QRunnable.__init__(self)
        QtCore.QObject.__init__(self)
        self.analysis = analysis
        self.solver = solver
        self.case_dir = case_dir
        self.update_objects()

    def start_elmer(self, binary, working_dir):
        p = subprocess.Popen(
                [binary], cwd=working_dir, stdout=subprocess.PIPE,
                stdin=subprocess.PIPE)
        out, err = p.communicate()
        return (p.returncode, out, err)

    def run(self):
        status = _Status()
        binary = self._check_elmer_binary(status)
        self._check_case_dir(status)
        self._check_analysis(status)

        progress_bar = App.Base.ProgressIndicator()
        if status.success:
            progress_bar.start("Executing Simulation...", 0)
            try:
                grid_ret = self.write_input_files(self.case_dir)
                status.meshOut = grid_ret[0]
                status.meshErr = grid_ret[1]
            except OSError as e:
                status.error("create_inp_failed", e.strerror)
            ret_code, status.solverOut, status.solverErr = self.start_elmer(
                    binary, self.case_dir)
            if ret_code != 0:
                status.error("exec_solver_failed", ret_code)

        progress_bar.stop()
        self.finished.emit(status)

    def find_elmer_binary(self):
        return "ElmerSolver"

    def write_input_files(self, working_dir):
        writer = FemInputWriterElmer.Writer(
                self.analysis, self.solver, self.mesh, self.materials_linear,
                self.materials_nonlinear, self.fixed_constraints,
                self.displacement_constraints, self.contact_constraints,
                self.planerotation_constraints, self.transform_constraints,
                self.selfweight_constraints, self.force_constraints,
                self.pressure_constraints, self.temperature_constraints,
                self.heatflux_constraints, self.initialtemperature_constraints,
                self.beam_sections, self.shell_thicknesses,
                self.fluid_sections, self.elmer_free_text)
        return writer.write_all(self.SIF_NAME, working_dir)

    def _check_analysis(self, status):
        if self.mesh is None:
            status.error("mesh_missing")
        if self.elmer_free_text is None:
            status.error("no_freetext")
        elif self.elmer_free_text.Text == "":
            status.error("freetext_empty")

    def _check_elmer_binary(self, status):
        binary = self.find_elmer_binary()
        if binary is None:
            status.failed("Couldn't find elmer binary.")
            return None
        return binary

    def _check_case_dir(self, status):
        if not os.path.exists(self.case_dir):
            status.error("cd_non_existent", self.case_dir)
            return None
        if not os.path.isdir(self.case_dir):
            status.error("cd_not_dir", self.case_dir)
            return None

    def load_results(self):
        self.results_present = False
        print('Result loading for Elmer not implemented yet!')
