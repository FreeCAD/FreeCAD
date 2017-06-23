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

from PySide import QtCore
import subprocess
import io
import os
import os.path
import shutil
import tempfile
import distutils.spawn

import FreeCAD as App
from FreeCAD import Console
import FemInputWriterElmer
import FemDefsElmer
import Report


_ELMER_SUBDIR = "elmer"
_ELMERSOLVER_BIN = "ElmerSolver"
_ELMERGRID_BIN = "ElmerGrid"

_OUTPUT_OBJ_NAME = "ElmerSolverOutput"

_ERROR_TITLE = "Missing Prerequisites"
_ERROR_TEXT = (
        "Could not start ElmerSolver because of missing prerequisites."
        " Please try again after the following errors have been resolved:")
_INFO_TITLE = "Info"
_INFO_TEXT = None

_SOLVER_ERROR_TITLE = "ElmerSolver Failed"
_SOLVER_ERROR_TEXT = "Execution of ElmerSolver failed."
_SOLVER_INFO_TITLE = "Info to ElmerSolver"
_SOLVER_INFO_TEXT = None


def runSimulation(analysis, solver, caseDir=None):
    report = Report.Data()
    if caseDir is None:
        if _useTempDir():
            caseDir = tempfile.mkdtemp()
        else:
            caseDir = dirFromSettings(report)
    elmerBin, gridBin = getBinaries(report)
    checkAnalysis(analysis, solver, report)
    if report.isValid():
        _wipeDir(caseDir)
        writer = FemInputWriterElmer.Writer(
                analysis, solver, caseDir, gridBin)
        writer.writeInputFiles(report)
        runner = SolverRunner(elmerBin, caseDir, analysis)
        runner.finished.connect(_finished)
        QtCore.QThreadPool.globalInstance().start(runner)
    else:
        title = _INFO_TITLE if report.isValid() else _ERROR_TITLE
        text = _INFO_TEXT if report.isValid() else _ERROR_TEXT
        Report.display(report, title, text)
    if _useTempDir():
        shutil.rmtree(caseDir)


def dirFromSettings(report):
    caseDir = None
    workingDir = (App.ParamGet(
        "User parameter:BaseApp/Preferences/Mod/Fem/General")
        .GetString("WorkingDir"))
    if not os.path.exists(workingDir):
        report.appendError("wd_not_existent", workingDir)
    elif not os.path.isdir(workingDir):
        report.appendError("wd_not_directory", workingDir)
    else:
        caseDir = os.path.join(workingDir, _ELMER_SUBDIR)
        if os.path.exists(caseDir) and not os.path.isdir(caseDir):
            report.appendError("cd_not_directory", caseDir)
        elif not os.path.exists(caseDir):
            try:
                os.mkdir(caseDir)
            except OSError as e:
                report.appendError(
                        "cd_not_created", caseDir, e.strerror)
    return caseDir


def getBinaries(report):
    if not _checkExecutable(_ELMERSOLVER_BIN):
        report.appendError("elmersolver_not_found")
    if not _checkExecutable(_ELMERGRID_BIN):
        report.appendError("elmergrid_not_found")
    return _ELMERSOLVER_BIN, _ELMERGRID_BIN


def checkAnalysis(analysis, solver, report):
    _checkAnalysisCommon(analysis, solver, report)
    if solver.AnalysisType == FemDefsElmer.STATIC:
        pass
    elif solver.AnalysisType == FemDefsElmer.FREQUENCY:
        pass
    elif solver.AnalysisType == FemDefsElmer.THERMOMECH:
        pass


def _checkAnalysisCommon(analysis, solver, report):
    meshes = _getOfType(analysis, "Fem::FemMeshObject")
    if len(meshes) == 0:
        report.appendError("mesh_missing")
    elif len(meshes) > 1:
        report.appendError("too_many_meshes")
    elif not (hasattr(meshes[0], "Proxy")
            and meshes[0].Proxy.Type == "FemMeshGmsh"):
        report.appendError("unsupported_mesh")
    
    
def _getOfType(analysis, baseType, pyType=None):
    matching = []
    for m in analysis.Member:
        if m.isDerivedFrom(baseType):
            if pyType is None:
                matching.append(m)
            elif hasattr(m, "Proxy") and m.Proxy.Type == pyType:
                matching.append(m)
    return matching


def _getFirstOfType(analysis, baseType, pyType=None):
    objs = _getOfType(analysis, baseType, pyType)
    return objs[0] if objs else None


def _checkExecutable(path):
    return distutils.spawn.find_executable(path) is not None


def _useTempDir():
    workingDir = (App.ParamGet(
        "User parameter:BaseApp/Preferences/Mod/Fem/General")
        .GetString("WorkingDir"))
    return workingDir == ""


def _wipeDir(directory):
    for node in os.listdir(directory):
        path = os.path.join(directory, node)
        if os.path.isfile(path):
            os.unlink(path)
        elif os.path.isdir(path):
            shutil.rmtree(path)


def _finished(report):
    _updateLog(report.analysis, report.output)
    title = _SOLVER_INFO_TITLE if report.isValid() else _SOLVER_ERROR_TITLE
    text = _SOLVER_INFO_TEXT if report.isValid() else _SOLVER_ERROR_TEXT
    Report.display(report, title, text)


def _updateLog(analysis, log):
    outputObj = _getOutputObj(analysis)
    outputObj.Text = log


def _getOutputObj(analysis):
    objs = _getOfType(analysis, "App::TextDocument")
    outputObj = None
    for o in objs:
        if o.Name.startswith(_OUTPUT_OBJ_NAME):
            outputObj = o
            break
    if outputObj is None:
        outputObj = App.ActiveDocument.addObject(
                "App::TextDocument", _OUTPUT_OBJ_NAME)
        analysis.Member += [outputObj]
    outputObj.ReadOnly = True
    return outputObj


class SolverRunner(QtCore.QObject, QtCore.QRunnable):

    finished = QtCore.Signal(object)

    def __init__(self, elmerBin, directory, analysis):
        QtCore.QRunnable.__init__(self)
        QtCore.QObject.__init__(self)
        self._elmerBin = elmerBin
        self._directory = directory
        self._analysis = analysis

    def run(self):
        report = SolverReport(self._analysis)
        progressBar = App.Base.ProgressIndicator()
        progressBar.start("Running ElmerSolver...", 0)
        try:
            p = subprocess.Popen(
                    [self._elmerBin], cwd=self._directory,
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            report.output = p.communicate()[0]
            retcode = p.wait()
            if retcode != 0:
                report.appendError("elmer_failed", retcode)
        finally:
            progressBar.stop()
            self.finished.emit(report)


class SolverReport(Report.Data):

    def __init__(self, analysis):
        super(SolverReport, self).__init__()
        self.output = ""
        self.analysis = analysis
