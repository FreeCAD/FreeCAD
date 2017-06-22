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
import FemToolsCcx
import FreeCAD
import ObjectsFem
import tempfile
import unittest

mesh_name = 'Mesh'

home_path = FreeCAD.getHomePath()
temp_dir = tempfile.gettempdir() + '/FEM_unittests'
test_file_dir = home_path + 'Mod/Fem/test_files/ccx'


# define some locations fot the analysis tests
# since they are also used in the helper def which create results they should stay global for the module
static_base_name = 'cube_static'
static_analysis_dir = temp_dir + '/FEM_static'
static_save_fc_file = static_analysis_dir + '/' + static_base_name + '.fcstd'
static_analysis_inp_file = test_file_dir + '/' + static_base_name + '.inp'
static_expected_values = test_file_dir + "/cube_static_expected_values"

frequency_base_name = 'cube_frequency'
frequency_analysis_dir = temp_dir + '/FEM_frequency'
frequency_save_fc_file = frequency_analysis_dir + '/' + frequency_base_name + '.fcstd'
frequency_analysis_inp_file = test_file_dir + '/' + frequency_base_name + '.inp'
frequency_expected_values = test_file_dir + "/cube_frequency_expected_values"

thermomech_base_name = 'spine_thermomech'
thermomech_analysis_dir = temp_dir + '/FEM_thermomech'
thermomech_save_fc_file = thermomech_analysis_dir + '/' + thermomech_base_name + '.fcstd'
thermomech_analysis_inp_file = test_file_dir + '/' + thermomech_base_name + '.inp'
thermomech_expected_values = test_file_dir + "/spine_thermomech_expected_values"


