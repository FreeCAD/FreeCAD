# ***************************************************************************
# *   Copyright (c) 2015 - FreeCAD Developers                               *
# *   Author: Przemo Firszt <przemo@firszt.eu>                              *
# *   Author: Bernd Hahnebach <bernd@bimstatik.org>                         *
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
import FreeCAD
import ObjectsFem
import unittest
from . import utilstest as testtools
from .utilstest import fcc_print


class TestCcxTools(unittest.TestCase):
    fcc_print('import TestCcxTools')

    def setUp(self):
        self.doc_name = "TestCcxTools"
        try:
            FreeCAD.setActiveDocument(self.doc_name)
        except:
            FreeCAD.newDocument(self.doc_name)
        finally:
            FreeCAD.setActiveDocument(self.doc_name)
        self.active_doc = FreeCAD.ActiveDocument
        self.mesh_name = 'Mesh'
        self.temp_dir = testtools.get_fem_test_tmp_dir()
        self.test_file_dir = testtools.get_fem_test_home_dir() + 'ccx/'

    def test_1_static_analysis(self):
        fcc_print('--------------- Start of FEM tests ---------------')
        box = self.active_doc.addObject("Part::Box", "Box")
        fcc_print('Checking FEM new analysis...')
        analysis = ObjectsFem.makeAnalysis(self.active_doc, 'Analysis')
        self.assertTrue(analysis, "FemTest of new analysis failed")

        fcc_print('Checking FEM new solver...')
        solver_object = ObjectsFem.makeSolverCalculixCcxTools(self.active_doc, 'CalculiX')
        solver_object.AnalysisType = 'static'
        solver_object.GeometricalNonlinearity = 'linear'
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = 'default'
        solver_object.IterationsControlParameterTimeUse = False
        solver_object.EigenmodesCount = 10
        solver_object.EigenmodeHighLimit = 1000000.0
        solver_object.EigenmodeLowLimit = 0.0
        self.assertTrue(solver_object, "FemTest of new solver failed")
        analysis.addObject(solver_object)

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
        mesh_object = self.active_doc.addObject('Fem::FemMeshObject', self.mesh_name)
        mesh_object.FemMesh = mesh
        self.assertTrue(mesh, "FemTest of new mesh failed")
        analysis.addObject(mesh_object)

        self.active_doc.recompute()

        static_analysis_dir = testtools.get_unit_test_tmp_dir(self.temp_dir, 'FEM_ccx_static/')
        fea = ccxtools.FemToolsCcx(analysis, solver_object, test_mode=True)

        fcc_print('Setting up working directory {}'.format(static_analysis_dir))
        fea.setup_working_dir(static_analysis_dir)
        self.assertTrue(True if fea.working_dir == static_analysis_dir else False,
                        "Setting working directory {} failed".format(static_analysis_dir))

        fcc_print('Checking FEM inp file prerequisites for static analysis...')
        error = fea.check_prerequisites()
        self.assertFalse(error, "ccxtools check_prerequisites returned error message: {}".format(error))

        fcc_print('Checking FEM inp file write...')
        fcc_print('Writing {}/{}.inp for static analysis'.format(static_analysis_dir, self.mesh_name))
        error = fea.write_inp_file()
        self.assertFalse(error, "Writing failed")

        static_base_name = 'cube_static'
        static_analysis_inp_file = self.test_file_dir + static_base_name + '.inp'
        fcc_print('Comparing {} to {}/{}.inp'.format(static_analysis_inp_file, static_analysis_dir, self.mesh_name))
        ret = testtools.compare_inp_files(static_analysis_inp_file, static_analysis_dir + self.mesh_name + '.inp')
        self.assertFalse(ret, "ccxtools write_inp_file test failed.\n{}".format(ret))

        fcc_print('Setting up working directory to {} in order to read simulated calculations'.format(self.test_file_dir))
        fea.setup_working_dir(self.test_file_dir)
        self.assertTrue(True if fea.working_dir == self.test_file_dir else False,
                        "Setting working directory {} failed".format(self.test_file_dir))

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
        static_expected_values = self.test_file_dir + "cube_static_expected_values"
        ret = testtools.compare_stats(fea, static_expected_values, 'CalculiX_static_results')
        self.assertFalse(ret, "Invalid results read from .frd file")

        static_save_fc_file = static_analysis_dir + static_base_name + '.FCStd'
        fcc_print('Save FreeCAD file for static analysis to {}...'.format(static_save_fc_file))
        self.active_doc.saveAs(static_save_fc_file)
        fcc_print('--------------- End of FEM tests static and analysis ---------------')

    def test_2_static_multiple_material(self):
        fcc_print('--------------- Start of FEM ccxtools multiple material test ---------------')

        # create a CompSolid of two Boxes extract the CompSolid (we are able to remesh if needed)
        boxlow = self.active_doc.addObject("Part::Box", "BoxLower")
        boxupp = self.active_doc.addObject("Part::Box", "BoxUpper")
        boxupp.Placement.Base = (0, 0, 10)

        # for BooleanFragments Occt >=6.9 is needed
        '''
        import BOPTools.SplitFeatures
        bf = BOPTools.SplitFeatures.makeBooleanFragments(name='BooleanFragments')
        bf.Objects = [boxlow, boxupp]
        bf.Mode = "CompSolid"
        self.active_doc.recompute()
        bf.Proxy.execute(bf)
        bf.purgeTouched()
        for obj in bf.ViewObject.Proxy.claimChildren():
            obj.ViewObject.hide()
        self.active_doc.recompute()
        import CompoundTools.CompoundFilter
        cf = CompoundTools.CompoundFilter.makeCompoundFilter(name='MultiMatCompSolid')
        cf.Base = bf
        cf.FilterType = 'window-volume'
        cf.Proxy.execute(cf)
        cf.purgeTouched()
        cf.Base.ViewObject.hide()
        '''
        self.active_doc.recompute()
        if FreeCAD.GuiUp:
            import FreeCADGui
            FreeCADGui.ActiveDocument.activeView().viewAxonometric()
            FreeCADGui.SendMsgToActiveView("ViewFit")

        analysis = ObjectsFem.makeAnalysis(self.active_doc, 'Analysis')
        solver_object = ObjectsFem.makeSolverCalculixCcxTools(self.active_doc, 'CalculiXccxTools')
        solver_object.AnalysisType = 'static'
        solver_object.GeometricalNonlinearity = 'linear'
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = 'default'
        solver_object.IterationsControlParameterTimeUse = False
        analysis.addObject(solver_object)

        material_object_low = ObjectsFem.makeMaterialSolid(self.active_doc, 'MechanicalMaterialLow')
        mat = material_object_low.Material
        mat['Name'] = "Aluminium-Generic"
        mat['YoungsModulus'] = "70000 MPa"
        mat['PoissonRatio'] = "0.35"
        mat['Density'] = "2700  kg/m^3"
        material_object_low.Material = mat
        material_object_low.References = [(boxlow, 'Solid1')]
        analysis.addObject(material_object_low)

        material_object_upp = ObjectsFem.makeMaterialSolid(self.active_doc, 'MechanicalMaterialUpp')
        mat = material_object_upp.Material
        mat['Name'] = "Steel-Generic"
        mat['YoungsModulus'] = "200000 MPa"
        mat['PoissonRatio'] = "0.30"
        mat['Density'] = "7980 kg/m^3"
        material_object_upp.Material = mat
        material_object_upp.References = [(boxupp, 'Solid1')]
        analysis.addObject(material_object_upp)

        fixed_constraint = self.active_doc.addObject("Fem::ConstraintFixed", "ConstraintFixed")
        # fixed_constraint.References = [(cf, "Face3")]
        fixed_constraint.References = [(boxlow, "Face5")]
        analysis.addObject(fixed_constraint)

        pressure_constraint = self.active_doc.addObject("Fem::ConstraintPressure", "ConstraintPressure")
        # pressure_constraint.References = [(cf, "Face9")]
        pressure_constraint.References = [(boxupp, "Face6")]
        pressure_constraint.Pressure = 1000.0
        pressure_constraint.Reversed = False
        analysis.addObject(pressure_constraint)

        mesh = Fem.FemMesh()
        import femtest.testfiles.ccx.multimat_mesh as multimatmesh
        multimatmesh.create_nodes(mesh)
        multimatmesh.create_elements(mesh)
        mesh_object = self.active_doc.addObject('Fem::FemMeshObject', self.mesh_name)
        mesh_object.FemMesh = mesh
        analysis.addObject(mesh_object)

        self.active_doc.recompute()
        static_multiplemat_dir = testtools.get_unit_test_tmp_dir(self.temp_dir, 'FEM_ccx_multimat/')
        fea = ccxtools.FemToolsCcx(analysis, solver_object, test_mode=True)
        fea.setup_working_dir(static_multiplemat_dir)

        fcc_print('Checking FEM inp file prerequisites for ccxtools multimat analysis...')
        error = fea.check_prerequisites()
        self.assertFalse(error, "ccxtools check_prerequisites returned error message: {}".format(error))

        fcc_print('Checking FEM inp file write...')
        fcc_print('Writing {}/{}.inp for static multiple material'.format(static_multiplemat_dir, self.mesh_name))
        error = fea.write_inp_file()
        self.assertFalse(error, "Writing failed")
        static_base_name = 'multimat'
        static_analysis_inp_file = self.test_file_dir + static_base_name + '.inp'
        fcc_print('Comparing {} to {}/{}.inp'.format(static_analysis_inp_file, static_multiplemat_dir, self.mesh_name))
        ret = testtools.compare_inp_files(static_analysis_inp_file, static_multiplemat_dir + self.mesh_name + '.inp')
        self.assertFalse(ret, "ccxtools write_inp_file test failed.\n{}".format(ret))

        static_save_fc_file = static_multiplemat_dir + static_base_name + '.FCStd'
        fcc_print('Save FreeCAD file for static analysis to {}...'.format(static_save_fc_file))
        self.active_doc.saveAs(static_save_fc_file)
        fcc_print('--------------- End of FEM ccxtools multiple material test ---------------')

    def test_3_freq_analysis(self):
        fcc_print('--------------- Start of FEM tests ---------------')
        self.active_doc.addObject("Part::Box", "Box")
        fcc_print('Checking FEM new analysis...')
        analysis = ObjectsFem.makeAnalysis(self.active_doc, 'Analysis')
        self.assertTrue(analysis, "FemTest of new analysis failed")

        fcc_print('Checking FEM new solver...')
        solver_object = ObjectsFem.makeSolverCalculixCcxTools(self.active_doc, 'CalculiX')
        solver_object.AnalysisType = 'frequency'
        solver_object.GeometricalNonlinearity = 'linear'
        solver_object.ThermoMechSteadyState = False
        solver_object.MatrixSolverType = 'default'
        solver_object.IterationsControlParameterTimeUse = False
        solver_object.EigenmodesCount = 10
        solver_object.EigenmodeHighLimit = 1000000.0
        solver_object.EigenmodeLowLimit = 0.01
        self.assertTrue(solver_object, "FemTest of new solver failed")
        analysis.addObject(solver_object)

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

        fcc_print('Checking FEM new mesh...')
        from .testfiles.ccx.cube_mesh import create_nodes_cube
        from .testfiles.ccx.cube_mesh import create_elements_cube
        mesh = Fem.FemMesh()
        ret = create_nodes_cube(mesh)
        self.assertTrue(ret, "Import of mesh nodes failed")
        ret = create_elements_cube(mesh)
        self.assertTrue(ret, "Import of mesh volumes failed")
        mesh_object = self.active_doc.addObject('Fem::FemMeshObject', self.mesh_name)
        mesh_object.FemMesh = mesh
        self.assertTrue(mesh, "FemTest of new mesh failed")
        analysis.addObject(mesh_object)

        self.active_doc.recompute()

        frequency_analysis_dir = self.temp_dir + 'FEM_ccx_frequency/'
        fea = ccxtools.FemToolsCcx(analysis, solver_object, test_mode=True)

        fcc_print('Setting up working directory {}'.format(frequency_analysis_dir))
        fea.setup_working_dir(frequency_analysis_dir)
        self.assertTrue(True if fea.working_dir == frequency_analysis_dir else False,
                        "Setting working directory {} failed".format(frequency_analysis_dir))

        fcc_print('Checking FEM inp file prerequisites for frequency analysis...')
        error = fea.check_prerequisites()
        self.assertFalse(error, "ccxtools check_prerequisites returned error message: {}".format(error))

        fcc_print('Checking FEM inp file write...')
        fcc_print('Writing {}/{}.inp for frequency analysis'.format(frequency_analysis_dir, self.mesh_name))
        error = fea.write_inp_file()
        self.assertFalse(error, "Writing failed")

        frequency_base_name = 'cube_frequency'
        frequency_analysis_inp_file = self.test_file_dir + frequency_base_name + '.inp'
        fcc_print('Comparing {} to {}/{}.inp'.format(frequency_analysis_inp_file, frequency_analysis_dir, self.mesh_name))
        ret = testtools.compare_inp_files(frequency_analysis_inp_file, frequency_analysis_dir + self.mesh_name + '.inp')
        self.assertFalse(ret, "ccxtools write_inp_file test failed.\n{}".format(ret))

        fcc_print('Setting up working directory to {} in order to read simulated calculations'.format(self.test_file_dir))
        fea.setup_working_dir(self.test_file_dir)
        self.assertTrue(True if fea.working_dir == self.test_file_dir else False,
                        "Setting working directory {} failed".format(self.test_file_dir))

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
        frequency_expected_values = self.test_file_dir + "cube_frequency_expected_values"
        ret = testtools.compare_stats(fea, frequency_expected_values, 'CalculiX_frequency_mode_1_results')
        self.assertFalse(ret, "Invalid results read from .frd file")

        frequency_save_fc_file = frequency_analysis_dir + frequency_base_name + '.FCStd'
        fcc_print('Save FreeCAD file for frequency analysis to {}...'.format(frequency_save_fc_file))
        self.active_doc.saveAs(frequency_save_fc_file)
        fcc_print('--------------- End of FEM tests frequency analysis ---------------')

    def test_4_thermomech_analysis(self):
        fcc_print('--------------- Start of FEM tests ---------------')
        box = self.active_doc.addObject("Part::Box", "Box")
        box.Height = 25.4
        box.Width = 25.4
        box.Length = 203.2
        fcc_print('Checking FEM new analysis...')
        analysis = ObjectsFem.makeAnalysis(self.active_doc, 'Analysis')
        self.assertTrue(analysis, "FemTest of new analysis failed")

        fcc_print('Checking FEM new solver...')
        solver_object = ObjectsFem.makeSolverCalculixCcxTools(self.active_doc, 'CalculiX')
        solver_object.AnalysisType = 'thermomech'
        solver_object.GeometricalNonlinearity = 'linear'
        solver_object.ThermoMechSteadyState = True
        solver_object.MatrixSolverType = 'default'
        solver_object.IterationsThermoMechMaximum = 2000
        solver_object.IterationsControlParameterTimeUse = True
        self.assertTrue(solver_object, "FemTest of new solver failed")
        analysis.addObject(solver_object)

        fcc_print('Checking FEM new material...')
        material_object = ObjectsFem.makeMaterialSolid(self.active_doc, 'MechanicalMaterial')
        mat = material_object.Material
        mat['Name'] = "Steel-Generic"
        mat['YoungsModulus'] = "200000 MPa"
        mat['PoissonRatio'] = "0.30"
        mat['Density'] = "7900 kg/m^3"
        mat['ThermalConductivity'] = "43.27 W/m/K"  # SvdW: Change to Ansys model values
        mat['ThermalExpansionCoefficient'] = "12 um/m/K"
        mat['SpecificHeat'] = "500 J/kg/K"  # SvdW: Change to Ansys model values
        material_object.Material = mat
        self.assertTrue(material_object, "FemTest of new material failed")
        analysis.addObject(material_object)

        fcc_print('Checking FEM new fixed constraint...')
        fixed_constraint = self.active_doc.addObject("Fem::ConstraintFixed", "FemConstraintFixed")
        fixed_constraint.References = [(box, "Face1")]
        self.assertTrue(fixed_constraint, "FemTest of new fixed constraint failed")
        analysis.addObject(fixed_constraint)

        fcc_print('Checking FEM new initial temperature constraint...')
        initialtemperature_constraint = self.active_doc.addObject("Fem::ConstraintInitialTemperature", "FemConstraintInitialTemperature")
        initialtemperature_constraint.initialTemperature = 300.0
        self.assertTrue(initialtemperature_constraint, "FemTest of new initial temperature constraint failed")
        analysis.addObject(initialtemperature_constraint)

        fcc_print('Checking FEM new temperature constraint...')
        temperature_constraint = self.active_doc.addObject("Fem::ConstraintTemperature", "FemConstraintTemperature")
        temperature_constraint.References = [(box, "Face1")]
        temperature_constraint.Temperature = 310.93
        self.assertTrue(temperature_constraint, "FemTest of new temperature constraint failed")
        analysis.addObject(temperature_constraint)

        fcc_print('Checking FEM new heatflux constraint...')
        heatflux_constraint = self.active_doc.addObject("Fem::ConstraintHeatflux", "FemConstraintHeatflux")
        heatflux_constraint.References = [(box, "Face3"), (box, "Face4"), (box, "Face5"), (box, "Face6")]
        heatflux_constraint.AmbientTemp = 255.3722
        heatflux_constraint.FilmCoef = 5.678
        self.assertTrue(heatflux_constraint, "FemTest of new heatflux constraint failed")
        analysis.addObject(heatflux_constraint)

        fcc_print('Checking FEM new mesh...')
        from .testfiles.ccx.spine_mesh import create_nodes_spine
        from .testfiles.ccx.spine_mesh import create_elements_spine
        mesh = Fem.FemMesh()
        ret = create_nodes_spine(mesh)
        self.assertTrue(ret, "Import of mesh nodes failed")
        ret = create_elements_spine(mesh)
        self.assertTrue(ret, "Import of mesh volumes failed")
        mesh_object = self.active_doc.addObject('Fem::FemMeshObject', self.mesh_name)
        mesh_object.FemMesh = mesh
        self.assertTrue(mesh, "FemTest of new mesh failed")
        analysis.addObject(mesh_object)

        self.active_doc.recompute()

        thermomech_analysis_dir = self.temp_dir + 'FEM_ccx_thermomech/'
        fea = ccxtools.FemToolsCcx(analysis, test_mode=True)

        fcc_print('Setting up working directory {}'.format(thermomech_analysis_dir))
        fea.setup_working_dir(thermomech_analysis_dir)
        self.assertTrue(True if fea.working_dir == thermomech_analysis_dir else False,
                        "Setting working directory {} failed".format(thermomech_analysis_dir))

        fcc_print('Checking FEM inp file prerequisites for thermo-mechanical analysis...')
        error = fea.check_prerequisites()
        self.assertFalse(error, "ccxtools check_prerequisites returned error message: {}".format(error))

        fcc_print('Checking FEM inp file write...')
        fcc_print('Writing {}/{}.inp for thermomech analysis'.format(thermomech_analysis_dir, self.mesh_name))
        error = fea.write_inp_file()
        self.assertFalse(error, "Writing failed")

        thermomech_base_name = 'spine_thermomech'
        thermomech_analysis_inp_file = self.test_file_dir + thermomech_base_name + '.inp'
        fcc_print('Comparing {} to {}/{}.inp'.format(thermomech_analysis_inp_file, thermomech_analysis_dir, self.mesh_name))
        ret = testtools.compare_inp_files(thermomech_analysis_inp_file, thermomech_analysis_dir + self.mesh_name + '.inp')
        self.assertFalse(ret, "ccxtools write_inp_file test failed.\n{}".format(ret))

        fcc_print('Setting up working directory to {} in order to read simulated calculations'.format(self.test_file_dir))
        fea.setup_working_dir(self.test_file_dir)
        self.assertTrue(True if fea.working_dir == self.test_file_dir else False,
                        "Setting working directory {} failed".format(self.test_file_dir))

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
        thermomech_expected_values = self.test_file_dir + "spine_thermomech_expected_values"
        ret = testtools.compare_stats(fea, thermomech_expected_values, 'CalculiX_thermomech_results')
        self.assertFalse(ret, "Invalid results read from .frd file")

        thermomech_save_fc_file = thermomech_analysis_dir + thermomech_base_name + '.FCStd'
        fcc_print('Save FreeCAD file for thermomech analysis to {}...'.format(thermomech_save_fc_file))
        self.active_doc.saveAs(thermomech_save_fc_file)

        fcc_print('--------------- End of FEM tests thermomech analysis ---------------')

    def test_5_Flow1D_thermomech_analysis(self):
        fcc_print('--------------- Start of 1D Flow FEM tests ---------------')
        import Draft
        p1 = FreeCAD.Vector(0, 0, 50)
        p2 = FreeCAD.Vector(0, 0, -50)
        p3 = FreeCAD.Vector(0, 0, -4300)
        p4 = FreeCAD.Vector(4950, 0, -4300)
        p5 = FreeCAD.Vector(5000, 0, -4300)
        p6 = FreeCAD.Vector(8535.53, 0, -7835.53)
        p7 = FreeCAD.Vector(8569.88, 0, -7870.88)
        p8 = FreeCAD.Vector(12105.41, 0, -11406.41)
        p9 = FreeCAD.Vector(12140.76, 0, -11441.76)
        p10 = FreeCAD.Vector(13908.53, 0, -13209.53)
        p11 = FreeCAD.Vector(13943.88, 0, -13244.88)
        p12 = FreeCAD.Vector(15046.97, 0, -14347.97)
        p13 = FreeCAD.Vector(15046.97, 0, -7947.97)
        p14 = FreeCAD.Vector(15046.97, 0, -7847.97)
        p15 = FreeCAD.Vector(0, 0, 0)
        p16 = FreeCAD.Vector(0, 0, -2175)
        p17 = FreeCAD.Vector(2475, 0, -4300)
        p18 = FreeCAD.Vector(4975, 0, -4300)
        p19 = FreeCAD.Vector(6767.765, 0, -6067.765)
        p20 = FreeCAD.Vector(8552.705, 0, -7853.205)
        p21 = FreeCAD.Vector(10337.645, 0, -9638.645)
        p22 = FreeCAD.Vector(12123.085, 0, -11424.085)
        p23 = FreeCAD.Vector(13024.645, 0, -12325.645)
        p24 = FreeCAD.Vector(13926.205, 0, -13227.205)
        p25 = FreeCAD.Vector(14495.425, 0, -13796.425)
        p26 = FreeCAD.Vector(15046.97, 0, -11147.97)
        p27 = FreeCAD.Vector(15046.97, 0, -7897.97)
        points = [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27]
        line = Draft.makeWire(points, closed=False, face=False, support=None)
        fcc_print('Checking FEM new analysis...')
        analysis = ObjectsFem.makeAnalysis(self.active_doc, 'Analysis')
        self.assertTrue(analysis, "FemTest of new analysis failed")

        fcc_print('Checking FEM new solver...')
        solver_object = ObjectsFem.makeSolverCalculixCcxTools(self.active_doc, 'CalculiX')
        solver_object.AnalysisType = 'thermomech'
        solver_object.GeometricalNonlinearity = 'linear'
        solver_object.ThermoMechSteadyState = True
        solver_object.MatrixSolverType = 'default'
        solver_object.IterationsThermoMechMaximum = 2000
        solver_object.IterationsControlParameterTimeUse = False
        self.assertTrue(solver_object, "FemTest of new solver failed")
        analysis.addObject(solver_object)

        fcc_print('Checking FEM new material...')
        material_object = ObjectsFem.makeMaterialFluid(self.active_doc, 'FluidMaterial')
        mat = material_object.Material
        mat['Name'] = "Water"
        mat['Density'] = "998 kg/m^3"
        mat['SpecificHeat'] = "4.182 J/kg/K"
        mat['DynamicViscosity'] = "1.003e-3 kg/m/s"
        mat['VolumetricThermalExpansionCoefficient'] = "2.07e-4 m/m/K"
        mat['ThermalConductivity'] = "0.591 W/m/K"
        material_object.Material = mat
        self.assertTrue(material_object, "FemTest of new material failed")
        analysis.addObject(material_object)

        fcc_print('Checking FEM Flow1D inlet constraint...')
        Flow1d_inlet = ObjectsFem.makeElementFluid1D(self.active_doc, "ElementFluid1D")
        Flow1d_inlet.SectionType = 'Liquid'
        Flow1d_inlet.LiquidSectionType = 'PIPE INLET'
        Flow1d_inlet.InletPressure = 0.1
        Flow1d_inlet.References = [(line, "Edge1")]
        self.assertTrue(Flow1d_inlet, "FemTest of new Flow1D inlet constraint failed")
        analysis.addObject(Flow1d_inlet)

        fcc_print('Checking FEM new Flow1D entrance constraint...')
        Flow1d_entrance = ObjectsFem.makeElementFluid1D(self.active_doc, "ElementFluid1D")
        Flow1d_entrance.SectionType = 'Liquid'
        Flow1d_entrance.LiquidSectionType = 'PIPE ENTRANCE'
        Flow1d_entrance.EntrancePipeArea = 31416.00
        Flow1d_entrance.EntranceArea = 25133.00
        Flow1d_entrance.References = [(line, "Edge2")]
        self.assertTrue(Flow1d_entrance, "FemTest of new Flow1D entrance constraint failed")
        analysis.addObject(Flow1d_entrance)

        fcc_print('Checking FEM new Flow1D manning constraint...')
        Flow1d_manning = ObjectsFem.makeElementFluid1D(self.active_doc, "ElementFluid1D")
        Flow1d_manning.SectionType = 'Liquid'
        Flow1d_manning.LiquidSectionType = 'PIPE MANNING'
        Flow1d_manning.ManningArea = 31416
        Flow1d_manning.ManningRadius = 50
        Flow1d_manning.ManningCoefficient = 0.002
        Flow1d_manning.References = [(line, "Edge3"), (line, "Edge5")]
        self.assertTrue(Flow1d_manning, "FemTest of new Flow1D manning constraint failed")
        analysis.addObject(Flow1d_manning)

        fcc_print('Checking FEM new Flow1D bend constraint...')
        Flow1d_bend = ObjectsFem.makeElementFluid1D(self.active_doc, "ElementFluid1D")
        Flow1d_bend.SectionType = 'Liquid'
        Flow1d_bend.LiquidSectionType = 'PIPE BEND'
        Flow1d_bend.BendPipeArea = 31416
        Flow1d_bend.BendRadiusDiameter = 1.5
        Flow1d_bend.BendAngle = 45
        Flow1d_bend.BendLossCoefficient = 0.4
        Flow1d_bend.References = [(line, "Edge4")]
        self.assertTrue(Flow1d_bend, "FemTest of new Flow1D bend constraint failed")
        analysis.addObject(Flow1d_bend)

        fcc_print('Checking FEM new Flow1D enlargement constraint...')
        Flow1d_enlargement = ObjectsFem.makeElementFluid1D(self.active_doc, "ElementFluid1D")
        Flow1d_enlargement.SectionType = 'Liquid'
        Flow1d_enlargement.LiquidSectionType = 'PIPE ENLARGEMENT'
        Flow1d_enlargement.EnlargeArea1 = 31416.00
        Flow1d_enlargement.EnlargeArea2 = 70686.00
        Flow1d_enlargement.References = [(line, "Edge6")]
        self.assertTrue(Flow1d_enlargement, "FemTest of new Flow1D enlargement constraint failed")
        analysis.addObject(Flow1d_enlargement)

        fcc_print('Checking FEM new Flow1D manning constraint...')
        Flow1d_manning1 = ObjectsFem.makeElementFluid1D(self.active_doc, "ElementFluid1D")
        Flow1d_manning1.SectionType = 'Liquid'
        Flow1d_manning1.LiquidSectionType = 'PIPE MANNING'
        Flow1d_manning1.ManningArea = 70686.00
        Flow1d_manning1.ManningRadius = 75
        Flow1d_manning1.ManningCoefficient = 0.002
        Flow1d_manning1.References = [(line, "Edge7")]
        self.assertTrue(Flow1d_manning1, "FemTest of new Flow1D manning constraint failed")
        analysis.addObject(Flow1d_manning1)

        fcc_print('Checking FEM new Flow1D contraction constraint...')
        Flow1d_contraction = ObjectsFem.makeElementFluid1D(self.active_doc, "ElementFluid1D")
        Flow1d_contraction.SectionType = 'Liquid'
        Flow1d_contraction.LiquidSectionType = 'PIPE CONTRACTION'
        Flow1d_contraction.ContractArea1 = 70686
        Flow1d_contraction.ContractArea2 = 17671
        Flow1d_contraction.References = [(line, "Edge8")]
        self.assertTrue(Flow1d_contraction, "FemTest of new Flow1D contraction constraint failed")
        analysis.addObject(Flow1d_contraction)

        fcc_print('Checking FEM new Flow1D manning constraint...')
        Flow1d_manning2 = ObjectsFem.makeElementFluid1D(self.active_doc, "ElementFluid1D")
        Flow1d_manning2.SectionType = 'Liquid'
        Flow1d_manning2.LiquidSectionType = 'PIPE MANNING'
        Flow1d_manning2.ManningArea = 17671.00
        Flow1d_manning2.ManningRadius = 37.5
        Flow1d_manning2.ManningCoefficient = 0.002
        Flow1d_manning2.References = [(line, "Edge11"), (line, "Edge9")]
        self.assertTrue(Flow1d_manning2, "FemTest of new Flow1D manning constraint failed")
        analysis.addObject(Flow1d_manning2)

        fcc_print('Checking FEM new Flow1D gate valve constraint...')
        Flow1d_gate_valve = ObjectsFem.makeElementFluid1D(self.active_doc, "ElementFluid1D")
        Flow1d_gate_valve.SectionType = 'Liquid'
        Flow1d_gate_valve.LiquidSectionType = 'PIPE GATE VALVE'
        Flow1d_gate_valve.GateValvePipeArea = 17671
        Flow1d_gate_valve.GateValveClosingCoeff = 0.5
        Flow1d_gate_valve.References = [(line, "Edge10")]
        self.assertTrue(Flow1d_gate_valve, "FemTest of new Flow1D gate valve constraint failed")
        analysis.addObject(Flow1d_gate_valve)

        fcc_print('Checking FEM new Flow1D enlargement constraint...')
        Flow1d_enlargement1 = ObjectsFem.makeElementFluid1D(self.active_doc, "ElementFluid1D")
        Flow1d_enlargement1.SectionType = 'Liquid'
        Flow1d_enlargement1.LiquidSectionType = 'PIPE ENLARGEMENT'
        Flow1d_enlargement1.EnlargeArea1 = 17671
        Flow1d_enlargement1.EnlargeArea2 = 1e12
        Flow1d_enlargement1.References = [(line, "Edge12")]
        self.assertTrue(Flow1d_enlargement1, "FemTest of new Flow1D enlargement constraint failed")
        analysis.addObject(Flow1d_enlargement1)

        fcc_print('Checking FEM Flow1D outlet constraint...')
        Flow1d_outlet = ObjectsFem.makeElementFluid1D(self.active_doc, "ElementFluid1D")
        Flow1d_outlet.SectionType = 'Liquid'
        Flow1d_outlet.LiquidSectionType = 'PIPE OUTLET'
        Flow1d_outlet.OutletPressure = 0.1
        Flow1d_outlet.References = [(line, "Edge13")]
        self.assertTrue(Flow1d_outlet, "FemTest of new Flow1D inlet constraint failed")
        analysis.addObject(Flow1d_outlet)

        fcc_print('Checking FEM self weight constraint...')
        Flow1d_self_weight = ObjectsFem.makeConstraintSelfWeight(self.active_doc, "ConstraintSelfWeight")
        Flow1d_self_weight.Gravity_x = 0.0
        Flow1d_self_weight.Gravity_y = 0.0
        Flow1d_self_weight.Gravity_z = -1.0
        self.assertTrue(Flow1d_outlet, "FemTest of new Flow1D self weight constraint failed")
        analysis.addObject(Flow1d_self_weight)

        fcc_print('Checking FEM new mesh...')
        from .testfiles.ccx.Flow1D_mesh import create_nodes_Flow1D
        from .testfiles.ccx.Flow1D_mesh import create_elements_Flow1D
        mesh = Fem.FemMesh()
        ret = create_nodes_Flow1D(mesh)
        self.assertTrue(ret, "Import of mesh nodes failed")
        ret = create_elements_Flow1D(mesh)
        self.assertTrue(ret, "Import of mesh volumes failed")
        mesh_object = self.active_doc.addObject('Fem::FemMeshObject', self.mesh_name)
        mesh_object.FemMesh = mesh
        self.assertTrue(mesh, "FemTest of new mesh failed")
        analysis.addObject(mesh_object)

        self.active_doc.recompute()

        Flow1D_thermomech_analysis_dir = self.temp_dir + 'FEM_ccx_Flow1D_thermomech/'
        fea = ccxtools.FemToolsCcx(analysis, test_mode=True)

        fcc_print('Setting up working directory {}'.format(Flow1D_thermomech_analysis_dir))
        fea.setup_working_dir(Flow1D_thermomech_analysis_dir)
        self.assertTrue(True if fea.working_dir == Flow1D_thermomech_analysis_dir else False,
                        "Setting working directory {} failed".format(Flow1D_thermomech_analysis_dir))

        fcc_print('Checking FEM inp file prerequisites for thermo-mechanical analysis...')
        error = fea.check_prerequisites()
        self.assertFalse(error, "ccxtools check_prerequisites returned error message: {}".format(error))

        fcc_print('Checking FEM inp file write...')
        fcc_print('Writing {}/{}.inp for thermomech analysis'.format(Flow1D_thermomech_analysis_dir, self.mesh_name))
        error = fea.write_inp_file()
        self.assertFalse(error, "Writing failed")

        Flow1D_thermomech_base_name = 'Flow1D_thermomech'
        Flow1D_thermomech_analysis_inp_file = self.test_file_dir + Flow1D_thermomech_base_name + '.inp'
        fcc_print('Comparing {} to {}/{}.inp'.format(Flow1D_thermomech_analysis_inp_file, Flow1D_thermomech_analysis_dir, self.mesh_name))
        ret = testtools.compare_inp_files(Flow1D_thermomech_analysis_inp_file, Flow1D_thermomech_analysis_dir + self.mesh_name + '.inp')
        self.assertFalse(ret, "ccxtools write_inp_file test failed.\n{}".format(ret))

        fcc_print('Setting up working directory to {} in order to read simulated calculations'.format(self.test_file_dir))
        fea.setup_working_dir(self.test_file_dir)
        self.assertTrue(True if fea.working_dir == self.test_file_dir else False,
                        "Setting working directory {} failed".format(self.test_file_dir))

        fcc_print('Setting base name to read test {}.frd file...'.format('Flow1D_thermomech'))
        fea.set_base_name(Flow1D_thermomech_base_name)
        self.assertTrue(True if fea.base_name == Flow1D_thermomech_base_name else False,
                        "Setting base name to {} failed".format(Flow1D_thermomech_base_name))

        fcc_print('Setting inp file name to read test {}.frd file...'.format('Flow1D_thermomech'))
        fea.set_inp_file_name()
        self.assertTrue(True if fea.inp_file_name == Flow1D_thermomech_analysis_inp_file else False,
                        "Setting inp file name to {} failed".format(Flow1D_thermomech_analysis_inp_file))

        fcc_print('Checking FEM frd file read from Flow1D thermomech analysis...')
        fea.load_results()
        self.assertTrue(fea.results_present, "Cannot read results from {}.frd frd file".format(fea.base_name))

        fcc_print('Reading stats from result object for Flow1D thermomech analysis...')
        Flow1D_thermomech_expected_values = self.test_file_dir + "Flow1D_thermomech_expected_values"
        stat_types = ["U1", "U2", "U3", "Uabs", "Sabs", "MaxPrin", "MidPrin", "MinPrin", "MaxShear", "Peeq", "Temp", "MFlow", "NPress"]
        ret = testtools.compare_stats(fea, Flow1D_thermomech_expected_values, stat_types, 'CalculiX_thermomech_time_1_0_results')
        self.assertFalse(ret, "Invalid results read from .frd file")

        Flow1D_thermomech_save_fc_file = Flow1D_thermomech_analysis_dir + Flow1D_thermomech_base_name + '.FCStd'
        fcc_print('Save FreeCAD file for thermomech analysis to {}...'.format(Flow1D_thermomech_save_fc_file))
        self.active_doc.saveAs(Flow1D_thermomech_save_fc_file)

        fcc_print('--------------- End of FEM tests FLow 1D thermomech analysis ---------------')

    def tearDown(self):
        FreeCAD.closeDocument(self.doc_name)
        pass


