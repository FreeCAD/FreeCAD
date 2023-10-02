# ***************************************************************************
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "Objects FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import sys
import unittest
from os.path import join

import FreeCAD

import ObjectsFem
from . import support_utils as testtools
from .support_utils import fcc_print


class TestObjectCreate(unittest.TestCase):
    fcc_print("import TestObjectCreate")

    # ********************************************************************************************
    def setUp(
        self
    ):
        # setUp is executed before every test

        # new document
        self.document = FreeCAD.newDocument(self.__class__.__name__)

    # ********************************************************************************************
    def tearDown(
        self
    ):
        # tearDown is executed after every test
        FreeCAD.closeDocument(self.document.Name)

    # ********************************************************************************************
    def test_00print(
        self
    ):
        # since method name starts with 00 this will be run first
        # this test just prints a line with stars

        fcc_print("\n{0}\n{1} run FEM TestObjectCreate tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            58 * "*"
        ))

    # ********************************************************************************************
    def test_femobjects_make(
        self
    ):
        doc = create_all_fem_objects_doc(self.document)

        # count the def make in ObjectsFem module
        # if FEM VTK post processing is disabled, we are not able to create VTK post objects
        if "BUILD_FEM_VTK" in FreeCAD.__cmake__:
            count_defmake = testtools.get_defmake_count()
        else:
            count_defmake = testtools.get_defmake_count(False)
        # TODO if the children are added to the analysis, they show up twice on Tree
        # thus they are not added to the analysis group ATM
        # https://forum.freecad.org/viewtopic.php?t=25283
        # thus they should not be counted
        # solver children: equations --> 9
        # gmsh mesh children: group, region, boundary layer --> 3
        # result children: mesh result --> 1
        # post pipeline children: region, scalar, cut, wrap --> 5
        # analysis itself is not in analysis group --> 1
        # thus: -19

        self.assertEqual(len(doc.Analysis.Group), count_defmake - 19)
        self.assertEqual(len(doc.Objects), count_defmake)

        fcc_print("doc objects count: {}, method: {}".format(
            len(doc.Objects),
            sys._getframe().f_code.co_name)
        )

        # save the file
        save_fc_file = join(
            testtools.get_fem_test_tmp_dir("objects_create_all"),
            "all_objects.FCStd"
        )
        fcc_print(
            "Save FreeCAD all objects file to {} ..."
            .format(save_fc_file)
        )
        self.document.saveAs(save_fc_file)


