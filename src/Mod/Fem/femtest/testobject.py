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
from . import testtools
# from .testtools import fcc_print


class FemObject(unittest.TestCase):

    def setUp(self):
        self.doc_name = "TestsFemObject"
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
        # is = 43 (just copy in empty file to test, or run unit test case, it is printed)
        # TODO if the equations and gmsh mesh childs are added to the analysis,
        # they show up twice on Tree (on solver resp. gemsh mesh obj and on analysis)
        # https://forum.freecadweb.org/viewtopic.php?t=25283

        doc.recompute()
        self.assertEqual(len(analysis.Group), testtools.get_defmake_count() - 1)  # because of the analysis itself count -1

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
        # is = 43 (just copy in empty file to test)

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
        # is = 43 (just copy in empty file to test)

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
        # is = 43 (just copy in empty file to test)

    def tearDown(self):
        FreeCAD.closeDocument(self.doc_name)
        pass
