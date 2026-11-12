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

# Off the face or edge both ways, never right at the tolerance where the last bit would decide
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
    # Past the box by more than the limit no node is kept, the search filters by a box too
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

    def test_nodes_by_a_loose_vertex_or_edge(self):
        # A node within the own tolerance of a loose vertex or edge lies on every face meeting there
        box = Part.makeBox(10, 10, 10)
        corner = next(v for v in box.Vertexes if v.Point.Length < 1e-9)
        corner.fixTolerance(1e-4)
        rng = random.Random(1945)
        mesh = Fem.FemMesh()
        by_corner = (set(), set())
        for r in (2e-5, 5e-5, 9e-5, 5e-4):
            for _ in range(32):
                way = V(rng.gauss(0, 1), rng.gauss(0, 1), rng.gauss(0, 1)).normalize()
                # Out in the octant beyond the vertex, the vertex is the closest point
                p = corner.Point - V(abs(way.x), abs(way.y), abs(way.z)) * r
                by_corner[r > 1e-4].add(mesh.addNode(p.x, p.y, p.z))
        # Off the bottom face by half the loose tolerance, far from the loose vertex
        by_corner[1].add(mesh.addNode(5.0, 5.0, -5e-5))
        # In every box but out of the tolerance, the loose vertex turns them away, right at it is on
        by_corner[1].add(mesh.addNode(*(V(-1, -1, -1).normalize() * 1.5e-4)))
        by_corner[1].add(mesh.addNode(-5e-5, 5.0, 0.0))
        by_corner[0].add(mesh.addNode(-1e-4, 0.0, 0.0))
        solid = box.Solids[0]
        with self.subTest(element="Solid"):
            # The solid takes its loosest tolerance everywhere, its box must reach as far
            limit = solid.getTolerance(1)
            kept = {
                n for n, p in mesh.Nodes.items() if solid.distToShape(Part.Vertex(p))[0] < limit
            }
            self.assertEqual(set(mesh.getNodesBySolid(solid)), kept)
        loose = next(
            e
            for e in box.Edges
            if e.BoundBox.ZLength > 1 and e.BoundBox.XMax < 1e-9 and e.BoundBox.YMax < 1e-9
        )
        loose.fixTolerance(1e-4)
        by_edge = (set(), set())
        for r in (2e-5, 9e-5, 1.2e-4, 5e-4):
            by_edge[r > 1e-4].add(mesh.addNode(-r * 0.6, -r * 0.8, 5.0))
        cases = [
            (by_corner, lambda s: any(v.isSame(corner) for v in s.Vertexes)),
            (by_edge, lambda s: s.isSame(loose) or any(e.isSame(loose) for e in s.Edges)),
        ]
        for (on, off), meets in cases:
            elements = [f for f in box.Faces if meets(f)] + [e for e in box.Edges if meets(e)]
            for index, element in enumerate(elements, 1):
                with self.subTest(element=f"{element.ShapeType}{index}", nodes=len(on)):
                    if element.ShapeType == "Face":
                        found = set(mesh.getNodesByFace(element))
                    else:
                        found = set(mesh.getNodesByEdge(element))
                    self.assertEqual((len(on - found), len(off & found)), (0, 0))
        # Right at the tolerance of an ordinary vertex too, the one every face and edge shares
        plain = Part.makeBox(10, 10, 10)
        tip = next(v for v in plain.Vertexes if v.Point.Length < 1e-9)
        mesh = Fem.FemMesh()
        node = mesh.addNode(-tip.Tolerance, 0.0, 0.0)
        with self.subTest(element="ordinary vertex"):
            self.assertEqual(mesh.getNodesByVertex(tip), [node])
            for element in plain.Faces + plain.Edges:
                if any(v.isSame(tip) for v in element.Vertexes):
                    if element.ShapeType == "Face":
                        self.assertEqual(mesh.getNodesByFace(element), [node])
                    else:
                        self.assertEqual(mesh.getNodesByEdge(element), [node])
        # A vertex left off the curve of its edge, within its own tolerance
        line = Part.makeLine(V(0, 0, 0), V(10, 0, 0))
        shifted = Part.Vertex(V(0, 8e-5, 0))
        shifted.fixTolerance(1e-4)
        edge = line.replaceShape([(line.Vertexes[0], shifted)])
        mesh = Fem.FemMesh()
        node = mesh.addNode(0, 1.5e-4, 0)
        with self.subTest(element="shifted vertex"):
            self.assertEqual(mesh.getNodesByEdge(edge), [node])
        # A seam moved 8e-5 off its sphere in the BRep text, where the radius 4 ends the curves
        brep = Part.makeSphere(4).exportBrepToString()
        self.assertEqual(brep.count("0 0 1 4\nPolygon3D"), 1)
        sphere = Part.Shape()
        sphere.importBrepFromString(brep.replace("0 0 1 4\nPolygon3D", "0 0 1 4.00008\nPolygon3D"))
        for part in sphere.Edges + sphere.Vertexes:
            part.fixTolerance(1e-4)
        self.assertTrue(sphere.isValid())
        mesh = Fem.FemMesh()
        node = mesh.addNode(4.00015, 0.0, 0.0)
        for element, search in (
            ("seam face", mesh.getNodesByFace),
            ("seam solid", mesh.getNodesBySolid),
        ):
            with self.subTest(element=element):
                shape = sphere.Faces[0] if element == "seam face" else sphere.Solids[0]
                self.assertEqual(search(shape), [node])

    def test_nodes_out_of_a_short_box(self):
        # BRepBndLib::Add cuts the box of a fillet torus patch short by 0.3, a whole torus keeps it
        solid = Part.makeBox(11.7, 9.65, 7.44)
        cone = Part.makeCone(2.70, 0.322, 2.11, V(6.04, 3.25, 6.58))
        cone.rotate(cone.BoundBox.Center, V(-0.271, 1.19, -1.12).normalize(), 165.0)
        solid = solid.fuse(cone).removeSplitter().Solids[0]
        rim = [
            e
            for e in solid.Edges
            if isinstance(e.Curve, Part.Circle) and abs(e.Curve.Radius - 2.70) < 1e-9
        ]
        solid = solid.makeFillet(0.790, rim).Solids[0]
        faces = [f for f in solid.Faces if isinstance(f.Surface, Part.Toroid)]
        self.assertTrue(faces and all(f.Surface.MajorRadius < f.Surface.MinorRadius for f in faces))
        nurbs = Part.makeSphere(4, V(1, 2, 3)).toNurbs()

        def out_of(bound, points):
            # How far the furthest point lies out of a box, 0 inside
            return max(
                max(
                    bound.XMin - p.x,
                    p.x - bound.XMax,
                    bound.YMin - p.y,
                    p.y - bound.YMax,
                    bound.ZMin - p.z,
                    p.z - bound.ZMax,
                    0.0,
                )
                for p in points
            )

        # A plain torus turned askew, and one offset from another : Add cuts their patch boxes short
        patches = []
        for big, offset in ((2, 0.0), (3, 0.3)):
            turned = Part.Toroid()
            # The setter wants R above r at every step
            turned.MajorRadius = 100
            turned.MinorRadius = 1
            turned.MajorRadius = big
            turned.Axis = V(1, 2, 3).normalize()
            surface = Part.OffsetSurface(turned, offset) if offset else turned
            patch = surface.toShape(0, 6, 5, 6)
            grid = [
                patch.valueAt(6 * i / 40, 5 + j / 40) for i in range(2, 39) for j in range(2, 39)
            ]
            self.assertGreater(out_of(patch.BoundBox, grid), 1e-4)
            patches.append((patch, None))
        cases = [(f, solid) for f in faces] + [(nurbs.Faces[0], nurbs.Solids[0])] + patches
        for face, solid in cases:
            u0, u1, v0, v1 = face.ParameterRange
            mesh = Fem.FemMesh()
            nodes = set()
            # Inside the face only, its border is a matter for the measure, not for the box
            for i in range(2, 39):
                for j in range(2, 39):
                    u = u0 + (u1 - u0) * i / 40
                    v = v0 + (v1 - v0) * j / 40
                    if face.isPartOfDomain(u, v):
                        nodes.add(mesh.addNode(*face.valueAt(u, v)))
            # BRepExtrema prunes by the same short box, every node it keeps the box keeps too
            searches = [(face, face.Tolerance, mesh.getNodesByFace)]
            if solid is not None:
                searches.append((solid, solid.getTolerance(1), mesh.getNodesBySolid))
            for shape, limit, search in searches:
                with self.subTest(surface=type(face.Surface).__name__, element=shape.ShapeType):
                    found = set(search(shape))
                    kept = {
                        n for n in nodes if shape.distToShape(Part.Vertex(mesh.Nodes[n]))[0] < limit
                    }
                    self.assertGreater(len(kept), 0.9 * len(nodes))
                    self.assertEqual(kept - found, set())
        # A parabola sketched flat then placed askew : Add misses where it goes furthest
        flat = Part.Parabola()
        flat.Focal = 2.0
        arc = Part.Edge(flat, -3.0, 3.0)
        arc.Placement = FreeCAD.Placement(V(1, 1, 1), FreeCAD.Rotation(V(1, 1, 0), 35))
        laid = [arc.valueAt(-3.0 + 6.0 * i / 200) for i in range(1, 200)]
        self.assertGreater(out_of(arc.BoundBox, laid), 1e-4)
        mesh = Fem.FemMesh()
        nodes = {mesh.addNode(p.x, p.y, p.z) for p in laid}
        with self.subTest(curve="placed parabola"):
            self.assertEqual(nodes - set(mesh.getNodesByEdge(arc)), set())

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