# ************************************************************************************************
# ************************************************************************************************
class TestObjectType(unittest.TestCase):
    fcc_print("import TestObjectType")

    # ********************************************************************************************
    def setUp(
        self
    ):
        # setUp is executed before every test

        # new document
        self.document = FreeCAD.newDocument(self.__class__.__name__)

    # ********************************************************************************************
    def tearDown(
        self
    ):
        # tearDown is executed after every test
        FreeCAD.closeDocument(self.document.Name)

    # ********************************************************************************************
    def test_00print(
        self
    ):
        # since method name starts with 00 this will be run first
        # this test just prints a line with stars

        fcc_print("\n{0}\n{1} run FEM TestObjectType tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            60 * "*"
        ))

    # ********************************************************************************************
    def test_femobjects_type(
        self
    ):
        doc = self.document
        create_all_fem_objects_doc

        from femtools.femutils import type_of_obj
        self.assertEqual(
            "Fem::FemAnalysis",
            type_of_obj(ObjectsFem.makeAnalysis(doc))
        )
        self.assertEqual(
            "Fem::ConstantVacuumPermittivity",
            type_of_obj(ObjectsFem.makeConstantVacuumPermittivity(doc))
        )
        self.assertEqual(
            "Fem::ConstraintBearing",
            type_of_obj(ObjectsFem.makeConstraintBearing(doc))
        )
        self.assertEqual(
            "Fem::ConstraintBodyHeatSource",
            type_of_obj(ObjectsFem.makeConstraintBodyHeatSource(doc))
        )
        self.assertEqual(
            "Fem::ConstraintContact",
            type_of_obj(ObjectsFem.makeConstraintContact(doc))
        )
        self.assertEqual(
            "Fem::ConstraintCurrentDensity",
            type_of_obj(ObjectsFem.makeConstraintCurrentDensity(doc))
        )
        self.assertEqual(
            "Fem::ConstraintDisplacement",
            type_of_obj(ObjectsFem.makeConstraintDisplacement(doc))
        )
        self.assertEqual(
            "Fem::ConstraintElectrostaticPotential",
            type_of_obj(ObjectsFem.makeConstraintElectrostaticPotential(doc))
        )
        self.assertEqual(
            "Fem::ConstraintFixed",
            type_of_obj(ObjectsFem.makeConstraintFixed(doc))
        )
        self.assertEqual(
            "Fem::ConstraintFlowVelocity",
            type_of_obj(ObjectsFem.makeConstraintFlowVelocity(doc))
        )
        self.assertEqual(
            "Fem::ConstraintFluidBoundary",
            type_of_obj(ObjectsFem.makeConstraintFluidBoundary(doc))
        )
        self.assertEqual(
            "Fem::ConstraintSpring",
            type_of_obj(ObjectsFem.makeConstraintSpring(doc))
        )
        self.assertEqual(
            "Fem::ConstraintForce",
            type_of_obj(ObjectsFem.makeConstraintForce(doc))
        )
        self.assertEqual(
            "Fem::ConstraintGear",
            type_of_obj(ObjectsFem.makeConstraintGear(doc))
        )
        self.assertEqual(
            "Fem::ConstraintHeatflux",
            type_of_obj(ObjectsFem.makeConstraintHeatflux(doc))
        )
        self.assertEqual(
            "Fem::ConstraintInitialFlowVelocity",
            type_of_obj(ObjectsFem.makeConstraintInitialFlowVelocity(doc))
        )
        self.assertEqual(
            "Fem::ConstraintInitialPressure",
            type_of_obj(ObjectsFem.makeConstraintInitialPressure(doc))
        )
        self.assertEqual(
            "Fem::ConstraintInitialTemperature",
            type_of_obj(ObjectsFem.makeConstraintInitialTemperature(doc))
        )
        self.assertEqual(
            "Fem::ConstraintMagnetization",
            type_of_obj(ObjectsFem.makeConstraintMagnetization(doc))
        )
        self.assertEqual(
            "Fem::ConstraintPlaneRotation",
            type_of_obj(ObjectsFem.makeConstraintPlaneRotation(doc))
        )
        self.assertEqual(
            "Fem::ConstraintPressure",
            type_of_obj(ObjectsFem.makeConstraintPressure(doc))
        )
        self.assertEqual(
            "Fem::ConstraintPulley",
            type_of_obj(ObjectsFem.makeConstraintPulley(doc))
        )
        self.assertEqual(
            "Fem::ConstraintSectionPrint",
            type_of_obj(ObjectsFem.makeConstraintSectionPrint(doc))
        )
        self.assertEqual(
            "Fem::ConstraintSelfWeight",
            type_of_obj(ObjectsFem.makeConstraintSelfWeight(doc))
        )
        self.assertEqual(
            "Fem::ConstraintCentrif",
            type_of_obj(ObjectsFem.makeConstraintCentrif(doc))
        )
        self.assertEqual(
            "Fem::ConstraintTemperature",
            type_of_obj(ObjectsFem.makeConstraintTemperature(doc))
        )
        self.assertEqual(
            "Fem::ConstraintTie",
            type_of_obj(ObjectsFem.makeConstraintTie(doc))
        )
        self.assertEqual(
            "Fem::ConstraintTransform",
            type_of_obj(ObjectsFem.makeConstraintTransform(doc))
        )
        self.assertEqual(
            "Fem::ElementFluid1D",
            type_of_obj(ObjectsFem.makeElementFluid1D(doc))
        )
        self.assertEqual(
            "Fem::ElementGeometry1D",
            type_of_obj(ObjectsFem.makeElementGeometry1D(doc))
        )
        self.assertEqual(
            "Fem::ElementGeometry2D",
            type_of_obj(ObjectsFem.makeElementGeometry2D(doc))
        )
        self.assertEqual(
            "Fem::ElementRotation1D",
            type_of_obj(ObjectsFem.makeElementRotation1D(doc))
        )
        materialsolid = ObjectsFem.makeMaterialSolid(doc)
        self.assertEqual(
            "Fem::MaterialCommon",
            type_of_obj(ObjectsFem.makeMaterialFluid(doc))
        )
        self.assertEqual(
            "Fem::MaterialCommon",
            type_of_obj(materialsolid))
        self.assertEqual(
            "Fem::MaterialMechanicalNonlinear",
            type_of_obj(ObjectsFem.makeMaterialMechanicalNonlinear(doc, materialsolid))
        )
        self.assertEqual(
            "Fem::MaterialReinforced",
            type_of_obj(ObjectsFem.makeMaterialReinforced(doc))
        )
        mesh = ObjectsFem.makeMeshGmsh(doc)
        self.assertEqual(
            "Fem::FemMeshGmsh",
            type_of_obj(mesh))
        self.assertEqual(
            "Fem::MeshBoundaryLayer",
            type_of_obj(ObjectsFem.makeMeshBoundaryLayer(doc, mesh))
        )
        self.assertEqual(
            "Fem::MeshGroup",
            type_of_obj(ObjectsFem.makeMeshGroup(doc, mesh))
        )
        self.assertEqual(
            "Fem::MeshRegion",
            type_of_obj(ObjectsFem.makeMeshRegion(doc, mesh))
        )
        self.assertEqual(
            "Fem::FemMeshShapeNetgenObject",
            type_of_obj(ObjectsFem.makeMeshNetgen(doc))
        )
        self.assertEqual(
            "Fem::MeshResult",
            type_of_obj(ObjectsFem.makeMeshResult(doc))
        )
        self.assertEqual(
            "Fem::ResultMechanical",
            type_of_obj(ObjectsFem.makeResultMechanical(doc))
        )
        solverelmer = ObjectsFem.makeSolverElmer(doc)
        self.assertEqual(
            "Fem::SolverCcxTools",
            type_of_obj(ObjectsFem.makeSolverCalculixCcxTools(doc))
        )
        self.assertEqual(
            "Fem::SolverCalculix",
            type_of_obj(ObjectsFem.makeSolverCalculix(doc))
        )
        self.assertEqual(
            "Fem::SolverElmer",
            type_of_obj(solverelmer)
        )
        self.assertEqual(
            "Fem::SolverMystran",
            type_of_obj(ObjectsFem.makeSolverMystran(doc))
        )
        self.assertEqual(
            "Fem::SolverZ88",
            type_of_obj(ObjectsFem.makeSolverZ88(doc))
        )
        self.assertEqual(
            "Fem::EquationElmerDeformation",
            type_of_obj(ObjectsFem.makeEquationDeformation(doc, solverelmer))
        )
        self.assertEqual(
            "Fem::EquationElmerElasticity",
            type_of_obj(ObjectsFem.makeEquationElasticity(doc, solverelmer))
        )
        self.assertEqual(
            "Fem::EquationElmerElectricforce",
            type_of_obj(ObjectsFem.makeEquationElectricforce(doc, solverelmer))
        )
        self.assertEqual(
            "Fem::EquationElmerElectrostatic",
            type_of_obj(ObjectsFem.makeEquationElectrostatic(doc, solverelmer))
        )
        self.assertEqual(
            "Fem::EquationElmerFlow",
            type_of_obj(ObjectsFem.makeEquationFlow(doc, solverelmer))
        )
        self.assertEqual(
            "Fem::EquationElmerFlux",
            type_of_obj(ObjectsFem.makeEquationFlux(doc, solverelmer))
        )
        self.assertEqual(
            "Fem::EquationElmerHeat",
            type_of_obj(ObjectsFem.makeEquationHeat(doc, solverelmer))
        )
        self.assertEqual(
            "Fem::EquationElmerMagnetodynamic2D",
            type_of_obj(ObjectsFem.makeEquationMagnetodynamic2D(doc, solverelmer))
        )
        self.assertEqual(
            "Fem::EquationElmerMagnetodynamic",
            type_of_obj(ObjectsFem.makeEquationMagnetodynamic(doc, solverelmer))
        )

        fcc_print("doc objects count: {}, method: {}".format(
            len(doc.Objects),
            sys._getframe().f_code.co_name)
        )
        # TODO: vtk post objs, thus 5 obj less than test_femobjects_make
        self.assertEqual(len(doc.Objects), testtools.get_defmake_count(False))
        # TODO: use different type for material fluid and material solid

    # ********************************************************************************************
    def test_femobjects_isoftype(
        self
    ):
        doc = self.document

        from femtools.femutils import is_of_type
        self.assertTrue(is_of_type(
            ObjectsFem.makeAnalysis(doc),
            "Fem::FemAnalysis"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstantVacuumPermittivity(doc),
            "Fem::ConstantVacuumPermittivity"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintBearing(doc),
            "Fem::ConstraintBearing"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintBodyHeatSource(doc),
            "Fem::ConstraintBodyHeatSource"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintContact(doc),
            "Fem::ConstraintContact"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintCurrentDensity(doc),
            "Fem::ConstraintCurrentDensity"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintDisplacement(doc),
            "Fem::ConstraintDisplacement"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintElectrostaticPotential(doc),
            "Fem::ConstraintElectrostaticPotential"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintFixed(doc),
            "Fem::ConstraintFixed"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintFlowVelocity(doc),
            "Fem::ConstraintFlowVelocity"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintFluidBoundary(doc),
            "Fem::ConstraintFluidBoundary"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintMagnetization(doc),
            "Fem::ConstraintMagnetization"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintSpring(doc),
            "Fem::ConstraintSpring"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintForce(doc),
            "Fem::ConstraintForce"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintGear(doc),
            "Fem::ConstraintGear"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintHeatflux(doc),
            "Fem::ConstraintHeatflux"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintInitialFlowVelocity(doc),
            "Fem::ConstraintInitialFlowVelocity"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintInitialPressure(doc),
            "Fem::ConstraintInitialPressure"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintInitialTemperature(doc),
            "Fem::ConstraintInitialTemperature"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintPlaneRotation(doc),
            "Fem::ConstraintPlaneRotation"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintPressure(doc),
            "Fem::ConstraintPressure"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintPulley(doc),
            "Fem::ConstraintPulley"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintSectionPrint(doc),
            "Fem::ConstraintSectionPrint"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintSelfWeight(doc),
            "Fem::ConstraintSelfWeight"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintCentrif(doc),
            "Fem::ConstraintCentrif"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintTemperature(doc),
            "Fem::ConstraintTemperature"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintTie(doc),
            "Fem::ConstraintTie"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeConstraintTransform(doc),
            "Fem::ConstraintTransform"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeElementFluid1D(doc),
            "Fem::ElementFluid1D"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeElementGeometry1D(doc),
            "Fem::ElementGeometry1D"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeElementGeometry2D(doc),
            "Fem::ElementGeometry2D"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeElementRotation1D(doc),
            "Fem::ElementRotation1D"
        ))
        materialsolid = ObjectsFem.makeMaterialSolid(doc)
        self.assertTrue(is_of_type(
            ObjectsFem.makeMaterialFluid(doc),
            "Fem::MaterialCommon"
        ))
        self.assertTrue(is_of_type(
            materialsolid,
            "Fem::MaterialCommon"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeMaterialMechanicalNonlinear(doc, materialsolid),
            "Fem::MaterialMechanicalNonlinear"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeMaterialReinforced(doc),
            "Fem::MaterialReinforced"
        ))
        mesh = ObjectsFem.makeMeshGmsh(doc)
        self.assertTrue(is_of_type(
            mesh,
            "Fem::FemMeshGmsh"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeMeshBoundaryLayer(doc, mesh),
            "Fem::MeshBoundaryLayer"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeMeshGroup(doc, mesh),
            "Fem::MeshGroup"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeMeshRegion(doc, mesh),
            "Fem::MeshRegion"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeMeshNetgen(doc),
            "Fem::FemMeshShapeNetgenObject"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeMeshResult(doc),
            "Fem::MeshResult"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeResultMechanical(doc),
            "Fem::ResultMechanical"
        ))
        solverelmer = ObjectsFem.makeSolverElmer(doc)
        self.assertTrue(is_of_type(
            ObjectsFem.makeSolverCalculixCcxTools(doc),
            "Fem::SolverCcxTools"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeSolverCalculix(doc),
            "Fem::SolverCalculix"
        ))
        self.assertTrue(is_of_type(
            solverelmer,
            "Fem::SolverElmer"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeSolverMystran(doc),
            "Fem::SolverMystran"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeSolverZ88(doc),
            "Fem::SolverZ88"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeEquationDeformation(doc, solverelmer),
            "Fem::EquationElmerDeformation"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeEquationElasticity(doc, solverelmer),
            "Fem::EquationElmerElasticity"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeEquationElectricforce(doc, solverelmer),
            "Fem::EquationElmerElectricforce"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeEquationElectrostatic(doc, solverelmer),
            "Fem::EquationElmerElectrostatic"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeEquationFlow(doc, solverelmer),
            "Fem::EquationElmerFlow"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeEquationFlux(doc, solverelmer),
            "Fem::EquationElmerFlux"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeEquationHeat(doc, solverelmer),
            "Fem::EquationElmerHeat"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeEquationMagnetodynamic2D(doc, solverelmer),
            "Fem::EquationElmerMagnetodynamic2D"
        ))
        self.assertTrue(is_of_type(
            ObjectsFem.makeEquationMagnetodynamic(doc, solverelmer),
            "Fem::EquationElmerMagnetodynamic"
        ))

        fcc_print("doc objects count: {}, method: {}".format(
            len(doc.Objects),
            sys._getframe().f_code.co_name)
        )
        # TODO: vtk post objs, thus 5 obj less than test_femobjects_make
        self.assertEqual(len(doc.Objects), testtools.get_defmake_count(False))

    # ********************************************************************************************
    def test_femobjects_derivedfromfem(
        self
    ):
        # try to add all possible True types from inheritance chain see
        # https://forum.freecad.org/viewtopic.php?f=10&t=32625
        doc = self.document

        from femtools.femutils import is_derived_from

        # FemAnalysis
        analysis = ObjectsFem.makeAnalysis(doc)
        self.assertTrue(is_derived_from(
            analysis,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            analysis,
            "Fem::FemAnalysis"
        ))

        # ConstantVacuumPermittivity
        constant_vacuumpermittivity = ObjectsFem.makeConstantVacuumPermittivity(doc)
        self.assertTrue(is_derived_from(
            constant_vacuumpermittivity,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constant_vacuumpermittivity,
            "Fem::ConstraintPython"
        ))
        self.assertTrue(is_derived_from(
            constant_vacuumpermittivity,
            "Fem::ConstantVacuumPermittivity"
        ))

        # ConstraintBearing
        constraint_bearing = ObjectsFem.makeConstraintBearing(doc)
        self.assertTrue(is_derived_from(
            constraint_bearing,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_bearing,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_bearing,
            "Fem::ConstraintBearing"
        ))

        # ConstraintBodyHeatSource
        constraint_body_heat_source = ObjectsFem.makeConstraintBodyHeatSource(doc)
        self.assertTrue(is_derived_from(
            constraint_body_heat_source,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_body_heat_source,
            "Fem::ConstraintPython"
        ))
        self.assertTrue(is_derived_from(
            constraint_body_heat_source,
            "Fem::ConstraintBodyHeatSource"
        ))

        # ConstraintContact
        constraint_contact = ObjectsFem.makeConstraintContact(doc)
        self.assertTrue(is_derived_from(
            constraint_contact,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_contact,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_contact,
            "Fem::ConstraintContact"
        ))

        # ConstraintCurrentDensity
        constraint_currentdensity = ObjectsFem.makeConstraintCurrentDensity(doc)
        self.assertTrue(is_derived_from(
            constraint_currentdensity,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_currentdensity,
            "Fem::ConstraintPython"
        ))
        self.assertTrue(is_derived_from(
            constraint_currentdensity,
            "Fem::ConstraintCurrentDensity"
        ))

        # ConstraintDisplacement
        constraint_displacement = ObjectsFem.makeConstraintDisplacement(doc)
        self.assertTrue(is_derived_from(
            constraint_displacement,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_displacement,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_displacement,
            "Fem::ConstraintDisplacement"
        ))

        # ConstraintElectrostaticPotential
        constraint_electorstatic_potential = ObjectsFem.makeConstraintElectrostaticPotential(doc)
        self.assertTrue(is_derived_from(
            constraint_electorstatic_potential,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_electorstatic_potential,
            "Fem::ConstraintPython"
        ))
        self.assertTrue(is_derived_from(
            constraint_electorstatic_potential,
            "Fem::ConstraintElectrostaticPotential"
        ))

        # ConstraintFixed
        constraint_fixed = ObjectsFem.makeConstraintFixed(doc)
        self.assertTrue(is_derived_from(
            constraint_fixed,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_fixed,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_fixed,
            "Fem::ConstraintFixed"
        ))

        # ConstraintFlowVelocity
        constraint_flow_velocity = ObjectsFem.makeConstraintFlowVelocity(doc)
        self.assertTrue(is_derived_from(
            constraint_flow_velocity,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_flow_velocity,
            "Fem::ConstraintPython"
        ))
        self.assertTrue(is_derived_from(
            constraint_flow_velocity,
            "Fem::ConstraintFlowVelocity"
        ))

        # ConstraintFluidBoundary
        constraint_fluid_boundary = ObjectsFem.makeConstraintFluidBoundary(doc)
        self.assertTrue(is_derived_from(
            constraint_fluid_boundary,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_fluid_boundary,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_fluid_boundary,
            "Fem::ConstraintFluidBoundary"
        ))

        # ConstraintMagnetization
        constraint_magnetization = ObjectsFem.makeConstraintMagnetization(doc)
        self.assertTrue(is_derived_from(
            constraint_magnetization,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_magnetization,
            "Fem::ConstraintPython"
        ))
        self.assertTrue(is_derived_from(
            constraint_magnetization,
            "Fem::ConstraintMagnetization"
        ))

        # ConstraintSpring
        constraint_spring = ObjectsFem.makeConstraintSpring(doc)
        self.assertTrue(is_derived_from(
            constraint_spring,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_spring,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_spring,
            "Fem::ConstraintSpring"
        ))

        # ConstraintForce
        constraint_force = ObjectsFem.makeConstraintForce(doc)
        self.assertTrue(is_derived_from(
            constraint_force,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_force,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_force,
            "Fem::ConstraintForce"
        ))

        # ConstraintGear
        constraint_gear = ObjectsFem.makeConstraintGear(doc)
        self.assertTrue(is_derived_from(
            constraint_gear,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_gear,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_gear,
            "Fem::ConstraintGear"
        ))

        # ConstraintHeatflux
        constraint_heat_flux = ObjectsFem.makeConstraintHeatflux(doc)
        self.assertTrue(is_derived_from(
            constraint_heat_flux,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_heat_flux,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_heat_flux,
            "Fem::ConstraintHeatflux"
        ))

        # ConstraintInitialFlowVelocity
        constraint_initial_flow_velocity = ObjectsFem.makeConstraintInitialFlowVelocity(doc)
        self.assertTrue(is_derived_from(
            constraint_initial_flow_velocity,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_initial_flow_velocity,
            "Fem::ConstraintPython"
        ))
        self.assertTrue(is_derived_from(
            constraint_initial_flow_velocity,
            "Fem::ConstraintInitialFlowVelocity"
        ))

        # ConstraintInitialPressure
        constraint_initial_pressure = ObjectsFem.makeConstraintInitialPressure(doc)
        self.assertTrue(is_derived_from(
            constraint_initial_pressure,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_initial_pressure,
            "Fem::ConstraintPython"
        ))
        self.assertTrue(is_derived_from(
            constraint_initial_pressure,
            "Fem::ConstraintInitialPressure"
        ))

        # ConstraintInitialTemperature
        constraint_initial_temperature = ObjectsFem.makeConstraintInitialTemperature(doc)
        self.assertTrue(is_derived_from(
            constraint_initial_temperature,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_initial_temperature,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_initial_temperature,
            "Fem::ConstraintInitialTemperature"
        ))

        # ConstraintPlaneRotation
        constraint_plane_rotation = ObjectsFem.makeConstraintPlaneRotation(doc)
        self.assertTrue(is_derived_from(
            constraint_plane_rotation,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_plane_rotation,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_plane_rotation,
            "Fem::ConstraintPlaneRotation"
        ))

        # ConstraintPressure
        constraint_pressure = ObjectsFem.makeConstraintPressure(doc)
        self.assertTrue(is_derived_from(
            constraint_pressure,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_pressure,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_pressure,
            "Fem::ConstraintPressure"
        ))

        # ConstraintPulley
        constraint_pulley = ObjectsFem.makeConstraintPulley(doc)
        self.assertTrue(is_derived_from(
            constraint_pulley,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_pulley,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_pulley,
            "Fem::ConstraintPulley"
        ))

        # ConstraintSectionPrint
        constraint_self_weight = ObjectsFem.makeConstraintSectionPrint(doc)
        self.assertTrue(is_derived_from(
            constraint_self_weight,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_self_weight,
            "Fem::ConstraintPython"
        ))
        self.assertTrue(is_derived_from(
            constraint_self_weight,
            "Fem::ConstraintSectionPrint"
        ))

        # ConstraintSelfWeight
        constraint_self_weight = ObjectsFem.makeConstraintSelfWeight(doc)
        self.assertTrue(is_derived_from(
            constraint_self_weight,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_self_weight,
            "Fem::ConstraintPython"
        ))
        self.assertTrue(is_derived_from(
            constraint_self_weight,
            "Fem::ConstraintSelfWeight"
        ))

        # ConstraintCentrif
        constraint_centrif = ObjectsFem.makeConstraintCentrif(doc)
        self.assertTrue(is_derived_from(
            constraint_centrif,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_centrif,
            "Fem::ConstraintPython"
        ))
        self.assertTrue(is_derived_from(
            constraint_centrif,
            "Fem::ConstraintCentrif"
        ))

        # ConstraintTemperature
        constraint_temperature = ObjectsFem.makeConstraintTemperature(doc)
        self.assertTrue(is_derived_from(
            constraint_temperature,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_temperature,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_temperature,
            "Fem::ConstraintTemperature"
        ))

        # ConstraintTie
        constraint_tie = ObjectsFem.makeConstraintTie(doc)
        self.assertTrue(is_derived_from(
            constraint_tie,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_tie,
            "Fem::Constraint"
        ))
        self.assertTrue(is_derived_from(
            constraint_tie,
            "Fem::ConstraintPython"
        ))
        self.assertTrue(is_derived_from(
            constraint_tie,
            "Fem::ConstraintTie"
        ))

        # ConstraintTransform
        constraint_transform = ObjectsFem.makeConstraintTransform(doc)
        self.assertTrue(is_derived_from(
            constraint_transform,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            constraint_transform,
            "Fem::ConstraintTransform"
        ))

        # ElementFluid1D
        fluid1d = ObjectsFem.makeElementFluid1D(doc)
        self.assertTrue(is_derived_from(
            fluid1d,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            fluid1d,
            "Fem::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            fluid1d,
            "Fem::ElementFluid1D"
        ))

        # ElementGeometry1D
        geometry1d = ObjectsFem.makeElementGeometry1D(doc)
        self.assertTrue(is_derived_from(
            geometry1d,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            geometry1d,
            "Fem::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            geometry1d,
            "Fem::ElementGeometry1D"
        ))

        # ElementGeometry2D
        geometry2d = ObjectsFem.makeElementGeometry2D(doc)
        self.assertTrue(is_derived_from(
            geometry2d,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            geometry2d,
            "Fem::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            geometry2d,
            "Fem::ElementGeometry2D"
        ))

        # ElementRotation1D
        rotation1d = ObjectsFem.makeElementRotation1D(doc)
        self.assertTrue(is_derived_from(
            rotation1d,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            rotation1d,
            "Fem::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            rotation1d,
            "Fem::ElementRotation1D"
        ))

        # Material Fluid
        material_fluid = ObjectsFem.makeMaterialFluid(doc)
        self.assertTrue(is_derived_from(
            material_fluid,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            material_fluid,
            "App::MaterialObjectPython"
        ))
        self.assertTrue(is_derived_from(
            material_fluid,
            "Fem::MaterialCommon"
        ))

        # Material Solid
        material_solid = ObjectsFem.makeMaterialSolid(doc)
        self.assertTrue(is_derived_from(
            material_solid,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            material_solid,
            "App::MaterialObjectPython"
        ))
        self.assertTrue(is_derived_from(
            material_solid,
            "Fem::MaterialCommon"
        ))

        # MaterialMechanicalNonlinear
        material_nonlinear = ObjectsFem.makeMaterialMechanicalNonlinear(doc, material_solid)
        self.assertTrue(is_derived_from(
            material_nonlinear,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            material_nonlinear,
            "Fem::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            material_nonlinear,
            "Fem::MaterialMechanicalNonlinear"
        ))

        # MaterialReinforced
        material_reinforced = ObjectsFem.makeMaterialReinforced(doc)
        self.assertTrue(is_derived_from(
            material_reinforced,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            material_reinforced,
            "App::MaterialObjectPython"
        ))
        self.assertTrue(is_derived_from(
            material_reinforced,
            "Fem::MaterialReinforced"
        ))

        # FemMeshGmsh
        mesh_gmsh = ObjectsFem.makeMeshGmsh(doc)
        self.assertTrue(is_derived_from(
            mesh_gmsh,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            mesh_gmsh,
            "Fem::FemMeshObjectPython"
        ))
        self.assertTrue(is_derived_from(
            mesh_gmsh,
            "Fem::FemMeshGmsh"
        ))

        # MeshBoundaryLayer
        mesh_boundarylayer = ObjectsFem.makeMeshBoundaryLayer(doc, mesh_gmsh)
        self.assertTrue(is_derived_from(
            mesh_boundarylayer,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            mesh_boundarylayer,
            "Fem::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            mesh_boundarylayer,
            "Fem::MeshBoundaryLayer"
        ))

        # MeshGroup
        mesh_group = ObjectsFem.makeMeshGroup(doc, mesh_gmsh)
        self.assertTrue(is_derived_from(
            mesh_group,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            mesh_group,
            "Fem::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            mesh_group,
            "Fem::MeshGroup"
        ))

        # MeshRegion
        mesh_region = ObjectsFem.makeMeshRegion(doc, mesh_gmsh)
        self.assertTrue(is_derived_from(
            mesh_region,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            mesh_region,
            "Fem::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            mesh_region,
            "Fem::MeshRegion"
        ))

        # FemMeshShapeNetgenObject
        mesh_netgen = ObjectsFem.makeMeshNetgen(doc)
        self.assertTrue(is_derived_from(
            mesh_netgen,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            mesh_netgen,
            "Fem::FemMeshShapeNetgenObject"
        ))

        # MeshResult
        mesh_result = ObjectsFem.makeMeshResult(doc)
        self.assertTrue(is_derived_from(
            mesh_result,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            mesh_result,
            "Fem::FemMeshObjectPython"
        ))
        self.assertTrue(is_derived_from(
            mesh_result,
            "Fem::MeshResult"
        ))

        # ResultMechanical
        result_mechanical = ObjectsFem.makeResultMechanical(doc)
        self.assertTrue(is_derived_from(
            result_mechanical,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            result_mechanical,
            "Fem::FemResultObjectPython"
        ))
        self.assertTrue(is_derived_from(
            result_mechanical,
            "Fem::ResultMechanical"
        ))

        # SolverCcxTools
        solver_ccxtools = ObjectsFem.makeSolverCalculixCcxTools(doc)
        self.assertTrue(is_derived_from(
            solver_ccxtools,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            solver_ccxtools,
            "Fem::FemSolverObject"
        ))
        self.assertTrue(is_derived_from(
            solver_ccxtools,
            "Fem::FemSolverObjectPython"
        ))
        self.assertTrue(is_derived_from(
            solver_ccxtools,
            "Fem::SolverCcxTools"
        ))

        # SolverCalculix
        solver_calculix = ObjectsFem.makeSolverCalculix(doc)
        self.assertTrue(is_derived_from(
            solver_calculix,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            solver_calculix,
            "Fem::FemSolverObject"
        ))
        self.assertTrue(is_derived_from(
            solver_calculix,
            "Fem::FemSolverObjectPython"
        ))
        self.assertTrue(is_derived_from(
            solver_calculix,
            "Fem::SolverCalculix"
        ))

        # SolverElmer
        solver_elmer = ObjectsFem.makeSolverElmer(doc)
        self.assertTrue(is_derived_from(
            solver_elmer,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            solver_elmer,
            "Fem::FemSolverObject"
        ))
        self.assertTrue(is_derived_from(
            solver_elmer,
            "Fem::FemSolverObjectPython"
        ))
        self.assertTrue(is_derived_from(
            solver_elmer,
            "Fem::SolverElmer"
        ))

        # SolverMystran
        solver_mystran = ObjectsFem.makeSolverMystran(doc)
        self.assertTrue(is_derived_from(
            solver_mystran,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            solver_mystran,
            "Fem::FemSolverObject"
        ))
        self.assertTrue(is_derived_from(
            solver_mystran,
            "Fem::FemSolverObjectPython"
        ))
        self.assertTrue(is_derived_from(
            solver_mystran,
            "Fem::SolverMystran"
        ))

        # SolverZ88
        solver_z88 = ObjectsFem.makeSolverZ88(doc)
        self.assertTrue(is_derived_from(
            solver_z88,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            solver_z88,
            "Fem::FemSolverObject"
        ))
        self.assertTrue(is_derived_from(
            solver_z88,
            "Fem::FemSolverObjectPython"
        ))
        self.assertTrue(is_derived_from(
            solver_z88,
            "Fem::SolverZ88"
        ))

        # EquationElmerDeformation
        equation_deformation = ObjectsFem.makeEquationDeformation(doc, solver_elmer)
        self.assertTrue(is_derived_from(
            equation_deformation,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            equation_deformation,
            "App::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            equation_deformation,
            "Fem::EquationElmerDeformation"
        ))

        # EquationElmerElasticity
        equation_elasticity = ObjectsFem.makeEquationElasticity(doc, solver_elmer)
        self.assertTrue(is_derived_from(
            equation_elasticity,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            equation_elasticity,
            "App::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            equation_elasticity,
            "Fem::EquationElmerElasticity"
        ))

        # EquationElmerElectricforce
        equation_elasticity = ObjectsFem.makeEquationElectricforce(doc, solver_elmer)
        self.assertTrue(is_derived_from(
            equation_elasticity,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            equation_elasticity,
            "App::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            equation_elasticity,
            "Fem::EquationElmerElectricforce"
        ))

        # EquationElmerElectrostatic
        equation_electrostatic = ObjectsFem.makeEquationElectrostatic(doc, solver_elmer)
        self.assertTrue(is_derived_from(
            equation_electrostatic,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            equation_electrostatic,
            "App::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            equation_electrostatic,
            "Fem::EquationElmerElectrostatic"
        ))

        # EquationElmerFlow
        equation_flow = ObjectsFem.makeEquationFlow(doc, solver_elmer)
        self.assertTrue(is_derived_from(
            equation_flow,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            equation_flow,
            "App::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            equation_flow,
            "Fem::EquationElmerFlow"
        ))

        # EquationElmerFlux
        equation_flux = ObjectsFem.makeEquationFlux(doc, solver_elmer)
        self.assertTrue(is_derived_from(
            equation_flux,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            equation_flux,
            "App::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            equation_flux,
            "Fem::EquationElmerFlux"
        ))

        # EquationElmerHeat
        equation_heat = ObjectsFem.makeEquationHeat(doc, solver_elmer)
        self.assertTrue(is_derived_from(
            equation_heat,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            equation_heat,
            "App::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            equation_heat,
            "Fem::EquationElmerHeat"
        ))

        # EquationElmerMagnetodynamic2D
        equation_magnetodynamic2D = ObjectsFem.makeEquationMagnetodynamic2D(doc, solver_elmer)
        self.assertTrue(is_derived_from(
            equation_magnetodynamic2D,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            equation_magnetodynamic2D,
            "App::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            equation_magnetodynamic2D,
            "Fem::EquationElmerMagnetodynamic2D"
        ))

        # EquationElmerMagnetodynamic
        equation_magnetodynamic = ObjectsFem.makeEquationMagnetodynamic(doc, solver_elmer)
        self.assertTrue(is_derived_from(
            equation_magnetodynamic,
            "App::DocumentObject"
        ))
        self.assertTrue(is_derived_from(
            equation_magnetodynamic,
            "App::FeaturePython"
        ))
        self.assertTrue(is_derived_from(
            equation_magnetodynamic,
            "Fem::EquationElmerMagnetodynamic"
        ))

        fcc_print("doc objects count: {}, method: {}".format(
            len(doc.Objects),
            sys._getframe().f_code.co_name)
        )
        # TODO: vtk post objs, thus 5 obj less than test_femobjects_make
        self.assertEqual(len(doc.Objects), testtools.get_defmake_count(False))
        # TODO constraint transform is not derived from "Fem::Constraint" ?!?

    # ********************************************************************************************
    def test_femobjects_derivedfromstd(
        self
    ):
        # only the last True type is used
        doc = self.document

        self.assertTrue(
            ObjectsFem.makeAnalysis(
                doc
            ).isDerivedFrom("Fem::FemAnalysis")
        )
        self.assertTrue(
            ObjectsFem.makeConstantVacuumPermittivity(
                doc
            ).isDerivedFrom("Fem::ConstraintPython")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintBearing(
                doc
            ).isDerivedFrom("Fem::ConstraintBearing")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintBodyHeatSource(
                doc
            ).isDerivedFrom("Fem::ConstraintPython")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintContact(
                doc
            ).isDerivedFrom("Fem::ConstraintContact")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintCurrentDensity(
                doc
            ).isDerivedFrom("Fem::ConstraintPython")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintDisplacement(
                doc
            ).isDerivedFrom("Fem::ConstraintDisplacement")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintElectrostaticPotential(
                doc
            ).isDerivedFrom("Fem::ConstraintPython")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintFixed(
                doc).isDerivedFrom("Fem::ConstraintFixed")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintFlowVelocity(
                doc).isDerivedFrom("Fem::ConstraintPython")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintFluidBoundary(
                doc
            ).isDerivedFrom("Fem::ConstraintFluidBoundary")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintMagnetization(
                doc
            ).isDerivedFrom("Fem::ConstraintPython")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintSpring(
                doc
            ).isDerivedFrom("Fem::ConstraintSpring")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintForce(
                doc
            ).isDerivedFrom("Fem::ConstraintForce")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintGear(
                doc
            ).isDerivedFrom("Fem::ConstraintGear")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintHeatflux(
                doc
            ).isDerivedFrom("Fem::ConstraintHeatflux")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintInitialFlowVelocity(
                doc
            ).isDerivedFrom("Fem::ConstraintPython")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintInitialPressure(
                doc
            ).isDerivedFrom("Fem::ConstraintPython")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintInitialTemperature(
                doc
            ).isDerivedFrom("Fem::ConstraintInitialTemperature")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintPlaneRotation(
                doc
            ).isDerivedFrom("Fem::ConstraintPlaneRotation")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintPressure(
                doc
            ).isDerivedFrom("Fem::ConstraintPressure")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintPulley(
                doc
            ).isDerivedFrom("Fem::ConstraintPulley")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintSectionPrint(
                doc
            ).isDerivedFrom("Fem::ConstraintPython")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintSelfWeight(
                doc
            ).isDerivedFrom("Fem::ConstraintPython")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintCentrif(
                doc
            ).isDerivedFrom("Fem::ConstraintPython")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintTemperature(
                doc
            ).isDerivedFrom("Fem::ConstraintTemperature")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintTie(
                doc
            ).isDerivedFrom("Fem::ConstraintPython")
        )
        self.assertTrue(
            ObjectsFem.makeConstraintTransform(
                doc).isDerivedFrom("Fem::ConstraintTransform")
        )
        self.assertTrue(
            ObjectsFem.makeElementFluid1D(
                doc
            ).isDerivedFrom("Fem::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeElementGeometry1D(
                doc
            ).isDerivedFrom("Fem::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeElementGeometry2D(
                doc
            ).isDerivedFrom("Fem::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeElementRotation1D(
                doc
            ).isDerivedFrom("Fem::FeaturePython")
        )
        materialsolid = ObjectsFem.makeMaterialSolid(doc)
        self.assertTrue(
            ObjectsFem.makeMaterialFluid(
                doc
            ).isDerivedFrom("App::MaterialObjectPython")
        )
        self.assertTrue(
            materialsolid.isDerivedFrom("App::MaterialObjectPython")
        )
        self.assertTrue(
            ObjectsFem.makeMaterialMechanicalNonlinear(
                doc,
                materialsolid
            ).isDerivedFrom("Fem::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeMaterialReinforced(
                doc
            ).isDerivedFrom("App::MaterialObjectPython")
        )
        mesh = ObjectsFem.makeMeshGmsh(doc)
        self.assertTrue(
            mesh.isDerivedFrom("Fem::FemMeshObjectPython")
        )
        self.assertTrue(
            ObjectsFem.makeMeshBoundaryLayer(
                doc,
                mesh
            ).isDerivedFrom("Fem::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeMeshGroup(
                doc,
                mesh
            ).isDerivedFrom("Fem::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeMeshRegion(
                doc,
                mesh
            ).isDerivedFrom("Fem::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeMeshNetgen(
                doc
            ).isDerivedFrom("Fem::FemMeshShapeNetgenObject")
        )
        self.assertTrue(
            ObjectsFem.makeMeshResult(
                doc
            ).isDerivedFrom("Fem::FemMeshObjectPython")
        )
        self.assertTrue(
            ObjectsFem.makeResultMechanical(
                doc
            ).isDerivedFrom("Fem::FemResultObjectPython")
        )
        solverelmer = ObjectsFem.makeSolverElmer(doc)
        self.assertTrue(
            ObjectsFem.makeSolverCalculixCcxTools(
                doc
            ).isDerivedFrom("Fem::FemSolverObjectPython")
        )
        self.assertTrue(
            ObjectsFem.makeSolverCalculix(
                doc
            ).isDerivedFrom("Fem::FemSolverObjectPython")
        )
        self.assertTrue(
            solverelmer.isDerivedFrom("Fem::FemSolverObjectPython")
        )
        self.assertTrue(
            ObjectsFem.makeSolverMystran(
                doc
            ).isDerivedFrom("Fem::FemSolverObjectPython")
        )
        self.assertTrue(
            ObjectsFem.makeSolverZ88(
                doc
            ).isDerivedFrom("Fem::FemSolverObjectPython")
        )
        self.assertTrue(
            ObjectsFem.makeEquationDeformation(
                doc,
                solverelmer
            ).isDerivedFrom("App::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeEquationElasticity(
                doc,
                solverelmer
            ).isDerivedFrom("App::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeEquationElectricforce(
                doc,
                solverelmer
            ).isDerivedFrom("App::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeEquationElectrostatic(
                doc,
                solverelmer
            ).isDerivedFrom("App::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeEquationFlow(
                doc,
                solverelmer
            ).isDerivedFrom("App::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeEquationFlux(
                doc,
                solverelmer
            ).isDerivedFrom("App::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeEquationHeat(
                doc,
                solverelmer
            ).isDerivedFrom("App::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeEquationMagnetodynamic2D(
                doc,
                solverelmer
            ).isDerivedFrom("App::FeaturePython")
        )
        self.assertTrue(
            ObjectsFem.makeEquationMagnetodynamic(
                doc,
                solverelmer
            ).isDerivedFrom("App::FeaturePython")
        )

        fcc_print("doc objects count: {}, method: {}".format(
            len(doc.Objects),
            sys._getframe().f_code.co_name)
        )
        # TODO: vtk post objs, thus 5 obj less than test_femobjects_make
        self.assertEqual(len(doc.Objects), testtools.get_defmake_count(False))


