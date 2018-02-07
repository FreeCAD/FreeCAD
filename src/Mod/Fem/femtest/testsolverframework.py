# Unit test for the FEM module

# ***************************************************************************
# *   Copyright (c) 2015 - FreeCAD Developers                               *
# *   Author: Przemo Firszt <przemo@firszt.eu>                              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/

import Fem
from femtools import ccxtools
import femresult.resulttools as resulttools
import FreeCAD
import ObjectsFem
import femsolver.run
import tempfile
import unittest
import os
from .testtools import fcc_print
from .testtools import get_defmake_count
from .testtools import compare_inp_files
from .testtools import compare_files
from .testtools import compare_stats
from .testtools import force_unix_line_ends
from .testtools import collect_python_modules


mesh_name = 'Mesh'
stat_types = ["U1", "U2", "U3", "Uabs", "Sabs", "MaxPrin", "MidPrin", "MinPrin", "MaxShear", "Peeq", "Temp", "MFlow", "NPress"]

home_path = FreeCAD.getHomePath()
temp_dir = tempfile.gettempdir() + '/FEM_unittests/'
if not os.path.exists(temp_dir):
    os.makedirs(temp_dir)
test_file_dir = home_path + 'Mod/Fem/femtest/testfiles/ccx/'
test_file_dir_elmer = home_path + 'Mod/Fem/femtest/testfiles/elmer/'

# define some locations fot the analysis tests
# since they are also used in the helper def which create results they should stay global for the module
static_base_name = 'cube_static'
static_analysis_dir = temp_dir + 'FEM_ccx_static/'
static_save_fc_file = static_analysis_dir + static_base_name + '.fcstd'
static_analysis_inp_file = test_file_dir + static_base_name + '.inp'
static_expected_values = test_file_dir + "cube_static_expected_values"

frequency_base_name = 'cube_frequency'
frequency_analysis_dir = temp_dir + 'FEM_ccx_frequency/'
frequency_save_fc_file = frequency_analysis_dir + frequency_base_name + '.fcstd'
frequency_analysis_inp_file = test_file_dir + frequency_base_name + '.inp'
frequency_expected_values = test_file_dir + "cube_frequency_expected_values"

thermomech_base_name = 'spine_thermomech'
thermomech_analysis_dir = temp_dir + 'FEM_ccx_thermomech/'
thermomech_save_fc_file = thermomech_analysis_dir + thermomech_base_name + '.fcstd'
thermomech_analysis_inp_file = test_file_dir + thermomech_base_name + '.inp'
thermomech_expected_values = test_file_dir + "spine_thermomech_expected_values"

Flow1D_thermomech_base_name = 'Flow1D_thermomech'
Flow1D_thermomech_analysis_dir = temp_dir + 'FEM_ccx_Flow1D_thermomech/'
Flow1D_thermomech_save_fc_file = Flow1D_thermomech_analysis_dir + Flow1D_thermomech_base_name + '.fcstd'
Flow1D_thermomech_analysis_inp_file = test_file_dir + Flow1D_thermomech_base_name + '.inp'
Flow1D_thermomech_expected_values = test_file_dir + "Flow1D_thermomech_expected_values"

solverframework_analysis_dir = temp_dir + 'FEM_solverframework/'
solverframework_save_fc_file = solverframework_analysis_dir + static_base_name + '.fcstd'


