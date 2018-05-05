# ***************************************************************************
# *   Copyright (c) 2015 - FreeCAD Developers                               *
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


import FreeCAD
import ObjectsFem
import unittest
from . import testtools
from .testtools import fcc_print


class FemTest(unittest.TestCase):

    def setUp(self):
        try:
            FreeCAD.setActiveDocument("FemTest")
        except:
            FreeCAD.newDocument("FemTest")
        finally:
            FreeCAD.setActiveDocument("FemTest")
        self.active_doc = FreeCAD.ActiveDocument

    def test_read_frd_massflow_networkpressure(self):
        # read data from frd file
        frd_file = testtools.get_fem_test_home_dir() + 'ccx/Flow1D_thermomech.frd'
        import feminout.importCcxFrdResults as importCcxFrdResults
        frd_content = importCcxFrdResults.readResult(frd_file)

        # do something with the read data
        frd_content_len = []
        for key in sorted(frd_content.keys()):
            frd_content_len.append(len(frd_content[key]))
        print('read data')
        print(frd_content_len)
        print(sorted(frd_content.keys()))
        # print(frd_content)
        read_mflow = frd_content['Results'][12]['mflow']
        read_npressure = frd_content['Results'][12]['npressure']
        res_len = [
            len(read_mflow),
            len(read_npressure)
        ]
        print(res_len)
        print(read_mflow)
        print(read_npressure)

        # create the expected data
        print('\nexpected data')
        efc = {}  # expected frd content
        efc['Nodes'] = {
            2: FreeCAD.Vector(0.0, 0.0, -50.0),
            3: FreeCAD.Vector(0.0, 0.0, -4300.0),
            4: FreeCAD.Vector(4950.0, 0.0, -4300.0),
            5: FreeCAD.Vector(5000.0, 0.0, -4300.0),
            6: FreeCAD.Vector(8535.53, 0.0, -7835.53),
            7: FreeCAD.Vector(8569.88, 0.0, -7870.88),
            8: FreeCAD.Vector(12105.4, 0.0, -11406.4),
            9: FreeCAD.Vector(12140.8, 0.0, -11441.8),
            10: FreeCAD.Vector(13908.5, 0.0, -13209.5),
            11: FreeCAD.Vector(13943.9, 0.0, -13244.9),
            12: FreeCAD.Vector(15047.0, 0.0, -14348.0),
            13: FreeCAD.Vector(15047.0, 0.0, -7947.97),
            15: FreeCAD.Vector(0.0, 0.0, 0.0),
            16: FreeCAD.Vector(0.0, 0.0, -2175.0),
            17: FreeCAD.Vector(2475.0, 0.0, -4300.0),
            18: FreeCAD.Vector(4975.0, 0.0, -4300.0),
            19: FreeCAD.Vector(6767.77, 0.0, -6067.77),
            20: FreeCAD.Vector(8552.71, 0.0, -7853.21),
            21: FreeCAD.Vector(10337.6, 0.0, -9638.64),
            22: FreeCAD.Vector(12123.1, 0.0, -11424.1),
            23: FreeCAD.Vector(13024.6, 0.0, -12325.6),
            24: FreeCAD.Vector(13926.2, 0.0, -13227.2),
            25: FreeCAD.Vector(14495.4, 0.0, -13796.4),
            26: FreeCAD.Vector(15047.0, 0.0, -11148.0),
            27: FreeCAD.Vector(15047.0, 0.0, -7897.97)
        }
        efc['Seg2Elem'] = {
            1: (15, 2),
            13: (13, 27)
        }
        efc['Seg3Elem'] = {}
        '''   deleted during reading because of the inout file
        efc['Seg3Elem'] = {
            2: (2, 16, 3),
            3: (3, 17, 4),
            4: (4, 18, 5),
            5: (5, 19, 6),
            6: (6, 20, 7),
            7: (7, 21, 8),
            8: (8, 22, 9),
            9: (9, 23, 10),
            10: (10, 24, 11),
            11: (11, 25, 12),
            12: (12, 26, 13)
        }
        '''
        efc['Tria3Elem'] = efc['Tria6Elem'] = efc['Quad4Elem'] = efc['Quad8Elem'] = {}  # faces
        efc['Tetra4Elem'] = efc['Tetra10Elem'] = efc['Hexa8Elem'] = efc['Hexa20Elem'] = efc['Penta6Elem'] = efc['Penta15Elem'] = {}  # volumes
        efc['Results'] = [
            {'time': 0.00390625},
            {'time': 0.0078125},
            {'time': 0.0136719},
            {'time': 0.0224609},
            {'time': 0.0356445},
            {'time': 0.0554199},
            {'time': 0.085083},
            {'time': 0.129578},
            {'time': 0.19632},
            {'time': 0.296432},
            {'time': 0.446602},
            {'time': 0.671856},
            {
                'number': 0,
                'time': 1.0,
                'mflow': {
                    1: 78.3918,  # added during reading because of the inout file
                    2: 78.3918,
                    3: 78.3918,
                    4: 78.3918,
                    5: 78.3918,
                    6: 78.3918,
                    7: 78.3918,
                    8: 78.3918,
                    9: 78.3918,
                    10: 78.3918,
                    11: 78.3918,
                    12: 78.3918,
                    13: 78.3918,
                    15: 78.3918,
                    16: 78.3918,
                    17: 78.3918,
                    18: 78.3918,
                    19: 78.3918,
                    20: 78.3918,
                    21: 78.3918,
                    22: 78.3918,
                    23: 78.3918,
                    24: 78.3918,
                    25: 78.3918,
                    26: 78.3918,
                    27: 78.3918,
                    28: 78.3918  # added during reading because of the inout file
                },
                'npressure': {
                    1: 0.1,  # added during reading because of the inout file
                    2: 0.1,
                    3: 0.134840,
                    4: 0.128261,
                    5: 0.127949,
                    6: 0.155918,
                    7: 0.157797,
                    8: 0.191647,
                    9: 0.178953,
                    10: 0.180849,
                    11: 0.161476,
                    12: 0.162658,
                    13: 0.1,
                    15: 0.1,
                    16: 0.117420,
                    17: 0.131551,
                    18: 0.128105,
                    19: 0.141934,
                    20: 0.156857,
                    21: 0.174722,
                    22: 0.185300,
                    23: 0.179901,
                    24: 0.171162,
                    25: 0.162067,
                    26: 0.131329,
                    27: 0.1,
                    28: 0.1  # added during reading because of the inout file
                }
            }
        ]
        expected_frd_content = efc

        # do something with the expected data
        expected_frd_content_len = []
        for key in sorted(expected_frd_content.keys()):
            expected_frd_content_len.append(len(expected_frd_content[key]))
        print(expected_frd_content_len)
        print(sorted(expected_frd_content.keys()))
        # expected results
        expected_mflow = expected_frd_content['Results'][12]['mflow']
        expected_npressure = expected_frd_content['Results'][12]['npressure']
        expected_res_len = [
            len(expected_mflow),
            len(expected_npressure)
        ]
        print(expected_res_len)
        print(expected_mflow)
        print(expected_npressure)

        # tests
        self.assertEqual(frd_content_len, expected_frd_content_len, "Length's of read frd data values are unexpected")
        self.assertEqual(frd_content['Nodes'], expected_frd_content['Nodes'], "Values of read node data are unexpected")
        self.assertEqual(frd_content['Seg2Elem'], expected_frd_content['Seg2Elem'], "Values of read Seg2 data are unexpected")
        self.assertEqual(frd_content['Seg3Elem'], expected_frd_content['Seg3Elem'], "Values of read Seg3 data are unexpected")
        self.assertEqual(res_len, expected_res_len, "Length's of read result data values are unexpected")
        self.assertEqual(read_mflow, expected_mflow, "Values of read mflow result data are unexpected")
        self.assertEqual(read_npressure, expected_npressure, "Values of read npressure result data are unexpected")

    def test_femobjects_make(self):
        doc = self.active_doc
        analysis = ObjectsFem.makeAnalysis(doc)

        analysis.addObject(ObjectsFem.makeConstraintBearing(doc))
        analysis.addObject(ObjectsFem.makeConstraintBodyHeatSource(doc))
        analysis.addObject(ObjectsFem.makeConstraintContact(doc))
        analysis.addObject(ObjectsFem.makeConstraintDisplacement(doc))
        analysis.addObject(ObjectsFem.makeConstraintElectrostaticPotential(doc))
        analysis.addObject(ObjectsFem.makeConstraintFixed(doc))
        analysis.addObject(ObjectsFem.makeConstraintFlowVelocity(doc))
        analysis.addObject(ObjectsFem.makeConstraintFluidBoundary(doc))
        analysis.addObject(ObjectsFem.makeConstraintForce(doc))
        analysis.addObject(ObjectsFem.makeConstraintGear(doc))
        analysis.addObject(ObjectsFem.makeConstraintHeatflux(doc))
        analysis.addObject(ObjectsFem.makeConstraintInitialFlowVelocity(doc))
        analysis.addObject(ObjectsFem.makeConstraintInitialTemperature(doc))
        analysis.addObject(ObjectsFem.makeConstraintPlaneRotation(doc))
        analysis.addObject(ObjectsFem.makeConstraintPressure(doc))
        analysis.addObject(ObjectsFem.makeConstraintPulley(doc))
        analysis.addObject(ObjectsFem.makeConstraintSelfWeight(doc))
        analysis.addObject(ObjectsFem.makeConstraintTemperature(doc))
        analysis.addObject(ObjectsFem.makeConstraintTransform(doc))

        analysis.addObject(ObjectsFem.makeElementFluid1D(doc))
        analysis.addObject(ObjectsFem.makeElementGeometry1D(doc))
        analysis.addObject(ObjectsFem.makeElementGeometry2D(doc))
        analysis.addObject(ObjectsFem.makeElementRotation1D(doc))

        analysis.addObject(ObjectsFem.makeMaterialFluid(doc))
        mat = analysis.addObject(ObjectsFem.makeMaterialSolid(doc))[0]
        analysis.addObject(ObjectsFem.makeMaterialMechanicalNonlinear(doc, mat))

        msh = analysis.addObject(ObjectsFem.makeMeshGmsh(doc))[0]
        analysis.addObject(ObjectsFem.makeMeshBoundaryLayer(doc, msh))
        analysis.addObject(ObjectsFem.makeMeshGroup(doc, msh))
        analysis.addObject(ObjectsFem.makeMeshRegion(doc, msh))
        analysis.addObject(ObjectsFem.makeMeshNetgen(doc))
        analysis.addObject(ObjectsFem.makeMeshResult(doc))

        analysis.addObject(ObjectsFem.makeResultMechanical(doc))

        analysis.addObject(ObjectsFem.makeSolverCalculixCcxTools(doc))
        analysis.addObject(ObjectsFem.makeSolverCalculix(doc))
        sol = analysis.addObject(ObjectsFem.makeSolverElmer(doc))[0]
        analysis.addObject(ObjectsFem.makeSolverZ88(doc))

        analysis.addObject(ObjectsFem.makeEquationElasticity(doc, sol))
        analysis.addObject(ObjectsFem.makeEquationElectrostatic(doc, sol))
        analysis.addObject(ObjectsFem.makeEquationFlow(doc, sol))
        analysis.addObject(ObjectsFem.makeEquationFluxsolver(doc, sol))
        analysis.addObject(ObjectsFem.makeEquationHeat(doc, sol))
        # TODO the equations show up twice on Tree (on solver and on analysis), if they are added to the analysis group

        doc.recompute()
        self.assertEqual(len(analysis.Group), testtools.get_defmake_count() - 1)  # because of the analysis itself count -1

    def test_femobjects_type(self):
        doc = self.active_doc

        from femtools.femutils import typeOfObj
        self.assertEqual('Fem::FemAnalysis', typeOfObj(ObjectsFem.makeAnalysis(doc)))
        self.assertEqual('Fem::ConstraintBearing', typeOfObj(ObjectsFem.makeConstraintBearing(doc)))
        self.assertEqual('Fem::ConstraintBodyHeatSource', typeOfObj(ObjectsFem.makeConstraintBodyHeatSource(doc)))
        self.assertEqual('Fem::ConstraintContact', typeOfObj(ObjectsFem.makeConstraintContact(doc)))
        self.assertEqual('Fem::ConstraintDisplacement', typeOfObj(ObjectsFem.makeConstraintDisplacement(doc)))
        self.assertEqual('Fem::ConstraintElectrostaticPotential', typeOfObj(ObjectsFem.makeConstraintElectrostaticPotential(doc)))
        self.assertEqual('Fem::ConstraintFixed', typeOfObj(ObjectsFem.makeConstraintFixed(doc)))
        self.assertEqual('Fem::ConstraintFlowVelocity', typeOfObj(ObjectsFem.makeConstraintFlowVelocity(doc)))
        self.assertEqual('Fem::ConstraintFluidBoundary', typeOfObj(ObjectsFem.makeConstraintFluidBoundary(doc)))
        self.assertEqual('Fem::ConstraintForce', typeOfObj(ObjectsFem.makeConstraintForce(doc)))
        self.assertEqual('Fem::ConstraintGear', typeOfObj(ObjectsFem.makeConstraintGear(doc)))
        self.assertEqual('Fem::ConstraintHeatflux', typeOfObj(ObjectsFem.makeConstraintHeatflux(doc)))
        self.assertEqual('Fem::ConstraintInitialFlowVelocity', typeOfObj(ObjectsFem.makeConstraintInitialFlowVelocity(doc)))
        self.assertEqual('Fem::ConstraintInitialTemperature', typeOfObj(ObjectsFem.makeConstraintInitialTemperature(doc)))
        self.assertEqual('Fem::ConstraintPlaneRotation', typeOfObj(ObjectsFem.makeConstraintPlaneRotation(doc)))
        self.assertEqual('Fem::ConstraintPressure', typeOfObj(ObjectsFem.makeConstraintPressure(doc)))
        self.assertEqual('Fem::ConstraintPulley', typeOfObj(ObjectsFem.makeConstraintPulley(doc)))
        self.assertEqual('Fem::ConstraintSelfWeight', typeOfObj(ObjectsFem.makeConstraintSelfWeight(doc)))
        self.assertEqual('Fem::ConstraintTemperature', typeOfObj(ObjectsFem.makeConstraintTemperature(doc)))
        self.assertEqual('Fem::ConstraintTransform', typeOfObj(ObjectsFem.makeConstraintTransform(doc)))
        self.assertEqual('Fem::FemElementFluid1D', typeOfObj(ObjectsFem.makeElementFluid1D(doc)))
        self.assertEqual('Fem::FemElementGeometry1D', typeOfObj(ObjectsFem.makeElementGeometry1D(doc)))
        self.assertEqual('Fem::FemElementGeometry2D', typeOfObj(ObjectsFem.makeElementGeometry2D(doc)))
        self.assertEqual('Fem::FemElementRotation1D', typeOfObj(ObjectsFem.makeElementRotation1D(doc)))
        materialsolid = ObjectsFem.makeMaterialSolid(doc)
        self.assertEqual('Fem::Material', typeOfObj(ObjectsFem.makeMaterialFluid(doc)))
        self.assertEqual('Fem::Material', typeOfObj(materialsolid))
        self.assertEqual('Fem::MaterialMechanicalNonlinear', typeOfObj(ObjectsFem.makeMaterialMechanicalNonlinear(doc, materialsolid)))
        mesh = ObjectsFem.makeMeshGmsh(doc)
        self.assertEqual('Fem::FemMeshGmsh', typeOfObj(mesh))
        self.assertEqual('Fem::FemMeshBoundaryLayer', typeOfObj(ObjectsFem.makeMeshBoundaryLayer(doc, mesh)))
        self.assertEqual('Fem::FemMeshGroup', typeOfObj(ObjectsFem.makeMeshGroup(doc, mesh)))
        self.assertEqual('Fem::FemMeshRegion', typeOfObj(ObjectsFem.makeMeshRegion(doc, mesh)))
        self.assertEqual('Fem::FemMeshShapeNetgenObject', typeOfObj(ObjectsFem.makeMeshNetgen(doc)))
        self.assertEqual('Fem::FemMeshResult', typeOfObj(ObjectsFem.makeMeshResult(doc)))
        self.assertEqual('Fem::FemResultMechanical', typeOfObj(ObjectsFem.makeResultMechanical(doc)))
        solverelmer = ObjectsFem.makeSolverElmer(doc)
        self.assertEqual('Fem::FemSolverCalculixCcxTools', typeOfObj(ObjectsFem.makeSolverCalculixCcxTools(doc)))
        self.assertEqual('Fem::FemSolverObjectCalculix', typeOfObj(ObjectsFem.makeSolverCalculix(doc)))
        self.assertEqual('Fem::FemSolverObjectElmer', typeOfObj(solverelmer))
        self.assertEqual('Fem::FemSolverObjectZ88', typeOfObj(ObjectsFem.makeSolverZ88(doc)))
        self.assertEqual('Fem::FemEquationElmerElasticity', typeOfObj(ObjectsFem.makeEquationElasticity(doc, solverelmer)))
        self.assertEqual('Fem::FemEquationElmerElectrostatic', typeOfObj(ObjectsFem.makeEquationElectrostatic(doc, solverelmer)))
        self.assertEqual('Fem::FemEquationElmerFlow', typeOfObj(ObjectsFem.makeEquationFlow(doc, solverelmer)))
        self.assertEqual('Fem::FemEquationElmerFluxsolver', typeOfObj(ObjectsFem.makeEquationFluxsolver(doc, solverelmer)))
        self.assertEqual('Fem::FemEquationElmerHeat', typeOfObj(ObjectsFem.makeEquationHeat(doc, solverelmer)))
        # TODO: equation linear missing, equation nonlinear missing, use different type for fluid and solid material

    def test_femobjects_isoftypenew(self):
        doc = self.active_doc

        from femtools.femutils import isOfTypeNew
        self.assertTrue(isOfTypeNew(ObjectsFem.makeAnalysis(doc), 'Fem::FemAnalysis'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintBearing(doc), 'Fem::ConstraintBearing'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintBodyHeatSource(doc), 'Fem::ConstraintBodyHeatSource'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintContact(doc), 'Fem::ConstraintContact'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintDisplacement(doc), 'Fem::ConstraintDisplacement'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintElectrostaticPotential(doc), 'Fem::ConstraintElectrostaticPotential'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintFixed(doc), 'Fem::ConstraintFixed'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintFlowVelocity(doc), 'Fem::ConstraintFlowVelocity'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintFluidBoundary(doc), 'Fem::ConstraintFluidBoundary'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintForce(doc), 'Fem::ConstraintForce'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintGear(doc), 'Fem::ConstraintGear'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintHeatflux(doc), 'Fem::ConstraintHeatflux'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintInitialFlowVelocity(doc), 'Fem::ConstraintInitialFlowVelocity'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintInitialTemperature(doc), 'Fem::ConstraintInitialTemperature'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintPlaneRotation(doc), 'Fem::ConstraintPlaneRotation'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintPressure(doc), 'Fem::ConstraintPressure'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintPulley(doc), 'Fem::ConstraintPulley'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintSelfWeight(doc), 'Fem::ConstraintSelfWeight'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintTemperature(doc), 'Fem::ConstraintTemperature'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeConstraintTransform(doc), 'Fem::ConstraintTransform'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeElementFluid1D(doc), 'Fem::FemElementFluid1D'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeElementGeometry1D(doc), 'Fem::FemElementGeometry1D'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeElementGeometry2D(doc), 'Fem::FemElementGeometry2D'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeElementRotation1D(doc), 'Fem::FemElementRotation1D'))
        materialsolid = ObjectsFem.makeMaterialSolid(doc)
        self.assertTrue(isOfTypeNew(ObjectsFem.makeMaterialFluid(doc), 'Fem::Material'))
        self.assertTrue(isOfTypeNew(materialsolid, 'Fem::Material'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeMaterialMechanicalNonlinear(doc, materialsolid), 'Fem::MaterialMechanicalNonlinear'))
        mesh = ObjectsFem.makeMeshGmsh(doc)
        self.assertTrue(isOfTypeNew(mesh, 'Fem::FemMeshGmsh'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeMeshBoundaryLayer(doc, mesh), 'Fem::FemMeshBoundaryLayer'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeMeshGroup(doc, mesh), 'Fem::FemMeshGroup'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeMeshRegion(doc, mesh), 'Fem::FemMeshRegion'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeMeshNetgen(doc), 'Fem::FemMeshShapeNetgenObject'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeMeshResult(doc), 'Fem::FemMeshResult'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeResultMechanical(doc), 'Fem::FemResultMechanical'))
        solverelmer = ObjectsFem.makeSolverElmer(doc)
        self.assertTrue(isOfTypeNew(ObjectsFem.makeSolverCalculixCcxTools(doc), 'Fem::FemSolverCalculixCcxTools'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeSolverCalculix(doc), 'Fem::FemSolverObjectCalculix'))
        self.assertTrue(isOfTypeNew(solverelmer, 'Fem::FemSolverObjectElmer'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeSolverZ88(doc), 'Fem::FemSolverObjectZ88'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeEquationElasticity(doc, solverelmer), 'Fem::FemEquationElmerElasticity'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeEquationElectrostatic(doc, solverelmer), 'Fem::FemEquationElmerElectrostatic'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeEquationFlow(doc, solverelmer), 'Fem::FemEquationElmerFlow'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeEquationFluxsolver(doc, solverelmer), 'Fem::FemEquationElmerFluxsolver'))
        self.assertTrue(isOfTypeNew(ObjectsFem.makeEquationHeat(doc, solverelmer), 'Fem::FemEquationElmerHeat'))

    def test_femobjects_derivedfromfem(self):
        doc = self.active_doc

        from femtools.femutils import isDerivedFrom as isDerivedFromFem
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeAnalysis(doc), 'Fem::FemAnalysis'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintBearing(doc), 'Fem::ConstraintBearing'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintBodyHeatSource(doc), 'Fem::ConstraintBodyHeatSource'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintContact(doc), 'Fem::ConstraintContact'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintDisplacement(doc), 'Fem::ConstraintDisplacement'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintElectrostaticPotential(doc), 'Fem::ConstraintElectrostaticPotential'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintFixed(doc), 'Fem::ConstraintFixed'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintFlowVelocity(doc), 'Fem::ConstraintFlowVelocity'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintFluidBoundary(doc), 'Fem::ConstraintFluidBoundary'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintForce(doc), 'Fem::ConstraintForce'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintGear(doc), 'Fem::ConstraintGear'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintHeatflux(doc), 'Fem::ConstraintHeatflux'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintInitialFlowVelocity(doc), 'Fem::ConstraintInitialFlowVelocity'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintInitialTemperature(doc), 'Fem::ConstraintInitialTemperature'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintPlaneRotation(doc), 'Fem::ConstraintPlaneRotation'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintPressure(doc), 'Fem::ConstraintPressure'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintPulley(doc), 'Fem::ConstraintPulley'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintSelfWeight(doc), 'Fem::ConstraintSelfWeight'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintTemperature(doc), 'Fem::ConstraintTemperature'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeConstraintTransform(doc), 'Fem::ConstraintTransform'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeElementFluid1D(doc), 'Fem::FemElementFluid1D'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeElementGeometry1D(doc), 'Fem::FemElementGeometry1D'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeElementGeometry2D(doc), 'Fem::FemElementGeometry2D'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeElementRotation1D(doc), 'Fem::FemElementRotation1D'))
        materialsolid = ObjectsFem.makeMaterialSolid(doc)
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeMaterialFluid(doc), 'Fem::Material'))
        self.assertTrue(isDerivedFromFem(materialsolid, 'Fem::Material'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeMaterialMechanicalNonlinear(doc, materialsolid), 'Fem::MaterialMechanicalNonlinear'))
        mesh = ObjectsFem.makeMeshGmsh(doc)
        self.assertTrue(isDerivedFromFem(mesh, 'Fem::FemMeshGmsh'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeMeshBoundaryLayer(doc, mesh), 'Fem::FemMeshBoundaryLayer'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeMeshGroup(doc, mesh), 'Fem::FemMeshGroup'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeMeshRegion(doc, mesh), 'Fem::FemMeshRegion'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeMeshNetgen(doc), 'Fem::FemMeshShapeNetgenObject'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeMeshResult(doc), 'Fem::FemMeshResult'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeResultMechanical(doc), 'Fem::FemResultMechanical'))
        solverelmer = ObjectsFem.makeSolverElmer(doc)
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeSolverCalculixCcxTools(doc), 'Fem::FemSolverCalculixCcxTools'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeSolverCalculix(doc), 'Fem::FemSolverObjectCalculix'))
        self.assertTrue(isDerivedFromFem(solverelmer, 'Fem::FemSolverObjectElmer'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeSolverZ88(doc), 'Fem::FemSolverObjectZ88'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeEquationElasticity(doc, solverelmer), 'Fem::FemEquationElmerElasticity'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeEquationElectrostatic(doc, solverelmer), 'Fem::FemEquationElmerElectrostatic'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeEquationFlow(doc, solverelmer), 'Fem::FemEquationElmerFlow'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeEquationFluxsolver(doc, solverelmer), 'Fem::FemEquationElmerFluxsolver'))
        self.assertTrue(isDerivedFromFem(ObjectsFem.makeEquationHeat(doc, solverelmer), 'Fem::FemEquationElmerHeat'))

    def test_femobjects_derivedfromstd(self):
        doc = self.active_doc

        self.assertTrue(ObjectsFem.makeAnalysis(doc).isDerivedFrom('Fem::FemAnalysis'))
        self.assertTrue(ObjectsFem.makeConstraintBearing(doc).isDerivedFrom('Fem::ConstraintBearing'))
        self.assertTrue(ObjectsFem.makeConstraintBodyHeatSource(doc).isDerivedFrom('Fem::ConstraintPython'))
        self.assertTrue(ObjectsFem.makeConstraintContact(doc).isDerivedFrom('Fem::ConstraintContact'))
        self.assertTrue(ObjectsFem.makeConstraintDisplacement(doc).isDerivedFrom('Fem::ConstraintDisplacement'))
        self.assertTrue(ObjectsFem.makeConstraintElectrostaticPotential(doc).isDerivedFrom('Fem::ConstraintPython'))
        self.assertTrue(ObjectsFem.makeConstraintFixed(doc).isDerivedFrom('Fem::ConstraintFixed'))
        self.assertTrue(ObjectsFem.makeConstraintFlowVelocity(doc).isDerivedFrom('Fem::ConstraintPython'))
        self.assertTrue(ObjectsFem.makeConstraintFluidBoundary(doc).isDerivedFrom('Fem::ConstraintFluidBoundary'))
        self.assertTrue(ObjectsFem.makeConstraintForce(doc).isDerivedFrom('Fem::ConstraintForce'))
        self.assertTrue(ObjectsFem.makeConstraintGear(doc).isDerivedFrom('Fem::ConstraintGear'))
        self.assertTrue(ObjectsFem.makeConstraintHeatflux(doc).isDerivedFrom('Fem::ConstraintHeatflux'))
        self.assertTrue(ObjectsFem.makeConstraintInitialFlowVelocity(doc).isDerivedFrom('Fem::ConstraintPython'))
        self.assertTrue(ObjectsFem.makeConstraintInitialTemperature(doc).isDerivedFrom('Fem::ConstraintInitialTemperature'))
        self.assertTrue(ObjectsFem.makeConstraintPlaneRotation(doc).isDerivedFrom('Fem::ConstraintPlaneRotation'))
        self.assertTrue(ObjectsFem.makeConstraintPressure(doc).isDerivedFrom('Fem::ConstraintPressure'))
        self.assertTrue(ObjectsFem.makeConstraintPulley(doc).isDerivedFrom('Fem::ConstraintPulley'))
        self.assertTrue(ObjectsFem.makeConstraintSelfWeight(doc).isDerivedFrom('Fem::ConstraintPython'))
        self.assertTrue(ObjectsFem.makeConstraintTemperature(doc).isDerivedFrom('Fem::ConstraintTemperature'))
        self.assertTrue(ObjectsFem.makeConstraintTransform(doc).isDerivedFrom('Fem::ConstraintTransform'))
        self.assertTrue(ObjectsFem.makeElementFluid1D(doc).isDerivedFrom('Fem::FeaturePython'))
        self.assertTrue(ObjectsFem.makeElementGeometry1D(doc).isDerivedFrom('Fem::FeaturePython'))
        self.assertTrue(ObjectsFem.makeElementGeometry2D(doc).isDerivedFrom('Fem::FeaturePython'))
        self.assertTrue(ObjectsFem.makeElementRotation1D(doc).isDerivedFrom('Fem::FeaturePython'))
        materialsolid = ObjectsFem.makeMaterialSolid(doc)
        self.assertTrue(ObjectsFem.makeMaterialFluid(doc).isDerivedFrom('App::MaterialObjectPython'))
        self.assertTrue(materialsolid.isDerivedFrom('App::MaterialObjectPython'))
        self.assertTrue(ObjectsFem.makeMaterialMechanicalNonlinear(doc, materialsolid).isDerivedFrom('Fem::FeaturePython'))
        mesh = ObjectsFem.makeMeshGmsh(doc)
        self.assertTrue(mesh.isDerivedFrom('Fem::FemMeshObjectPython'))
        self.assertTrue(ObjectsFem.makeMeshBoundaryLayer(doc, mesh).isDerivedFrom('Fem::FeaturePython'))
        self.assertTrue(ObjectsFem.makeMeshGroup(doc, mesh).isDerivedFrom('Fem::FeaturePython'))
        self.assertTrue(ObjectsFem.makeMeshRegion(doc, mesh).isDerivedFrom('Fem::FeaturePython'))
        self.assertTrue(ObjectsFem.makeMeshNetgen(doc).isDerivedFrom('Fem::FemMeshShapeNetgenObject'))
        self.assertTrue(ObjectsFem.makeMeshResult(doc).isDerivedFrom('Fem::FemMeshObjectPython'))
        self.assertTrue(ObjectsFem.makeResultMechanical(doc).isDerivedFrom('Fem::FemResultObjectPython'))
        solverelmer = ObjectsFem.makeSolverElmer(doc)
        self.assertTrue(ObjectsFem.makeSolverCalculixCcxTools(doc).isDerivedFrom('Fem::FemSolverObjectPython'))
        self.assertTrue(ObjectsFem.makeSolverCalculix(doc).isDerivedFrom('Fem::FemSolverObjectPython'))
        self.assertTrue(solverelmer.isDerivedFrom('Fem::FemSolverObjectPython'))
        self.assertTrue(ObjectsFem.makeSolverZ88(doc).isDerivedFrom('Fem::FemSolverObjectPython'))
        self.assertTrue(ObjectsFem.makeEquationElasticity(doc, solverelmer).isDerivedFrom('App::FeaturePython'))
        self.assertTrue(ObjectsFem.makeEquationElectrostatic(doc, solverelmer).isDerivedFrom('App::FeaturePython'))
        self.assertTrue(ObjectsFem.makeEquationFlow(doc, solverelmer).isDerivedFrom('App::FeaturePython'))
        self.assertTrue(ObjectsFem.makeEquationFluxsolver(doc, solverelmer).isDerivedFrom('App::FeaturePython'))
        self.assertTrue(ObjectsFem.makeEquationHeat(doc, solverelmer).isDerivedFrom('App::FeaturePython'))

    def test_adding_refshaps(self):
        doc = self.active_doc
        slab = doc.addObject("Part::Plane", "Face")
        slab.Length = 500.00
        slab.Width = 500.00
        cf = ObjectsFem.makeConstraintFixed(doc)
        ref_eles = []
        # FreeCAD list property seam not to support append, thus we need some workaround, which is on many elements even much faster
        for i, face in enumerate(slab.Shape.Edges):
            ref_eles.append("Edge%d" % (i + 1))
        cf.References = [(slab, ref_eles)]
        doc.recompute()
        expected_reflist = [(slab, ('Edge1', 'Edge2', 'Edge3', 'Edge4'))]
        assert_err_message = 'Adding reference shapes did not result in expected list ' + str(cf.References) + ' != ' + str(expected_reflist)
        self.assertEqual(cf.References, expected_reflist, assert_err_message)

    def test_pyimport_all_FEM_modules(self):
        # we're going to try to import all python modules from FreeCAD Fem
        pymodules = []

        # collect all Python modules in Fem
        pymodules += testtools.collect_python_modules('')  # Fem main dir
        pymodules += testtools.collect_python_modules('feminout')
        pymodules += testtools.collect_python_modules('femmesh')
        pymodules += testtools.collect_python_modules('femresult')
        pymodules += testtools.collect_python_modules('femtest')
        pymodules += testtools.collect_python_modules('femobjects')
        if FreeCAD.GuiUp:
            pymodules += testtools.collect_python_modules('femcommands')
            pymodules += testtools.collect_python_modules('femguiobjects')
        pymodules += testtools.collect_python_modules('femsolver')
        pymodules += testtools.collect_python_modules('femsolver/elmer')
        pymodules += testtools.collect_python_modules('femsolver/elmer/equations')
        pymodules += testtools.collect_python_modules('femsolver/z88')
        pymodules += testtools.collect_python_modules('femsolver/calculix')

        # import all collected modules
        # fcc_print(pymodules)
        for mod in pymodules:
            fcc_print('Try importing {0} ...'.format(mod))
            try:
                im = __import__('{0}'.format(mod))
            except:
                im = False
            if not im:
                __import__('{0}'.format(mod))  # to get an error message what was going wrong
            self.assertTrue(im, 'Problem importing {0}'.format(mod))

    def tearDown(self):
        FreeCAD.closeDocument("FemTest")
        pass
