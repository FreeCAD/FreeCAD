# Unit test for the FEM module

#***************************************************************************
#*   Copyright (c) 2015 - FreeCAD Developers                               *
#*   Author: Przemo Firszt <przemo@firszt.eu>                              *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/

import Fem
import FemToolsCcx
import FreeCAD
import FemAnalysis
import FemSolverCalculix
import MechanicalMaterial
import csv
import tempfile
import unittest

mesh_name = 'Mesh'

home_path = FreeCAD.getHomePath()
temp_dir = tempfile.gettempdir()
test_file_dir = home_path + 'Mod/Fem/test_files/ccx'

static_base_name = 'cube_static'
frequency_base_name = 'cube_frequency'
static_analysis_dir = temp_dir + '/FEM_static'
frequency_analysis_dir = temp_dir + '/FEM_frequency'
static_analysis_inp_file = test_file_dir + '/' + static_base_name + '.inp'
static_expected_values = test_file_dir + "/cube_static_expected_values"
frequency_analysis_inp_file = test_file_dir + '/' + frequency_base_name + '.inp'
frequency_expected_values = test_file_dir + "/cube_frequency_expected_values"
mesh_points_file = test_file_dir + '/mesh_points.csv'
mesh_volumes_file = test_file_dir + '/mesh_volumes.csv'


def fcc_print(message):
    FreeCAD.Console.PrintMessage('{} \n'.format(message))


