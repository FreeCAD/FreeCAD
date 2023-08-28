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

__title__ = "Mesh FEM unit tests"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

import unittest
from os.path import join

import FreeCAD

import Fem
from . import support_utils as testtools
from .support_utils import fcc_print


class TestMeshCommon(unittest.TestCase):
    fcc_print("import TestMeshCommon")

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

        fcc_print("\n{0}\n{1} run FEM TestMeshCommon tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            60 * "*"
        ))

    # ********************************************************************************************
    def test_mesh_seg2_python(
        self
    ):
        seg2 = Fem.FemMesh()
        seg2.addNode(0, 0, 0, 1)
        seg2.addNode(2, 0, 0, 2)
        seg2.addNode(4, 0, 0, 3)
        seg2.addEdge([1, 2])
        seg2.addEdge([2, 3], 2)

        node_data = [
            seg2.NodeCount,
            seg2.Nodes
        ]
        edge_data = [
            seg2.EdgeCount,
            seg2.Edges[0],
            seg2.getElementNodes(seg2.Edges[0]),
            seg2.Edges[1],
            seg2.getElementNodes(seg2.Edges[1])
        ]
        expected_nodes = [
            3,
            {
                1: FreeCAD.Vector(0.0, 0.0, 0.0),
                2: FreeCAD.Vector(2.0, 0.0, 0.0),
                3: FreeCAD.Vector(4.0, 0.0, 0.0)
            }
        ]
        expected_edges = [2, 1, (1, 2), 2, (2, 3)]
        self.assertEqual(
            node_data,
            expected_nodes,
            "Nodes of Python created seg2 element are unexpected"
        )
        self.assertEqual(
            edge_data, expected_edges,
            "Edges of Python created seg2 element are unexpected"
        )

    # ********************************************************************************************
    def test_mesh_seg3_python(
        self
    ):
        seg3 = Fem.FemMesh()
        seg3.addNode(0, 0, 0, 1)
        seg3.addNode(1, 0, 0, 2)
        seg3.addNode(2, 0, 0, 3)
        seg3.addNode(3, 0, 0, 4)
        seg3.addNode(4, 0, 0, 5)
        seg3.addEdge([1, 3, 2])
        seg3.addEdge([3, 5, 4], 2)

        node_data = [seg3.NodeCount, seg3.Nodes]
        edge_data = [
            seg3.EdgeCount,
            seg3.Edges[0],
            seg3.getElementNodes(seg3.Edges[0]),
            seg3.Edges[1],
            seg3.getElementNodes(seg3.Edges[1])
        ]
        expected_nodes = [
            5, {
                1: FreeCAD.Vector(0.0, 0.0, 0.0),
                2: FreeCAD.Vector(1.0, 0.0, 0.0),
                3: FreeCAD.Vector(2.0, 0.0, 0.0),
                4: FreeCAD.Vector(3.0, 0.0, 0.0),
                5: FreeCAD.Vector(4.0, 0.0, 0.0)
            }
        ]
        expected_edges = [2, 1, (1, 3, 2), 2, (3, 5, 4)]
        self.assertEqual(
            node_data,
            expected_nodes,
            "Nodes of Python created seg3 element are unexpected"
        )
        self.assertEqual(
            edge_data,
            expected_edges,
            "Edges of Python created seg3 element are unexpected"
        )

    # ********************************************************************************************
    def test_unv_save_load(
        self
    ):
        tetra10 = Fem.FemMesh()
        tetra10.addNode(6, 12, 18, 1)
        tetra10.addNode(0, 0, 18, 2)
        tetra10.addNode(12, 0, 18, 3)
        tetra10.addNode(6, 6, 0, 4)

        tetra10.addNode(3, 6, 18, 5)
        tetra10.addNode(6, 0, 18, 6)
        tetra10.addNode(9, 6, 18, 7)

        tetra10.addNode(6, 9, 9, 8)
        tetra10.addNode(3, 3, 9, 9)
        tetra10.addNode(9, 3, 9, 10)
        tetra10.addVolume([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])

        unv_file = join(testtools.get_fem_test_tmp_dir("mesh_common_unv_save"), "tetra10_mesh.unv")
        tetra10.write(unv_file)
        newmesh = Fem.read(unv_file)
        expected = (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
        self.assertEqual(
            newmesh.getElementNodes(1),
            expected,
            "Nodes order of quadratic volume element is unexpected"
        )

    # ********************************************************************************************
    def test_writeAbaqus_precision(
        self
    ):
        # https://forum.freecad.org/viewtopic.php?f=18&t=22759#p176669
        # ccx reads only F20.0 (i. e. Fortran floating point field 20 chars wide)
        # thus precision is set to 13 in writeAbaqus
        seg2 = Fem.FemMesh()
        seg2.addNode(0, 0, 0, 1)
        seg2.addNode(
            # 3456789012345678901234567
            -5000000000000000000.1,
            -1.123456789123456e-14,
            -0.1234567890123456789e-101,
            2
        )
        seg2.addEdge([1, 2])

        inp_file = join(testtools.get_fem_test_tmp_dir("mesh_common_inp_preci"), "seg2_mesh.inp")
        seg2.writeABAQUS(inp_file, 1, False)

        read_file = open(inp_file, "r")
        read_node_line = "line was not found"
        for ln in read_file:
            ln = ln.strip()
            if ln.startswith("2, -5"):
                read_node_line = ln
        read_file.close()

        #                  1234567  12345678901234567890  12345678901234567890
        expected_win = "2, -5e+018, -1.123456789123e-014, -1.234567890123e-102"
        expected_lin = "2, -5e+18, -1.123456789123e-14, -1.234567890123e-102"
        expected = [expected_lin, expected_win]
        self.assertTrue(
            True if read_node_line in expected else False,
            "Problem in test_writeAbaqus_precision, \n{0}\n{1}".format(
                read_node_line,
                expected
            )
        )


# ************************************************************************************************
# ************************************************************************************************
class TestMeshEleTetra10(unittest.TestCase):
    fcc_print("import TestMeshEleTetra10")

    # ********************************************************************************************
    def setUp(
        self
    ):
        # setUp is executed before every test

        # new document
        self.document = FreeCAD.newDocument(self.__class__.__name__)

        # more inits
        self.elem = "tetra10"

        self.base_testfile = join(
            testtools.get_fem_test_home_dir(),
            "mesh",
            (self.elem + "_mesh.")
        )

        # 10 node tetrahedron --> tetra10
        femmesh = Fem.FemMesh()
        femmesh.addNode(6, 12, 18, 1)
        femmesh.addNode(0, 0, 18, 2)
        femmesh.addNode(12, 0, 18, 3)
        femmesh.addNode(6, 6, 0, 4)
        femmesh.addNode(3, 6, 18, 5)
        femmesh.addNode(6, 0, 18, 6)
        femmesh.addNode(9, 6, 18, 7)
        femmesh.addNode(6, 9, 9, 8)
        femmesh.addNode(3, 3, 9, 9)
        femmesh.addNode(9, 3, 9, 10)
        femmesh.addVolume([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
        self.femmesh = femmesh
        self.expected_nodes = {
            "count": 10,
            "nodes": {
                1: FreeCAD.Vector(6.0, 12.0, 18.0),
                2: FreeCAD.Vector(0.0, 0.0, 18.0),
                3: FreeCAD.Vector(12.0, 0.0, 18.0),
                4: FreeCAD.Vector(6.0, 6.0, 0.0),
                5: FreeCAD.Vector(3.0, 6.0, 18.0),
                6: FreeCAD.Vector(6.0, 0.0, 18.0),
                7: FreeCAD.Vector(9.0, 6.0, 18.0),
                8: FreeCAD.Vector(6.0, 9.0, 9.0),
                9: FreeCAD.Vector(3.0, 3.0, 9.0),
                10: FreeCAD.Vector(9.0, 3.0, 9.0),
            }
        }
        self.expected_elem = {
            "volcount": 1,
            "tetcount": 1,
            "volumes": [1, (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)]
        }
        """
        fcc_print("\n")
        fcc_print(expected_nodes)
        fcc_print(expected_elem)
        fcc_print("\n")
        """

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

        fcc_print("\n{0}\n{1} run FEM TestMeshEleTetra10 tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            56 * "*"
        ))

    # ********************************************************************************************
    def get_file_paths(
        self,
        file_extension
    ):
        testfile = self.base_testfile + file_extension
        outfile = join(
            testtools.get_fem_test_tmp_dir("mesh_elements_" + self.elem + "_" + file_extension),
            self.elem + "_mesh." + file_extension
        )

        # fcc_print("\n")
        # fcc_print(outfile)
        # fcc_print(testfile)
        return (outfile, testfile)

    # ********************************************************************************************
    def compare_mesh_files(
        self,
        femmesh_testfile,
        femmesh_outfile,
        filetyp
    ):

        # """
        fcc_print([
            femmesh_testfile.Volumes[0],
            femmesh_testfile.getElementNodes(femmesh_outfile.Volumes[0])
        ])
        # """

        # test reading the test mesh
        self.assertEqual(
            femmesh_testfile.Nodes,
            self.expected_nodes["nodes"],
            "Test reading {} mesh to {} file failed. Nodes are different.\n".format(
                self.elem,
                filetyp
            )
        )
        self.assertEqual(
            [
                femmesh_testfile.Volumes[0],
                femmesh_testfile.getElementNodes(femmesh_outfile.Volumes[0])
            ],
            self.expected_elem["volumes"],
            "Test reading {} mesh to {} file failed. Volumes are different.\n".format(
                self.elem,
                filetyp
            )
        )
        # test reading the written mesh
        self.assertEqual(
            femmesh_outfile.Nodes,
            self.expected_nodes["nodes"],
            "Test reading {} mesh to {} file failed. Nodes are different.\n".format(
                self.elem,
                filetyp
            )
        )
        self.assertEqual(
            [
                femmesh_outfile.Volumes[0],
                femmesh_outfile.getElementNodes(femmesh_outfile.Volumes[0])
            ],
            self.expected_elem["volumes"],
            "Test reading {} mesh to {} file failed. Volumes are different.\n".format(
                self.elem,
                filetyp
            )
        )
        # test if both are equal
        self.assertEqual(
            femmesh_outfile.Nodes,
            femmesh_testfile.Nodes,
            "Test reading {} mesh to {} file failed. Nodes are different.\n".format(
                self.elem,
                filetyp
            )
        )
        self.assertEqual(
            femmesh_outfile.Volumes,
            femmesh_testfile.Volumes,
            "Test reading {} mesh to {} file failed. Volumes are different.\n".format(
                self.elem,
                filetyp
            )
        )

    # ********************************************************************************************
    def test_tetra10_create(
        self
    ):
        # tetra10 element: creating by Python
        node_data = {
            "count": self.femmesh.NodeCount,
            "nodes": self.femmesh.Nodes
        }
        elem_data = {
            "volcount": self.femmesh.VolumeCount,
            "tetcount": self.femmesh.TetraCount,
            "volumes": [
                self.femmesh.Volumes[0],
                self.femmesh.getElementNodes(self.femmesh.Volumes[0])
            ]
        }
        self.assertEqual(
            node_data,
            self.expected_nodes,
            "Nodes of Python created " + self.elem + "mesh element are unexpected"
        )
        self.assertEqual(
            elem_data,
            self.expected_elem,
            "Elements of Python created " + self.elem + "mesh element are unexpected"
        )
        """
        obj = doc.addObject("Fem::FemMeshObject" , elem)
        obj.FemMesh = femmesh
        obj.Placement.Base = (30,50,0)
        obj.ViewObject.DisplayMode = "Faces, Wireframe & Nodes"
        """

    # ********************************************************************************************
    def test_tetra10_inp(
        self
    ):
        # tetra10 element: reading from and writing to inp mesh file format

        file_extension = "inp"
        outfile, testfile = self.get_file_paths(file_extension)

        self.femmesh.writeABAQUS(outfile, 1, False)  # write the mesh
        femmesh_outfile = Fem.read(outfile)  # read the mesh from written mesh
        femmesh_testfile = Fem.read(testfile)  # read the mesh from test mesh

        self.compare_mesh_files(
            femmesh_testfile,
            femmesh_outfile,
            file_extension
        )

    # ********************************************************************************************
    def test_tetra10_unv(
        self
    ):
        # tetra10 element: reading from and writing to unv mesh file format

        file_extension = "unv"
        outfile, testfile = self.get_file_paths(file_extension)

        self.femmesh.write(outfile)  # write the mesh
        femmesh_outfile = Fem.read(outfile)  # read the mesh from written mesh
        femmesh_testfile = Fem.read(testfile)  # read the mesh from test mesh

        self.compare_mesh_files(
            femmesh_testfile,
            femmesh_outfile,
            file_extension
        )

    # ********************************************************************************************
    def test_tetra10_vkt(
        self
    ):
        # tetra10 element: reading from and writing to unv mesh file format

        file_extension = "vtk"
        outfile, testfile = self.get_file_paths(file_extension)

        if "BUILD_FEM_VTK" in FreeCAD.__cmake__:
            self.femmesh.write(outfile)  # write the mesh
            femmesh_outfile = Fem.read(outfile)  # read the mesh from written mesh
            femmesh_testfile = Fem.read(testfile)  # read the mesh from test mesh

            self.compare_mesh_files(
                femmesh_testfile,
                femmesh_outfile,
                file_extension
            )
        else:
            fcc_print("FEM_VTK post processing is disabled.")

    # ********************************************************************************************
    def test_tetra10_yml(
        self
    ):
        # tetra10 element: reading from and writing to yaml/json mesh file format

        file_extension = "yml"
        outfile, testfile = self.get_file_paths(file_extension)

        # TODO: implement yaml/json mesh reader writer method calls in C++
        # self.femmesh.write(outfile)  # write the mesh
        # femmesh_testfile = Fem.read(outfile)  # read the mesh from written mesh
        # femmesh_outfile = Fem.read(testfile)  # read the mesh from test mesh
        # directly use Python methods to read and write files
        from feminout.importYamlJsonMesh import write
        write(outfile, self.femmesh)
        from feminout.importYamlJsonMesh import read
        femmesh_testfile = read(outfile)
        femmesh_outfile = read(testfile)

        self.compare_mesh_files(
            femmesh_testfile,
            femmesh_outfile,
            file_extension
        )

    # ********************************************************************************************
    def test_tetra10_z88(
        self
    ):
        # tetra10 element: reading from and writing to z88 mesh file format

        file_extension = "z88"
        outfile, testfile = self.get_file_paths(file_extension)

        self.femmesh.write(outfile)  # write the mesh
        femmesh_testfile = Fem.read(outfile)  # read the mesh from written mesh
        femmesh_outfile = Fem.read(testfile)  # read the mesh from test mesh

        self.compare_mesh_files(
            femmesh_testfile,
            femmesh_outfile,
            file_extension
        )


# ************************************************************************************************
# ************************************************************************************************
# TODO: add elements to group with another type. Should be empty at the end.
class TestMeshGroups(unittest.TestCase):
    fcc_print("import TestMeshGroups")

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

        fcc_print("\n{0}\n{1} run FEM TestMeshGroups tests {2}\n{0}".format(
            100 * "*",
            10 * "*",
            57 * "*"
        ))

    # ********************************************************************************************
    def test_add_groups(self):
        """
        Create different groups with different names. Check whether the
        ids are correct, the names are correct, and whether the GroupCount is
        correct.
        """

        from femexamples.meshes.mesh_canticcx_tetra10 import create_elements
        from femexamples.meshes.mesh_canticcx_tetra10 import create_nodes

        fm = Fem.FemMesh()
        control = create_nodes(fm)
        if not control:
            fcc_print("failed to create nodes")
        control = create_elements(fm)
        if not control:
            fcc_print("failed to create elements")

        # information
        # fcc_print(fm)

        expected_dict = {}
        expected_dict["ids"] = []
        expected_dict["names"] = [
            "MyNodeGroup",
            "MyEdgeGroup",
            "MyVolumeGroup",
            "My0DElementGroup",
            "MyBallGroup"
        ]
        expected_dict["types"] = [
            "Node",
            "Edge",
            "Volume",
            "0DElement",
            "Ball"
        ]
        expected_dict["count"] = fm.GroupCount + 5
        result_dict = {}

        mygrpids = []
        for (name, typ) in zip(expected_dict["names"], expected_dict["types"]):
            mygrpids.append(fm.addGroup(name, typ))

        expected_dict["ids"] = sorted(tuple(mygrpids))

        # fcc_print("expected dict")
        # fcc_print(expected_dict)

        result_dict["count"] = fm.GroupCount
        result_dict["ids"] = sorted(fm.Groups)
        result_dict["types"] = list([fm.getGroupElementType(g)
                                     for g in fm.Groups])
        result_dict["names"] = list([fm.getGroupName(g) for g in fm.Groups])

        # fcc_print("result dict")
        # fcc_print(result_dict)

        self.assertEqual(
            expected_dict,
            result_dict,
            msg="expected: {0}\n\nresult: {1}\n\n differ".format(expected_dict, result_dict)
        )

    def test_delete_groups(self):
        """
        Adds a number of groups to FemMesh and deletes them
        afterwards. Checks whether GroupCount is OK
        """
        from femexamples.meshes.mesh_canticcx_tetra10 import create_elements
        from femexamples.meshes.mesh_canticcx_tetra10 import create_nodes

        fm = Fem.FemMesh()
        control = create_nodes(fm)
        if not control:
            fcc_print("failed to create nodes")
        control = create_elements(fm)
        if not control:
            fcc_print("failed to create elements")

        # information
        # fcc_print(fm)
        old_group_count = fm.GroupCount
        myids = []
        for i in range(1000):
            myids.append(fm.addGroup("group" + str(i), "Node"))
        for grpid in myids:
            fm.removeGroup(grpid)
        new_group_count = fm.GroupCount
        self.assertEqual(
            old_group_count,
            new_group_count,
            msg=(
                "GroupCount before and after adding and deleting groups differ: {0} != {1}"
                .format(old_group_count, new_group_count)
            )
        )

    def test_add_group_elements(self):
        """
        Add a node group, add elements to it. Verify that elements added
        and elements in getGroupElements are the same.
        """
        from femexamples.meshes.mesh_canticcx_tetra10 import create_elements
        from femexamples.meshes.mesh_canticcx_tetra10 import create_nodes

        fm = Fem.FemMesh()
        control = create_nodes(fm)
        if not control:
            fcc_print("failed to create nodes")
        control = create_elements(fm)
        if not control:
            fcc_print("failed to create elements")

        # information
        # fcc_print(fm)

        elements_to_be_added = [1, 2, 3, 4, 49, 64, 88, 100, 102, 188, 189, 190, 191]
        myid = fm.addGroup("mynodegroup", "Node")

        # fcc_print(fm.getGroupElements(myid))

        fm.addGroupElements(myid, elements_to_be_added)
        elements_returned = list(fm.getGroupElements(myid))  # returns tuple
        # fcc_print(elements_returned)
        self.assertEqual(
            elements_to_be_added,
            elements_returned,
            msg=(
                "elements to be added {0} and elements returned {1} differ".
                format(elements_to_be_added, elements_returned)
            )
        )
