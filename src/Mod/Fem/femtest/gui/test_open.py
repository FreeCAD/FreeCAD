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

__title__ = "Open files FEM Gui unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import tempfile
import unittest
from os.path import join

import FreeCAD

from femtest.app import support_utils as testtools
from femtest.app.support_utils import fcc_print
from femtest.app.test_object import create_all_fem_objects_doc


"""
FIXME TODO HACK
Important note!
Delete build directory (at least in fem the objects and vpobjects directories)
if not migrate will not be used because the old modules might still be in the
build directory, thus the test will fail
rm -rf Mod/Fem/
FIXME TODO HACK
"""


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

        fcc_print("\n{0}\n{1} run FEM TestObjectOpen tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            60 * "*"
        ))

    # ********************************************************************************************
    def test_femobjects_open_head(
        self
    ):
        fcc_print("load master head document objects")

        # get a document with all FEM objects
        self.document = create_all_fem_objects_doc(self.document)

        # save and load the document
        file_path = join(tempfile.gettempdir(), "all_objects_head.FCStd")
        self.document.saveAs(file_path)
        FreeCAD.closeDocument(self.document.Name)
        self.document = FreeCAD.open(file_path)

        # FeaturePythons view provider
        self.compare_feature_pythons_class_gui(self.document)

        # standard name changed
        from femsolver.elmer.equations.flux import ViewProxy
        self.assertEqual(
            ViewProxy,
            self.document.Flux.ViewObject.Proxy.__class__
        )

    # ********************************************************************************************
    def test_femobjects_open_de9b3fb438(
        self
    ):
        # the number in method name is the FreeCAD commit the document was created with
        # https://github.com/FreeCAD/FreeCAD/commit/de9b3fb438
        # the document was created by running the object create unit test
        # FreeCAD --run-test "femtest.app.test_object.TestObjectCreate.test_femobjects_make"
        fcc_print("load old document objects")
        FreeCAD.closeDocument(self.document.Name)  # close the empty document from setUp first
        self.document = FreeCAD.open(join(self.test_file_dir, "all_objects_de9b3fb438.FCStd"))

        # FeaturePythons view provider
        self.compare_feature_pythons_class_gui(self.document)

        # standard name changed
        from femsolver.elmer.equations.flux import ViewProxy
        self.assertEqual(
            ViewProxy,
            self.document.Fluxsolver.ViewObject.Proxy.__class__
        )

    # ********************************************************************************************
    def compare_feature_pythons_class_gui(
        self,
        doc
    ):
        import ObjectsFem
        from femtools.femutils import type_of_obj

        # see comments at file end, the code was created by some python code

        from femviewprovider.view_constraint_bodyheatsource import VPConstraintBodyHeatSource
        self.assertEqual(
            VPConstraintBodyHeatSource,
            doc.ConstraintBodyHeatSource.ViewObject.Proxy.__class__
        )

        self.assertEqual(
            "Fem::ConstraintCurrentDensity",
            type_of_obj(ObjectsFem.makeConstraintCurrentDensity(doc))
        )

        from femviewprovider.view_constraint_electrostaticpotential \
            import VPConstraintElectroStaticPotential
        self.assertEqual(
            VPConstraintElectroStaticPotential,
            doc.ConstraintElectrostaticPotential.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_constraint_flowvelocity import VPConstraintFlowVelocity
        self.assertEqual(
            VPConstraintFlowVelocity,
            doc.ConstraintFlowVelocity.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_constraint_initialflowvelocity \
            import VPConstraintInitialFlowVelocity
        self.assertEqual(
            VPConstraintInitialFlowVelocity,
            doc.ConstraintInitialFlowVelocity.ViewObject.Proxy.__class__
        )

        self.assertEqual(
            "Fem::ConstraintMagnetization",
            type_of_obj(ObjectsFem.makeConstraintMagnetization(doc))
        )

        from femviewprovider.view_constraint_selfweight import VPConstraintSelfWeight
        self.assertEqual(
            VPConstraintSelfWeight,
            doc.ConstraintSelfWeight.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_constraint_tie import VPConstraintTie
        self.assertEqual(
            VPConstraintTie,
            doc.ConstraintTie.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_element_fluid1D import VPElementFluid1D
        self.assertEqual(
            VPElementFluid1D,
            doc.ElementFluid1D.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_element_geometry1D import VPElementGeometry1D
        self.assertEqual(
            VPElementGeometry1D,
            doc.ElementGeometry1D.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_element_geometry2D import VPElementGeometry2D
        self.assertEqual(
            VPElementGeometry2D,
            doc.ElementGeometry2D.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_element_rotation1D import VPElementRotation1D
        self.assertEqual(
            VPElementRotation1D,
            doc.ElementRotation1D.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_material_common import VPMaterialCommon
        self.assertEqual(
            VPMaterialCommon,
            doc.MaterialFluid.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_material_common import VPMaterialCommon
        self.assertEqual(
            VPMaterialCommon,
            doc.MaterialSolid.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_material_mechanicalnonlinear import VPMaterialMechanicalNonlinear
        self.assertEqual(
            VPMaterialMechanicalNonlinear,
            doc.MaterialMechanicalNonlinear.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_material_reinforced import VPMaterialReinforced
        self.assertEqual(
            VPMaterialReinforced,
            doc.MaterialReinforced.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_mesh_gmsh import VPMeshGmsh
        self.assertEqual(
            VPMeshGmsh,
            doc.MeshGmsh.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_mesh_boundarylayer import VPMeshBoundaryLayer
        self.assertEqual(
            VPMeshBoundaryLayer,
            doc.MeshBoundaryLayer.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_mesh_group import VPMeshGroup
        self.assertEqual(
            VPMeshGroup,
            doc.MeshGroup.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_mesh_region import VPMeshRegion
        self.assertEqual(
            VPMeshRegion,
            doc.MeshRegion.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_mesh_result import VPFemMeshResult
        self.assertEqual(
            VPFemMeshResult,
            doc.MeshResult.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_result_mechanical import VPResultMechanical
        self.assertEqual(
            VPResultMechanical,
            doc.ResultMechanical.ViewObject.Proxy.__class__
        )

        from femviewprovider.view_solver_ccxtools import VPSolverCcxTools
        self.assertEqual(
            VPSolverCcxTools,
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

        from femsolver.elmer.equations.heat import ViewProxy
        self.assertEqual(
            ViewProxy,
            doc.Heat.ViewObject.Proxy.__class__
        )

        self.assertEqual(
            "Fem::EquationElmerMagnetodynamic2D",
            type_of_obj(ObjectsFem.makeEquationMagnetodynamic2D(doc))
        )

        self.assertEqual(
            "Fem::EquationElmerMagnetodynamic",
            type_of_obj(ObjectsFem.makeEquationMagnetodynamic(doc))
        )


"""
# code was generated by the following code from a document with all objects
# run test_object.test_femobjects_make how to create such document
# in tmp in FEM_unittests will be the file with all objects
# the doc.Name of some objects needs to be edited after run
# because obj. name != proxy type
# elmer equation objects need to be edited after
# material solid and material fluid

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
