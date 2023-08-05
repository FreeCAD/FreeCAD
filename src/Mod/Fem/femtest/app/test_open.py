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

__title__ = "Open files FEM App unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import platform
import tempfile
import unittest
from os.path import join

import FreeCAD

from . import support_utils as testtools
from .support_utils import fcc_print


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

        # new document
        self.document = FreeCAD.newDocument(self.__class__.__name__)

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
        from .test_object import create_all_fem_objects_doc
        self.document = create_all_fem_objects_doc(self.document)

        # save and load the document
        file_path = join(tempfile.gettempdir(), "all_objects_head.FCStd")
        self.document.saveAs(file_path)
        FreeCAD.closeDocument(self.document.Name)
        self.document = FreeCAD.open(file_path)

        # C++ objects
        self.compare_cpp_objs(self.document)
        # FeaturePythons objects
        self.compare_feature_pythons_class_app(self.document)

        # standard name changed
        from femsolver.elmer.equations.flux import Proxy
        self.assertEqual(
            Proxy,
            self.document.Flux.Proxy.__class__
        )

    # ********************************************************************************************
    def test_femobjects_open_de9b3fb438(
        self
    ):
        # migration modules fail on s390x (big endian) and trigger OOMs,
        # https://bugs.debian.org/984952 and
        # https://bugs.launchpad.net/ubuntu/+source/freecad/+bug/1918474.
        if platform.machine() == "s390x":
            return

        # the number in method name is the FreeCAD commit the document was created with
        # https://github.com/FreeCAD/FreeCAD/commit/de9b3fb438
        # the document was created by running the object create unit test
        # FreeCAD --run-test "femtest.app.test_object.TestObjectCreate.test_femobjects_make"
        fcc_print("load old document objects")
        FreeCAD.closeDocument(self.document.Name)  # close the empty document from setUp first
        self.document = FreeCAD.open(join(self.test_file_dir, "all_objects_de9b3fb438.FCStd"))

        # C++ objects
        self.compare_cpp_objs(self.document)
        # FeaturePythons objects
        self.compare_feature_pythons_class_app(self.document)

        # standard name changed
        from femsolver.elmer.equations.flux import Proxy
        self.assertEqual(
            Proxy,
            self.document.Fluxsolver.Proxy.__class__
        )

    # ********************************************************************************************
    def compare_cpp_objs(
        self,
        doc
    ):
        from femtools.femutils import type_of_obj

        self.assertEqual(
            "Fem::FemAnalysis",
            type_of_obj(doc.Analysis)
        )
        # TODO other C++ objects and view provider
        # Is just checking the type sufficient?
        # If there is a type there is at least a object with correct type ;-)

    # ********************************************************************************************
    def compare_feature_pythons_class_app(
        self,
        doc
    ):
        import ObjectsFem
        from femtools.femutils import type_of_obj

        # see comments at file end, the code was created by some python code
        """
        # see code lines after comment block for the smarter version
        # but this makes it easy to understand what is happening
        self.assertEqual(
            "<class 'femobjects.constraint_bodyheatsource.ConstraintBodyHeatSource'>",
            str(doc.ConstraintBodyHeatSource.Proxy.__class__)
        )
        """
        from femobjects.constraint_bodyheatsource import ConstraintBodyHeatSource
        self.assertEqual(
            ConstraintBodyHeatSource,
            doc.ConstraintBodyHeatSource.Proxy.__class__
        )

        self.assertEqual(
            "Fem::ConstraintCurrentDensity",
            type_of_obj(ObjectsFem.makeConstraintCurrentDensity(doc))
        )

        from femobjects.constraint_electrostaticpotential import ConstraintElectrostaticPotential
        self.assertEqual(
            ConstraintElectrostaticPotential,
            doc.ConstraintElectrostaticPotential.Proxy.__class__
        )

        from femobjects.constraint_flowvelocity import ConstraintFlowVelocity
        self.assertEqual(
            ConstraintFlowVelocity,
            doc.ConstraintFlowVelocity.Proxy.__class__
        )

        from femobjects.constraint_initialflowvelocity import ConstraintInitialFlowVelocity
        self.assertEqual(
            ConstraintInitialFlowVelocity,
            doc.ConstraintInitialFlowVelocity.Proxy.__class__
        )

        self.assertEqual(
            "Fem::ConstraintMagnetization",
            type_of_obj(ObjectsFem.makeConstraintMagnetization(doc))
        )

        from femobjects.constraint_selfweight import ConstraintSelfWeight
        self.assertEqual(
            ConstraintSelfWeight,
            doc.ConstraintSelfWeight.Proxy.__class__
        )

        from femobjects.constraint_tie import ConstraintTie
        self.assertEqual(
            ConstraintTie,
            doc.ConstraintTie.Proxy.__class__
        )

        from femobjects.element_fluid1D import ElementFluid1D
        self.assertEqual(
            ElementFluid1D,
            doc.ElementFluid1D.Proxy.__class__
        )

        from femobjects.element_geometry1D import ElementGeometry1D
        self.assertEqual(
            ElementGeometry1D,
            doc.ElementGeometry1D.Proxy.__class__
        )

        from femobjects.element_geometry2D import ElementGeometry2D
        self.assertEqual(
            ElementGeometry2D,
            doc.ElementGeometry2D.Proxy.__class__
        )

        from femobjects.element_rotation1D import ElementRotation1D
        self.assertEqual(
            ElementRotation1D,
            doc.ElementRotation1D.Proxy.__class__
        )

        from femobjects.material_common import MaterialCommon
        self.assertEqual(
            MaterialCommon,
            doc.MaterialFluid.Proxy.__class__
        )

        from femobjects.material_common import MaterialCommon
        self.assertEqual(
            MaterialCommon,
            doc.MaterialSolid.Proxy.__class__
        )

        from femobjects.material_mechanicalnonlinear import MaterialMechanicalNonlinear
        self.assertEqual(
            MaterialMechanicalNonlinear,
            doc.MaterialMechanicalNonlinear.Proxy.__class__
        )

        from femobjects.material_reinforced import MaterialReinforced
        self.assertEqual(
            MaterialReinforced,
            doc.MaterialReinforced.Proxy.__class__
        )

        from femobjects.mesh_gmsh import MeshGmsh
        self.assertEqual(
            MeshGmsh,
            doc.MeshGmsh.Proxy.__class__
        )

        from femobjects.mesh_boundarylayer import MeshBoundaryLayer
        self.assertEqual(
            MeshBoundaryLayer,
            doc.MeshBoundaryLayer.Proxy.__class__
        )

        from femobjects.mesh_group import MeshGroup
        self.assertEqual(
            MeshGroup,
            doc.MeshGroup.Proxy.__class__
        )

        from femobjects.mesh_region import MeshRegion
        self.assertEqual(
            MeshRegion,
            doc.MeshRegion.Proxy.__class__
        )

        from femobjects.mesh_result import MeshResult
        self.assertEqual(
            MeshResult,
            doc.MeshResult.Proxy.__class__
        )

        from femobjects.result_mechanical import ResultMechanical
        self.assertEqual(
            ResultMechanical,
            doc.ResultMechanical.Proxy.__class__
        )

        from femobjects.solver_ccxtools import SolverCcxTools
        self.assertEqual(
            SolverCcxTools,
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

        from femsolver.elmer.equations.heat import Proxy
        self.assertEqual(
            Proxy,
            doc.Heat.Proxy.__class__
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

for o in App.ActiveDocument.Objects:
    if hasattr(o, "Proxy"):
        o.Proxy.__class__

"""