class FemTest(unittest.TestCase):
    def setUp(self):
        try:
            FreeCAD.setActiveDocument("FemTest")
        except:
            FreeCAD.newDocument("FemTest")
        finally:
            FreeCAD.setActiveDocument("FemTest")
        self.active_doc = FreeCAD.ActiveDocument

    def test_unv_save_load(self):
        tetra10 = Fem.FemMesh()
        tetra10.addNode(6, 12, 18, 1)
        tetra10.addNode(0, 0, 18, 2)
        tetra10.addNode(12, 0, 18, 3)
        tetra10.addNode(6, 6, 0, 4)

        tetra10.addNode(3, 6, 18, 5)
        tetra10.addNode(6, 0, 18, 6)
        tetra10.addNode(9, 6, 18, 7)

        tetra10.addNode(6, 9, 9, 8)
        tetra10.addNode(3, 3, 9, 9)
        tetra10.addNode(9, 3, 9, 10)
        tetra10.addVolume([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])

        unv_file = temp_dir + '/tetra10_mesh.unv'
        tetra10.write(unv_file)
        newmesh = Fem.read(unv_file)
        expected = (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
        self.assertEqual(newmesh.getElementNodes(1), expected, "Nodes order of quadratic volume element is unexpected")

    def tearDown(self):
        FreeCAD.closeDocument("FemTest")
        pass


class FemCcxAnalysisTest(unittest.TestCase):

    def setUp(self):
        try:
            FreeCAD.setActiveDocument("FemTest")
        except:
            FreeCAD.newDocument("FemTest")
        finally:
            FreeCAD.setActiveDocument("FemTest")
        self.active_doc = FreeCAD.ActiveDocument

    def test_static_freq_analysis(self):
        # static
        fcc_print('--------------- Start of FEM tests ---------------')
        box = self.active_doc.addObject("Part::Box", "Box")
        fcc_print('Checking FEM new analysis...')
        analysis = ObjectsFem.makeAnalysis('Analysis')
        self.assertTrue(analysis, "FemTest of new analysis failed")

        fcc_print('Checking FEM new solver...')
        solver_object = ObjectsFem.makeSolverCalculix('CalculiX')
        solver_object.GeometricalNonlinearity = 'linear'
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = 'default'
        solver_object.IterationsControlParameterTimeUse = False
        solver_object.EigenmodesCount = 10
        solver_object.EigenmodeHighLimit = 1000000.0
        solver_object.EigenmodeLowLimit = 0.0
        self.assertTrue(solver_object, "FemTest of new solver failed")
        analysis.Member = analysis.Member + [solver_object]

        fcc_print('Checking FEM new material...')
        new_material_object = ObjectsFem.makeMaterialSolid('MechanicalMaterial')
        mat = new_material_object.Material
        mat['Name'] = "Steel-Generic"
        mat['YoungsModulus'] = "200000 MPa"
        mat['PoissonRatio'] = "0.30"
        mat['Density'] = "7900 kg/m^3"
        new_material_object.Material = mat
        self.assertTrue(new_material_object, "FemTest of new material failed")
        analysis.Member = analysis.Member + [new_material_object]

        fcc_print('Checking FEM new fixed constraint...')
        fixed_constraint = self.active_doc.addObject("Fem::ConstraintFixed", "FemConstraintFixed")
        fixed_constraint.References = [(box, "Face1")]
        self.assertTrue(fixed_constraint, "FemTest of new fixed constraint failed")
        analysis.Member = analysis.Member + [fixed_constraint]

        fcc_print('Checking FEM new force constraint...')
        force_constraint = self.active_doc.addObject("Fem::ConstraintForce", "FemConstraintForce")
        force_constraint.References = [(box, "Face6")]
        force_constraint.Force = 40000.0
        force_constraint.Direction = (box, ["Edge5"])
        self.active_doc.recompute()
        force_constraint.Reversed = True
        self.active_doc.recompute()
        self.assertTrue(force_constraint, "FemTest of new force constraint failed")
        analysis.Member = analysis.Member + [force_constraint]

        fcc_print('Checking FEM new pressure constraint...')
        pressure_constraint = self.active_doc.addObject("Fem::ConstraintPressure", "FemConstraintPressure")
        pressure_constraint.References = [(box, "Face2")]
        pressure_constraint.Pressure = 1000.0
        pressure_constraint.Reversed = False
        self.assertTrue(pressure_constraint, "FemTest of new pressure constraint failed")
        analysis.Member = analysis.Member + [pressure_constraint]

        fcc_print('Checking FEM new mesh...')
        from test_files.ccx.cube_mesh import create_nodes_cube, create_elements_cube
        mesh = Fem.FemMesh()
        ret = create_nodes_cube(mesh)
        self.assertTrue(ret, "Import of mesh nodes failed")
        ret = create_elements_cube(mesh)
        self.assertTrue(ret, "Import of mesh volumes failed")
        mesh_object = self.active_doc.addObject('Fem::FemMeshObject', mesh_name)
        mesh_object.FemMesh = mesh
        self.assertTrue(mesh, "FemTest of new mesh failed")
        analysis.Member = analysis.Member + [mesh_object]

        self.active_doc.recompute()

        fea = FemToolsCcx.FemToolsCcx(analysis, solver_object, test_mode=True)
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
        ret = compare_inp_files(static_analysis_inp_file, static_analysis_dir + "/" + mesh_name + '.inp')
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
        self.assertTrue(fea.results_present, "Cannot read results from {}.frd frd file".format(fea.base_name))

        fcc_print('Reading stats from result object for static analysis...')
        ret = compare_stats(fea, static_expected_values)
        self.assertFalse(ret, "Invalid results read from .frd file")

        fcc_print('Save FreeCAD file for static analysis to {}...'.format(static_save_fc_file))
        self.active_doc.saveAs(static_save_fc_file)

        # frequency
        fcc_print('Setting analysis type to \'frequency\"')
        fea.set_analysis_type("frequency")
        self.assertTrue(True if fea.analysis_type == 'frequency' else False, "Setting anlysis type to \'frequency\' failed")

        fcc_print('Setting up working directory to {} in order to write frequency calculations'.format(frequency_analysis_dir))
        fea.setup_working_dir(frequency_analysis_dir)
        self.assertTrue(True if fea.working_dir == frequency_analysis_dir else False,
                        "Setting working directory {} failed".format(frequency_analysis_dir))

        fcc_print('Checking FEM inp file prerequisites for frequency analysis...')
        error = fea.check_prerequisites()
        self.assertFalse(error, "FemToolsCcx check_prerequisites returned error message: {}".format(error))

        fcc_print('Writing {}/{}.inp for frequency analysis'.format(frequency_analysis_dir, mesh_name))
        error = fea.write_inp_file()
        self.assertFalse(error, "Writing failed")

        fcc_print('Comparing {} to {}/{}.inp'.format(frequency_analysis_inp_file, frequency_analysis_dir, mesh_name))
        ret = compare_inp_files(frequency_analysis_inp_file, frequency_analysis_dir + "/" + mesh_name + '.inp')
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
        self.assertTrue(fea.results_present, "Cannot read results from {}.frd frd file".format(fea.base_name))

        fcc_print('Reading stats from result object for frequency analysis...')
        ret = compare_stats(fea, frequency_expected_values)
        self.assertFalse(ret, "Invalid results read from .frd file")

        fcc_print('Save FreeCAD file for frequency analysis to {}...'.format(frequency_save_fc_file))
        self.active_doc.saveAs(frequency_save_fc_file)

        fcc_print('--------------- End of FEM tests static and frequency analysis ---------------')

    def test_thermomech_analysis(self):
        fcc_print('--------------- Start of FEM tests ---------------')
        box = self.active_doc.addObject("Part::Box", "Box")
        box.Height = 25.4
        box.Width = 25.4
        box.Length = 203.2
        fcc_print('Checking FEM new analysis...')
        analysis = ObjectsFem.makeAnalysis('Analysis')
        self.assertTrue(analysis, "FemTest of new analysis failed")

        fcc_print('Checking FEM new solver...')
        solver_object = ObjectsFem.makeSolverCalculix('CalculiX')
        solver_object.AnalysisType = 'thermomech'
        solver_object.GeometricalNonlinearity = 'linear'
        solver_object.ThermoMechSteadyState = True
        solver_object.MatrixSolverType = 'default'
        solver_object.IterationsThermoMechMaximum = 2000
        solver_object.IterationsControlParameterTimeUse = True
        self.assertTrue(solver_object, "FemTest of new solver failed")
        analysis.Member = analysis.Member + [solver_object]

        fcc_print('Checking FEM new material...')
        new_material_object = ObjectsFem.makeMaterialSolid('MechanicalMaterial')
        mat = new_material_object.Material
        mat['Name'] = "Steel-Generic"
        mat['YoungsModulus'] = "200000 MPa"
        mat['PoissonRatio'] = "0.30"
        mat['Density'] = "7900 kg/m^3"
        mat['ThermalConductivity'] = "43.27 W/m/K"  # SvdW: Change to Ansys model values
        mat['ThermalExpansionCoefficient'] = "12 um/m/K"
        mat['SpecificHeat'] = "500 J/kg/K"  # SvdW: Change to Ansys model values
        new_material_object.Material = mat
        self.assertTrue(new_material_object, "FemTest of new material failed")
        analysis.Member = analysis.Member + [new_material_object]

        fcc_print('Checking FEM new fixed constraint...')
        fixed_constraint = self.active_doc.addObject("Fem::ConstraintFixed", "FemConstraintFixed")
        fixed_constraint.References = [(box, "Face1")]
        self.assertTrue(fixed_constraint, "FemTest of new fixed constraint failed")
        analysis.Member = analysis.Member + [fixed_constraint]

        fcc_print('Checking FEM new initial temperature constraint...')
        initialtemperature_constraint = self.active_doc.addObject("Fem::ConstraintInitialTemperature", "FemConstraintInitialTemperature")
        initialtemperature_constraint.initialTemperature = 300.0
        self.assertTrue(initialtemperature_constraint, "FemTest of new initial temperature constraint failed")
        analysis.Member = analysis.Member + [initialtemperature_constraint]

        fcc_print('Checking FEM new temperature constraint...')
        temperature_constraint = self.active_doc.addObject("Fem::ConstraintTemperature", "FemConstraintTemperature")
        temperature_constraint.References = [(box, "Face1")]
        temperature_constraint.Temperature = 310.93
        self.assertTrue(temperature_constraint, "FemTest of new temperature constraint failed")
        analysis.Member = analysis.Member + [temperature_constraint]

        fcc_print('Checking FEM new heatflux constraint...')
        heatflux_constraint = self.active_doc.addObject("Fem::ConstraintHeatflux", "FemConstraintHeatflux")
        heatflux_constraint.References = [(box, "Face3"), (box, "Face4"), (box, "Face5"), (box, "Face6")]
        heatflux_constraint.AmbientTemp = 255.3722
        heatflux_constraint.FilmCoef = 5.678
        self.assertTrue(heatflux_constraint, "FemTest of new heatflux constraint failed")
        analysis.Member = analysis.Member + [heatflux_constraint]

        fcc_print('Checking FEM new mesh...')
        from test_files.ccx.spine_mesh import create_nodes_spine, create_elements_spine
        mesh = Fem.FemMesh()
        ret = create_nodes_spine(mesh)
        self.assertTrue(ret, "Import of mesh nodes failed")
        ret = create_elements_spine(mesh)
        self.assertTrue(ret, "Import of mesh volumes failed")
        mesh_object = self.active_doc.addObject('Fem::FemMeshObject', mesh_name)
        mesh_object.FemMesh = mesh
        self.assertTrue(mesh, "FemTest of new mesh failed")
        analysis.Member = analysis.Member + [mesh_object]

        self.active_doc.recompute()

        fea = FemToolsCcx.FemToolsCcx(analysis, test_mode=True)
        fcc_print('Setting up working directory {}'.format(thermomech_analysis_dir))
        fea.setup_working_dir(thermomech_analysis_dir)
        self.assertTrue(True if fea.working_dir == thermomech_analysis_dir else False,
                        "Setting working directory {} failed".format(thermomech_analysis_dir))

        fcc_print('Setting analysis type to \'thermomech\"')
        fea.set_analysis_type("thermomech")
        self.assertTrue(True if fea.analysis_type == 'thermomech' else False, "Setting anlysis type to \'thermomech\' failed")

        fcc_print('Checking FEM inp file prerequisites for thermo-mechanical analysis...')
        error = fea.check_prerequisites()
        self.assertFalse(error, "FemToolsCcx check_prerequisites returned error message: {}".format(error))

        fcc_print('Checking FEM inp file write...')

        fcc_print('Writing {}/{}.inp for thermomech analysis'.format(thermomech_analysis_dir, mesh_name))
        error = fea.write_inp_file()
        self.assertFalse(error, "Writing failed")

        fcc_print('Comparing {} to {}/{}.inp'.format(thermomech_analysis_inp_file, thermomech_analysis_dir, mesh_name))
        ret = compare_inp_files(thermomech_analysis_inp_file, thermomech_analysis_dir + "/" + mesh_name + '.inp')
        self.assertFalse(ret, "FemToolsCcx write_inp_file test failed.\n{}".format(ret))

        fcc_print('Setting up working directory to {} in order to read simulated calculations'.format(test_file_dir))
        fea.setup_working_dir(test_file_dir)
        self.assertTrue(True if fea.working_dir == test_file_dir else False,
                        "Setting working directory {} failed".format(test_file_dir))

        fcc_print('Setting base name to read test {}.frd file...'.format('spine_thermomech'))
        fea.set_base_name(thermomech_base_name)
        self.assertTrue(True if fea.base_name == thermomech_base_name else False,
                        "Setting base name to {} failed".format(thermomech_base_name))

        fcc_print('Setting inp file name to read test {}.frd file...'.format('spine_thermomech'))
        fea.set_inp_file_name()
        self.assertTrue(True if fea.inp_file_name == thermomech_analysis_inp_file else False,
                        "Setting inp file name to {} failed".format(thermomech_analysis_inp_file))

        fcc_print('Checking FEM frd file read from thermomech analysis...')
        fea.load_results()
        self.assertTrue(fea.results_present, "Cannot read results from {}.frd frd file".format(fea.base_name))

        fcc_print('Reading stats from result object for thermomech analysis...')
        ret = compare_stats(fea, thermomech_expected_values)
        self.assertFalse(ret, "Invalid results read from .frd file")

        fcc_print('Save FreeCAD file for thermomech analysis to {}...'.format(thermomech_save_fc_file))
        self.active_doc.saveAs(thermomech_save_fc_file)

        fcc_print('--------------- End of FEM tests thermomech analysis ---------------')

    def tearDown(self):
        FreeCAD.closeDocument("FemTest")
        pass


# helpers
def fcc_print(message):
    FreeCAD.Console.PrintMessage('{} \n'.format(message))


def compare_inp_files(file_name1, file_name2):
    file1 = open(file_name1, 'r')
    f1 = file1.readlines()
    file1.close()
    lf1 = [l for l in f1 if not (l.startswith('**   written ') or l.startswith('**   file '))]
    lf1 = force_unix_line_ends(lf1)
    file2 = open(file_name2, 'r')
    f2 = file2.readlines()
    file2.close()
    lf2 = [l for l in f2 if not (l.startswith('**   written ') or l.startswith('**   file '))]
    lf2 = force_unix_line_ends(lf2)
    import difflib
    diff = difflib.unified_diff(lf1, lf2, n=0)
    result = ''
    for l in diff:
        result += l
    if result:
        result = "Comparing {} to {} failed!\n".format(file_name1, file_name2) + result
    return result


def compare_stats(fea, stat_file=None):
    if stat_file:
        sf = open(stat_file, 'r')
        sf_content = sf.readlines()
        sf.close()
        sf_content = force_unix_line_ends(sf_content)
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


def force_unix_line_ends(line_list):
    new_line_list = []
    for l in line_list:
        if l.endswith("\r\n"):
            l = l[:-2] + '\n'
        new_line_list.append(l)
    return new_line_list


def runTestFem():
    '''run FEM unit test
    for more information on how to run a specific test class or a test def see
    file src/Mod/Test/__init__
    https://forum.freecadweb.org/viewtopic.php?f=10&t=22190#p175546
    '''
    import Test
    import sys
    current_module = sys.modules[__name__]
    Test.runTestsFromModule(current_module)


def create_test_results():
    # run FEM unit tests
    runTestFem()

    import os
    import shutil
    import FemGui
    import FemToolsCcx

    # static and frequency cube
    FreeCAD.open(static_save_fc_file)
    FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.Analysis)
    fea = FemToolsCcx.FemToolsCcx()

    # static
    fea.reset_all()
    fea.run()

    fea.load_results()
    stat_types = ["U1", "U2", "U3", "Uabs", "Sabs"]
    stats_static = []  # we only have one result object so we are fine
    for s in stat_types:
        stats_static.append("{}: {}\n".format(s, fea.get_stats(s)))
    static_expected_values_file = static_analysis_dir + '/cube_static_expected_values'
    f = open(static_expected_values_file, 'w')
    for s in stats_static:
        f.write(s)
    f.close()

    # could be added in FemToolsCcx to the self object as an Attribut
    frd_result_file = os.path.splitext(fea.inp_file_name)[0] + '.frd'
    dat_result_file = os.path.splitext(fea.inp_file_name)[0] + '.dat'

    frd_static_test_result_file = static_analysis_dir + '/cube_static.frd'
    dat_static_test_result_file = static_analysis_dir + '/cube_static.dat'
    shutil.copyfile(frd_result_file, frd_static_test_result_file)
    shutil.copyfile(dat_result_file, dat_static_test_result_file)

    # frequency
    fea.reset_all()
    fea.set_analysis_type('frequency')
    fea.solver.EigenmodesCount = 1  # we should only have one result object
    fea.run()

    fea.load_results()
    stats_frequency = []  # since we set eigenmodeno. we only have one result object so we are fine
    for s in stat_types:
        stats_frequency.append("{}: {}\n".format(s, fea.get_stats(s)))
    frequency_expected_values_file = frequency_analysis_dir + '/cube_frequency_expected_values'
    f = open(frequency_expected_values_file, 'w')
    for s in stats_frequency:
        f.write(s)
    f.close()

    frd_frequency_test_result_file = frequency_analysis_dir + '/cube_frequency.frd'
    dat_frequency_test_result_file = frequency_analysis_dir + '/cube_frequency.dat'
    shutil.copyfile(frd_result_file, frd_frequency_test_result_file)
    shutil.copyfile(dat_result_file, dat_frequency_test_result_file)

    # thermomech
    FreeCAD.open(thermomech_save_fc_file)
    FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.Analysis)
    fea = FemToolsCcx.FemToolsCcx()
    fea.reset_all()
    fea.run()

    fea.load_results()
    stat_types = ["U1", "U2", "U3", "Uabs", "Sabs"]
    stats_thermomech = []  # we only have one result object so we are fine
    for s in stat_types:
        stats_thermomech.append("{}: {}\n".format(s, fea.get_stats(s)))
    thermomech_expected_values_file = thermomech_analysis_dir + '/spine_thermomech_expected_values'
    f = open(thermomech_expected_values_file, 'w')
    for s in stats_thermomech:
        f.write(s)
    f.close()

    # could be added in FemToolsCcx to the self object as an Attribut
    frd_result_file = os.path.splitext(fea.inp_file_name)[0] + '.frd'
    dat_result_file = os.path.splitext(fea.inp_file_name)[0] + '.dat'

    frd_thermomech_test_result_file = thermomech_analysis_dir + '/spine_thermomech.frd'
    dat_thermomech_test_result_file = thermomech_analysis_dir + '/spine_thermomech.dat'
    shutil.copyfile(frd_result_file, frd_thermomech_test_result_file)
    shutil.copyfile(dat_result_file, dat_thermomech_test_result_file)

    print('Results copied to the appropriate FEM test dirs in: ' + temp_dir)


'''
update the results of FEM unit tests:

import TestFem
TestFem.create_test_results()

copy result files from your_temp_directory/FEM_unittests/   test directories into the src dirctory
compare the results with git difftool
run make
start FreeCAD and run FEM unit test
if FEM unit test is fine --> commit new FEM unit test results

TODO compare the inp file of the helper with the inp file of FEM unit tests
TODO the better way: move the result creation inside the TestFem and add some preference to deactivate this because it needs ccx
'''
