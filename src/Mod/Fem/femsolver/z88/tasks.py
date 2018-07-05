# ***************************************************************************
# *                                                                         *
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


__title__ = "Z88 Tasks"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import os
import subprocess
import os.path

import FreeCAD as App
import femtools.femutils as FemUtils
import feminout.importZ88O2Results as importZ88O2Results

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
        _inputFileName = os.path.splitext(os.path.basename(path))[0]  # AFAIK empty for z88
        # print(path)
        # print(_inputFileName)


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
        self.load_results_z88o2()

    def purge_results(self):
        for m in FemUtils.getMember(self.analysis, "Fem::FemResultObject"):
            if FemUtils.isOfType(m.Mesh, "FemMeshResult"):
                self.analysis.Document.removeObject(m.Mesh.Name)
            self.analysis.Document.removeObject(m.Name)
        App.ActiveDocument.recompute()

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
        # TODO: check if all RefShapes inside the object really have the same type
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
