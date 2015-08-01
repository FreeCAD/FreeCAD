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
import FemTools
import FreeCAD
import MechanicalAnalysis
import csv
import hashlib
import unittest

# md5sum of src/Mod/Fem/test_file.inp
# All changes in ccxInpWriter resulting in changes of the .inp file should
# be reflected in src/Mod/Fem/test_file.inp and the m5d_standard variable
# should be updated
md5_standard = "5f1739821b1c741a6f35655af102bb14"
mesh_name = 'Mesh'
working_dir = '/tmp/FEM/'
mesh_points_file = 'Mod/Fem/mesh_points.csv'
mesh_volumes_file = 'Mod/Fem/mesh_volumes.csv'


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
        self.analysis = MechanicalAnalysis.makeMechanicalAnalysis('MechanicalAnalysis')
        self.active_doc.recompute()

    def create_new_mesh(self):
        self.mesh_object = self.active_doc.addObject('Fem::FemMeshObject', mesh_name)
        self.mesh = Fem.FemMesh()
        with open(mesh_points_file, 'r') as points_file:
            reader = csv.reader(points_file)
            for p in reader:
                self.mesh.addNode(float(p[0]), float(p[1]), float(p[2]), int(p[3]))

        with open(mesh_volumes_file, 'r') as volumes_file:
            reader = csv.reader(volumes_file)
            for _v in reader:
                v = [int(x) for x in _v]
                self.mesh.addVolume([v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9]])

        self.mesh_object.FemMesh = self.mesh
        self.active_doc.recompute()

    def create_new_material(self):
        self.new_material_object = self.active_doc.addObject("App::MaterialObjectPython", 'MechanicalMaterial')
        mat = self.new_material_object.Material
        mat['Name'] = "Test Material"
        mat['YoungsModulus'] = "20000 MPa"
        mat['PoissonRatio'] = "0.36"
        mat['Density'] = "1000 kg/m^3"
        self.new_material_object.Material = mat

    def create_fixed_constraint(self):
        self.fixed_constraint = self.active_doc.addObject("Fem::ConstraintFixed", "FemConstraintFixed")
        self.fixed_constraint.References = [(self.box, "Face1")]

    def create_force_constraint(self):
        self.force_constraint = self.active_doc.addObject("Fem::ConstraintForce", "FemConstraintForce")
        self.force_constraint.References = [(self.box, "Face3")]
        self.force_constraint.Force = 10.000000
        self.force_constraint.Direction = (self.box, ["Edge12"])
        self.force_constraint.Reversed = True

    def create_pressure_constraint(self):
        self.pressure_constraint = self.active_doc.addObject("Fem::ConstraintPressure", "FemConstraintPressure")
        self.pressure_constraint.References = [(self.box, "Face4")]
        self.pressure_constraint.Pressure = 10.000000
        self.pressure_constraint.Reversed = True

    def test_new_analysis(self):
        FreeCAD.Console.PrintMessage('\nChecking FEM new analysis...\n')
        self.create_new_analysis()
        self.assertTrue(self.analysis, "FemTest of new analysis failed")

        FreeCAD.Console.PrintMessage('\nChecking FEM new mesh...\n')
        self.create_new_mesh()
        self.assertTrue(self.mesh, "FemTest of new mesh failed")
        self.analysis.Member = self.analysis.Member + [self.mesh_object]

        FreeCAD.Console.PrintMessage('\nChecking FEM new material...\n')
        self.create_new_material()
        self.assertTrue(self.new_material_object, "FemTest of new material failed")
        self.analysis.Member = self.analysis.Member + [self.new_material_object]

        FreeCAD.Console.PrintMessage('\nChecking FEM new fixed constraint...\n')
        self.create_fixed_constraint()
        self.assertTrue(self.fixed_constraint, "FemTest of new fixed constraint failed")
        self.analysis.Member = self.analysis.Member + [self.fixed_constraint]

        FreeCAD.Console.PrintMessage('\nChecking FEM new force constraint...\n')
        self.create_force_constraint()
        self.assertTrue(self.force_constraint, "FemTest of new force constraint failed")
        self.analysis.Member = self.analysis.Member + [self.force_constraint]

        FreeCAD.Console.PrintMessage('\nChecking FEM new pressure constraint...\n')
        self.create_pressure_constraint()
        self.assertTrue(self.pressure_constraint, "FemTest of new pressure constraint failed")
        self.analysis.Member = self.analysis.Member + [self.pressure_constraint]

        fea = FemTools.FemTools(self.analysis)
        FreeCAD.Console.PrintMessage('\nChecking FEM inp file prerequisites...\n')
        error = fea.check_prerequisites()
        self.assertFalse(error, "FemTools check_prerequisites returned error message: {}".format(error))

        FreeCAD.Console.PrintMessage('\nChecking FEM inp file write...\n')
        fea.setup_working_dir(working_dir)
        error = fea.write_inp_file()
        md5_test = hashlib.md5(open(working_dir + mesh_name + '.inp', 'rb').read()).hexdigest()
        self.assertEqual(md5_standard, md5_test, "FemTools write_inp_file failed. md5 \
                         sums don't match. md5 for the test file is ()".format(md5_test))

    def tearDown(self):
        FreeCAD.closeDocument("FemTest")
        pass