class FemTest(unittest.TestCase):

    def setUp(self):
        try:
            FreeCAD.setActiveDocument("FemTest")
        except:
            FreeCAD.newDocument("FemTest")
        finally:
            FreeCAD.setActiveDocument("FemTest")
        self.active_doc = FreeCAD.ActiveDocument
        self.box = self.active_doc.addObject("Part::Box", "Box")
        self.active_doc.recompute()

    def create_new_analysis(self):
        self.analysis = FemAnalysis.makeFemAnalysis('MechanicalAnalysis')
        self.active_doc.recompute()

    def create_new_solver(self):
        self.solver_object = FemSolverCalculix.makeFemSolverCalculix('CalculiX')
        self.active_doc.recompute()

    def create_new_mesh(self):
        self.mesh_object = self.active_doc.addObject('Fem::FemMeshObject', mesh_name)
        self.mesh = Fem.FemMesh()
        with open(mesh_points_file, 'r') as points_file:
            reader = csv.reader(points_file)
            for p in reader:
                self.mesh.addNode(float(p[1]), float(p[2]), float(p[3]), int(p[0]))
                
        with open(mesh_volumes_file, 'r') as volumes_file:
            reader = csv.reader(volumes_file)
            for v in reader:
                self.mesh.addVolume([int(v[2]), int(v[1]), int(v[3]), int(v[4]), int(v[5]),
                                    int(v[7]), int(v[6]), int(v[9]), int(v[8]), int(v[10])],
                                    int(v[0]))
                
        self.mesh_object.FemMesh = self.mesh
        self.active_doc.recompute()

    def create_new_material(self):
        self.new_material_object = MechanicalMaterial.makeMechanicalMaterial('MechanicalMaterial')
        mat = self.new_material_object.Material
        mat['Name'] = "Steel-Generic"
        mat['YoungsModulus'] = "200000 MPa"
        mat['PoissonRatio'] = "0.30"
        mat['Density'] = "7900 kg/m^3"
        self.new_material_object.Material = mat

    def create_fixed_constraint(self):
        self.fixed_constraint = self.active_doc.addObject("Fem::ConstraintFixed", "FemConstraintFixed")
        self.fixed_constraint.References = [(self.box, "Face1")]

    def create_force_constraint(self):
        self.force_constraint = self.active_doc.addObject("Fem::ConstraintForce", "FemConstraintForce")
        self.force_constraint.References = [(self.box, "Face6")]
        self.force_constraint.Force = 40000.0
        self.force_constraint.Direction = (self.box, ["Edge5"])
        self.force_constraint.Reversed = True

    def create_pressure_constraint(self):
        self.pressure_constraint = self.active_doc.addObject("Fem::ConstraintPressure", "FemConstraintPressure")
        self.pressure_constraint.References = [(self.box, "Face2")]
        self.pressure_constraint.Pressure = 1000.0
        self.pressure_constraint.Reversed = False

    def force_unix_line_ends(self, line_list):
        new_line_list = []
        for l in line_list:
            if l.endswith("\r\n"):
                l = l[:-2] + '\n'
            new_line_list.append(l)
        return new_line_list

    def compare_inp_files(self, file_name1, file_name2):
        file1 = open(file_name1, 'r')
        f1 = file1.readlines()
        file1.close()
        lf1 = [l for l in f1 if not l.startswith('**   written ')]
        lf1 = self.force_unix_line_ends(lf1)
        file2 = open(file_name2, 'r')
        f2 = file2.readlines()
        file2.close()
        lf2 = [l for l in f2 if not l.startswith('**   written ')]
        lf2 = self.force_unix_line_ends(lf2)
        import difflib
        diff = difflib.unified_diff(lf1, lf2, n=0)
        result = ''
        for l in diff:
            result += l
        if result:
            result = "Comparing {} to {} failed!\n".format(file_name1, file_name2) + result
        return result

    def compare_stats(self, fea, stat_file=None):
        if stat_file:
            sf = open(stat_file, 'r')
            sf_content = sf.readlines()
            sf.close()
            sf_content = self.force_unix_line_ends(sf_content)
        stat_types = ["U1", "U2", "U3", "Uabs", "Sabs"]
        stats = []
        for s in stat_types:
            stats.append("{}: {}\n".format(s, fea.get_stats(s)))
        if sf_content != stats:
            fcc_print("Expected stats from {}".format(stat_file))
            fcc_print(sf_content)
            fcc_print("Stats read from {}.frd file".format(fea.base_name))
            fcc_print(stats)
            return True
        return False

    def test_new_analysis(self):
        fcc_print('--------------- Start of FEM tests ---------------')
        fcc_print('Checking FEM new analysis...')
        self.create_new_analysis()
        self.assertTrue(self.analysis, "FemTest of new analysis failed")

        fcc_print('Checking FEM new solver...')
        self.create_new_solver()
        self.assertTrue(self.solver_object, "FemTest of new solver failed")
        self.analysis.Member = self.analysis.Member + [self.solver_object]

        fcc_print('Checking FEM new mesh...')
        self.create_new_mesh()
        self.assertTrue(self.mesh, "FemTest of new mesh failed")
        self.analysis.Member = self.analysis.Member + [self.mesh_object]

        fcc_print('Checking FEM new material...')
        self.create_new_material()
        self.assertTrue(self.new_material_object, "FemTest of new material failed")
        self.analysis.Member = self.analysis.Member + [self.new_material_object]

        fcc_print('Checking FEM new fixed constraint...')
        self.create_fixed_constraint()
        self.assertTrue(self.fixed_constraint, "FemTest of new fixed constraint failed")
        self.analysis.Member = self.analysis.Member + [self.fixed_constraint]

        fcc_print('Checking FEM new force constraint...')
        self.create_force_constraint()
        self.assertTrue(self.force_constraint, "FemTest of new force constraint failed")
        self.analysis.Member = self.analysis.Member + [self.force_constraint]

        fcc_print('Checking FEM new pressure constraint...')
        self.create_pressure_constraint()
        self.assertTrue(self.pressure_constraint, "FemTest of new pressure constraint failed")
        self.analysis.Member = self.analysis.Member + [self.pressure_constraint]

        fea = FemToolsCcx.FemToolsCcx(self.analysis, test_mode=True)
        fcc_print('Setting up working directory {}'.format(static_analysis_dir))
        fea.setup_working_dir(static_analysis_dir)
        self.assertTrue(True if fea.working_dir == static_analysis_dir else False,
                        "Setting working directory {} failed".format(static_analysis_dir))

        fcc_print('Checking FEM inp file prerequisites for static analysis...')
        error = fea.check_prerequisites()
        self.assertFalse(error, "FemToolsCcx check_prerequisites returned error message: {}".format(error))

        fcc_print('Checking FEM inp file write...')

        fcc_print('Setting analysis type to \'static\"')
        fea.set_analysis_type("static")
        self.assertTrue(True if fea.analysis_type == 'static' else False, "Setting anlysis type to \'static\' failed")

        fcc_print('Writing {}/{}.inp for static analysis'.format(static_analysis_dir, mesh_name))
        error = fea.write_inp_file()
        self.assertFalse(error, "Writing failed")

        fcc_print('Comparing {} to {}/{}.inp'.format(static_analysis_inp_file, static_analysis_dir, mesh_name))
        ret = self.compare_inp_files(static_analysis_inp_file, static_analysis_dir + "/" + mesh_name + '.inp')
        self.assertFalse(ret, "FemToolsCcx write_inp_file test failed.\n{}".format(ret))

        fcc_print('Setting up working directory to {} in order to read simulated calculations'.format(test_file_dir))
        fea.setup_working_dir(test_file_dir)
        self.assertTrue(True if fea.working_dir == test_file_dir else False,
                        "Setting working directory {} failed".format(test_file_dir))

        fcc_print('Setting base name to read test {}.frd file...'.format('cube_static'))
        fea.set_base_name(static_base_name)
        self.assertTrue(True if fea.base_name == static_base_name else False,
                        "Setting base name to {} failed".format(static_base_name))

        fcc_print('Setting inp file name to read test {}.frd file...'.format('cube_static'))
        fea.set_inp_file_name()
        self.assertTrue(True if fea.inp_file_name == static_analysis_inp_file else False,
                        "Setting inp file name to {} failed".format(static_analysis_inp_file))

        fcc_print('Checking FEM frd file read from static analysis...')
        fea.load_results()
        fcc_print('Result object created as \"{}\"'.format(fea.result_object.Name))
        self.assertTrue(fea.results_present, "Cannot read results from {}.frd frd file".format(fea.base_name))

        fcc_print('Reading stats from result object for static analysis...')
        ret = self.compare_stats(fea, static_expected_values)
        self.assertFalse(ret, "Invalid results read from .frd file")

        fcc_print('Setting analysis type to \'frequency\"')
        fea.set_analysis_type("frequency")
        self.assertTrue(True if fea.analysis_type == 'frequency' else False, "Setting anlysis type to \'frequency\' failed")

        fcc_print('Setting up working directory to {} in order to write frequency calculations'.format(frequency_analysis_dir))
        fea.setup_working_dir(frequency_analysis_dir)
        self.assertTrue(True if fea.working_dir == frequency_analysis_dir else False,
                        "Setting working directory {} failed".format(frequency_analysis_dir))

        fcc_print('Setting eigenmode calculation parameters')
        fea.set_eigenmode_parameters(number=10, limit_low=0.0, limit_high=1000000.0)
        self.assertTrue(True if fea.eigenmode_parameters == (10, 0.0, 1000000.0) else False,
                        "Setting eigenmode calculation parameters failed")

        fcc_print('Checking FEM inp file prerequisites for frequency analysis...')
        error = fea.check_prerequisites()
        self.assertFalse(error, "FemToolsCcx check_prerequisites returned error message: {}".format(error))

        fcc_print('Writing {}/{}.inp for frequency analysis'.format(frequency_analysis_dir, mesh_name))
        error = fea.write_inp_file()
        self.assertFalse(error, "Writing failed")

        fcc_print('Comparing {} to {}/{}.inp'.format(frequency_analysis_inp_file, frequency_analysis_dir, mesh_name))
        ret = self.compare_inp_files(frequency_analysis_inp_file, frequency_analysis_dir + "/" + mesh_name + '.inp')
        self.assertFalse(ret, "FemToolsCcx write_inp_file test failed.\n{}".format(ret))

        fcc_print('Setting up working directory to {} in order to read simulated calculations'.format(test_file_dir))
        fea.setup_working_dir(test_file_dir)
        self.assertTrue(True if fea.working_dir == test_file_dir else False,
                        "Setting working directory {} failed".format(test_file_dir))

        fcc_print('Setting base name to read test {}.frd file...'.format(frequency_base_name))
        fea.set_base_name(frequency_base_name)
        self.assertTrue(True if fea.base_name == frequency_base_name else False,
                        "Setting base name to {} failed".format(frequency_base_name))

        fcc_print('Setting inp file name to read test {}.frd file...'.format('cube_frequency'))
        fea.set_inp_file_name()
        self.assertTrue(True if fea.inp_file_name == frequency_analysis_inp_file else False,
                        "Setting inp file name to {} failed".format(frequency_analysis_inp_file))

        fcc_print('Checking FEM frd file read from frequency analysis...')
        fea.load_results()

        fcc_print('Last result object created as \"{}\"'.format(fea.result_object.Name))
        self.assertTrue(fea.results_present, "Cannot read results from {}.frd frd file".format(fea.base_name))

        fcc_print('Reading stats from result object for frequency analysis...')
        ret = self.compare_stats(fea, frequency_expected_values)
        self.assertFalse(ret, "Invalid results read from .frd file")

        fcc_print('--------------- End of FEM tests ---------------')

    def tearDown(self):
        FreeCAD.closeDocument("FemTest")
        pass


