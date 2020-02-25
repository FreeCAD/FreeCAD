# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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


__title__ = "OpenSees Tasks"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import os
import subprocess
import os.path

import FreeCAD
import femtools.femutils as femutils
import femtools.membertools as membertools

from .. import run
from .. import settings
from . import writer


_inputFileName = None


class Check(run.Check):

    def run(self):
        self.pushStatus("Checking analysis...\n")
        # self.checkMesh()
        # self.checkMaterial()


class Prepare(run.Prepare):

    def run(self):
        global _inputFileName
        self.pushStatus("Preparing input files...\n")
        w = writer.FemInputWriterOpenSees(
            self.analysis,
            self.solver,
            None, # membertools.get_mesh_to_solve(self.analysis)[0],
            membertools.AnalysisMember(self.analysis),
            self.directory
        )
        path = w.write_opensees_input_file()
        # report to user if task succeeded
        if path is not None:
            self.pushStatus("Write completed!")
        else:
            self.pushStatus("Writing OpenSees input files failed!")
        _inputFileName = os.path.splitext(os.path.basename(path))[0]


class Solve(run.Solve):

    def run(self):
        if not _inputFileName:
            # TODO do not run solver, do not try to read results in a smarter way than an Exception
            raise Exception("Error on writing OpenSees input file.\n")
        infile = _inputFileName + ".in"
        FreeCAD.Console.PrintError("OpenSees-info: infile: {} \n\n".format(infile))

        self.pushStatus("Executing solver...\n")
        binary = settings.get_binary("OpenSees")
        # if something goes wrong the binary path could be set for debugging
        # binary = "/home/hugo/bin/m"
        self._process = subprocess.Popen(
            [binary, "-f", infile],
            cwd=self.directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        self.signalAbort.add(self._process.terminate)
        self._process.communicate()
        self.signalAbort.remove(self._process.terminate)
        # for chatching the output see CalculiX or Elmer solver task module


class Results(run.Results):

    def run(self):
        if not _inputFileName:
            # TODO do not run solver, do not try to read results in a smarter way than an Exception
            raise Exception("Error on writing OpenSees input file.\n")
        self.load_results_opensees()

    def load_results_opensees(self):
        res_file = os.path.join(self.directory, "2DPlaneStress.out.m0.1.vtu")
        FreeCAD.Console.PrintError("OpenSees-info: refile: " + res_file + " \n")
        if os.path.isfile(res_file):
            result_name_prefix = "OpenSees_" + "2DPlaneStress"
            from feminout.importVTKResults import importVtkVtkResult as importVTU
            resobj = importVTU(res_file, result_name_prefix)
            if FreeCAD.GuiUp:
                resobj.ViewObject.DisplayMode = "Surface with Edges"
                resobj.ViewObject.Field = "DisplacementVector"
                import FreeCADGui
                FreeCADGui.SendMsgToActiveView("ViewFit")
                FreeCADGui.ActiveDocument.activeView().viewIsometric()
            self.solver.Document.recompute()
            self.analysis.addObject(resobj)
            # TODO add a wrap factor
        else:
            raise Exception(
                "FEM: No results found at {}!".format(res_file))

##  @}
