# ***************************************************************************
# *   Copyright (c) 2018 - FreeCAD Developers                               *
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
from . import utilstest as testtools
from .utilstest import fcc_print


class TestObjectCreate(unittest.TestCase):
    fcc_print('import TestObjectCreate')

    def setUp(self):
        self.doc_name = "TestObjectCreate"
        try:
            FreeCAD.setActiveDocument(self.doc_name)
        except:
            FreeCAD.newDocument(self.doc_name)
        finally:
            FreeCAD.setActiveDocument(self.doc_name)
        self.active_doc = FreeCAD.ActiveDocument

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

        res = analysis.addObject(ObjectsFem.makeResultMechanical(doc))[0]
        if "BUILD_FEM_VTK" in FreeCAD.__cmake__:
            vres = analysis.addObject(ObjectsFem.makePostVtkResult(doc, res))[0]
            analysis.addObject(ObjectsFem.makePostVtkFilterClipRegion(doc, vres))
            analysis.addObject(ObjectsFem.makePostVtkFilterClipScalar(doc, vres))
            analysis.addObject(ObjectsFem.makePostVtkFilterCutFunction(doc, vres))
            analysis.addObject(ObjectsFem.makePostVtkFilterWarp(doc, vres))

        analysis.addObject(ObjectsFem.makeSolverCalculixCcxTools(doc))
        analysis.addObject(ObjectsFem.makeSolverCalculix(doc))
        sol = analysis.addObject(ObjectsFem.makeSolverElmer(doc))[0]
        analysis.addObject(ObjectsFem.makeSolverZ88(doc))

        analysis.addObject(ObjectsFem.makeEquationElasticity(doc, sol))
        analysis.addObject(ObjectsFem.makeEquationElectrostatic(doc, sol))
        analysis.addObject(ObjectsFem.makeEquationFlow(doc, sol))
        analysis.addObject(ObjectsFem.makeEquationFluxsolver(doc, sol))
        analysis.addObject(ObjectsFem.makeEquationHeat(doc, sol))
        # is = 48 (just copy in empty file to test, or run unit test case, it is printed)
        # TODO if the equations and gmsh mesh childs are added to the analysis,
        # they show up twice on Tree (on solver resp. gemsh mesh obj and on analysis)
        # https://forum.freecadweb.org/viewtopic.php?t=25283

        doc.recompute()

        # if FEM VTK post processing is disabled, we are not able to create VTK post objects
        if "BUILD_FEM_VTK" in FreeCAD.__cmake__:
            fem_vtk_post = True
        else:
            fem_vtk_post = False
        self.assertEqual(len(analysis.Group), testtools.get_defmake_count(fem_vtk_post) - 1)  # because of the analysis itself count -1

    def tearDown(self):
        FreeCAD.closeDocument(self.doc_name)
        pass