# helper
def create_all_fem_objects_doc(
    doc
):
    analysis = ObjectsFem.makeAnalysis(doc)

    analysis.addObject(ObjectsFem.makeConstantVacuumPermittivity(doc))
    analysis.addObject(ObjectsFem.makeConstraintBearing(doc))
    analysis.addObject(ObjectsFem.makeConstraintBodyHeatSource(doc))
    analysis.addObject(ObjectsFem.makeConstraintContact(doc))
    analysis.addObject(ObjectsFem.makeConstraintCurrentDensity(doc))
    analysis.addObject(ObjectsFem.makeConstraintDisplacement(doc))
    analysis.addObject(ObjectsFem.makeConstraintElectrostaticPotential(doc))
    analysis.addObject(ObjectsFem.makeConstraintFixed(doc))
    analysis.addObject(ObjectsFem.makeConstraintFlowVelocity(doc))
    analysis.addObject(ObjectsFem.makeConstraintFluidBoundary(doc))
    analysis.addObject(ObjectsFem.makeConstraintSpring(doc))
    analysis.addObject(ObjectsFem.makeConstraintForce(doc))
    analysis.addObject(ObjectsFem.makeConstraintGear(doc))
    analysis.addObject(ObjectsFem.makeConstraintHeatflux(doc))
    analysis.addObject(ObjectsFem.makeConstraintInitialFlowVelocity(doc))
    analysis.addObject(ObjectsFem.makeConstraintInitialPressure(doc))
    analysis.addObject(ObjectsFem.makeConstraintInitialTemperature(doc))
    analysis.addObject(ObjectsFem.makeConstraintMagnetization(doc))
    analysis.addObject(ObjectsFem.makeConstraintPlaneRotation(doc))
    analysis.addObject(ObjectsFem.makeConstraintPressure(doc))
    analysis.addObject(ObjectsFem.makeConstraintPulley(doc))
    analysis.addObject(ObjectsFem.makeConstraintSectionPrint(doc))
    analysis.addObject(ObjectsFem.makeConstraintSelfWeight(doc))
    analysis.addObject(ObjectsFem.makeConstraintCentrif(doc))
    analysis.addObject(ObjectsFem.makeConstraintTemperature(doc))
    analysis.addObject(ObjectsFem.makeConstraintTie(doc))
    analysis.addObject(ObjectsFem.makeConstraintTransform(doc))

    analysis.addObject(ObjectsFem.makeElementFluid1D(doc))
    analysis.addObject(ObjectsFem.makeElementGeometry1D(doc))
    analysis.addObject(ObjectsFem.makeElementGeometry2D(doc))
    analysis.addObject(ObjectsFem.makeElementRotation1D(doc))

    analysis.addObject(ObjectsFem.makeMaterialFluid(doc))
    mat = analysis.addObject(ObjectsFem.makeMaterialSolid(doc))[0]
    analysis.addObject(ObjectsFem.makeMaterialMechanicalNonlinear(doc, mat))
    analysis.addObject(ObjectsFem.makeMaterialReinforced(doc))

    msh = analysis.addObject(ObjectsFem.makeMeshGmsh(doc))[0]
    ObjectsFem.makeMeshBoundaryLayer(doc, msh)
    ObjectsFem.makeMeshGroup(doc, msh)
    ObjectsFem.makeMeshRegion(doc, msh)
    analysis.addObject(ObjectsFem.makeMeshNetgen(doc))
    rm = ObjectsFem.makeMeshResult(doc)

    res = analysis.addObject(ObjectsFem.makeResultMechanical(doc))[0]
    res.Mesh = rm
    if "BUILD_FEM_VTK" in FreeCAD.__cmake__:
        vres = analysis.addObject(ObjectsFem.makePostVtkResult(doc, res))[0]
        ObjectsFem.makePostVtkFilterClipRegion(doc, vres)
        ObjectsFem.makePostVtkFilterClipScalar(doc, vres)
        ObjectsFem.makePostVtkFilterContours(doc, vres)
        ObjectsFem.makePostVtkFilterCutFunction(doc, vres)
        ObjectsFem.makePostVtkFilterWarp(doc, vres)

    analysis.addObject(ObjectsFem.makeSolverCalculixCcxTools(doc))
    analysis.addObject(ObjectsFem.makeSolverCalculix(doc))
    sol = analysis.addObject(ObjectsFem.makeSolverElmer(doc))[0]
    analysis.addObject(ObjectsFem.makeSolverMystran(doc))
    analysis.addObject(ObjectsFem.makeSolverZ88(doc))

    ObjectsFem.makeEquationDeformation(doc, sol)
    ObjectsFem.makeEquationElasticity(doc, sol)
    ObjectsFem.makeEquationElectricforce(doc, sol)
    ObjectsFem.makeEquationElectrostatic(doc, sol)
    ObjectsFem.makeEquationFlow(doc, sol)
    ObjectsFem.makeEquationFlux(doc, sol)
    ObjectsFem.makeEquationHeat(doc, sol)
    ObjectsFem.makeEquationMagnetodynamic2D(doc, sol)
    ObjectsFem.makeEquationMagnetodynamic(doc, sol)

    doc.recompute()

    return doc
