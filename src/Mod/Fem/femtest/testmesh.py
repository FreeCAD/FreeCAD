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


import Fem
import FreeCAD
import unittest
from . import testtools
from .testtools import fcc_print


class FemMeshTest(unittest.TestCase):

    def setUp(self):
        try:
            FreeCAD.setActiveDocument("FemTest")
        except:
            FreeCAD.newDocument("FemTest")
        finally:
            FreeCAD.setActiveDocument("FemTest")
        self.active_doc = FreeCAD.ActiveDocument

    def test_mesh_seg2_python(self):
        seg2 = Fem.FemMesh()
        seg2.addNode(0, 0, 0, 1)
        seg2.addNode(2, 0, 0, 2)
        seg2.addNode(4, 0, 0, 3)
        seg2.addEdge([1, 2])
        seg2.addEdge([2, 3], 2)

        node_data = [seg2.NodeCount, seg2.Nodes]
        edge_data = [seg2.EdgeCount, seg2.Edges[0], seg2.getElementNodes(seg2.Edges[0]), seg2.Edges[1], seg2.getElementNodes(seg2.Edges[1])]
        expected_nodes = [3, {1: FreeCAD.Vector(0.0, 0.0, 0.0), 2: FreeCAD.Vector(2.0, 0.0, 0.0), 3: FreeCAD.Vector(4.0, 0.0, 0.0)}]
        expected_edges = [2, 1, (1, 2), 2, (2, 3)]
        self.assertEqual(node_data, expected_nodes, "Nodes of Python created seg2 element are unexpected")
        self.assertEqual(edge_data, expected_edges, "Edges of Python created seg2 element are unexpected")

    def test_mesh_seg3_python(self):
        seg3 = Fem.FemMesh()
        seg3.addNode(0, 0, 0, 1)
        seg3.addNode(1, 0, 0, 2)
        seg3.addNode(2, 0, 0, 3)
        seg3.addNode(3, 0, 0, 4)
        seg3.addNode(4, 0, 0, 5)
        seg3.addEdge([1, 3, 2])
        seg3.addEdge([3, 5, 4], 2)

        node_data = [seg3.NodeCount, seg3.Nodes]
        edge_data = [seg3.EdgeCount, seg3.Edges[0], seg3.getElementNodes(seg3.Edges[0]), seg3.Edges[1], seg3.getElementNodes(seg3.Edges[1])]
        expected_nodes = [5, {1: FreeCAD.Vector(0.0, 0.0, 0.0), 2: FreeCAD.Vector(1.0, 0.0, 0.0), 3: FreeCAD.Vector(2.0, 0.0, 0.0), 4: FreeCAD.Vector(3.0, 0.0, 0.0), 5: FreeCAD.Vector(4.0, 0.0, 0.0)}]
        expected_edges = [2, 1, (1, 3, 2), 2, (3, 5, 4)]
        self.assertEqual(node_data, expected_nodes, "Nodes of Python created seg3 element are unexpected")
        self.assertEqual(edge_data, expected_edges, "Edges of Python created seg3 element are unexpected")

    def test_mesh_tetra10(self):
        # 10 node tetrahedron --> tetra10
        elem = 'tetra10'
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

        node_data = {
            'count': femmesh.NodeCount,
            'nodes': femmesh.Nodes
        }
        elem_data = {'volcount': femmesh.VolumeCount, 'tetcount': femmesh.TetraCount, 'volumes': {
            femmesh.Volumes[0], femmesh.getElementNodes(femmesh.Volumes[0]),
        }}
        expected_nodes = {'count': 10, 'nodes': {
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
        }}
        expected_elem = {'volcount': 1, 'tetcount': 1, 'volumes': {
            1, (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
        }}
        '''
        fcc_print('\n')
        fcc_print(elem_data)
        fcc_print(expected_elem)
        fcc_print('\n')
        '''
        self.assertEqual(node_data, expected_nodes, "Nodes of Python created " + elem + "mesh element are unexpected")
        self.assertEqual(elem_data, expected_elem, "Elements of Python created " + elem + "mesh element are unexpected")
        '''
        obj = doc.addObject("Fem::FemMeshObject" , elem)
        obj.FemMesh = femmesh
        obj.Placement.Base = (30,50,0)
        obj.ViewObject.DisplayMode = "Faces, Wireframe & Nodes"
        '''

        fcc_print(elem + 'export tests.\n')
        base_outfile = testtools.get_fem_test_tmp_dir() + '/' + elem + '_mesh.'
        base_testfile = testtools.get_fem_test_home_dir() + 'mesh/' + elem + '_mesh.'

        filetyp = 'inp'
        outfile = base_outfile + filetyp
        testfile = base_testfile + filetyp
        femmesh.writeABAQUS(outfile, 1, False)
        import feminout.importToolsFem
        import feminout.importInpMesh
        femmesh_outfile = feminout.importToolsFem.make_femmesh(feminout.importInpMesh.read_inp(outfile))
        femmesh_testfile = feminout.importToolsFem.make_femmesh(feminout.importInpMesh.read_inp(testfile))
        self.assertEqual(femmesh_outfile.Nodes, femmesh_testfile.Nodes, "Test writing " + elem + " mesh to " + filetyp + " file failed. Nodes are different.\n")
        self.assertEqual(femmesh_outfile.Volumes, femmesh_testfile.Volumes, "Test writing " + elem + " mesh to " + filetyp + " file failed. Volumes are different.\n")

        filetyp = 'unv'
        outfile = base_outfile + filetyp
        testfile = base_testfile + filetyp
        femmesh.write(outfile)
        femmesh_outfile = Fem.read(outfile)
        femmesh_testfile = Fem.read(testfile)
        self.assertEqual(femmesh_outfile.Nodes, femmesh_testfile.Nodes, "Test writing " + elem + " mesh to " + filetyp + " file failed. Nodes are different.\n")
        self.assertEqual(femmesh_outfile.Volumes, femmesh_testfile.Volumes, "Test writing " + elem + " mesh to " + filetyp + " file failed. Volumes are different.\n")

    def test_unv_save_load(self):
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

        unv_file = testtools.get_fem_test_tmp_dir() + '/tetra10_mesh.unv'
        tetra10.write(unv_file)
        newmesh = Fem.read(unv_file)
        expected = (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
        self.assertEqual(newmesh.getElementNodes(1), expected, "Nodes order of quadratic volume element is unexpected")

    def test_writeAbaqus_precision(self):
        # https://forum.freecadweb.org/viewtopic.php?f=18&t=22759#p176669
        # ccx reads only F20.0 (i. e. Fortran floating point field 20 chars wide)
        # thus precision is set to 13 in writeAbaqus
        seg2 = Fem.FemMesh()
        seg2.addNode(0, 0, 0, 1)
        #            1234567890123456789012  1234567890123456789012  123456789012345678901234567
        seg2.addNode(-5000000000000000000.1, -1.123456789123456e-14, -0.1234567890123456789e-101, 2)
        seg2.addEdge([1, 2])

        inp_file = testtools.get_fem_test_tmp_dir() + '/seg2_mesh.inp'
        seg2.writeABAQUS(inp_file, 1, False)

        read_file = open(inp_file, 'r')
        read_node_line = 'line was not found'
        for l in read_file:
            l = l.strip()
            if l.startswith('2, -5'):
                read_node_line = l
        read_file.close()

        #                  1234567  12345678901234567890  12345678901234567890
        expected_win = '2, -5e+018, -1.123456789123e-014, -1.234567890123e-102'
        expected_lin = '2, -5e+18, -1.123456789123e-14, -1.234567890123e-102'
        expected = [expected_lin, expected_win]
        self.assertTrue(True if read_node_line in expected else False,
                        "Problem in test_writeAbaqus_precision, \n{0}\n{1}".format(read_node_line, expected))

    def tearDown(self):
        FreeCAD.closeDocument("FemTest")
        pass
