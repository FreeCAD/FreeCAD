# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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
# ***************************************************************************

__title__ = "Open files FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import unittest
import tempfile
from os.path import join

import FreeCAD

from . import support_utils as testtools
from .support_utils import fcc_print


"""
# TODO: separate unit test:
# std document name of object ==  obj type
ATM:
for elmer equation obj: name != proxy type
material solid and material fluid obj: name != proxy type

# in addition for FeaturePythons
# std document name of object ==  the class name (for for femsolver obj)
# all femsolver objects class name is Proxy
"""


class TestObjectOpen(unittest.TestCase):
    fcc_print("import TestObjectOpen")

    # ********************************************************************************************
    def setUp(
        self
    ):
        # setUp is executed before every test
        doc_name = self.__class__.__name__
        self.document = FreeCAD.newDocument(doc_name)

        self.test_file_dir = join(
            testtools.get_fem_test_home_dir(),
            "open"
        )

    def test_00print(
        self
    ):
        fcc_print("\n{0}\n{1} run FEM TestObjectOpen tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            60 * "*"
        ))

    # ********************************************************************************************
    def test_femobjects_open_head(
        self
    ):
        # FreeCAD --run-test "femtest.app.test_object.TestObjectCreate.test_femobjects_make"
        fcc_print("load master head document objects")
        from .test_object import create_all_fem_objects_doc
        doc = create_all_fem_objects_doc(self.document)

        # todo save and load the document
        file_path = join(tempfile.gettempdir(), "all_objects_head.FCStd")
        doc.saveAs(file_path)
        self.document = FreeCAD.open(file_path)

        from femtools.femutils import type_of_obj

        # C++ objects
        self.assertEqual(
            "Fem::FemAnalysis",
            type_of_obj(doc.Analysis)
        )
        # TODO other C++ objects and view provider
        # Is just checking the type sufficient?
        # If there is a type there is at least a object with correct type ;-)

        # FeaturePythons
        # objects
        self.compare_feature_pythons_class_app(doc)
        # view provider
        self.compare_feature_pythons_class_gui(doc)

    # ********************************************************************************************
    def compare_feature_pythons_class_app(
        self,
        doc
    ):
        # see comments at file end, the code was created by some python code
        """
        # see code lines after comment block for the smarter version
        # but this makes it easy to understand what is happening
        self.assertEqual(
            "<class 'femobjects.constraint_bodyheatsource.ConstraintBodyHeatSource'>",
            str(doc.ConstraintBodyHeatSource.Proxy.__class__)
        )
        """
        from femobjects._FemConstraintBodyHeatSource import Proxy
        self.assertEqual(
            Proxy,
            doc.ConstraintBodyHeatSource.Proxy.__class__
        )

        from femobjects._FemConstraintElectrostaticPotential import Proxy
        self.assertEqual(
            Proxy,
            doc.ConstraintElectrostaticPotential.Proxy.__class__
        )

        from femobjects._FemConstraintFlowVelocity import Proxy
        self.assertEqual(
            Proxy,
            doc.ConstraintFlowVelocity.Proxy.__class__
        )

        from femobjects._FemConstraintInitialFlowVelocity import Proxy
        self.assertEqual(
            Proxy,
            doc.ConstraintInitialFlowVelocity.Proxy.__class__
        )

        from femobjects._FemConstraintSelfWeight import _FemConstraintSelfWeight
        self.assertEqual(
            _FemConstraintSelfWeight,
            doc.ConstraintSelfWeight.Proxy.__class__
        )

        from femobjects._FemConstraintTie import _FemConstraintTie
        self.assertEqual(
            _FemConstraintTie,
            doc.ConstraintTie.Proxy.__class__
        )

        from femobjects._FemElementFluid1D import _FemElementFluid1D
        self.assertEqual(
            _FemElementFluid1D,
            doc.ElementFluid1D.Proxy.__class__
        )

        from femobjects._FemElementGeometry1D import _FemElementGeometry1D
        self.assertEqual(
            _FemElementGeometry1D,
            doc.ElementGeometry1D.Proxy.__class__
        )

        from femobjects._FemElementGeometry2D import _FemElementGeometry2D
        self.assertEqual(
            _FemElementGeometry2D,
            doc.ElementGeometry2D.Proxy.__class__
        )

        from femobjects._FemElementRotation1D import _FemElementRotation1D
        self.assertEqual(
            _FemElementRotation1D,
            doc.ElementRotation1D.Proxy.__class__
        )

        from femobjects._FemMaterial import _FemMaterial
        self.assertEqual(
            _FemMaterial,
            doc.MechanicalSolidMaterial.Proxy.__class__
        )

        from femobjects._FemMaterial import _FemMaterial
        self.assertEqual(
            _FemMaterial,
            doc.FluidMaterial.Proxy.__class__
        )

        from femobjects._FemMaterialMechanicalNonlinear import _FemMaterialMechanicalNonlinear
        self.assertEqual(
            _FemMaterialMechanicalNonlinear,
            doc.MaterialMechanicalNonlinear.Proxy.__class__
        )

        from femobjects._FemMaterialReinforced import _FemMaterialReinforced
        self.assertEqual(
            _FemMaterialReinforced,
            doc.MaterialReinforced.Proxy.__class__
        )

        from femobjects._FemMeshGmsh import _FemMeshGmsh
        self.assertEqual(
            _FemMeshGmsh,
            doc.MeshGmsh.Proxy.__class__
        )

        from femobjects._FemMeshBoundaryLayer import _FemMeshBoundaryLayer
        self.assertEqual(
            _FemMeshBoundaryLayer,
            doc.MeshBoundaryLayer.Proxy.__class__
        )

        from femobjects._FemMeshGroup import _FemMeshGroup
        self.assertEqual(
            _FemMeshGroup,
            doc.MeshGroup.Proxy.__class__
        )

        from femobjects._FemMeshRegion import _FemMeshRegion
        self.assertEqual(
            _FemMeshRegion,
            doc.MeshRegion.Proxy.__class__
        )

        from femobjects._FemMeshResult import _FemMeshResult
        self.assertEqual(
            _FemMeshResult,
            doc.MeshResult.Proxy.__class__
        )

        from femobjects._FemResultMechanical import _FemResultMechanical
        self.assertEqual(
            _FemResultMechanical,
            doc.ResultMechanical.Proxy.__class__
        )

        from femobjects._FemSolverCalculix import _FemSolverCalculix
        self.assertEqual(
            _FemSolverCalculix,
            doc.SolverCcxTools.Proxy.__class__
        )

        from femsolver.calculix.solver import Proxy
        self.assertEqual(
            Proxy,
            doc.SolverCalculix.Proxy.__class__
        )

        from femsolver.elmer.solver import Proxy
        self.assertEqual(
            Proxy,
            doc.SolverElmer.Proxy.__class__
        )

        from femsolver.z88.solver import Proxy
        self.assertEqual(
            Proxy,
            doc.SolverZ88.Proxy.__class__
        )

        from femsolver.elmer.equations.elasticity import Proxy
        self.assertEqual(
            Proxy,
            doc.Elasticity.Proxy.__class__
        )

        from femsolver.elmer.equations.electrostatic import Proxy
        self.assertEqual(
            Proxy,
            doc.Electrostatic.Proxy.__class__
        )

        from femsolver.elmer.equations.flow import Proxy
        self.assertEqual(
            Proxy,
            doc.Flow.Proxy.__class__
        )

        from femsolver.elmer.equations.fluxsolver import Proxy
        self.assertEqual(
            Proxy,
            doc.Fluxsolver.Proxy.__class__
        )

        from femsolver.elmer.equations.heat import Proxy
        self.assertEqual(
            Proxy,
            doc.Heat.Proxy.__class__
        )

    # ********************************************************************************************
    def compare_feature_pythons_class_gui(
        self,
        doc
    ):
        # see comments at file end, the code was created by some python code
        if not FreeCAD.GuiUp:
            FreeCAD.closeDocument(doc.Name)
            return

        from femguiobjects._ViewProviderFemConstraintBodyHeatSource import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.ConstraintBodyHeatSource.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemConstraintElectrostaticPotential import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.ConstraintElectrostaticPotential.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemConstraintFlowVelocity import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.ConstraintFlowVelocity.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemConstraintInitialFlowVelocity import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.ConstraintInitialFlowVelocity.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemConstraintSelfWeight import _ViewProviderFemConstraintSelfWeight
        self.assertEqual(
            _ViewProviderFemConstraintSelfWeight,
            doc.ConstraintSelfWeight.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemConstraintTie import _ViewProviderFemConstraintTie
        self.assertEqual(
            _ViewProviderFemConstraintTie,
            doc.ConstraintTie.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemElementFluid1D import _ViewProviderFemElementFluid1D
        self.assertEqual(
            _ViewProviderFemElementFluid1D,
            doc.ElementFluid1D.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemElementGeometry1D import _ViewProviderFemElementGeometry1D
        self.assertEqual(
            _ViewProviderFemElementGeometry1D,
            doc.ElementGeometry1D.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemElementGeometry2D import _ViewProviderFemElementGeometry2D
        self.assertEqual(
            _ViewProviderFemElementGeometry2D,
            doc.ElementGeometry2D.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemElementRotation1D import _ViewProviderFemElementRotation1D
        self.assertEqual(
            _ViewProviderFemElementRotation1D,
            doc.ElementRotation1D.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemMaterial import _ViewProviderFemMaterial
        self.assertEqual(
            _ViewProviderFemMaterial,
            doc.MechanicalSolidMaterial.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemMaterial import _ViewProviderFemMaterial
        self.assertEqual(
            _ViewProviderFemMaterial,
            doc.FluidMaterial.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemMaterialMechanicalNonlinear import _ViewProviderFemMaterialMechanicalNonlinear
        self.assertEqual(
            _ViewProviderFemMaterialMechanicalNonlinear,
            doc.MaterialMechanicalNonlinear.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemMaterialReinforced import _ViewProviderFemMaterialReinforced
        self.assertEqual(
            _ViewProviderFemMaterialReinforced,
            doc.MaterialReinforced.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemMeshGmsh import _ViewProviderFemMeshGmsh
        self.assertEqual(
            _ViewProviderFemMeshGmsh,
            doc.MeshGmsh.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemMeshBoundaryLayer import _ViewProviderFemMeshBoundaryLayer
        self.assertEqual(
            _ViewProviderFemMeshBoundaryLayer,
            doc.MeshBoundaryLayer.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemMeshGroup import _ViewProviderFemMeshGroup
        self.assertEqual(
            _ViewProviderFemMeshGroup,
            doc.MeshGroup.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemMeshRegion import _ViewProviderFemMeshRegion
        self.assertEqual(
            _ViewProviderFemMeshRegion,
            doc.MeshRegion.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemMeshResult import _ViewProviderFemMeshResult
        self.assertEqual(
            _ViewProviderFemMeshResult,
            doc.MeshResult.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemResultMechanical import _ViewProviderFemResultMechanical
        self.assertEqual(
            _ViewProviderFemResultMechanical,
            doc.ResultMechanical.ViewObject.Proxy.__class__
        )

        from femguiobjects._ViewProviderFemSolverCalculix import _ViewProviderFemSolverCalculix
        self.assertEqual(
            _ViewProviderFemSolverCalculix,
            doc.SolverCcxTools.ViewObject.Proxy.__class__
        )

        from femsolver.calculix.solver import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.SolverCalculix.ViewObject.Proxy.__class__
        )

        from femsolver.elmer.solver import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.SolverElmer.ViewObject.Proxy.__class__
        )

        from femsolver.z88.solver import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.SolverZ88.ViewObject.Proxy.__class__
        )

        from femsolver.elmer.equations.elasticity import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.Elasticity.ViewObject.Proxy.__class__
        )

        from femsolver.elmer.equations.electrostatic import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.Electrostatic.ViewObject.Proxy.__class__
        )

        from femsolver.elmer.equations.flow import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.Flow.ViewObject.Proxy.__class__
        )

        from femsolver.elmer.equations.fluxsolver import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.Fluxsolver.ViewObject.Proxy.__class__
        )

        from femsolver.elmer.equations.heat import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.Heat.ViewObject.Proxy.__class__
        )

    # ********************************************************************************************
    def tearDown(
        self
    ):
        # setUp is executed after every test
        # FreeCAD.closeDocument(self.document.Name)
        pass


"""
# code was generated by the following code from a document with all objects
# run test_object.test_femobjects_make how to create such document
# in tmp in FEM_unittests will be the file with all objects
# the doc.Name of some objects needs to be edited after run
# because obj. name != proxy type
# elmer equation objects need to be edited after
# material solid and material fluid

# objects
from femtools.femutils import type_of_obj
for o in App.ActiveDocument.Objects:
    if hasattr(o, "Proxy"):
        module_with_class = str(o.Proxy.__class__).lstrip("<class '").rstrip("'>")
        class_of_o = module_with_class.split(".")[-1]
        o_name_from_type = type_of_obj(o).lstrip('Fem::')
        module_to_load = module_with_class.rstrip(class_of_o).rstrip(".")
        print("        from {} import {}".format(module_to_load, class_of_o))
        print("        self.assertEqual(")
        print("            {},".format(class_of_o))
        print("            doc.{}.Proxy.__class__".format(o_name_from_type))
        print("        )")
        print("")

#view providers
from femtools.femutils import type_of_obj
for o in App.ActiveDocument.Objects:
    if hasattr(o, "Proxy"):
        vp_module_with_class = str(o.ViewObject.Proxy.__class__).lstrip("<class '").rstrip("'>")
        vp_class_of_o = vp_module_with_class.split(".")[-1]
        o_name_from_type = type_of_obj(o).lstrip('Fem::')
        vp_module_to_load = vp_module_with_class.rstrip(vp_class_of_o).rstrip(".")
        print("        from {} import {}".format(vp_module_to_load, vp_class_of_o))
        print("        self.assertEqual(")
        print("            {},".format(vp_class_of_o))
        print("            doc.{}.ViewObject.Proxy.__class__".format(o_name_from_type))
        print("        )")
        print("")

for o in App.ActiveDocument.Objects:
    if hasattr(o, "Proxy"):
        o.Proxy.__class__

"""
