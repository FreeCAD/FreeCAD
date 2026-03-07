# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
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

__title__ = "GMSH FEM unit tests"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

import unittest
import importlib
import shutil
from os.path import join

import FreeCAD

import Fem
from femexamples import manager
from femtools.femutils import is_derived_from
from femmesh import gmshtools
from . import support_utils as testtools
from .support_utils import fcc_print

def generate_gmesh_samples_from_example_doc(doc, datapath):
    # used to process a example file into vtk mesh files and store it in datapath
    # this is intended as manual step to generate the correct meshes to witch the tests
    # later compare. Run this only if you want to recreate the golden test meshes which
    # are added into the source code
    # Note: Run this from the source folder!
    #
    # from withing freecad with the example files open, run:
    # from femtest.app import test_gmsh
    # test_gmsh.generate_gmesh_samples_from_example_doc(App.ActiveDocument, path_to_src_data_folder)

    # collect all gmsh objects
    gmsh = []
    for obj in doc.Objects:
        if is_derived_from(obj, "Fem::FemMeshGmsh"):
            gmsh.append(obj)

    # process all gmsh objects
    for mesh in gmsh:

        # 1. Run gmsh to create the mesh
        tool = gmshtools.GmshTools(mesh)
        tool.create_mesh()

        # 2. Derive file name from group name and build path
        filename = mesh.getParentGroup().Label + ".vtk"
        path = join(datapath, filename)

        # find the vtk file gmsh created and copy it into the location
        # (we do not export from FemMesh, as the exporter is extremely limited and reimport
        #  gives other results)
        shutil.copyfile(tool.temp_file_mesh, path)

class TestGMSHBase(unittest.TestCase):

    # ********************************************************************************************
    def tearDown(self):
        # tearDown is executed after every test
        if hasattr(self, "doc"):
            FreeCAD.closeDocument(self.doc.Name)

    # ********************************************************************************************
    def load_example_file(self, name):
        # opens a example file to process for testing
        module = importlib.import_module(f"femexamples.{name}")
        self.doc = module.setup()

        if FreeCAD.GuiUp:
            import FreeCADGui
            FreeCADGui.SendMsgToActiveView("ViewFit")

    def load_and_run_example_file(self, name):
        # opens and runs example file to process for testing
        self.doc = manager.run_example(name, run_solver=True)

        if FreeCAD.GuiUp:
            import FreeCADGui
            FreeCADGui.SendMsgToActiveView("ViewFit")

    def get_gmsh_objects(self):
        result = []
        for obj in self.doc.Objects:
            if is_derived_from(obj, "Fem::FemMeshGmsh"):
                result.append(obj)

        return result

    def execute_gmsh(self, obj):
        tool = gmshtools.GmshTools(obj)
        tool.create_mesh()

    # ********************************************************************************************
    def compare_exact_mesh_to_sample(self, mesh_obj):
        # compare generated mesh to sample to be the exact same. This should only be used for
        # meshing algorithms that gurantee to define exact node counts and locations, as well as
        # elements. If things can vary over gmsh versions this should not be used.

        # load the sample mesh we want to compare to
        name = mesh_obj.getParentGroup().Label
        path = join(testtools.get_fem_test_home_dir(), "gmsh", name+".vtk")
        sample = Fem.FemMesh()
        sample.read(path)

        # test reading the test mesh
        mesh = mesh_obj.FemMesh

        self.assertEqual(
            mesh.NodeCount,
            sample.NodeCount,
            f"Generated mesh does not have the same Node count as the golden sample: {name}"
        )

        # compare node locations!
        for idx in range(1,mesh.NodeCount+1):
            self.assertTrue(mesh.Nodes[idx].isEqual(sample.Nodes[idx], 1e-3),
                            f"Generated mesh does not have the same Node locations as golden sample: {name}")

        self.assertEqual(
            mesh.EdgeCount,
            sample.EdgeCount,
            f"Generated mesh does not have the same edge count as the golden sample: {name}"
        )

        self.assertEqual(
            mesh.TriangleCount,
            sample.TriangleCount,
            f"Generated mesh does not have the same triangle count as the golden sample: {name}"
        )

        self.assertEqual(
            mesh.QuadrangleCount,
            sample.QuadrangleCount,
            f"Generated mesh does not have the same quadrangle count as the golden sample: {name}"
        )

        self.assertEqual(
            mesh.PolygonCount,
            sample.PolygonCount,
            f"Generated mesh does not have the same polygon count as the golden sample: {name}"
        )

        self.assertEqual(
            mesh.VolumeCount,
            sample.VolumeCount,
            f"Generated mesh does not have the same volume count as the golden sample: {name}"
        )

        self.assertEqual(
            mesh.TetraCount,
            sample.TetraCount,
            f"Generated mesh does not have the same tetrahedra count as the golden sample: {name}"
        )

        self.assertEqual(
            mesh.HexaCount,
            sample.HexaCount,
            f"Generated mesh does not have the same hexahedra count as the golden sample: {name}"
        )

        self.assertEqual(
            mesh.PyramidCount,
            sample.PyramidCount,
            f"Generated mesh does not have the same pyramid count as the golden sample: {name}"
        )

        self.assertEqual(
            mesh.PrismCount,
            sample.PrismCount,
            f"Generated mesh does not have the same prism count as the golden sample: {name}"
        )

        self.assertEqual(
            mesh.PolyhedronCount,
            sample.PolyhedronCount,
            f"Generated mesh does not have the same polyhedra count as the golden sample: {name}"
        )

    def compare_fuzzy_mesh_to_sample(self, mesh_obj):
        # compare generated mesh to sample to be the roughly the same. This accounts for sligth variations between
        # gmsh versions. The meshes do not to be the exact same, but can varie sligthly.

        # load the sample mesh we want to compare to
        name = mesh_obj.getParentGroup().Label
        path = join(testtools.get_fem_test_home_dir(), "gmsh", name+".vtk")
        sample = Fem.FemMesh()
        sample.read(path)

        # test reading the test mesh
        mesh = mesh_obj.FemMesh

        def diff(val1, val2):
            if val2 == 0:
                if val1 == 0:
                    return 0
                else:
                    return 1

            return abs(1-val1/val2)

        allowed_diff = 0.05 # 5% diff allowed

        self.assertLess(
            diff(mesh.NodeCount, sample.NodeCount),
            allowed_diff,
            f"Generated mesh does not have the same Node count as the golden sample: {name}"
        )

        self.assertLess(
            diff(mesh.EdgeCount, sample.EdgeCount),
            allowed_diff,
            f"Generated mesh does not have the same edge count as the golden sample: {name}"
        )

        self.assertLess(
            diff(mesh.TriangleCount, sample.TriangleCount),
            allowed_diff,
            f"Generated mesh does not have the same triangle count as the golden sample: {name}"
        )

        self.assertLess(
            diff(mesh.QuadrangleCount, sample.QuadrangleCount),
            allowed_diff,
            f"Generated mesh does not have the same quadrangle count as the golden sample: {name}"
        )

        self.assertLess(
            diff(mesh.PolygonCount, sample.PolygonCount),
            allowed_diff,
            f"Generated mesh does not have the same polygon count as the golden sample: {name}"
        )

        self.assertLess(
            diff(mesh.VolumeCount, sample.VolumeCount),
            allowed_diff,
            f"Generated mesh does not have the same volume count as the golden sample: {name}"
        )

        self.assertLess(
            diff(mesh.TetraCount, sample.TetraCount),
            allowed_diff,
            f"Generated mesh does not have the same tetrahedra count as the golden sample: {name}"
        )

        self.assertLess(
            diff(mesh.HexaCount, sample.HexaCount),
            allowed_diff,
            f"Generated mesh does not have the same hexahedra count as the golden sample: {name}"
        )

        self.assertLess(
            diff(mesh.PyramidCount, sample.PyramidCount),
            allowed_diff,
            f"Generated mesh does not have the same pyramid count as the golden sample: {name}"
        )

        self.assertLess(
            diff(mesh.PrismCount, sample.PrismCount),
            allowed_diff,
            f"Generated mesh does not have the same prism count as the golden sample: {name}"
        )

        self.assertLess(
            diff(mesh.PolyhedronCount, sample.PolyhedronCount),
            allowed_diff,
            f"Generated mesh does not have the same polyhedra count as the golden sample: {name}"
        )


