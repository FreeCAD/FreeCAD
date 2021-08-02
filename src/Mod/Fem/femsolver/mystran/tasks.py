# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver Mystran tasks"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"

## \addtogroup FEM
#  @{

import os
import os.path
import subprocess

import FreeCAD


try:
    import hfcMystranNeuIn
    result_reading = True
except Exception:
    FreeCAD.Console.PrintWarning("Module to read results not found.\n")
    result_reading = False


from . import writer
from .. import run
from .. import settings
from femmesh import meshsetsgetter
from femtools import femutils
from femtools import membertools


_inputFileName = None


class Check(run.Check):

    def run(self):
        self.pushStatus("Checking analysis...\n")
        self.check_mesh_exists()
        self.check_material_exists()
        self.check_material_single()  # no multiple material
        self.check_geos_beamsection_single()  # no multiple beamsection
        self.check_geos_shellthickness_single()  # no multiple shellsection
        self.check_geos_beamsection_and_shellthickness()  # either beams or shells


class Prepare(run.Prepare):

    def run(self):
        global _inputFileName
        self.pushStatus("Preparing input files...\n")

        mesh_obj = membertools.get_mesh_to_solve(self.analysis)[0]  # pre check done already

        # get mesh set data
        # TODO evaluate if it makes sense to add new task
        # between check and prepare to the solver frame work
        meshdatagetter = meshsetsgetter.MeshSetsGetter(
            self.analysis,
            self.solver,
            mesh_obj,
            membertools.AnalysisMember(self.analysis),
        )
        meshdatagetter.get_mesh_sets()

        # write input file
        w = writer.FemInputWriterMystran(
            self.analysis,
            self.solver,
            mesh_obj,
            meshdatagetter.member,
            self.directory,
            meshdatagetter.mat_geo_sets
        )
        path = w.write_solver_input()
        # report to user if task succeeded
        if path != "":
            self.pushStatus("Write completed!")
        else:
            self.pushStatus("Writing CalculiX input file failed!")
        _inputFileName = os.path.splitext(os.path.basename(path))[0]


class Solve(run.Solve):

    def run(self):
        # print(_inputFileName)
        if not _inputFileName:
            # TODO do not run solver, do not try to read results in a smarter way than an Exception
            raise Exception("Error on writing Mystran input file.\n")
        infile = _inputFileName + ".bdf"

        # TODO use solver framework status system
        FreeCAD.Console.PrintMessage("Mystran: solver input file: {} \n\n".format(infile))

        # get binary
        self.pushStatus("Get solver...\n")
        binary = settings.get_binary("Mystran")
        # use preferences editor to add a group Mystran and the prefs:
        # "UseStandardMystranLocation" --> bool, set to False
        # "mystranBinaryPath, string" --> the binary path
        if binary is None:
            return  # a print has been made in settings module

        # run solver
        self.pushStatus("Executing solver...\n")
        self._process = subprocess.Popen(
            args=[binary, infile],  # pass empty param fails! [binary, "", infile]
            cwd=self.directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
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
        if result_reading is True:
            self.load_results()  # ToDo in all solvers generischer name

    def purge_results(self):
        for m in membertools.get_member(self.analysis, "Fem::FemResultObject"):
            if femutils.is_of_type(m.Mesh, "Fem::MeshResult"):
                self.analysis.Document.removeObject(m.Mesh.Name)
            self.analysis.Document.removeObject(m.Name)
        self.analysis.Document.recompute()
        # deletes all results from any solver
        # TODO: delete only the mystran results, fix in all solver

    def load_results(self):
        self.pushStatus("Import results...\n")
        neu_result_file = os.path.join(self.directory, _inputFileName + ".NEU")
        if os.path.isfile(neu_result_file):
            hfcMystranNeuIn.import_neu(neu_result_file)
            # Workaround to move result object into analysis
            for o in self.analysis.Document.Objects:
                if o.Name == "Displacement0":
                    self.analysis.addObject(o)
                    break
        else:
            # TODO: use solver framework error and status message system
            FreeCAD.Console.PrintError(
                "FEM: No results found at {}!\n".format(neu_result_file)
            )
            return


##  @}
