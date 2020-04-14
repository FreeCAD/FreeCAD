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
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import os.path
import subprocess
import sys

import FreeCAD

from . import writer
from .. import run
from .. import settings
from femtools import femutils
from femtools import membertools


class Check(run.Check):

    def run(self):
        self.pushStatus("Checking analysis...\n")
        if (self.checkMesh()):
            self.checkMeshType()
        self.checkMaterial()
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
        self.pushStatus("Preparing input files...\n")
        FreeCAD.Console.PrintMessage("Machine testmode: {}\n".format(self.testmode))
        if self.testmode:
            w = writer.Writer(self.solver, self.directory, True)  # test mode
        else:
            w = writer.Writer(self.solver, self.directory)
        try:
            w.write()
            self.checkHandled(w)
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
        self.pushStatus("Executing solver...\n")
        binary = settings.get_binary("ElmerSolver")
        if binary is not None:
            self._process = subprocess.Popen(
                [binary], cwd=self.directory,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
            self.signalAbort.add(self._process.terminate)
            output = self._observeSolver(self._process)
            self._process.communicate()
            self.signalAbort.remove(self._process.terminate)
            if not self.aborted:
                self._updateOutput(output)
        else:
            self.report.error("ElmerSolver executable not found.")
            self.fail()

    def _updateOutput(self, output):
        if self.solver.ElmerOutput is None:
            self._createOutput()
        if sys.version_info.major >= 3:
            self.solver.ElmerOutput.Text = output
        else:
            self.solver.ElmerOutput.Text = output.decode("utf-8")

    def _createOutput(self):
        self.solver.ElmerOutput = self.analysis.Document.addObject(
            "App::TextDocument", self.solver.Name + "Output")
        self.solver.ElmerOutput.Label = self.solver.Label + "Output"
        self.solver.ElmerOutput.ReadOnly = True
        self.analysis.addObject(self.solver.ElmerOutput)


class Results(run.Results):

    def run(self):
        if self.solver.ElmerResult is None:
            self._createResults()
        postPath = self._getResultFile()
        self.solver.ElmerResult.read(postPath)
        self.solver.ElmerResult.getLastPostObject().touch()
        self.solver.Document.recompute()

    def _createResults(self):
        self.solver.ElmerResult = self.analysis.Document.addObject(
            "Fem::FemPostPipeline", self.solver.Name + "Result")
        self.solver.ElmerResult.Label = self.solver.Label + "Result"
        self.analysis.addObject(self.solver.ElmerResult)

    def _getResultFile(self):
        postPath = None
        # elmer post file path changed with version x.x
        # see https://forum.freecadweb.org/viewtopic.php?f=18&t=42732
        # workaround
        possible_post_file_0 = os.path.join(self.directory, "case0001.vtu")
        possible_post_file_t = os.path.join(self.directory, "case_t0001.vtu")
        if os.path.isfile(possible_post_file_0):
            postPath = possible_post_file_0
        elif os.path.isfile(possible_post_file_t):
            postPath = possible_post_file_t
        else:
            self.report.error("Result file not found.")
            self.fail()
        return postPath

##  @}
