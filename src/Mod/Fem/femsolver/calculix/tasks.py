# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver CalculiX tasks"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import os
import os.path
import subprocess

import FreeCAD

from . import writer
from .. import run
from .. import settings
from feminout import importCcxDatResults
from feminout import importCcxFrdResults
from femmesh import meshsetsgetter
from femtools import femutils
from femtools import membertools


_inputFileName = None


class Check(run.Check):

    def run(self):
        self.pushStatus("Checking analysis member...\n")
        self.check_mesh_exists()

        # workaround use Calculix ccxtools pre checks
        from femtools.checksanalysis import check_member_for_solver_calculix
        message = check_member_for_solver_calculix(
            self.analysis,
            self.solver,
            membertools.get_mesh_to_solve(self.analysis)[0],
            membertools.AnalysisMember(self.analysis)
        )
        if message:
            text = "CalculiX can not be started...\n"
            self.report.error("{}{}".format(text, message))
            self.fail()
            return


class Prepare(run.Prepare):

    def run(self):
        global _inputFileName
        self.pushStatus("Preparing input...\n")

        # get mesh set data
        # TODO evaluate if it makes sense to add new task
        # between check and prepare to the solver frame work
        mesh_obj = membertools.get_mesh_to_solve(self.analysis)[0]  # pre check done already
        meshdatagetter = meshsetsgetter.MeshSetsGetter(
            self.analysis,
            self.solver,
            mesh_obj,
            membertools.AnalysisMember(self.analysis),
        )
        meshdatagetter.get_mesh_sets()

        # write solver input
        w = writer.FemInputWriterCcx(
            self.analysis,
            self.solver,
            mesh_obj,
            meshdatagetter.member,
            self.directory,
            meshdatagetter.mat_geo_sets
        )
        path = w.write_solver_input()
        # report to user if task succeeded
        if path != "" and os.path.isfile(path):
            self.pushStatus("Writing solver input completed.")
        else:
            self.pushStatus("Writing solver input failed.")
            self.fail()
        _inputFileName = os.path.splitext(os.path.basename(path))[0]


class Solve(run.Solve):

    def run(self):
        self.pushStatus("Executing solver...\n")

        # get solver binary
        self.pushStatus("Get solver binary...\n")
        binary = settings.get_binary("Calculix")
        if binary is None:
            self.pushStatus("Error: The Calculix binary has not been found!")
            self.fail()
            return

        # run solver
        self._process = subprocess.Popen(
            [binary, "-i", _inputFileName],
            cwd=self.directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        self.signalAbort.add(self._process.terminate)
        # output = self._observeSolver(self._process)
        self._process.communicate()
        self.signalAbort.remove(self._process.terminate)
        # if not self.aborted:
        #     self._updateOutput(output)
        # del output   # get flake8 quiet


class Results(run.Results):

    def run(self):
        prefs = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Fem/General")
        if not prefs.GetBool("KeepResultsOnReRun", False):
            self.purge_results()
        self.load_results()

    def purge_results(self):
        self.pushStatus("Purge existing results...\n")
        # TODO dat file will not be removed
        # TODO implement a generic purge method
        # TODO results from other solvers will be removed too
        # the user should decide if purge should only
        # delete this solver results or results from all solvers
        for m in membertools.get_member(self.analysis, "Fem::FemResultObject"):
            if m.Mesh and femutils.is_of_type(m.Mesh, "Fem::MeshResult"):
                self.analysis.Document.removeObject(m.Mesh.Name)
            self.analysis.Document.removeObject(m.Name)
        self.analysis.Document.recompute()

    def load_results(self):
        self.pushStatus("Import new results...\n")
        self.load_ccxfrd_results()
        self.load_ccxdat_results()

    def load_ccxfrd_results(self):
        frd_result_file = os.path.join(
            self.directory, _inputFileName + ".frd")
        if os.path.isfile(frd_result_file):
            result_name_prefix = "CalculiX_" + self.solver.AnalysisType + "_"
            importCcxFrdResults.importFrd(
                frd_result_file, self.analysis, result_name_prefix)
        else:
            # TODO: use solver framework status message system
            FreeCAD.Console.PrintError(
                "FEM: No results found at {}!\n"
                .format(frd_result_file)
            )
            self.fail()

    def load_ccxdat_results(self):
        dat_result_file = os.path.join(
            self.directory, _inputFileName + ".dat")
        if os.path.isfile(dat_result_file):
            mode_frequencies = importCcxDatResults.import_dat(
                dat_result_file, self.analysis)
        else:
            # TODO: use solver framework status message system
            FreeCAD.Console.PrintError(
                "FEM: No results found at {}!\n"
                .format(dat_result_file)
            )
            self.fail()
        if mode_frequencies:
            for m in membertools.get_member(self.analysis, "Fem::FemResultObject"):
                if m.Eigenmode > 0:
                    for mf in mode_frequencies:
                        if m.Eigenmode == mf["eigenmode"]:
                            m.EigenmodeFrequency = mf["frequency"]

##  @}