class TestObjectType(unittest.TestCase):
    fcc_print('import TestObjectType')

    def setUp(self):
        self.doc_name = "TestObjectType"
        try:
            FreeCAD.setActiveDocument(self.doc_name)
        except:
            FreeCAD.newDocument(self.doc_name)
        finally:
            FreeCAD.setActiveDocument(self.doc_name)
        self.active_doc = FreeCAD.ActiveDocument

    def test_femobjects_type(self):
        doc = self.active_doc

        from femtools.femutils import type_of_obj
        self.assertEqual('Fem::FemAnalysis', type_of_obj(ObjectsFem.makeAnalysis(doc)))
        self.assertEqual('Fem::ConstraintBearing', type_of_obj(ObjectsFem.makeConstraintBearing(doc)))
        self.assertEqual('Fem::ConstraintBodyHeatSource', type_of_obj(ObjectsFem.makeConstraintBodyHeatSource(doc)))
        self.assertEqual('Fem::ConstraintContact', type_of_obj(ObjectsFem.makeConstraintContact(doc)))
        self.assertEqual('Fem::ConstraintDisplacement', type_of_obj(ObjectsFem.makeConstraintDisplacement(doc)))
        self.assertEqual('Fem::ConstraintElectrostaticPotential', type_of_obj(ObjectsFem.makeConstraintElectrostaticPotential(doc)))
        self.assertEqual('Fem::ConstraintFixed', type_of_obj(ObjectsFem.makeConstraintFixed(doc)))
        self.assertEqual('Fem::ConstraintFlowVelocity', type_of_obj(ObjectsFem.makeConstraintFlowVelocity(doc)))
        self.assertEqual('Fem::ConstraintFluidBoundary', type_of_obj(ObjectsFem.makeConstraintFluidBoundary(doc)))
        self.assertEqual('Fem::ConstraintForce', type_of_obj(ObjectsFem.makeConstraintForce(doc)))
        self.assertEqual('Fem::ConstraintGear', type_of_obj(ObjectsFem.makeConstraintGear(doc)))
        self.assertEqual('Fem::ConstraintHeatflux', type_of_obj(ObjectsFem.makeConstraintHeatflux(doc)))
        self.assertEqual('Fem::ConstraintInitialFlowVelocity', type_of_obj(ObjectsFem.makeConstraintInitialFlowVelocity(doc)))
        self.assertEqual('Fem::ConstraintInitialTemperature', type_of_obj(ObjectsFem.makeConstraintInitialTemperature(doc)))
        self.assertEqual('Fem::ConstraintPlaneRotation', type_of_obj(ObjectsFem.makeConstraintPlaneRotation(doc)))
        self.assertEqual('Fem::ConstraintPressure', type_of_obj(ObjectsFem.makeConstraintPressure(doc)))
        self.assertEqual('Fem::ConstraintPulley', type_of_obj(ObjectsFem.makeConstraintPulley(doc)))
        self.assertEqual('Fem::ConstraintSelfWeight', type_of_obj(ObjectsFem.makeConstraintSelfWeight(doc)))
        self.assertEqual('Fem::ConstraintTemperature', type_of_obj(ObjectsFem.makeConstraintTemperature(doc)))
        self.assertEqual('Fem::ConstraintTransform', type_of_obj(ObjectsFem.makeConstraintTransform(doc)))
        self.assertEqual('Fem::FemElementFluid1D', type_of_obj(ObjectsFem.makeElementFluid1D(doc)))
        self.assertEqual('Fem::FemElementGeometry1D', type_of_obj(ObjectsFem.makeElementGeometry1D(doc)))
        self.assertEqual('Fem::FemElementGeometry2D', type_of_obj(ObjectsFem.makeElementGeometry2D(doc)))
        self.assertEqual('Fem::FemElementRotation1D', type_of_obj(ObjectsFem.makeElementRotation1D(doc)))
        materialsolid = ObjectsFem.makeMaterialSolid(doc)
        self.assertEqual('Fem::Material', type_of_obj(ObjectsFem.makeMaterialFluid(doc)))
        self.assertEqual('Fem::Material', type_of_obj(materialsolid))
        self.assertEqual('Fem::MaterialMechanicalNonlinear', type_of_obj(ObjectsFem.makeMaterialMechanicalNonlinear(doc, materialsolid)))
        mesh = ObjectsFem.makeMeshGmsh(doc)
        self.assertEqual('Fem::FemMeshGmsh', type_of_obj(mesh))
        self.assertEqual('Fem::FemMeshBoundaryLayer', type_of_obj(ObjectsFem.makeMeshBoundaryLayer(doc, mesh)))
        self.assertEqual('Fem::FemMeshGroup', type_of_obj(ObjectsFem.makeMeshGroup(doc, mesh)))
        self.assertEqual('Fem::FemMeshRegion', type_of_obj(ObjectsFem.makeMeshRegion(doc, mesh)))
        self.assertEqual('Fem::FemMeshShapeNetgenObject', type_of_obj(ObjectsFem.makeMeshNetgen(doc)))
        self.assertEqual('Fem::FemMeshResult', type_of_obj(ObjectsFem.makeMeshResult(doc)))
        self.assertEqual('Fem::FemResultMechanical', type_of_obj(ObjectsFem.makeResultMechanical(doc)))
        solverelmer = ObjectsFem.makeSolverElmer(doc)
        self.assertEqual('Fem::FemSolverCalculixCcxTools', type_of_obj(ObjectsFem.makeSolverCalculixCcxTools(doc)))
        self.assertEqual('Fem::FemSolverObjectCalculix', type_of_obj(ObjectsFem.makeSolverCalculix(doc)))
        self.assertEqual('Fem::FemSolverObjectElmer', type_of_obj(solverelmer))
        self.assertEqual('Fem::FemSolverObjectZ88', type_of_obj(ObjectsFem.makeSolverZ88(doc)))
        self.assertEqual('Fem::FemEquationElmerElasticity', type_of_obj(ObjectsFem.makeEquationElasticity(doc, solverelmer)))
        self.assertEqual('Fem::FemEquationElmerElectrostatic', type_of_obj(ObjectsFem.makeEquationElectrostatic(doc, solverelmer)))
        self.assertEqual('Fem::FemEquationElmerFlow', type_of_obj(ObjectsFem.makeEquationFlow(doc, solverelmer)))
        self.assertEqual('Fem::FemEquationElmerFluxsolver', type_of_obj(ObjectsFem.makeEquationFluxsolver(doc, solverelmer)))
        self.assertEqual('Fem::FemEquationElmerHeat', type_of_obj(ObjectsFem.makeEquationHeat(doc, solverelmer)))
        # is = 43 (just copy in empty file to test)
        # TODO: vtk post objs
        # TODO: use different type for fluid and solid material

    def test_femobjects_isoftype(self):
        doc = self.active_doc

        from femtools.femutils import is_of_type
        self.assertTrue(is_of_type(ObjectsFem.makeAnalysis(doc), 'Fem::FemAnalysis'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintBearing(doc), 'Fem::ConstraintBearing'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintBodyHeatSource(doc), 'Fem::ConstraintBodyHeatSource'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintContact(doc), 'Fem::ConstraintContact'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintDisplacement(doc), 'Fem::ConstraintDisplacement'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintElectrostaticPotential(doc), 'Fem::ConstraintElectrostaticPotential'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintFixed(doc), 'Fem::ConstraintFixed'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintFlowVelocity(doc), 'Fem::ConstraintFlowVelocity'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintFluidBoundary(doc), 'Fem::ConstraintFluidBoundary'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintForce(doc), 'Fem::ConstraintForce'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintGear(doc), 'Fem::ConstraintGear'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintHeatflux(doc), 'Fem::ConstraintHeatflux'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintInitialFlowVelocity(doc), 'Fem::ConstraintInitialFlowVelocity'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintInitialTemperature(doc), 'Fem::ConstraintInitialTemperature'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintPlaneRotation(doc), 'Fem::ConstraintPlaneRotation'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintPressure(doc), 'Fem::ConstraintPressure'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintPulley(doc), 'Fem::ConstraintPulley'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintSelfWeight(doc), 'Fem::ConstraintSelfWeight'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintTemperature(doc), 'Fem::ConstraintTemperature'))
        self.assertTrue(is_of_type(ObjectsFem.makeConstraintTransform(doc), 'Fem::ConstraintTransform'))
        self.assertTrue(is_of_type(ObjectsFem.makeElementFluid1D(doc), 'Fem::FemElementFluid1D'))
        self.assertTrue(is_of_type(ObjectsFem.makeElementGeometry1D(doc), 'Fem::FemElementGeometry1D'))
        self.assertTrue(is_of_type(ObjectsFem.makeElementGeometry2D(doc), 'Fem::FemElementGeometry2D'))
        self.assertTrue(is_of_type(ObjectsFem.makeElementRotation1D(doc), 'Fem::FemElementRotation1D'))
        materialsolid = ObjectsFem.makeMaterialSolid(doc)
        self.assertTrue(is_of_type(ObjectsFem.makeMaterialFluid(doc), 'Fem::Material'))
        self.assertTrue(is_of_type(materialsolid, 'Fem::Material'))
        self.assertTrue(is_of_type(ObjectsFem.makeMaterialMechanicalNonlinear(doc, materialsolid), 'Fem::MaterialMechanicalNonlinear'))
        mesh = ObjectsFem.makeMeshGmsh(doc)
        self.assertTrue(is_of_type(mesh, 'Fem::FemMeshGmsh'))
        self.assertTrue(is_of_type(ObjectsFem.makeMeshBoundaryLayer(doc, mesh), 'Fem::FemMeshBoundaryLayer'))
        self.assertTrue(is_of_type(ObjectsFem.makeMeshGroup(doc, mesh), 'Fem::FemMeshGroup'))
        self.assertTrue(is_of_type(ObjectsFem.makeMeshRegion(doc, mesh), 'Fem::FemMeshRegion'))
        self.assertTrue(is_of_type(ObjectsFem.makeMeshNetgen(doc), 'Fem::FemMeshShapeNetgenObject'))
        self.assertTrue(is_of_type(ObjectsFem.makeMeshResult(doc), 'Fem::FemMeshResult'))
        self.assertTrue(is_of_type(ObjectsFem.makeResultMechanical(doc), 'Fem::FemResultMechanical'))
        solverelmer = ObjectsFem.makeSolverElmer(doc)
        self.assertTrue(is_of_type(ObjectsFem.makeSolverCalculixCcxTools(doc), 'Fem::FemSolverCalculixCcxTools'))
        self.assertTrue(is_of_type(ObjectsFem.makeSolverCalculix(doc), 'Fem::FemSolverObjectCalculix'))
        self.assertTrue(is_of_type(solverelmer, 'Fem::FemSolverObjectElmer'))
        self.assertTrue(is_of_type(ObjectsFem.makeSolverZ88(doc), 'Fem::FemSolverObjectZ88'))
        self.assertTrue(is_of_type(ObjectsFem.makeEquationElasticity(doc, solverelmer), 'Fem::FemEquationElmerElasticity'))
        self.assertTrue(is_of_type(ObjectsFem.makeEquationElectrostatic(doc, solverelmer), 'Fem::FemEquationElmerElectrostatic'))
        self.assertTrue(is_of_type(ObjectsFem.makeEquationFlow(doc, solverelmer), 'Fem::FemEquationElmerFlow'))
        self.assertTrue(is_of_type(ObjectsFem.makeEquationFluxsolver(doc, solverelmer), 'Fem::FemEquationElmerFluxsolver'))
        self.assertTrue(is_of_type(ObjectsFem.makeEquationHeat(doc, solverelmer), 'Fem::FemEquationElmerHeat'))
        # is = 43 (just copy in empty file to test)

    def test_femobjects_derivedfromfem(self):
        # try to add all possible True types from inheritance chain see https://forum.freecadweb.org/viewtopic.php?f=10&t=32625
        doc = self.active_doc

        from femtools.femutils import is_derived_from

        materialsolid = ObjectsFem.makeMaterialSolid(doc)
        mesh = ObjectsFem.makeMeshGmsh(doc)
        solverelmer = ObjectsFem.makeSolverElmer(doc)

        self.assertTrue(is_derived_from(ObjectsFem.makeAnalysis(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeAnalysis(doc), 'Fem::FemAnalysis'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintBearing(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintBearing(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintBearing(doc), 'Fem::ConstraintBearing'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintBodyHeatSource(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintBodyHeatSource(doc), 'Fem::ConstraintPython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintBodyHeatSource(doc), 'Fem::ConstraintBodyHeatSource'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintContact(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintContact(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintContact(doc), 'Fem::ConstraintContact'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintDisplacement(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintDisplacement(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintDisplacement(doc), 'Fem::ConstraintDisplacement'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintElectrostaticPotential(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintElectrostaticPotential(doc), 'Fem::ConstraintPython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintElectrostaticPotential(doc), 'Fem::ConstraintElectrostaticPotential'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintFixed(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintFixed(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintFixed(doc), 'Fem::ConstraintFixed'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintFlowVelocity(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintFlowVelocity(doc), 'Fem::ConstraintPython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintFlowVelocity(doc), 'Fem::ConstraintFlowVelocity'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintFluidBoundary(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintFluidBoundary(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintFluidBoundary(doc), 'Fem::ConstraintFluidBoundary'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintForce(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintForce(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintForce(doc), 'Fem::ConstraintForce'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintGear(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintGear(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintGear(doc), 'Fem::ConstraintGear'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintHeatflux(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintHeatflux(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintHeatflux(doc), 'Fem::ConstraintHeatflux'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintInitialFlowVelocity(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintInitialFlowVelocity(doc), 'Fem::ConstraintPython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintInitialFlowVelocity(doc), 'Fem::ConstraintInitialFlowVelocity'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintInitialTemperature(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintInitialTemperature(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintInitialTemperature(doc), 'Fem::ConstraintInitialTemperature'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintPlaneRotation(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintPlaneRotation(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintPlaneRotation(doc), 'Fem::ConstraintPlaneRotation'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintPressure(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintPressure(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintPressure(doc), 'Fem::ConstraintPressure'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintPulley(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintPulley(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintPulley(doc), 'Fem::ConstraintPulley'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintSelfWeight(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintSelfWeight(doc), 'Fem::ConstraintPython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintSelfWeight(doc), 'Fem::ConstraintSelfWeight'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintTemperature(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintTemperature(doc), 'Fem::Constraint'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintTemperature(doc), 'Fem::ConstraintTemperature'))

        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintTransform(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeConstraintTransform(doc), 'Fem::ConstraintTransform'))

        self.assertTrue(is_derived_from(ObjectsFem.makeElementFluid1D(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeElementFluid1D(doc), 'Fem::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeElementFluid1D(doc), 'Fem::FemElementFluid1D'))

        self.assertTrue(is_derived_from(ObjectsFem.makeElementGeometry1D(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeElementGeometry1D(doc), 'Fem::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeElementGeometry1D(doc), 'Fem::FemElementGeometry1D'))

        self.assertTrue(is_derived_from(ObjectsFem.makeElementGeometry2D(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeElementGeometry2D(doc), 'Fem::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeElementGeometry2D(doc), 'Fem::FemElementGeometry2D'))

        self.assertTrue(is_derived_from(ObjectsFem.makeElementRotation1D(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeElementRotation1D(doc), 'Fem::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeElementRotation1D(doc), 'Fem::FemElementRotation1D'))

        self.assertTrue(is_derived_from(ObjectsFem.makeMaterialFluid(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMaterialFluid(doc), 'App::MaterialObjectPython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMaterialFluid(doc), 'Fem::Material'))

        self.assertTrue(is_derived_from(materialsolid, 'App::DocumentObject'))
        self.assertTrue(is_derived_from(materialsolid, 'App::MaterialObjectPython'))
        self.assertTrue(is_derived_from(materialsolid, 'Fem::Material'))

        self.assertTrue(is_derived_from(ObjectsFem.makeMaterialMechanicalNonlinear(doc, materialsolid), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMaterialMechanicalNonlinear(doc, materialsolid), 'Fem::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMaterialMechanicalNonlinear(doc, materialsolid), 'Fem::MaterialMechanicalNonlinear'))

        self.assertTrue(is_derived_from(mesh, 'App::DocumentObject'))
        self.assertTrue(is_derived_from(mesh, 'Fem::FemMeshObjectPython'))
        self.assertTrue(is_derived_from(mesh, 'Fem::FemMeshGmsh'))

        self.assertTrue(is_derived_from(ObjectsFem.makeMeshBoundaryLayer(doc, mesh), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMeshBoundaryLayer(doc, mesh), 'Fem::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMeshBoundaryLayer(doc, mesh), 'Fem::FemMeshBoundaryLayer'))

        self.assertTrue(is_derived_from(ObjectsFem.makeMeshGroup(doc, mesh), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMeshGroup(doc, mesh), 'Fem::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMeshGroup(doc, mesh), 'Fem::FemMeshGroup'))

        self.assertTrue(is_derived_from(ObjectsFem.makeMeshRegion(doc, mesh), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMeshRegion(doc, mesh), 'Fem::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMeshRegion(doc, mesh), 'Fem::FemMeshRegion'))

        self.assertTrue(is_derived_from(ObjectsFem.makeMeshNetgen(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMeshNetgen(doc), 'Fem::FemMeshShapeNetgenObject'))

        self.assertTrue(is_derived_from(ObjectsFem.makeMeshResult(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMeshResult(doc), 'Fem::FemMeshObjectPython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeMeshResult(doc), 'Fem::FemMeshResult'))

        self.assertTrue(is_derived_from(ObjectsFem.makeResultMechanical(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeResultMechanical(doc), 'Fem::FemResultObjectPython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeResultMechanical(doc), 'Fem::FemResultMechanical'))

        self.assertTrue(is_derived_from(ObjectsFem.makeSolverCalculixCcxTools(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeSolverCalculixCcxTools(doc), 'Fem::FemSolverObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeSolverCalculixCcxTools(doc), 'Fem::FemSolverObjectPython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeSolverCalculixCcxTools(doc), 'Fem::FemSolverCalculixCcxTools'))

        self.assertTrue(is_derived_from(ObjectsFem.makeSolverCalculix(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeSolverCalculix(doc), 'Fem::FemSolverObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeSolverCalculix(doc), 'Fem::FemSolverObjectPython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeSolverCalculix(doc), 'Fem::FemSolverObjectCalculix'))

        self.assertTrue(is_derived_from(solverelmer, 'App::DocumentObject'))
        self.assertTrue(is_derived_from(solverelmer, 'Fem::FemSolverObject'))
        self.assertTrue(is_derived_from(solverelmer, 'Fem::FemSolverObjectPython'))
        self.assertTrue(is_derived_from(solverelmer, 'Fem::FemSolverObjectElmer'))

        self.assertTrue(is_derived_from(ObjectsFem.makeSolverZ88(doc), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeSolverZ88(doc), 'Fem::FemSolverObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeSolverZ88(doc), 'Fem::FemSolverObjectPython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeSolverZ88(doc), 'Fem::FemSolverObjectZ88'))

        self.assertTrue(is_derived_from(ObjectsFem.makeEquationElasticity(doc, solverelmer), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeEquationElasticity(doc, solverelmer), 'App::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeEquationElasticity(doc, solverelmer), 'Fem::FemEquationElmerElasticity'))

        self.assertTrue(is_derived_from(ObjectsFem.makeEquationElectrostatic(doc, solverelmer), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeEquationElectrostatic(doc, solverelmer), 'App::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeEquationElectrostatic(doc, solverelmer), 'Fem::FemEquationElmerElectrostatic'))

        self.assertTrue(is_derived_from(ObjectsFem.makeEquationFlow(doc, solverelmer), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeEquationFlow(doc, solverelmer), 'App::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeEquationFlow(doc, solverelmer), 'Fem::FemEquationElmerFlow'))

        self.assertTrue(is_derived_from(ObjectsFem.makeEquationFluxsolver(doc, solverelmer), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeEquationFluxsolver(doc, solverelmer), 'App::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeEquationFluxsolver(doc, solverelmer), 'Fem::FemEquationElmerFluxsolver'))

        self.assertTrue(is_derived_from(ObjectsFem.makeEquationHeat(doc, solverelmer), 'App::DocumentObject'))
        self.assertTrue(is_derived_from(ObjectsFem.makeEquationHeat(doc, solverelmer), 'App::FeaturePython'))
        self.assertTrue(is_derived_from(ObjectsFem.makeEquationHeat(doc, solverelmer), 'Fem::FemEquationElmerHeat'))

    def test_femobjects_derivedfromstd(self):
        # only the last True type is used
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
        # is = 43 (just copy in empty file to test)

    def tearDown(self):
        FreeCAD.closeDocument(self.doc_name)
        pass
