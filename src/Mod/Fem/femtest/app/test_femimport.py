# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "Import FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import unittest

import FreeCAD

from .support_utils import fcc_print


class TestFemImport(unittest.TestCase):
    fcc_print("import TestFemImport")

    # ********************************************************************************************
    # no is document needed to test import Fem and import FemGui
    # thus neither setUp nor tearDown methods are needed

    def test_00print(
        self
    ):
        fcc_print("\n{0}\n{1} run FEM TestFemImport tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            61 * "*"
        ))

    # ********************************************************************************************
    def test_import_fem(
        self
    ):

        mod = "Fem"
        fcc_print("\n  Try importing {0} ...".format(mod))
        try:
            im = __import__("{0}".format(mod))
        except ImportError:
            im = False
        if not im:
            # to get an error message what was going wrong
            __import__("{0}".format(mod))
        self.assertTrue(im, "Problem importing {0}".format(mod))

        if FreeCAD.GuiUp:
            mod = "FemGui"
            fcc_print("  Try importing {0} ...".format(mod))
            try:
                im = __import__("{0}".format(mod))
            except ImportError:
                im = False
            if not im:
                # to get an error message what was going wrong
                __import__("{0}".format(mod))
            self.assertTrue(im, "Problem importing {0}".format(mod))


# ************************************************************************************************
# ************************************************************************************************
# to be sure this is run on very first of FEM test this is here and not in objects
class TestObjectExistance(unittest.TestCase):
    fcc_print("import TestObjectExistance")

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
        fcc_print("\n{0}\n{1} run FEM TestObjectExistance tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            55 * "*"
        ))

    # ********************************************************************************************
    def test_objects_existance(
        self
    ):

        expected_obj_types = [
            "Fem::Constraint",
            "Fem::ConstraintBearing",
            "Fem::ConstraintContact",
            "Fem::ConstraintDisplacement",
            "Fem::ConstraintFixed",
            "Fem::ConstraintFluidBoundary",
            "Fem::ConstraintForce",
            "Fem::ConstraintGear",
            "Fem::ConstraintHeatflux",
            "Fem::ConstraintInitialTemperature",
            "Fem::ConstraintPlaneRotation",
            "Fem::ConstraintPressure",
            "Fem::ConstraintPulley",
            "Fem::ConstraintPython",
            "Fem::ConstraintTemperature",
            "Fem::ConstraintTransform",
            "Fem::DocumentObject",
            "Fem::FeaturePython",
            "Fem::FemAnalysis",
            "Fem::FemAnalysisPython",
            "Fem::FemMeshObject",
            "Fem::FemMeshObjectPython",
            "Fem::FemMeshShapeNetgenObject",
            "Fem::FemMeshShapeObject",
            "Fem::FemResultObject",
            "Fem::FemResultObjectPython",
            "Fem::FemSetElementsObject",
            "Fem::FemSetFacesObject",
            "Fem::FemSetGeometryObject",
            "Fem::FemSetNodesObject",
            "Fem::FemSetObject",
            "Fem::FemSolverObject",
            "Fem::FemSolverObjectPython",
        ]

        expected_vtk_obj_types = [
            "Fem::FemPostClipFilter",
            "Fem::FemPostContoursFilter",
            "Fem::FemPostCutFilter",
            "Fem::FemPostDataAlongLineFilter",
            "Fem::FemPostDataAtPointFilter",
            "Fem::FemPostFilter",
            "Fem::FemPostFunction",
            "Fem::FemPostFunctionProvider",
            "Fem::FemPostObject",
            "Fem::FemPostPipeline",
            "Fem::FemPostPlaneFunction",
            "Fem::FemPostScalarClipFilter",
            "Fem::FemPostSphereFunction",
            "Fem::FemPostWarpVectorFilter",
        ]

        # if FEM VTK post processing is enabled, we need to add VTK post objects
        if "BUILD_FEM_VTK" in FreeCAD.__cmake__:
            expected_obj_types += expected_vtk_obj_types

        expected_len = len(expected_obj_types)
        expected_obj_types = sorted(expected_obj_types)

        doc = self.document

        # get the supportedTypes for FEM module

        # Fem needs do be imported to get the FEM document types
        # with the following instead of import Fem
        # flake8 and lgtm do not complain "Fem imported but unused"
        __import__("Fem")

        obj_types = []
        for obj_type in sorted(doc.supportedTypes()):
            if obj_type.startswith("Fem"):
                obj_types.append(obj_type)

        obj_types = sorted(obj_types)

        # test
        self.assertEqual(
            expected_len,
            len(obj_types)
        )

        self.assertEqual(
            expected_obj_types,
            obj_types
        )