def create_test_results():

    import shutil
    import os
    import FemGui
    import femresult.resulttools as resulttools
    from femtools import ccxtools

    stat_types = ["U1", "U2", "U3", "Uabs", "Sabs", "MaxPrin", "MidPrin", "MinPrin", "MaxShear", "Peeq", "Temp", "MFlow", "NPress"]
    temp_dir = testtools.get_fem_test_tmp_dir()
    static_analysis_dir = temp_dir + 'FEM_ccx_static/'
    frequency_analysis_dir = temp_dir + 'FEM_ccx_frequency/'
    thermomech_analysis_dir = temp_dir + 'FEM_ccx_thermomech/'
    Flow1D_thermomech_analysis_dir = temp_dir + 'FEM_ccx_Flow1D_thermomech/'

    # run all unit tests from this module
    import Test
    import sys
    current_module = sys.modules[__name__]
    Test.runTestsFromModule(current_module)

    # static cube
    FreeCAD.open(static_analysis_dir + 'cube_static.FCStd')
    FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.Analysis)
    fea = ccxtools.FemToolsCcx()

    print("create static result files")
    fea.reset_all()
    fea.run()
    fea.load_results()
    stats_static = []
    for s in stat_types:
        statval = resulttools.get_stats(FreeCAD.ActiveDocument.getObject('CalculiX_static_results'), s)
        stats_static.append("{0}: ({1:.14g}, {2:.14g}, {3:.14g})\n".format(s, statval[0], statval[1], statval[2]))
    static_expected_values_file = static_analysis_dir + 'cube_static_expected_values'
    f = open(static_expected_values_file, 'w')
    for s in stats_static:
        f.write(s)
    f.close()
    frd_result_file = os.path.splitext(fea.inp_file_name)[0] + '.frd'
    dat_result_file = os.path.splitext(fea.inp_file_name)[0] + '.dat'
    frd_static_test_result_file = static_analysis_dir + 'cube_static.frd'
    dat_static_test_result_file = static_analysis_dir + 'cube_static.dat'
    shutil.copyfile(frd_result_file, frd_static_test_result_file)
    shutil.copyfile(dat_result_file, dat_static_test_result_file)

    # frequency cube
    FreeCAD.open(frequency_analysis_dir + 'cube_frequency.FCStd')
    FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.Analysis)
    fea = ccxtools.FemToolsCcx()

    print("create frequency result files")
    fea.reset_all()
    fea.solver.EigenmodesCount = 1  # we should only have one result object
    fea.run()
    fea.load_results()
    stats_frequency = []
    for s in stat_types:
        statval = resulttools.get_stats(FreeCAD.ActiveDocument.getObject('CalculiX_frequency_mode_1_results'), s)
        stats_frequency.append("{0}: ({1:.14g}, {2:.14g}, {3:.14g})\n".format(s, statval[0], statval[1], statval[2]))
    frequency_expected_values_file = frequency_analysis_dir + 'cube_frequency_expected_values'
    f = open(frequency_expected_values_file, 'w')
    for s in stats_frequency:
        f.write(s)
    f.close()
    frd_frequency_test_result_file = frequency_analysis_dir + 'cube_frequency.frd'
    dat_frequency_test_result_file = frequency_analysis_dir + 'cube_frequency.dat'
    shutil.copyfile(frd_result_file, frd_frequency_test_result_file)
    shutil.copyfile(dat_result_file, dat_frequency_test_result_file)

    # thermomech
    print("create thermomech result files")
    FreeCAD.open(thermomech_analysis_dir + 'spine_thermomech.FCStd')
    FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.Analysis)
    fea = ccxtools.FemToolsCcx()
    fea.reset_all()
    fea.run()
    fea.load_results()
    stats_thermomech = []
    for s in stat_types:
        statval = resulttools.get_stats(FreeCAD.ActiveDocument.getObject('CalculiX_thermomech_results'), s)
        stats_thermomech.append("{0}: ({1:.14g}, {2:.14g}, {3:.14g})\n".format(s, statval[0], statval[1], statval[2]))
    thermomech_expected_values_file = thermomech_analysis_dir + 'spine_thermomech_expected_values'
    f = open(thermomech_expected_values_file, 'w')
    for s in stats_thermomech:
        f.write(s)
    f.close()
    frd_result_file = os.path.splitext(fea.inp_file_name)[0] + '.frd'
    dat_result_file = os.path.splitext(fea.inp_file_name)[0] + '.dat'
    frd_thermomech_test_result_file = thermomech_analysis_dir + 'spine_thermomech.frd'
    dat_thermomech_test_result_file = thermomech_analysis_dir + 'spine_thermomech.dat'
    shutil.copyfile(frd_result_file, frd_thermomech_test_result_file)
    shutil.copyfile(dat_result_file, dat_thermomech_test_result_file)
    print('Results copied to the appropriate FEM test dirs in: ' + temp_dir)

    # Flow1D
    print("create Flow1D result files")
    FreeCAD.open(Flow1D_thermomech_analysis_dir + 'Flow1D_thermomech.FCStd')
    FemGui.setActiveAnalysis(FreeCAD.ActiveDocument.Analysis)
    fea = ccxtools.FemToolsCcx()
    fea.reset_all()
    fea.run()
    fea.load_results()
    stats_flow1D = []
    for s in stat_types:
        statval = resulttools.get_stats(FreeCAD.ActiveDocument.getObject('CalculiX_thermomech_time_1_0_results'), s)
        stats_flow1D.append("{0}: ({1:.14g}, {2:.14g}, {3:.14g})\n".format(s, statval[0], statval[1], statval[2]))
    Flow1D_thermomech_expected_values_file = Flow1D_thermomech_analysis_dir + 'Flow1D_thermomech_expected_values'
    f = open(Flow1D_thermomech_expected_values_file, 'w')
    for s in stats_flow1D:
        f.write(s)
    f.close()
    frd_result_file = os.path.splitext(fea.inp_file_name)[0] + '.frd'
    dat_result_file = os.path.splitext(fea.inp_file_name)[0] + '.dat'
    frd_Flow1D_thermomech_test_result_file = Flow1D_thermomech_analysis_dir + 'Flow1D_thermomech.frd'
    dat_Flow1D_thermomech_test_result_file = Flow1D_thermomech_analysis_dir + 'Flow1D_thermomech.dat'
    shutil.copyfile(frd_result_file, frd_Flow1D_thermomech_test_result_file)
    shutil.copyfile(dat_result_file, dat_Flow1D_thermomech_test_result_file)
    print('Flow1D thermomech results copied to the appropriate FEM test dirs in: ' + temp_dir)


'''
update the results of FEM ccxtools unit tests:

from femtest.testccxtools import create_test_results
create_test_results()

copy result files from your_temp_directory/FEM_unittests/   test directories into the src directory
compare the results with git difftool
run make
start FreeCAD and run FEM unit test
if FEM unit test is fine --> commit new FEM unit test results

TODO compare the inp file of the helper with the inp file of FEM unit tests
TODO the better way: move the result creation inside the TestFem and add some preference to deactivate this because it needs ccx
'''