# ************************************************************************************************
# ************************************************************************************************
class TestGMSHTransfinite(TestGMSHBase):
    fcc_print("import TestGMSHTransfinite")

    # ********************************************************************************************
    def test_00print(self):
        # since method name starts with 00 this will be run first
        # this test just prints a line with stars

        fcc_print(
            "\n{0}\n{1} run FEM TestGMSHTransfinite tests {2}\n{0}".format(
                100 * "*", 10 * "*", 56 * "*"
            )
        )


    # ********************************************************************************************
    def test_GMSHTransfiniteManual(self):

        try:
            self.load_example_file("gmsh_transfinite_manual")
            gmshs = self.get_gmsh_objects()
            for gmsh in gmshs:
                self.execute_gmsh(gmsh)
                self.compare_exact_mesh_to_sample(gmsh)

                if FreeCAD.GuiUp:
                    import FreeCADGui
                    FreeCADGui.updateGui()
        except gmshtools.GmshError:
            # this exception is thrown if gmsh is not available. We pass in this case
            pass

    def test_GMSHTransfiniteAutomation(self):

        try:
            self.load_example_file("gmsh_transfinite_automation")
            gmshs = self.get_gmsh_objects()
            for gmsh in gmshs:
                self.execute_gmsh(gmsh)
                self.compare_exact_mesh_to_sample(gmsh)

                if FreeCAD.GuiUp:
                    import FreeCADGui
                    FreeCADGui.updateGui()

        except gmshtools.GmshError:
            # this exception is thrown if gmsh is not available. We pass in this case
            pass


class TestGMSHRefinements(TestGMSHBase):
    fcc_print("import TestGMSHRefinements")

    # ********************************************************************************************
    def test_00print(self):
        # since method name starts with 00 this will be run first
        # this test just prints a line with stars

        fcc_print(
            "\n{0}\n{1} run FEM TestGMSHRefinement tests {2}\n{0}".format(
                100 * "*", 10 * "*", 56 * "*"
            )
        )


    # ********************************************************************************************
    def test_GMSHAdaptiv(self):

        try:
            self.load_and_run_example_file("gmsh_adaptive")
            gmshs = self.get_gmsh_objects()
            for gmsh in gmshs:
                self.compare_fuzzy_mesh_to_sample(gmsh)
        except gmshtools.GmshError:
            # this exception is thrown if gmsh is not available. We pass in this case
            pass