# helpers
def open_cube_test():
    cube_file = test_file_dir + '/cube.fcstd'
    FreeCAD.open(cube_file)


def create_cube_test_results():
    import os
    import shutil
    cube_file = test_file_dir + '/cube.fcstd'

    FreeCAD.open(cube_file)
    import FemGui
    FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.MechanicalAnalysis)
    import FemToolsCcx
    fea = FemToolsCcx.FemToolsCcx()

    # static
    fea.reset_all()
    fea.run()

    fea.load_results()
    stat_types = ["U1", "U2", "U3", "Uabs", "Sabs"]
    stats_static = []  # we only have one result object so we are fine
    for s in stat_types:
        stats_static.append("{}: {}\n".format(s, fea.get_stats(s)))
    static_expected_values_file = temp_dir + '/cube_static_expected_values'
    f = open(static_expected_values_file, 'w')
    for s in stats_static:
        f.write(s)
    f.close()

    # could be added in FemToolsCcx to the self object as an Attribut
    frd_result_file = os.path.splitext(fea.inp_file_name)[0] + '.frd'
    dat_result_file = os.path.splitext(fea.inp_file_name)[0] + '.dat'

    frd_static_test_result_file = temp_dir + '/cube_static.frd'
    dat_static_test_result_file = temp_dir + '/cube_static.dat'
    shutil.copyfile(frd_result_file, frd_static_test_result_file)
    shutil.copyfile(dat_result_file, dat_static_test_result_file)

    # frequency
    fea.reset_all()
    fea.set_analysis_type('frequency')
    fea.set_eigenmode_parameters(1)  # we should only have one result object
    fea.run()

    fea.load_results()
    stats_frequency = []  # since we set eigenmodeno. we only have one result object so we are fine
    for s in stat_types:
        stats_frequency.append("{}: {}\n".format(s, fea.get_stats(s)))
    frequency_expected_values_file = temp_dir + '/cube_frequency_expected_values'
    f = open(frequency_expected_values_file, 'w')
    for s in stats_frequency:
        f.write(s)
    f.close()

    frd_frequency_test_result_file = temp_dir + '/cube_frequency.frd'
    dat_frequency_test_result_file = temp_dir + '/cube_frequency.dat'
    shutil.copyfile(frd_result_file, frd_frequency_test_result_file)
    shutil.copyfile(dat_result_file, dat_frequency_test_result_file)

    print('Results copied to: ' + temp_dir)


'''
update the results in FEM untit tests:
start FreeCAD

import TestFem
TestFem.create_cube_test_results()

copy result files from /tmp into the src dirctory
run make
start FreeCAD and run FEM unit test
if FEM unit test is fine --> commit new FEM unit test results

TODO compare the inp file of the helper with the inp file of FEM unit tests
'''
