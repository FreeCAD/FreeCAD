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

__title__  = "FreeCAD FEM solver Z88 tasks"
__author__ = "Bernd Hahnebach"
__url__    = "https://www.freecadweb.org"

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
from femtools import femutils
from femtools import membertools


class Check(run.Check):

    def run(self):
        self.pushStatus("Checking analysis...\n")
        self.checkMesh()
        self.checkMaterial()


class Prepare(run.Prepare):

    def run(self):
        self.pushStatus("Preparing input files...\n")
        w = writer.FemInputWriterZ88(
            self.analysis,
            self.solver,
            membertools.get_mesh_to_solve(self.analysis)[0],  # pre check has been done already
            membertools.AnalysisMember(self.analysis),
            self.directory
        )
        path = w.write_z88_input()
        # report to user if task succeeded
        if path is not None:
            self.pushStatus("Write completed!")
        else:
            self.pushStatus("Writing Z88 input files failed!")
        # print(path)


class Solve(run.Solve):

    def run(self):
        # AFAIK: z88r needs to be run twice, once in test mode and once in real solve mode
        # the subprocess was just copied, it seems to work :-)
        # TODO: search out for "Vektor GS" and "Vektor KOI" and print values
        # may be compared with the used ones
        self.pushStatus("Executing test solver...\n")
        binary = settings.get_binary("Z88")
        self._process = subprocess.Popen(
            [binary, "-t", "-choly"],
            cwd=self.directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        self.signalAbort.add(self._process.terminate)
        # output = self._observeSolver(self._process)
        self._process.communicate()
        self.signalAbort.remove(self._process.terminate)

        self.pushStatus("Executing real solver...\n")
        binary = settings.get_binary("Z88")
        self._process = subprocess.Popen(
            [binary, "-c", "-choly"],
            cwd=self.directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
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
        self.load_results_z88o2()

    def purge_results(self):
        for m in membertools.get_member(self.analysis, "Fem::FemResultObject"):
            if femutils.is_of_type(m.Mesh, "Fem::MeshResult"):
                self.analysis.Document.removeObject(m.Mesh.Name)
            self.analysis.Document.removeObject(m.Name)
        self.analysis.Document.recompute()

    def load_results_z88o2(self):
        disp_result_file = os.path.join(
            self.directory, "z88o2.txt")
        if os.path.isfile(disp_result_file):
            result_name_prefix = "Z88_" + self.solver.AnalysisType + "_"
            importZ88O2Results.import_z88_disp(
                disp_result_file, self.analysis, result_name_prefix)
        else:
            raise Exception(
                "FEM: No results found at {}!".format(disp_result_file))

##  @}
