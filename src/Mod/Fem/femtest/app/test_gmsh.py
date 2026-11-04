# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Stefan Tröger <stefantroeger@gmx.net>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

__title__ = "GMSH FEM unit tests"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

import unittest
import importlib
import shutil
import tempfile
from os.path import join

import FreeCAD

import Fem
import ObjectsFem
from femexamples import manager
from femtools.femutils import is_derived_from
from femmesh import gmshtools, meshtools
from . import support_utils as testtools
from .support_utils import fcc_print


def _has_ccx_binary():
    binary = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx").GetString(
        "ccxBinaryPath", ""
    )
    return shutil.which(binary or "ccx") is not None


def generate_gmesh_samples_from_example_doc(doc, datapath):
    # used to process a example file into vtk mesh files and store it in datapath
    # this is intended as manual step to generate the correct meshes to witch the tests
    # later compare. Run this only if you want to recreate the golden test meshes which
    # are added into the source code
    # Note: Run this from the source folder!
    #
    # from within freecad with the example files open, run:
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
    def setUp(self):
        self.document = FreeCAD.newDocument(self.__class__.__name__)

    # ********************************************************************************************
    def tearDown(self):
        # tearDown is executed after every test
        FreeCAD.closeDocument(self.document.Name)

    # ********************************************************************************************
    def load_example_file(self, name):
        # opens a example file to process for testing
        module = importlib.import_module(f"femexamples.{name}")
        self.doc = module.setup(doc=self.document)

        if FreeCAD.GuiUp:
            import FreeCADGui

            FreeCADGui.ActiveDocument.ActiveView.sendMessage("ViewFit")

    def load_and_run_example_file(self, name):
        # opens and runs example file to process for testing
        self.doc = manager.run_example(name, run_solver=True, doc=self.document)

        if FreeCAD.GuiUp:
            import FreeCADGui

            FreeCADGui.ActiveDocument.ActiveView.sendMessage("ViewFit")

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
        # meshing algorithms that guarantee to define exact node counts and locations, as well as
        # elements. If things can vary over gmsh versions this should not be used.

        # load the sample mesh we want to compare to
        name = mesh_obj.getParentGroup().Label
        path = join(testtools.get_fem_test_home_dir(), "gmsh", name + ".vtk")
        sample = Fem.FemMesh()
        sample.read(path)

        # test reading the test mesh
        mesh = mesh_obj.FemMesh

        self.assertEqual(
            mesh.NodeCount,
            sample.NodeCount,
            f"Generated mesh does not have the same Node count as the golden sample: {name}",
        )

        # compare node locations!
        for idx in range(1, mesh.NodeCount + 1):
            self.assertTrue(
                mesh.Nodes[idx].isEqual(sample.Nodes[idx], 1e-3),
                f"Generated mesh does not have the same Node locations as golden sample: {name}",
            )

        self.assertEqual(
            mesh.EdgeCount,
            sample.EdgeCount,
            f"Generated mesh does not have the same edge count as the golden sample: {name}",
        )

        self.assertEqual(
            mesh.TriangleCount,
            sample.TriangleCount,
            f"Generated mesh does not have the same triangle count as the golden sample: {name}",
        )

        self.assertEqual(
            mesh.QuadrangleCount,
            sample.QuadrangleCount,
            f"Generated mesh does not have the same quadrangle count as the golden sample: {name}",
        )

        self.assertEqual(
            mesh.PolygonCount,
            sample.PolygonCount,
            f"Generated mesh does not have the same polygon count as the golden sample: {name}",
        )

        self.assertEqual(
            mesh.VolumeCount,
            sample.VolumeCount,
            f"Generated mesh does not have the same volume count as the golden sample: {name}",
        )

        self.assertEqual(
            mesh.TetraCount,
            sample.TetraCount,
            f"Generated mesh does not have the same tetrahedra count as the golden sample: {name}",
        )

        self.assertEqual(
            mesh.HexaCount,
            sample.HexaCount,
            f"Generated mesh does not have the same hexahedra count as the golden sample: {name}",
        )

        self.assertEqual(
            mesh.PyramidCount,
            sample.PyramidCount,
            f"Generated mesh does not have the same pyramid count as the golden sample: {name}",
        )

        self.assertEqual(
            mesh.PrismCount,
            sample.PrismCount,
            f"Generated mesh does not have the same prism count as the golden sample: {name}",
        )

        self.assertEqual(
            mesh.PolyhedronCount,
            sample.PolyhedronCount,
            f"Generated mesh does not have the same polyhedra count as the golden sample: {name}",
        )

    def compare_fuzzy_mesh_to_sample(self, mesh_obj, allowed_diff=0.05):
        # compare generated mesh to sample to be the roughly the same. This accounts for slight variations
        # between gmsh versions. The meshes do not to be the exact same, but can vary slightly.
        #
        # allowed_diff is given as relation to 1 (5% = 0.05)

        # load the sample mesh we want to compare to
        name = mesh_obj.getParentGroup().Label
        path = join(testtools.get_fem_test_home_dir(), "gmsh", name + ".vtk")
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

            return abs(1 - val1 / val2)

        self.assertLess(
            diff(mesh.NodeCount, sample.NodeCount),
            allowed_diff,
            f"Generated mesh does not have the same Node count as the golden sample: {name}",
        )

        self.assertLess(
            diff(mesh.EdgeCount, sample.EdgeCount),
            allowed_diff,
            f"Generated mesh does not have the same edge count as the golden sample: {name}",
        )

        self.assertLess(
            diff(mesh.TriangleCount, sample.TriangleCount),
            allowed_diff,
            f"Generated mesh does not have the same triangle count as the golden sample: {name}",
        )

        self.assertLess(
            diff(mesh.QuadrangleCount, sample.QuadrangleCount),
            allowed_diff,
            f"Generated mesh does not have the same quadrangle count as the golden sample: {name}",
        )

        self.assertLess(
            diff(mesh.PolygonCount, sample.PolygonCount),
            allowed_diff,
            f"Generated mesh does not have the same polygon count as the golden sample: {name}",
        )

        self.assertLess(
            diff(mesh.VolumeCount, sample.VolumeCount),
            allowed_diff,
            f"Generated mesh does not have the same volume count as the golden sample: {name}",
        )

        self.assertLess(
            diff(mesh.TetraCount, sample.TetraCount),
            allowed_diff,
            f"Generated mesh does not have the same tetrahedra count as the golden sample: {name}",
        )

        self.assertLess(
            diff(mesh.HexaCount, sample.HexaCount),
            allowed_diff,
            f"Generated mesh does not have the same hexahedra count as the golden sample: {name}",
        )

        self.assertLess(
            diff(mesh.PyramidCount, sample.PyramidCount),
            allowed_diff,
            f"Generated mesh does not have the same pyramid count as the golden sample: {name}",
        )

        self.assertLess(
            diff(mesh.PrismCount, sample.PrismCount),
            allowed_diff,
            f"Generated mesh does not have the same prism count as the golden sample: {name}",
        )

        self.assertLess(
            diff(mesh.PolyhedronCount, sample.PolyhedronCount),
            allowed_diff,
            f"Generated mesh does not have the same polyhedra count as the golden sample: {name}",
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
    @unittest.skipUnless(_has_ccx_binary(), "CalculiX binary not found")
    def test_GMSHAdaptiv(self):

        try:
            self.load_and_run_example_file("gmsh_adaptive")
            gmshs = self.get_gmsh_objects()
            for gmsh in gmshs:
                # increased fuzzy factor, as man refinements mean small differences add up
                self.compare_fuzzy_mesh_to_sample(gmsh, 0.1)
        except gmshtools.GmshError:
            # this exception is thrown if gmsh is not available. We pass in this case
            pass


class TestGMSHAnalysisGroups(TestGMSHBase):
    fcc_print("import TestGMSHAnalysisGroups")

    def setUp(self):
        super().setUp()
        self.prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/General")
        self.addCleanup(
            self.prefs.SetBool,
            "AnalysisGroupMeshing",
            self.prefs.GetBool("AnalysisGroupMeshing", False),
        )
        self.prefs.SetBool("AnalysisGroupMeshing", True)
        self.box = self.make_box("Box")

    def make_box(self, name, x=0):
        box = self.document.addObject("Part::Box", name)
        box.Length, box.Width, box.Height = 10, 8, 6
        box.Placement.Base = FreeCAD.Vector(x, 0, 0)
        self.document.recompute()
        return box

    def fixed(self, *references):
        constraint = ObjectsFem.makeConstraintFixed(self.document, "Fixed")
        constraint.References = list(references)
        return constraint

    def run_mesh(self, shape, members, **properties):
        analysis = ObjectsFem.makeAnalysis(self.document, "Analysis")
        analysis.addObjects(members)
        mesh = ObjectsFem.makeMeshGmsh(self.document, "Mesh")
        mesh.Shape = shape
        mesh.CharacteristicLengthMax = "2 mm"
        for name, value in properties.items():
            setattr(mesh, name, value)
        analysis.addObject(mesh)
        self.document.recompute()
        try:
            self.execute_gmsh(mesh)
        except gmshtools.GmshError:
            self.skipTest("gmsh not found")
        return mesh

    def fragments(self):
        import BOPTools.SplitFeatures

        # Half as high as the box, the fragments split the Face2 it lies against
        small = self.make_box("Small", 10)
        small.Height = 3
        self.document.recompute()
        parts = BOPTools.SplitFeatures.makeBooleanFragments(name="Parts")
        parts.Objects = [self.box, small]
        parts.Mode = "CompSolid"
        self.document.recompute()
        return small, parts

    def groups_of(self, femmesh, member):
        groups = {}
        for group in femmesh.Groups:
            name = femmesh.getGroupName(group)
            if name == member.Name or name.startswith(member.Name + "_"):
                groups[name[len(member.Name) :]] = femmesh.getGroupElementType(group)
        return groups

    def read_nodes(self, femmesh, member):
        return sorted(
            meshtools.get_femmesh_groupdata_sets_by_name(femmesh, {"Object": member}, "Node")
        )

    def search(self, femmesh, *shapes):
        methods = {
            "Vertex": femmesh.getNodesByVertex,
            "Edge": femmesh.getNodesByEdge,
            "Face": femmesh.getNodesByFace,
            "Solid": femmesh.getNodesBySolid,
        }
        return sorted({node for shape in shapes for node in methods[shape.ShapeType](shape)})

    def assert_left_to_the_search(self, femmesh, constraint, *shapes):
        self.assertEqual(self.groups_of(femmesh, constraint), {})
        self.assertEqual(
            meshtools.get_femnodes_by_femobj_with_references(femmesh, {"Object": constraint}),
            self.search(femmesh, *shapes),
        )

    def test_groups_of_each_kind_of_reference(self):
        expected = {
            ("Vertex1",): {"_Nodes": "Node"},
            ("Edge1",): {"_Edges": "Edge", "_Nodes": "Node"},
            ("Face1",): {"_Faces": "Face", "_Nodes": "Node"},
            ("Solid1",): {"_Volumes": "Volume", "_Nodes": "Node"},
            ("Face1", "Edge6"): {"_Faces": "Face", "_Edges": "Edge", "_Nodes": "Node"},
        }
        for order in ("1st", "2nd"):
            for elements, groups in expected.items():
                with self.subTest(order=order, elements=elements):
                    constraint = self.fixed((self.box, elements))
                    femmesh = self.run_mesh(self.box, [constraint], ElementOrder=order).FemMesh
                    ids = {femmesh.getGroupName(group): group for group in femmesh.Groups}
                    # gmsh drops the point elements from the unv file of a build without vtk
                    if elements[0] == "Vertex1" and "Vertex1" not in ids:
                        self.skipTest("no vertex groups in this build")
                    self.assertEqual(self.groups_of(femmesh, constraint), groups)
                    shapes = [self.box.Shape.getElement(element) for element in elements]
                    self.assertEqual(
                        self.read_nodes(femmesh, constraint), self.search(femmesh, *shapes)
                    )
                    for element in elements:
                        element_type = femmesh.getGroupElementType(ids[element])
                        if element_type != "0DElement":
                            group = ids[f"{constraint.Name}_{element_type}s"]
                            self.assertEqual(
                                sorted(femmesh.getGroupElements(group)),
                                sorted(femmesh.getGroupElements(ids[element])),
                            )

    def test_groups_survive_saving_and_reopening(self):
        constraint = self.fixed((self.box, ("Face1",)))
        mesh = self.run_mesh(self.box, [constraint])
        before = self.read_nodes(mesh.FemMesh, constraint)
        self.assertTrue(before)
        with tempfile.TemporaryDirectory() as folder:
            path = join(folder, "groups.FCStd")
            self.document.saveCopy(path)
            reopened = FreeCAD.openDocument(path)
            self.addCleanup(FreeCAD.closeDocument, reopened.Name)
            after = self.read_nodes(
                reopened.getObject(mesh.Name).FemMesh, reopened.getObject(constraint.Name)
            )
        self.assertEqual(after, before)

    def test_a_mesh_group_labelled_like_a_face_does_not_replace_that_face(self):
        constraint = self.fixed((self.box, ("Face1",)))
        analysis = ObjectsFem.makeAnalysis(self.document, "Analysis")
        analysis.addObject(constraint)
        mesh = ObjectsFem.makeMeshGmsh(self.document, "Mesh")
        mesh.Shape = self.box
        analysis.addObject(mesh)
        group = ObjectsFem.makeMeshGroup(self.document, mesh, True, "UserGroup")
        group.Label = "Face1"
        group.References = [(self.box, ("Face2",))]
        self.document.recompute()
        try:
            self.execute_gmsh(mesh)
        except gmshtools.GmshError:
            self.skipTest("gmsh not found")
        self.assertEqual(
            self.read_nodes(mesh.FemMesh, constraint),
            self.search(mesh.FemMesh, self.box.Shape.Face1),
        )

    def test_a_reference_split_by_the_fragments_is_left_to_the_search(self):
        small, parts = self.fragments()
        constraint = self.fixed((self.box, ("Face2",)), (small, ("Face2",)))
        femmesh = self.run_mesh(parts, [constraint]).FemMesh
        self.assert_left_to_the_search(femmesh, constraint, self.box.Shape.Face2, small.Shape.Face2)

    def test_a_shape_group_the_mesh_does_not_have_is_left_to_the_search(self):
        # No volume in a surface mesh, no Solid1 group
        constraint = self.fixed((self.box, ("Solid1", "Face1")))
        # The end of the gmsh run hides an exception, the next member would lose its groups
        next_member = self.fixed((self.box, ("Face2",)))
        femmesh = self.run_mesh(self.box, [constraint, next_member], ElementDimension="2D").FemMesh
        self.assert_left_to_the_search(femmesh, constraint, self.box.Shape, self.box.Shape.Face1)
        self.assertEqual(
            self.read_nodes(femmesh, next_member), self.search(femmesh, self.box.Shape.Face2)
        )

    def test_a_reference_on_another_object_gets_no_group(self):
        # Same size and local position as the box, elsewhere through its container
        other = self.make_box("Other")
        container = self.document.addObject("App::Part", "Container")
        container.addObject(other)
        container.Placement.Base = FreeCAD.Vector(100, 0, 0)
        self.document.recompute()
        constraint = self.fixed((other, ("Face1",)))
        femmesh = self.run_mesh(self.box, [constraint]).FemMesh
        self.assertEqual(self.groups_of(femmesh, constraint), {})

    def test_each_member_of_two_fragments_reads_its_own_elements(self):
        # The numbering of the shape and the one of the groups must stay the same
        _, parts = self.fragments()
        constraints = [
            self.fixed((parts, (f"Face{number + 1}",))) for number in range(len(parts.Shape.Faces))
        ]
        materials = []
        for solid in ("Solid1", "Solid2"):
            material = ObjectsFem.makeMaterialSolid(self.document, "Material")
            material.References = [(parts, (solid,))]
            materials.append(material)
        femmesh = self.run_mesh(parts, constraints + materials).FemMesh
        for constraint, face in zip(constraints, parts.Shape.Faces):
            self.assertEqual(self.read_nodes(femmesh, constraint), self.search(femmesh, face))
        femobjects = [{"Object": material} for material in materials]
        self.assertTrue(meshtools.get_femelement_sets_from_group_data(femmesh, femobjects))
        for femobject, solid in zip(femobjects, parts.Shape.Solids):
            nodes = set()
            for volume in femobject["FEMElements"]:
                nodes.update(femmesh.getElementNodes(volume))
            self.assertEqual(sorted(nodes), self.search(femmesh, solid))

    def test_a_member_named_like_the_start_of_another_reads_its_own_group(self):
        longer = ObjectsFem.makeConstraintFixed(self.document, "Fixed_1")
        longer.References = [(self.box, ("Face2",))]
        shorter = self.fixed((self.box, ("Face1",)))
        femmesh = self.run_mesh(self.box, [longer, shorter]).FemMesh
        self.assertEqual(
            self.read_nodes(femmesh, longer), self.search(femmesh, self.box.Shape.Face2)
        )
        self.assertEqual(
            self.read_nodes(femmesh, shorter), self.search(femmesh, self.box.Shape.Face1)
        )

    def test_a_coincident_face_of_a_compound_reads_its_own_group(self):
        compound = self.document.addObject("Part::Compound", "Compound")
        compound.Links = [self.box, self.make_box("Right", 10)]
        self.document.recompute()
        faces = compound.Shape.Faces
        twins = [i + 1 for i, face in enumerate(faces) if abs(face.CenterOfMass.x - 10) < 1e-9]
        self.assertEqual(len(twins), 2)
        name = f"Face{twins[1]}"
        constraint = self.fixed((compound, (name,)))
        femmesh = self.run_mesh(compound, [constraint], CoherenceMesh=False).FemMesh
        ids = {femmesh.getGroupName(group): group for group in femmesh.Groups}
        nodes = set()
        for element in femmesh.getGroupElements(ids[name]):
            nodes.update(femmesh.getElementNodes(element))
        self.assertEqual(self.read_nodes(femmesh, constraint), sorted(nodes))

    def test_a_material_without_elements_next_to_an_empty_one_has_no_group(self):
        small, parts = self.fragments()
        first = ObjectsFem.makeMaterialSolid(self.document, "First")
        first.References = [(small, ("Solid1",))]
        second = ObjectsFem.makeMaterialSolid(self.document, "Second")
        constraint = self.fixed((parts, ("Face1",)))
        femmesh = self.run_mesh(parts, [first, second, constraint]).FemMesh
        self.assertEqual(self.groups_of(femmesh, first), {})
        self.assertEqual(self.groups_of(femmesh, second), {})
        self.assertEqual(self.groups_of(femmesh, constraint), {"_Faces": "Face", "_Nodes": "Node"})
        # The end of the gmsh run hides an exception, ask without it
        self.assertEqual(
            meshtools.get_analysis_group_elements(first.getParentGroup(), parts),
            {first.Name: [], constraint.Name: ["Face1"]},
        )

    def test_setting_off_makes_no_group(self):
        self.prefs.SetBool("AnalysisGroupMeshing", False)
        constraint = self.fixed((self.box, ("Face1",)))
        femmesh = self.run_mesh(self.box, [constraint]).FemMesh
        self.assert_left_to_the_search(femmesh, constraint, self.box.Shape.Face1)
