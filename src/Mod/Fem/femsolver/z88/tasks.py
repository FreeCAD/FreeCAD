# ***************************************************************************
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

__title__ = "FreeCAD FEM solver Z88 tasks"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"

## \addtogroup FEM
#  @{

import os
import os.path
import subprocess

import FreeCAD

from . import writer
from .. import run
from .. import settings
from feminout import importZ88O2Results
from femmesh import meshsetsgetter
from femtools import femutils
from femtools import membertools

SOLVER_TYPES = ["sorcg", "siccg", "choly"]

class Check(run.Check):

    def run(self):
        self.pushStatus("Checking analysis member...\n")
        self.check_mesh_exists()
        self.check_material_exists()
        self.check_material_single()  # no multiple material
        self.check_geos_beamsection_single()  # no multiple beamsection
        self.check_geos_shellthickness_single()  # no multiple shellsection
        self.check_geos_beamsection_and_shellthickness()  # either beams or shells


class Prepare(run.Prepare):

    def run(self):
        self.pushStatus("Preparing solver input...\n")

        # get mesh set data
        # TODO see calculix tasks get mesh set data
        mesh_obj = membertools.get_mesh_to_solve(self.analysis)[0]  # pre check done already
        meshdatagetter = meshsetsgetter.MeshSetsGetter(
            self.analysis,
            self.solver,
            mesh_obj,
            membertools.AnalysisMember(self.analysis),
        )
        meshdatagetter.get_mesh_sets()

        # write solver input
        w = writer.FemInputWriterZ88(
            self.analysis,
            self.solver,
            mesh_obj,
            meshdatagetter.member,
            self.directory
        )
        path = w.write_solver_input()
        # report to user if task succeeded
        if path is not None:
            self.pushStatus("Writing solver input completed.")
        else:
            self.pushStatus("Writing solver input failed.")
            self.fail()
        # print(path)
        # z88 does not pass a main input file to the solver
        # it passes the directory all input files are in
        # not _inputFileName is needed


class Solve(run.Solve):

    def run(self):
        self.pushStatus("Executing test solver...\n")

        # get solver binary
        self.pushStatus("Get solver binary...\n")
        binary = settings.get_binary("Z88")
        if binary is None:
            self.fail()  # a print has been made in settings module
        
        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Z88")
        solver = SOLVER_TYPES
        solver = prefs.GetInt("Solver", 0)
        solver = SOLVER_TYPES[solver]
        self.pushStatus("Used solver: " + solver + "\n")

        # run solver test mode
        # AFAIK: z88r needs to be run twice
        # once in test mode and once in real solve mode
        # the subprocess was just copied, it works :-)
        # TODO: search out for "Vector GS" and "Vector KOI" and print values
        # may be compare with the used ones
        self.pushStatus("Executing solver in test mode...\n")
        self._process = subprocess.Popen(
            [binary, "-t", "-" + solver],
            cwd=self.directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        self.signalAbort.add(self._process.terminate)
        self._process.communicate()
        self.signalAbort.remove(self._process.terminate)

        # run solver real mode
        self.pushStatus("Executing solver in real mode...\n")
        binary = settings.get_binary("Z88")
        self._process = subprocess.Popen(
            [binary, "-c", "-" + solver],
            cwd=self.directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        self.signalAbort.add(self._process.terminate)
        self._process.communicate()
        self.signalAbort.remove(self._process.terminate)

        # for chatching the output see CalculiX or Elmer solver tasks module


class Results(run.Results):

    def run(self):
        prefs = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Fem/General")
        if not prefs.GetBool("KeepResultsOnReRun", False):
            self.purge_results()
        self.load_results()

    def purge_results(self):
        self.pushStatus("Purge existing results...\n")
        # TODO see calculix result tasks
        for m in membertools.get_member(self.analysis, "Fem::FemResultObject"):
            if femutils.is_of_type(m.Mesh, "Fem::MeshResult"):
                self.analysis.Document.removeObject(m.Mesh.Name)
            self.analysis.Document.removeObject(m.Name)
        self.analysis.Document.recompute()

    def load_results(self):
        self.pushStatus("Import new results...\n")
        # displacements from z88o2 file
        disp_result_file = os.path.join(
            self.directory, "z88o2.txt")
        if os.path.isfile(disp_result_file):
            result_name_prefix = "Z88_" + self.solver.AnalysisType + "_"
            importZ88O2Results.import_z88_disp(
                disp_result_file, self.analysis, result_name_prefix)
        else:
            # TODO: use solver framework status message system
            FreeCAD.Console.PrintError(
                "FEM: No results found at {}!\n"
                .format(disp_result_file)
            )
            self.fail()

##  @}
