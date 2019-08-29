# ***************************************************************************
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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

__title__ = "FreeCAD FEM solver CalculiX tasks"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import os
import subprocess
import os.path

import FreeCAD
import femtools.femutils as femutils
import feminout.importCcxFrdResults as importCcxFrdResults
import feminout.importCcxDatResults as importCcxDatResults

from .. import run
from .. import settings
from . import writer

if FreeCAD.GuiUp:
    from PySide import QtGui


_inputFileName = None


class Check(run.Check):

    def run(self):
        self.pushStatus("Checking analysis...\n")
        self.checkMesh()
        self.checkMaterial()


class Prepare(run.Prepare):

    def run(self):
        global _inputFileName
        self.pushStatus("Preparing input files...\n")
        c = _Container(self.analysis)
        w = writer.FemInputWriterCcx(
            self.analysis,
            self.solver,
            c.mesh,
            c.materials_linear,
            c.materials_nonlinear,
            c.constraints_fixed,
            c.constraints_displacement,
            c.constraints_contact,
            c.constraints_planerotation,
            c.constraints_transform,
            c.constraints_selfweight,
            c.constraints_force,
            c.constraints_pressure,
            c.constraints_temperature,
            c.constraints_heatflux,
            c.constraints_initialtemperature,
            c.beam_sections,
            c.beam_rotations,
            c.shell_thicknesses,
            c.fluid_sections,
            self.directory
        )
        path = w.write_calculix_input_file()
        # report to user if task succeeded
        if path != "":
            self.pushStatus("Write completed!")
        else:
            self.pushStatus("Writing CalculiX input file failed!")
        _inputFileName = os.path.splitext(os.path.basename(path))[0]


class Solve(run.Solve):

    def run(self):
        if not _inputFileName:
            # TODO do not run solver
            # do not try to read results in a smarter way than an Exception
            raise Exception('Error on writing CalculiX input file.\n')
        self.pushStatus("Executing solver...\n")
        binary = settings.get_binary("Calculix")
        self._process = subprocess.Popen(
            [binary, "-i", _inputFileName],
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
        if not _inputFileName:
            # TODO do not run solver
            # do not try to read results in a smarter way than an Exception
            raise Exception('Error on writing CalculiX input file.\n')
        prefs = FreeCAD.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Fem/General")
        if not prefs.GetBool("KeepResultsOnReRun", False):
            self.purge_results()
        self.load_results_ccxfrd()
        self.load_results_ccxdat()

    def purge_results(self):
        for m in femutils.get_member(self.analysis, "Fem::FemResultObject"):
            if femutils.is_of_type(m.Mesh, "Fem::FemMeshResult"):
                self.analysis.Document.removeObject(m.Mesh.Name)
            self.analysis.Document.removeObject(m.Name)
        FreeCAD.ActiveDocument.recompute()

    def load_results_ccxfrd(self):
        frd_result_file = os.path.join(
            self.directory, _inputFileName + '.frd')
        if os.path.isfile(frd_result_file):
            result_name_prefix = 'CalculiX_' + self.solver.AnalysisType + '_'
            importCcxFrdResults.importFrd(
                frd_result_file, self.analysis, result_name_prefix)
        else:
            raise Exception(
                'FEM: No results found at {}!'.format(frd_result_file))

    def load_results_ccxdat(self):
        dat_result_file = os.path.join(
            self.directory, _inputFileName + '.dat')
        if os.path.isfile(dat_result_file):
            mode_frequencies = importCcxDatResults.import_dat(
                dat_result_file, self.analysis)
        else:
            raise Exception(
                'FEM: No .dat results found at {}!'.format(dat_result_file))
        if mode_frequencies:
            for m in femutils.get_member(self.analysis, "Fem::FemResultObject"):
                if m.Eigenmode > 0:
                    for mf in mode_frequencies:
                        if m.Eigenmode == mf['eigenmode']:
                            m.EigenmodeFrequency = mf['frequency']


class _Container(object):

    def __init__(self, analysis):
        self.analysis = analysis

        # get mesh
        mesh, message = femutils.get_mesh_to_solve(self.analysis)
        if mesh is not None:
            self.mesh = mesh
        else:
            if FreeCAD.GuiUp:
                QtGui.QMessageBox.critical(
                    None,
                    "Missing prerequisite",
                    message
                )
            raise Exception(message + '\n')

        # get member
        # materials
        std_mats = self.get_several_member(
            'Fem::Material'
        )
        rei_mats = self.get_several_member(
            'Fem::MaterialReinforced'
        )
        self.materials_linear = std_mats + rei_mats

        self.materials_nonlinear = self.get_several_member(
            'Fem::MaterialMechanicalNonlinear'
        )

        # geometries
        self.beam_sections = self.get_several_member(
            'Fem::FemElementGeometry1D'
        )
        self.beam_rotations = self.get_several_member(
            'Fem::FemElementRotation1D'
        )
        self.fluid_sections = self.get_several_member(
            'Fem::FemElementFluid1D'
        )
        self.shell_thicknesses = self.get_several_member(
            'Fem::FemElementGeometry2D'
        )

        # constraints
        self.constraints_contact = self.get_several_member(
            'Fem::ConstraintContact'
        )
        self.constraints_displacement = self.get_several_member(
            'Fem::ConstraintDisplacement'
        )
        self.constraints_fixed = self.get_several_member(
            'Fem::ConstraintFixed'
        )
        self.constraints_force = self.get_several_member(
            'Fem::ConstraintForce'
        )
        self.constraints_heatflux = self.get_several_member(
            'Fem::ConstraintHeatflux'
        )
        self.constraints_initialtemperature = self.get_several_member(
            'Fem::ConstraintInitialTemperature'
        )
        self.constraints_planerotation = self.get_several_member(
            'Fem::ConstraintPlaneRotation'
        )
        self.constraints_pressure = self.get_several_member(
            'Fem::ConstraintPressure'
        )
        self.constraints_selfweight = self.get_several_member(
            'Fem::ConstraintSelfWeight'
        )
        self.constraints_temperature = self.get_several_member(
            'Fem::ConstraintTemperature'
        )
        self.constraints_transform = self.get_several_member(
            'Fem::ConstraintTransform'
        )

    def get_several_member(self, t):
        return femutils.get_several_member(self.analysis, t)

##  @}
