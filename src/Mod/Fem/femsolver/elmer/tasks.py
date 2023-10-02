# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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

__title__ = "FreeCAD FEM solver Elmer tasks"
__author__ = "Markus Hovorka"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import cmath
import os
import os.path
import subprocess
from platform import system

import FreeCAD

from . import writer
from .. import run
from .. import settings
from femtools import femutils
from femtools import membertools


class Check(run.Check):

    def run(self):
        self.pushStatus("Checking analysis...\n")
        if (self.check_mesh_exists()):
            self.checkMeshType()
        self.check_material_exists()
        self.checkEquations()

    def checkMeshType(self):
        mesh = membertools.get_single_member(self.analysis, "Fem::FemMeshObject")
        if not femutils.is_of_type(mesh, "Fem::FemMeshGmsh"):
            self.report.error(
                "Unsupported type of mesh. "
                "Mesh must be created with gmsh.")
            self.fail()
            return False
        return True

    def checkEquations(self):
        equations = self.solver.Group
        if not equations:
            self.report.error(
                "Solver has no equations. "
                "Add at least one equation.")
            self.fail()


class Prepare(run.Prepare):

    def run(self):
        # TODO print working dir to report console
        self.pushStatus("Preparing input files...\n")
        num_cores = settings.get_cores("ElmerGrid")
        self.pushStatus("Number of CPU cores to be used for the solver run: {}\n"
                        .format(num_cores))
        if self.testmode:
            # test mode: neither gmsh, nor elmergrid nor elmersolver binaries needed
            FreeCAD.Console.PrintMessage("Machine testmode: {}\n".format(self.testmode))
            w = writer.Writer(self.solver, self.directory, True)
        else:
            FreeCAD.Console.PrintLog("Machine testmode: {}\n".format(self.testmode))
            w = writer.Writer(self.solver, self.directory)
        try:
            w.write_solver_input()
            self.checkHandled(w)
            self.pushStatus("Writing solver input completed.")
        except writer.WriteError as e:
            self.report.error(str(e))
            self.fail()
        except IOError:
            self.report.error("Can't access working directory.")
            self.fail()

    def checkHandled(self, w):
        handled = w.getHandledConstraints()
        allConstraints = membertools.get_member(self.analysis, "Fem::Constraint")
        for obj in set(allConstraints) - handled:
            self.report.warning("Ignored constraint %s." % obj.Label)


