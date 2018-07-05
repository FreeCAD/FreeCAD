# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
# *   Copyright (c) 2017 - Bernd Hahnebach <bernd@bimstatik.org>            *
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


__title__ = "CalculiX Tasks"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import os
import subprocess
import os.path

import FreeCAD as App
import femtools.femutils as FemUtils
import feminout.importCcxFrdResults as importCcxFrdResults
import feminout.importCcxDatResults as importCcxDatResults

from .. import run
from .. import settings
from . import writer


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
            self.analysis, self.solver, c.mesh, c.materials_linear,
            c.materials_nonlinear, c.fixed_constraints,
            c.displacement_constraints, c.contact_constraints,
            c.planerotation_constraints, c.transform_constraints,
            c.selfweight_constraints, c.force_constraints,
            c.pressure_constraints, c.temperature_constraints,
            c.heatflux_constraints, c.initialtemperature_constraints,
            c.beam_sections, c.beam_rotations, c.shell_thicknesses, c.fluid_sections,
            self.directory)
        path = w.write_calculix_input_file()
        # report to user if task succeeded
        if path is not None:
            self.pushStatus("Write completed!")
        else:
            self.pushStatus("Writing CalculiX input file failed!")
        _inputFileName = os.path.splitext(os.path.basename(path))[0]


class Solve(run.Solve):

    def run(self):
        self.pushStatus("Executing solver...\n")
        binary = settings.getBinary("Calculix")
        self._process = subprocess.Popen(
            [binary, "-i", _inputFileName],
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

    def _observeSolver(self, process):
        output = ""
        line = process.stdout.readline()
        self.pushStatus(line)
        output += line
        line = process.stdout.readline()
        while line:
            line = "\n%s" % line.rstrip()
            self.pushStatus(line)
            output += line
            line = process.stdout.readline()
        return output


class Results(run.Results):

    def run(self):
        prefs = App.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Fem/General")
        if not prefs.GetBool("KeepResultsOnReRun", False):
            self.purge_results()
        self.load_results_ccxfrd()
        self.load_results_ccxdat()

    def purge_results(self):
        for m in FemUtils.getMember(self.analysis, "Fem::FemResultObject"):
            if FemUtils.isOfType(m.Mesh, "FemMeshResult"):
                self.analysis.Document.removeObject(m.Mesh.Name)
            self.analysis.Document.removeObject(m.Name)
        App.ActiveDocument.recompute()

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
            for m in FemUtils.getMember(self.analysis, "Fem::FemResultObject"):
                if m.Eigenmode > 0:
                    for mf in mode_frequencies:
                        if m.Eigenmode == mf['eigenmode']:
                            m.EigenmodeFrequency = mf['frequency']


class _Container(object):

    def __init__(self, analysis):
        self.mesh = None
        self.materials_linear = []
        self.materials_nonlinear = []
        self.fixed_constraints = []
        self.selfweight_constraints = []
        self.force_constraints = []
        self.pressure_constraints = []
        self.beam_sections = []
        self.beam_rotations = []
        self.fluid_sections = []
        self.shell_thicknesses = []
        self.displacement_constraints = []
        self.temperature_constraints = []
        self.heatflux_constraints = []
        self.initialtemperature_constraints = []
        self.planerotation_constraints = []
        self.contact_constraints = []
        self.transform_constraints = []

        for m in analysis.Group:
            if m.isDerivedFrom("Fem::FemMeshObject"):
                if not self.mesh:
                    self.mesh = m
                else:
                    raise Exception('FEM: Multiple mesh in analysis not yet supported!')
            elif m.isDerivedFrom("App::MaterialObjectPython"):
                material_linear_dict = {}
                material_linear_dict['Object'] = m
                self.materials_linear.append(material_linear_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "Fem::MaterialMechanicalNonlinear":
                material_nonlinear_dict = {}
                material_nonlinear_dict['Object'] = m
                self.materials_nonlinear.append(material_nonlinear_dict)
            elif m.isDerivedFrom("Fem::ConstraintFixed"):
                fixed_constraint_dict = {}
                fixed_constraint_dict['Object'] = m
                self.fixed_constraints.append(fixed_constraint_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "Fem::ConstraintSelfWeight":
                selfweight_dict = {}
                selfweight_dict['Object'] = m
                self.selfweight_constraints.append(selfweight_dict)
            elif m.isDerivedFrom("Fem::ConstraintForce"):
                force_constraint_dict = {}
                force_constraint_dict['Object'] = m
                force_constraint_dict['RefShapeType'] = self.get_refshape_type(m)
                self.force_constraints.append(force_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintPressure"):
                PressureObjectDict = {}
                PressureObjectDict['Object'] = m
                self.pressure_constraints.append(PressureObjectDict)
            elif m.isDerivedFrom("Fem::ConstraintDisplacement"):
                displacement_constraint_dict = {}
                displacement_constraint_dict['Object'] = m
                self.displacement_constraints.append(displacement_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintTemperature"):
                temperature_constraint_dict = {}
                temperature_constraint_dict['Object'] = m
                self.temperature_constraints.append(temperature_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintHeatflux"):
                heatflux_constraint_dict = {}
                heatflux_constraint_dict['Object'] = m
                self.heatflux_constraints.append(heatflux_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintInitialTemperature"):
                initialtemperature_constraint_dict = {}
                initialtemperature_constraint_dict['Object'] = m
                self.initialtemperature_constraints.append(
                    initialtemperature_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintPlaneRotation"):
                planerotation_constraint_dict = {}
                planerotation_constraint_dict['Object'] = m
                self.planerotation_constraints.append(planerotation_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintContact"):
                contact_constraint_dict = {}
                contact_constraint_dict['Object'] = m
                self.contact_constraints.append(contact_constraint_dict)
            elif m.isDerivedFrom("Fem::ConstraintTransform"):
                transform_constraint_dict = {}
                transform_constraint_dict['Object'] = m
                self.transform_constraints.append(transform_constraint_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "Fem::FemElementGeometry1D":
                beam_section_dict = {}
                beam_section_dict['Object'] = m
                self.beam_sections.append(beam_section_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "Fem::FemElementRotation1D":
                beam_rotation_dict = {}
                beam_rotation_dict['Object'] = m
                self.beam_rotations.append(beam_rotation_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "Fem::FemElementFluid1D":
                fluid_section_dict = {}
                fluid_section_dict['Object'] = m
                self.fluid_sections.append(fluid_section_dict)
            elif hasattr(m, "Proxy") and m.Proxy.Type == "Fem::FemElementGeometry2D":
                shell_thickness_dict = {}
                shell_thickness_dict['Object'] = m
                self.shell_thicknesses.append(shell_thickness_dict)

    def get_refshape_type(self, fem_doc_object):
        # returns the reference shape type
        # for force object:
        # in GUI defined frc_obj all frc_obj have at least one ref_shape and ref_shape have all the same shape type
        # for material object:
        # in GUI defined material_obj could have no RefShape and RefShapes could be different type
        # we're going to need the RefShapes to be the same type inside one fem_doc_object
        # TODO here: check if all RefShapes inside the object really have the same type
        import femmesh.meshtools as FemMeshTools
        if hasattr(fem_doc_object, 'References') and fem_doc_object.References:
            first_ref_obj = fem_doc_object.References[0]
            first_ref_shape = FemMeshTools.get_element(first_ref_obj[0], first_ref_obj[1][0])
            st = first_ref_shape.ShapeType
            print(fem_doc_object.Name + ' has ' + st + ' reference shapes.')
            return st
        else:
            print(fem_doc_object.Name + ' has empty References.')
            return ''