class SolverFrameWorkTest(unittest.TestCase):

    def setUp(self):
        try:
            FreeCAD.setActiveDocument("FemTest")
        except:
            FreeCAD.newDocument("FemTest")
        finally:
            FreeCAD.setActiveDocument("FemTest")
        self.active_doc = FreeCAD.ActiveDocument

    def test_solver_framework(self):
        fcc_print('--------------- Start of FEM tests  solver frame work ---------------')
        box = self.active_doc.addObject("Part::Box", "Box")
        fcc_print('Checking FEM new analysis...')
        analysis = ObjectsFem.makeAnalysis(self.active_doc, 'Analysis')
        self.assertTrue(analysis, "FemTest of new analysis failed")

        fcc_print('Checking FEM new material...')
        material_object = ObjectsFem.makeMaterialSolid(self.active_doc, 'MechanicalMaterial')
        mat = material_object.Material
        mat['Name'] = "Steel-Generic"
        mat['YoungsModulus'] = "200000 MPa"
        mat['PoissonRatio'] = "0.30"
        mat['Density'] = "7900 kg/m^3"
        material_object.Material = mat
        self.assertTrue(material_object, "FemTest of new material failed")
        analysis.addObject(material_object)

        fcc_print('Checking FEM new fixed constraint...')
        fixed_constraint = self.active_doc.addObject("Fem::ConstraintFixed", "FemConstraintFixed")
        fixed_constraint.References = [(box, "Face1")]
        self.assertTrue(fixed_constraint, "FemTest of new fixed constraint failed")
        analysis.addObject(fixed_constraint)

        fcc_print('Checking FEM new force constraint...')
        force_constraint = self.active_doc.addObject("Fem::ConstraintForce", "FemConstraintForce")
        force_constraint.References = [(box, "Face6")]
        force_constraint.Force = 40000.0
        force_constraint.Direction = (box, ["Edge5"])
        self.active_doc.recompute()
        force_constraint.Reversed = True
        self.active_doc.recompute()
        self.assertTrue(force_constraint, "FemTest of new force constraint failed")
        analysis.addObject(force_constraint)

        fcc_print('Checking FEM new pressure constraint...')
        pressure_constraint = self.active_doc.addObject("Fem::ConstraintPressure", "FemConstraintPressure")
        pressure_constraint.References = [(box, "Face2")]
        pressure_constraint.Pressure = 1000.0
        pressure_constraint.Reversed = False
        self.assertTrue(pressure_constraint, "FemTest of new pressure constraint failed")
        analysis.addObject(pressure_constraint)

        fcc_print('Checking FEM new mesh...')
        from .testfiles.ccx.cube_mesh import create_nodes_cube
        from .testfiles.ccx.cube_mesh import create_elements_cube
        mesh = Fem.FemMesh()
        ret = create_nodes_cube(mesh)
        self.assertTrue(ret, "Import of mesh nodes failed")
        ret = create_elements_cube(mesh)
        self.assertTrue(ret, "Import of mesh volumes failed")
        mesh_object = self.active_doc.addObject('Fem::FemMeshObject', mesh_name)
        mesh_object.FemMesh = mesh
        self.assertTrue(mesh, "FemTest of new mesh failed")
        analysis.addObject(mesh_object)

        self.active_doc.recompute()

        # solver frame work ccx solver
        fcc_print('Checking FEM solver for solver frame work...')
        solver_ccx2_object = ObjectsFem.makeSolverCalculix(self.active_doc, 'SolverCalculiX')
        solver_ccx2_object.AnalysisType = 'static'
        solver_ccx2_object.GeometricalNonlinearity = 'linear'
        solver_ccx2_object.ThermoMechSteadyState = False
        solver_ccx2_object.MatrixSolverType = 'default'
        solver_ccx2_object.IterationsControlParameterTimeUse = False
        solver_ccx2_object.EigenmodesCount = 10
        solver_ccx2_object.EigenmodeHighLimit = 1000000.0
        solver_ccx2_object.EigenmodeLowLimit = 0.0
        self.assertTrue(solver_ccx2_object, "FemTest of new ccx solver failed")
        analysis.addObject(solver_ccx2_object)

        fcc_print('Checking inpfile writing for solverframework_save_fc_file frame work...')
        if not os.path.exists(solverframework_analysis_dir):  # solver frameworkd does explicit not create a non existing directory
            os.makedirs(solverframework_analysis_dir)

        fcc_print('machine_ccx')
        machine_ccx = solver_ccx2_object.Proxy.createMachine(solver_ccx2_object, solverframework_analysis_dir)
        fcc_print('Machine testmode: ' + str(machine_ccx.testmode))
        machine_ccx.target = femsolver.run.PREPARE
        machine_ccx.start()
        machine_ccx.join()  # wait for the machine to finish.
        fcc_print('Comparing {} to {}/{}.inp'.format(static_analysis_inp_file, solverframework_analysis_dir, mesh_name))
        ret = compare_inp_files(static_analysis_inp_file, solverframework_analysis_dir + mesh_name + '.inp')
        self.assertFalse(ret, "ccxtools write_inp_file test failed.\n{}".format(ret))

        # use solver frame work elmer solver
        solver_elmer_object = ObjectsFem.makeSolverElmer(self.active_doc, 'SolverElmer')
        self.assertTrue(solver_elmer_object, "FemTest of elmer solver failed")
        analysis.addObject(solver_elmer_object)
        solver_elmer_eqobj = ObjectsFem.makeEquationElasticity(self.active_doc, solver_elmer_object)
        self.assertTrue(solver_elmer_eqobj, "FemTest of elmer elasticity equation failed")

        # set ThermalExpansionCoefficient, current elmer seems to need it even on simple elasticity analysis
        mat = material_object.Material
        mat['ThermalExpansionCoefficient'] = "0 um/m/K"  # FIXME elmer elasticity needs the dictionary key, otherwise it fails
        material_object.Material = mat

        mesh_gmsh = ObjectsFem.makeMeshGmsh(self.active_doc)
        mesh_gmsh.CharacteristicLengthMin = "9 mm"
        mesh_gmsh.FemMesh = mesh_object.FemMesh  # elmer needs a GMHS mesh object, FIXME error message on Python solver run
        mesh_gmsh.Part = box
        analysis.addObject(mesh_gmsh)
        self.active_doc.removeObject(mesh_object.Name)

        fcc_print('machine_elmer')
        machine_elmer = solver_elmer_object.Proxy.createMachine(solver_elmer_object, solverframework_analysis_dir, True)
        fcc_print('Machine testmode: ' + str(machine_elmer.testmode))
        machine_elmer.target = femsolver.run.PREPARE
        machine_elmer.start()
        machine_elmer.join()  # wait for the machine to finish.

        '''
        fcc_print('Test writing STARTINFO file')
        fcc_print('Comparing {} to {}'.format(test_file_dir_elmer + 'ELMERSOLVER_STARTINFO', solverframework_analysis_dir + 'ELMERSOLVER_STARTINFO'))
        ret = compare_files(test_file_dir_elmer + 'ELMERSOLVER_STARTINFO', solverframework_analysis_dir + 'ELMERSOLVER_STARTINFO')
        self.assertFalse(ret, "STARTINFO write file test failed.\n{}".format(ret))

        fcc_print('Test writing case file')
        fcc_print('Comparing {} to {}'.format(test_file_dir_elmer + 'case.sif', solverframework_analysis_dir + 'case.sif'))
        ret = compare_files(test_file_dir_elmer + 'case.sif', solverframework_analysis_dir + 'case.sif')
        self.assertFalse(ret, "case write file test failed.\n{}".format(ret))

        fcc_print('Test writing GMSH geo file')
        fcc_print('Comparing {} to {}'.format(test_file_dir_elmer + 'group_mesh.geo', solverframework_analysis_dir + 'group_mesh.geo'))
        ret = compare_files(test_file_dir_elmer + 'group_mesh.geo', solverframework_analysis_dir + 'group_mesh.geo')
        self.assertFalse(ret, "GMSH geo write file test failed.\n{}".format(ret))
        '''

        fcc_print('Save FreeCAD file for static2 analysis to {}...'.format(solverframework_save_fc_file))
        self.active_doc.saveAs(solverframework_save_fc_file)
        fcc_print('--------------- End of FEM tests solver frame work ---------------')

    def tearDown(self):
        FreeCAD.closeDocument("FemTest")
        pass