class Solve(run.Solve):

    def run(self):
        # on rerun the result file will not deleted before starting the solver
        # if the solver fails, the existing result from a former run file will be loaded
        # TODO: delete result file (may be delete all files which will be recreated)
        self.pushStatus("Executing solver...\n")
        binary = settings.get_binary("ElmerSolver")
        if binary is not None:
            # if ELMER_HOME is not set, set it.
            # Needed if elmer is compiled but not installed on Linux
            # http://www.elmerfem.org/forum/viewtopic.php?f=2&t=7119
            # https://stackoverflow.com/questions/1506010/how-to-use-export-with-python-on-linux
            # TODO move retrieving the param to solver settings module
            elparams = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Elmer")
            elmer_env = elparams.GetBool("SetElmerEnvVariables", False)
            if elmer_env is True and system() == "Linux" and "ELMER_HOME" not in os.environ:
                solvpath = os.path.split(binary)[0]
                if os.path.isdir(solvpath):
                    os.environ["ELMER_HOME"] = solvpath
                    os.environ["LD_LIBRARY_PATH"] = "$LD_LIBRARY_PATH:{}/modules".format(solvpath)
            # different call depending if with multithreading or not
            num_cores = settings.get_cores("ElmerSolver")
            self.pushStatus("Number of CPU cores to be used for the solver run: {}\n"
                            .format(num_cores))
            args = []
            if num_cores > 1:
                if system() != "Windows":
                    args.extend(["mpirun"])
                else:
                    args.extend(["mpiexec"])
                args.extend(["-np", str(num_cores)])
            args.extend([binary])
            if system() == "Windows":
                self._process = subprocess.Popen(
                    args,
                    cwd=self.directory,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    startupinfo=femutils.startProgramInfo("hide")
                )
            else:
                self._process = subprocess.Popen(
                    args,
                    cwd=self.directory,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE
                )
            self.signalAbort.add(self._process.terminate)
            output = self._observeSolver(self._process)
            self._process.communicate()
            self.signalAbort.remove(self._process.terminate)
            if not self.aborted:
                self._updateOutput(output)
        else:
            self.report.error("ElmerSolver binary not found.")
            self.pushStatus("Error: ElmerSolver binary has not been found!")
            self.fail()

    def _updateOutput(self, output):
        if self.solver.ElmerOutput is None:
            self._createOutput()
        # check if eigenmodes were calculated and if so append them to output
        output = self._calculateEigenfrequencies(output)
        self.solver.ElmerOutput.Text = output

    def _createOutput(self):
        self.solver.ElmerOutput = self.analysis.Document.addObject(
            "App::TextDocument", self.solver.Name + "Output")
        self.solver.ElmerOutput.Label = self.solver.Label + "Output"
        # App::TextDocument has no Attribute ReadOnly
        # TODO check if the attribute has been removed from App::TextDocument
        # self.solver.ElmerOutput.ReadOnly = True
        self.analysis.addObject(self.solver.ElmerOutput)
        self.solver.Document.recompute()

    def _calculateEigenfrequencies(self, output):
        # takes the EigenSolve results and performs the calculation
        # sqrt(aResult) / 2*PI but with aResult as complex number

        # first search the output file for the results
        OutputList = output.split("\n")
        modeNumber = 0
        modeCount = 0
        real = 0
        imaginary = 0
        haveImaginary = False
        FrequencyList = []
        for line in OutputList:
            LineList = line.split(" ")
            if (
                len(LineList) > 1
                and LineList[0] == "EigenSolve:"
                and LineList[1] == "Computed"
            ):
                # we found a result and take now the next LineList[2] lines
                modeCount = int(LineList[2])
                modeNumber = modeCount
                continue
            if modeCount > 0:
                for LineString in reversed(LineList):
                    # the output of Elmer may vary, we only know the last float
                    # is the imaginary and second to last float the real part
                    if self._isNumber(LineString):
                        if not haveImaginary:
                            imaginary = float(LineString)
                            haveImaginary = True
                        else:
                            real = float(LineString)
                            break
                eigenFreq = complex(real, imaginary)
                haveImaginary = False
                # now we can perform the calculation
                eigenFreq = cmath.sqrt(eigenFreq) / (2 * cmath.pi)
                # create an output line
                FrequencyList.append(
                    "Mode {}: {} Hz".format(modeNumber - modeCount + 1, eigenFreq.real)
                )
                modeCount = modeCount - 1
        if modeNumber > 0:
            # push the results and append to output
            self.pushStatus("\n\nEigenfrequency results:")
            output = output + "\n\nEigenfrequency results:"
            for i in range(0, modeNumber):
                output = output + "\n" + FrequencyList[i]
                self.pushStatus("\n" + FrequencyList[i])
            self.pushStatus("\n")
        return output

    def _isNumber(self, string):
        try:
            float(string)
            return True
        except ValueError:
            return False


