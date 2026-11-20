# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Valentin Pohu
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

__title__ = "FEM mesh node search unit tests"

import random
import unittest

import FreeCAD

import Fem
import Part
from .support_utils import fcc_print

V = FreeCAD.Vector

# Off the face or edge a node is laid for, both ways, never at its tolerance itself where the
# last bit of 2 measures owuld decide. Under 1e-3 inside a solid its classifier answers ON and
# the distance to the boundary decides, which keeps such a node out
TOLERANCE_FRACTIONS = (0.3, 3.0)
OFFSETS = (1e-4, 5e-4, 2e-3)


def shapes():
    bitten = Part.makeBox(10, 10, 10).cut(Part.makeCylinder(3, 12, V(5, 5, -1)))
    cone = Part.makeCone(8, 0, 10, V(0, 0, 0), V(0, 0, 1), 240).toNurbs()
    sphere = Part.makeSphere(4, V(1, 2, 3))
    return {"bitten box": bitten, "NURBS cone sector": cone, "sphere": sphere}


def nodes_around(shape, mesh, per_face=2, scattered=30):
    """Nodes on each face and edge and off them both ways, then scattered through the box"""
    rng = random.Random(7)
    loosest = shape.getTolerance(1)
    offsets = [f * loosest for f in TOLERANCE_FRACTIONS] + list(OFFSETS)
    for face in shape.Faces:
        u0, u1, v0, v1 = face.ParameterRange
        for i in range(per_face):
            for j in range(per_face):
                u = u0 + (u1 - u0) * (i + 0.5) / per_face
                v = v0 + (v1 - v0) * (j + 0.5) / per_face
                if not face.isPartOfDomain(u, v):
                    continue
                point = face.valueAt(u, v)
                normal = face.normalAt(u, v)
                mesh.addNode(*point)
                for offset in offsets:
                    mesh.addNode(*(point + normal * offset))
                    mesh.addNode(*(point - normal * offset))
    for edge in shape.Edges:
        if edge.Degenerated:
            continue
        for i in range(per_face + 1):
            t = edge.FirstParameter + (edge.LastParameter - edge.FirstParameter) * i / per_face
            point = edge.valueAt(t)
            along = edge.tangentAt(t)
            mesh.addNode(*point)
            for offset in offsets:
                way = V(rng.gauss(0, 1), rng.gauss(0, 1), rng.gauss(0, 1))
                # Square to the edge, the noe sit off it by the offset itself
                way = way - along * way.dot(along)
                mesh.addNode(*(point + way.normalize() * offset))
    box = shape.BoundBox
    for _ in range(scattered):
        mesh.addNode(
            rng.uniform(box.XMin, box.XMax),
            rng.uniform(box.YMin, box.YMax),
            rng.uniform(box.ZMin, box.ZMax),
        )


def closer_than(mesh, shape, limit):
    """The nodes closer than limit by OpenCASCADE's own distance, a fresh measure for each node"""
    # Past the box by more than the limit a node can't be kept, it saves the measure. The search
    # filters by a box as well, both keep every node closer than the limit
    box = shape.BoundBox
    box.enlarge(2 * limit)
    kept = set()
    for node, point in mesh.Nodes.items():
        if box.isInside(point) and shape.distToShape(Part.Vertex(point))[0] < limit:
            kept.add(node)
    return kept


class TestMeshNodeSearch(unittest.TestCase):
    fcc_print("import TestMeshNodeSearch")

    def setUp(self):
        self.document = FreeCAD.newDocument(self.__class__.__name__)

    def tearDown(self):
        FreeCAD.closeDocument(self.document.Name)

    def test_00print(self):
        fcc_print(
            "\n{0}\n{1} run FEM TestMeshNodeSearch tests {2}\n{0}".format(
                100 * "*", 10 * "*", 56 * "*"
            )
        )

    def test_nodes_match_dist_to_shape(self):
        # Measured once per node instead of twice, the search finds the very same nodes
        for name, shape in shapes().items():
            mesh = Fem.FemMesh()
            nodes_around(shape, mesh)
            self.check_searches(name, mesh, shape)

    def test_nodes_through_the_mesh_placement(self):
        # The nodes are measured where the mesh Placement puts them, turned and moved
        shape = shapes()["bitten box"]
        placement = FreeCAD.Placement(V(3, -2, 1), FreeCAD.Rotation(V(1, 1, 1), 30))
        back = placement.inverse()
        flat = Fem.FemMesh()
        nodes_around(shape, flat)
        mesh = Fem.FemMesh()
        for point in flat.Nodes.values():
            mesh.addNode(*back.multVec(point))
        mesh.Placement = placement
        self.check_searches("placed bitten box", mesh, shape)

    def check_searches(self, name, mesh, shape):
        solid = shape.Solids[0]
        with self.subTest(shape=name, element="solid"):
            # Pins the current result : nodes up to 1e-3 inside the solid are still left out
            expected = closer_than(mesh, solid, solid.getTolerance(1))
            self.assertEqual(set(mesh.getNodesBySolid(solid)), expected)
        for index, face in enumerate(shape.Faces, 1):
            with self.subTest(shape=name, element=f"Face{index}"):
                expected = closer_than(mesh, face, face.Tolerance)
                self.assertEqual(set(mesh.getNodesByFace(face)), expected)
        for index, edge in enumerate(shape.Edges, 1):
            with self.subTest(shape=name, element=f"Edge{index}"):
                expected = closer_than(mesh, edge, edge.Tolerance)
                self.assertEqual(set(mesh.getNodesByEdge(edge)), expected)
