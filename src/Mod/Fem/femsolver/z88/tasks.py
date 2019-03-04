# ***************************************************************************
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
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
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import os
import subprocess
import os.path

import FreeCAD
import femtools.femutils as femutils
import feminout.importZ88O2Results as importZ88O2Results

from .. import run
from .. import settings
from . import writer


class Check(run.Check):

    def run(self):
        self.pushStatus("Checking analysis...\n")
        self.checkMesh()
        self.checkMaterial()


class Prepare(run.Prepare):

    def run(self):
        self.pushStatus("Preparing input files...\n")
        c = _Container(self.analysis)
        w = writer.FemInputWriterZ88(
            self.analysis, self.solver, c.mesh, c.materials_linear,
            c.materials_nonlinear, c.fixed_constraints,
            c.displacement_constraints, c.contact_constraints,
            c.planerotation_constraints, c.transform_constraints,
            c.selfweight_constraints, c.force_constraints,
            c.pressure_constraints, c.temperature_constraints,
            c.heatflux_constraints, c.initialtemperature_constraints,
            c.beam_sections, c.beam_rotations, c.shell_thicknesses, c.fluid_sections,
            self.directory)
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
        # TODO: search out for "Vektor GS" and "Vektor KOI" and print values, may be compared with the used ones
        self.pushStatus("Executing test solver...\n")
        binary = settings.getBinary("Z88")
        self._process = subprocess.Popen(
            [binary, "-t", "-choly"],
            cwd=self.directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        self.signalAbort.add(self._process.terminate)
        output = self._observeSolver(self._process)
        self._process.communicate()
        self.signalAbort.remove(self._process.terminate)

        self.pushStatus("Executing real solver...\n")
        binary = settings.getBinary("Z88")
        self._process = subprocess.Popen(
            [binary, "-c", "-choly"],
            cwd=self.directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        self.signalAbort.add(self._process.terminate)
        output = self._observeSolver(self._process)
        self._process.communicate()
        self.signalAbort.remove(self._process.terminate)
        # if not self.aborted:
        #     self._updateOutput(output)
        del output   # get flake8 quiet


class Results(run.Results):

    def run(self):
        prefs = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Fem/General")
        if not prefs.GetBool("KeepResultsOnReRun", False):
            self.purge_results()
        self.load_results_z88o2()

    def purge_results(self):
        for m in femutils.get_member(self.analysis, "Fem::FemResultObject"):
            if femutils.is_of_type(m.Mesh, "Fem::FemMeshResult"):
                self.analysis.Document.removeObject(m.Mesh.Name)
            self.analysis.Document.removeObject(m.Name)
        FreeCAD.ActiveDocument.recompute()

    def load_results_z88o2(self):
        disp_result_file = os.path.join(
            self.directory, 'z88o2.txt')
        if os.path.isfile(disp_result_file):
            result_name_prefix = 'Z88_' + self.solver.AnalysisType + '_'
            importZ88O2Results.import_z88_disp(
                disp_result_file, self.analysis, result_name_prefix)
        else:
            raise Exception(
                'FEM: No results found at {}!'.format(disp_result_file))


class _Container(object):

    def __init__(self, analysis):
        self.analysis = analysis
        self.mesh = None
        self.materials_linear = self.get_several_member('Fem::Material')
        self.fixed_constraints = self.get_several_member('Fem::ConstraintFixed')
        self.force_constraints = self.get_several_member('Fem::ConstraintForce')
        self.beam_sections = self.get_several_member('Fem::FemElementGeometry1D')
        self.shell_thicknesses = self.get_several_member('Fem::FemElementGeometry2D')

        # constraints not supported by z88
        self.materials_nonlinear = []
        self.selfweight_constraints = []
        self.pressure_constraints = []
        self.beam_rotations = []
        self.fluid_sections = []
        self.displacement_constraints = []
        self.temperature_constraints = []
        self.heatflux_constraints = []
        self.initialtemperature_constraints = []
        self.planerotation_constraints = []
        self.contact_constraints = []
        self.transform_constraints = []

        for m in self.analysis.Group:
            if m.isDerivedFrom("Fem::FemMeshObject") and not femutils.is_of_type(m, 'Fem::FemMeshResult'):
                if not self.mesh:
                    self.mesh = m
                else:
                    raise Exception('FEM: Multiple mesh in analysis not yet supported!')

    def get_several_member(self, t):
        return femutils.get_several_member(self.analysis, t)

##  @}