class Results(run.Results):

    def run(self):
        if self.solver.SimulationType == "Steady State":
            self._handleStedyStateResult()
        else:
            self._handleTransientResults()

    def _handleStedyStateResult(self):
        if self.solver.ElmerResult is None:
            self._createResults()
        postPath = self._getResultFile()
        if postPath is None:
            self.pushStatus("\nNo result file was created.\n")
            self.fail()
            return
        self.solver.ElmerResult.read(postPath)
        # at the moment we scale the mesh back using Elmer
        # this might be changed in future, this commented code is left as info
        # self.solver.ElmerResult.scale(1000)

        # for eigen analyses the resulting values are by a factor 1000 to high
        # therefore scale all *EigenMode results
        self.solver.ElmerResult.ViewObject.transformField("displacement EigenMode1", 0.001)

        self.solver.ElmerResult.recomputeChildren()
        self.solver.Document.recompute()
        # recompute() updated the result mesh data
        # but not the shape and bar coloring
        self.solver.ElmerResult.ViewObject.updateColorBars()

    def _createResults(self):
        self.solver.ElmerResult = self.analysis.Document.addObject(
            "Fem::FemPostPipeline", self.solver.Name + "Result")
        self.solver.ElmerResult.Label = self.solver.ElmerResult.Name
        self.solver.ElmerResult.ViewObject.SelectionStyle = "BoundBox"
        self.analysis.addObject(self.solver.ElmerResult)
        # to assure the user sees something, set the default to Surface
        self.solver.ElmerResult.ViewObject.DisplayMode = "Surface"

    def _handleTransientResults(self):
        # for transient results we must create a result pipeline for every time
        # the connection between result files and and their time is in the FreeCAD.pvd file
        # therefore first open FreeCAD.pvd
        pvdFilePath = os.path.join(self.directory, "FreeCAD.pvd")
        if not os.path.exists(pvdFilePath):
            self.pushStatus("\nNo result file was created.\n")
            self.fail()
            return
        pvdFile = open(pvdFilePath, "r")
        # read all lines
        pvdContent = pvdFile.readlines()
        # skip header and footer line and evaluate all lines
        # a line has the form like this:
        # <DataSet timestep="   5.000E-02" group="" part="0" file="FreeCAD_t0001.vtu"/>
        # so .split("\"") gives as 2nd the time and as 7th the filename
        for i in range(0, len(pvdContent) - 2):
            # get time
            lineArray = pvdContent[i + 1].split("\"")
            time = float(lineArray[1])
            filename = os.path.join(self.directory, lineArray[7])
            if os.path.isfile(filename):
                self._createTimeResults(time, i + 1)
                self.solver.ElmerTimeResults[i].read(filename)

                # for eigen analyses the resulting values are by a factor 1000 to high
                # therefore scale all *EigenMode results
                self.solver.ElmerTimeResults[i].ViewObject.transformField(
                    "displacement EigenMode1", 0.001
                )

                self.solver.ElmerTimeResults[i].recomputeChildren()
                # recompute() will update the result mesh data
                # but not the shape and bar coloring
                self.solver.ElmerTimeResults[i].ViewObject.updateColorBars()
            else:
                self.pushStatus("\nResult file for time {} is missing.\n".format(time))
                self.fail()
                return
        self.solver.Document.recompute()

    def _createTimeResults(self, time, counter):
        # if self.solver.ElmerTimeResults[counter] exists, but time is different
        # recreate, other wise append
        # FreeCAD would replaces dots in object names with underscores, thus do the same
        newName = self.solver.Name + "_" + str(time).replace(".", "_") + "_" + "Result"
        if counter > len(self.solver.ElmerTimeResults):
            pipeline = self.analysis.Document.addObject(
                "Fem::FemPostPipeline", newName
            )
            # App::PropertyLinkList does not support append
            # thus we have to use a temporary list to append
            tmplist = self.solver.ElmerTimeResults
            tmplist.append(pipeline)
            self.solver.ElmerTimeResults = tmplist
            self._finishTimeResults(time, counter - 1)
        else:
            # recreate if time is not equal
            if self.solver.ElmerTimeResults[counter - 1].Name != newName:
                # store current list before removing object since object removal will automatically
                # remove entry from self.solver.ElmerTimeResults
                tmplist = self.solver.ElmerTimeResults
                self.analysis.Document.removeObject(
                    self.solver.ElmerTimeResults[counter - 1].Name
                )
                tmplist[counter - 1] = self.analysis.Document.addObject(
                    "Fem::FemPostPipeline", newName
                )
                self.solver.ElmerTimeResults = tmplist
                self._finishTimeResults(time, counter - 1)

    def _finishTimeResults(self, time, counter):
        # we purposely use the decimal dot in the label
        self.solver.ElmerTimeResults[counter].Label = (
            "{}_{}_Result"
            .format(self.solver.Name, time)
        )
        self.solver.ElmerTimeResults[counter].ViewObject.OnTopWhenSelected = True
        self.analysis.addObject(self.solver.ElmerTimeResults[counter])
        # to assure the user sees something, set the default to Surface
        self.solver.ElmerTimeResults[counter].ViewObject.DisplayMode = "Surface"

    def _getResultFile(self):
        postPath = None
        # elmer post file path changed with version x.x
        # see https://forum.freecad.org/viewtopic.php?f=18&t=42732
        # workaround
        possible_post_file_old = os.path.join(self.directory, "case0001.vtu")
        possible_post_file_single = os.path.join(self.directory, "FreeCAD_t0001.vtu")
        possible_post_file_multi = os.path.join(self.directory, "FreeCAD_t0001.pvtu")
        # depending on the currently set number of cores we try to load either
        # the multi-thread result or the single result
        if settings.get_cores("ElmerSolver") > 1:
            if os.path.isfile(possible_post_file_multi):
                postPath = possible_post_file_multi
            else:
                self.report.error("Result file not found.")
                self.fail()
        else:
            if os.path.isfile(possible_post_file_single):
                postPath = possible_post_file_single
            elif os.path.isfile(possible_post_file_old):
                postPath = possible_post_file_old
            else:
                self.report.error("Result file not found.")
                self.fail()
        return postPath

##  @}
