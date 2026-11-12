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

import math
import os
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

# Exact checks : on the shape under TRUTH_IN tolerance, off it from 2 on, left out in between
TOL_FRACTIONS = (0.1, 0.2, 2.5, 4.0)
TRUTH_IN = 0.25
FAR_OFFSETS = (1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 0.1)
RADII = (3e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 0.1, 0.5)
# A needle : a cone of radius 0.01 and height 100, its apex at z = 100
NEEDLE_SLOPE = 1e-4


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


def wedge_gap(x, y, degrees):
    """Distance in the xy plane to the sector from 0 to degrees around z, 0 inside it"""
    if math.degrees(math.atan2(y, x)) % 360 <= degrees:
        return 0.0
    gap = float("inf")
    for a in (0.0, math.radians(degrees)):
        dx, dy = math.cos(a), math.sin(a)
        t = max(0.0, x * dx + y * dy)
        gap = min(gap, math.hypot(x - t * dx, y - t * dy))
    return gap


def outline_gap(x, y, polygon):
    """Distance to the outline of the polygon, inside or out"""
    gap = float("inf")
    for i in range(len(polygon)):
        x0, y0 = polygon[i]
        x1, y1 = polygon[(i + 1) % len(polygon)]
        dx, dy = x1 - x0, y1 - y0
        t = max(0.0, min(1.0, ((x - x0) * dx + (y - y0) * dy) / (dx * dx + dy * dy)))
        gap = min(gap, math.hypot(x - x0 - t * dx, y - y0 - t * dy))
    return gap


def polygon_gap(x, y, polygon):
    """0 inside the polygon, else the distance to its outline"""
    inside = False
    for i in range(len(polygon)):
        x0, y0 = polygon[i]
        x1, y1 = polygon[(i + 1) % len(polygon)]
        if ((y0 > y) != (y1 > y)) and x < x0 + (y - y0) * (x1 - x0) / (y1 - y0):
            inside = not inside
    return 0.0 if inside else outline_gap(x, y, polygon)


def prism(polygon, height):
    outline = [V(x, y, 0) for x, y in polygon] + [V(polygon[0][0], polygon[0][1], 0)]
    return Part.Face(Part.makePolygon(outline)).extrude(V(0, 0, height))


def needle_side_gap(p):
    """Exact distance to the side of the needle"""
    r = math.hypot(p.x, p.y)
    norm = math.sqrt(1 + NEEDLE_SLOPE**2)
    if 100.0 - p.z + NEEDLE_SLOPE * r <= 0.0:
        return math.hypot(r, p.z - 100.0)
    return abs(r - NEEDLE_SLOPE * (100.0 - p.z)) / norm


def by_circle_start(radius, tol):
    """Nodes by the seam vertex of a cylinder base circle, each with its exact distance"""

    nodes = []
    for s in (-1.5e-7, -9.9e-8, -9.5e-8, -9e-8, -5e-8, 5e-8, 9e-8, 9.5e-8, 9.9e-8, 1.5e-7):
        way = V(math.cos(s / radius), math.sin(s / radius), 0)
        for f in TOL_FRACTIONS:
            for tilt in (0.2, 0.7, 1.2):
                d = f * tol
                nodes.append((way * (radius + d * math.cos(tilt)) - V(0, 0, d * math.sin(tilt)), d))
    return nodes


def kinked(points, degree):
    """One B-spline through a polyline, straight between its points, C0 at each of them"""
    curve = Part.BSplineCurve()
    if degree == 1:
        mults = [2] + [1] * (len(points) - 2) + [2]
        curve.buildFromPolesMultsKnots(points, mults, list(range(len(points))), False, 1)
        return curve
    poles = [points[0]]
    knots = [0.0]
    for a, b in zip(points, points[1:]):
        poles += [a + (b - a) * f for f in (0.25, 0.6, 1.0)]
        knots.append(knots[-1] + (b - a).Length ** 0.7)
    mults = [4] + [3] * (len(points) - 2) + [4]
    curve.buildFromPolesMultsKnots(poles, mults, knots, False, 3)
    return curve


def kinked_section(section, degree):
    """A face in the xz plane, its outline one kinked B-spline from the third point on"""
    pts = [V(x, 0, z) for x, z in section]
    edges = [kinked(pts[2:], degree).toShape()]
    edges += [
        Part.makeLine(a, b) for a, b in ((pts[-1], pts[0]), (pts[0], pts[1]), (pts[1], pts[2]))
    ]
    return Part.Face(Part.Wire(edges))


def points_around(shape, seed):
    """Random points, points on and off every face, points all around every edge and vertex"""
    rng = random.Random(seed)
    tol = shape.getTolerance(1)
    offsets = FAR_OFFSETS + tuple(f * tol for f in TOL_FRACTIONS)
    radii = RADII + tuple(f * tol for f in TOL_FRACTIONS)
    points = []
    bound = shape.BoundBox
    for _ in range(600):
        points.append(
            V(
                rng.uniform(bound.XMin - 1, bound.XMax + 1),
                rng.uniform(bound.YMin - 1, bound.YMax + 1),
                rng.uniform(bound.ZMin - 1, bound.ZMax + 1),
            )
        )
    for face in shape.Faces:
        u0, u1, v0, v1 = face.ParameterRange
        for i in range(6):
            for j in range(6):
                u = u0 + (u1 - u0) * (i + rng.random()) / 6
                v = v0 + (v1 - v0) * (j + rng.random()) / 6
                if not face.isPartOfDomain(u, v):
                    continue
                point = face.valueAt(u, v)
                try:
                    normal = face.normalAt(u, v)
                except Part.OCCError:
                    continue
                points.append(point)
                for d in offsets:
                    points.append(point + normal * d)
                    points.append(point - normal * d)
    corners = [v.Point for v in shape.Vertexes]
    for edge in shape.Edges:
        if not edge.Degenerated:
            first, last = edge.ParameterRange
            corners += [
                edge.valueAt(first + (last - first) * (i + rng.random()) / 4) for i in range(4)
            ]
    for centre in corners:
        for r in radii:
            # More directions close in, where thin Voronoi wedges hide
            for _ in range(64 if r <= 1e-6 else (12 if r <= 1e-5 else 4)):
                way = V(rng.gauss(0, 1), rng.gauss(0, 1), rng.gauss(0, 1))
                points.append(centre + way.normalize() * r)
    return points


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
        mesh.addNode(0, 2e-4, 0)
        with self.subTest(element="shifted vertex"):
            self.assertEqual(mesh.getNodesByEdge(edge), [node])
        # The ends of the curve stay on the edge, though both vertices moved out of reach
        first = Part.Vertex(V(0, 1e-4, 0))
        last = Part.Vertex(V(10, 1e-4, 0))
        first.fixTolerance(1e-4)
        last.fixTolerance(1e-4)
        edge = line.replaceShape([(line.Vertexes[0], first), (line.Vertexes[1], last)])
        mesh = Fem.FemMesh()
        nodes = [mesh.addNode(-5e-8, -5e-8, 0), mesh.addNode(10 + 5e-8, -5e-8, 0)]
        mesh.addNode(-1.2e-7, 0, 0)
        mesh.addNode(10 + 1.2e-7, 0, 0)
        with self.subTest(element="curve end"):
            self.assertEqual(mesh.getNodesByEdge(edge), nodes)
        # A sphere pole looser than its vertex is strict as an edge, 2**-13 lands on it unrounded
        loose_tol = 2.0**-13
        ball = Part.makeSphere(4)
        pole = max((e for e in ball.Edges if e.Degenerated), key=lambda e: e.Vertexes[0].Point.z)
        self.assertEqual(pole.Vertexes[0].Point.z, 4.0)
        pole.fixTolerance(loose_tol, Part.Edge)
        mesh = Fem.FemMesh()
        node = mesh.addNode(0, 0, 4 + 5e-5)
        mesh.addNode(0, 0, 4 + loose_tol)
        mesh.addNode(0, 0, 4 + 2e-4)
        with self.subTest(element="loose pole"):
            self.assertEqual(mesh.getNodesByFace(ball.Faces[0]), [node])
        # The same pole on a creased face, which goes the fallback way : apex up, opening down
        profile = kinked([V(0, 0, 5), V(1, 0, 3), V(3, 0, 2), V(4, 0, 0)], 3).toShape()
        spire = profile.revolve(V(0, 0, 0), V(0, 0, 1), 360).Faces[0]
        apex = next(e for e in spire.Edges if e.Degenerated)
        self.assertEqual(apex.Vertexes[0].Point.z, 5.0)
        apex.fixTolerance(loose_tol, Part.Edge)
        mesh = Fem.FemMesh()
        on = [mesh.addNode(0, 0, 5 + f * loose_tol) for f in (0.5, 0.9)]
        for f in (1.0, 1.1, 2.0):
            mesh.addNode(0, 0, 5 + f * loose_tol)
        with self.subTest(element="loose pole by a crease"):
            self.assertEqual(mesh.getNodesByFace(spire), on)
        # An edge looser than its vertices : right at its tolerance is off, as for any edge
        rod = Part.makeLine(V(0, 0, 0), V(10, 0, 0))
        rod.fixTolerance(1e-4, Part.Edge)
        mesh = Fem.FemMesh()
        nodes = [mesh.addNode(-0.9e-4, 0, 0), mesh.addNode(5, 0.9e-4, 0)]
        mesh.addNode(-1e-4, 0, 0)
        mesh.addNode(5, 1e-4, 0)
        with self.subTest(element="loose rod end"):
            self.assertEqual(mesh.getNodesByEdge(rod), nodes)
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
        # Torus offsets placed, reversed, nested, 2 pi round and one of the fillet's spindle torus
        ring = Part.Toroid()
        ring.MajorRadius = 100
        ring.MinorRadius = 1
        ring.MajorRadius = 3
        ring.Axis = V(1, 2, 3).normalize()
        placed = Part.OffsetSurface(ring, 0.3).toShape(0, 6, 5, 6)
        placed.Placement = FreeCAD.Placement(V(1, -2, 3), FreeCAD.Rotation(V(1, 2, 2), 40))
        reversed_patch = Part.OffsetSurface(ring, -0.4).toShape(1, 3, 5, 6)
        reversed_patch.reverse()
        nested = Part.OffsetSurface(Part.OffsetSurface(ring, 0.5), -0.2).toShape(0, 6, 5, 6)
        round_patch = Part.OffsetSurface(ring, 0.3).toShape(0, 2 * math.pi, 5, 6)
        # Mirrored, the frame turns indirect and the offset goes inward : the tube is r - d
        mirrored = Part.OffsetSurface(ring, 0.3).toShape(0, 6, 5, 6).mirror(V(0, 0, 0), V(1, 0, 0))
        grid = [
            mirrored.valueAt(6 * i / 40, 5 + j / 40) for i in range(2, 39) for j in range(2, 39)
        ]
        self.assertGreater(out_of(mirrored.BoundBox, grid), 1e-4)
        patches.append((mirrored, None))
        # Offset past the minor radius, R below r + |d| : the probes and the patch lie on 2 tubes
        deep = Part.Toroid()
        deep.MajorRadius = 100
        deep.MinorRadius = 0.9934
        deep.MajorRadius = 1.6555
        deep.Axis = V(-0.3479, -0.8892, 0.2971).normalize()
        deep.Center = V(-7.0178, -2.9331, 4.9143)
        far = Part.OffsetSurface(deep, -2.4422).toShape(-5.4355, -0.1582, 3.1889, 5.4419)
        far.Placement = FreeCAD.Placement(V(1, -2, 3), FreeCAD.Rotation(158.363, -35.3346, 96.9662))
        a, b, c, d = far.ParameterRange
        grid = [
            far.valueAt(a + (b - a) * i / 12, c + (d - c) * j / 12)
            for i in range(1, 12)
            for j in range(1, 12)
        ]
        self.assertGreater(out_of(far.BoundBox, grid), 1e-4)
        patches.append((far, None))
        spindle = Part.OffsetSurface(faces[0].Surface, -0.3).toShape(0.0, 0.5, 2.0, 4.0)
        patches += [(f, None) for f in (placed, reversed_patch, nested, round_patch, spindle)]
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
            with self.subTest(surface=type(face.Surface).__name__, element="Face"):
                # Read on the surface itself, every node laid on the face is on it
                self.assertEqual(nodes - set(mesh.getNodesByFace(face)), set())
            if solid is None:
                continue
            with self.subTest(surface=type(face.Surface).__name__, element="Solid"):
                # BRepExtrema prunes by the same short box, every node it keeps the box keeps too
                limit = solid.getTolerance(1)
                kept = {
                    n for n in nodes if solid.distToShape(Part.Vertex(mesh.Nodes[n]))[0] < limit
                }
                self.assertGreater(len(kept), 0.9 * len(nodes))
                self.assertEqual(kept - set(mesh.getNodesBySolid(solid)), set())
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

    def wrong_nodes(self, expected, found):
        return [
            "{} ({!r}, {!r}, {!r})".format("missed" if on else "extra", p.x, p.y, p.z)
            for n, (on, p) in expected.items()
            if (n in found) != on
        ]

    def check_near(self, name, shape, search, distance):
        """The nodes closer than the tolerance to a face or an edge, from its exact distance"""
        tol = shape.Tolerance
        mesh = Fem.FemMesh()
        expected = {}
        for point in points_around(shape, 1848):
            d = distance(point)
            if TRUTH_IN * tol < d < 2.0 * tol:
                continue
            expected[mesh.addNode(point.x, point.y, point.z)] = (d <= TRUTH_IN * tol, point)
        wrong = self.wrong_nodes(expected, set(search(mesh)(shape)))
        self.assertEqual(wrong[:5], [], f"{name} : {len(wrong)} wrong of {len(expected)} nodes")

    def test_nodes_by_face_shapes(self):
        box = Part.makeBox(10, 10, 10)
        top = next(f for f in box.Faces if f.BoundBox.ZMin > 9.9)
        cylinder = next(
            f for f in Part.makeCylinder(5, 10).Faces if isinstance(f.Surface, Part.Cylinder)
        )
        sphere = Part.makeSphere(5).Faces[0]
        # Seams all round, and the poles of a B-spline, where the old search lost nodes 1e-8 off
        torus = Part.makeTorus(5, 2).Faces[0]
        nurbs = Part.makeSphere(5).toNurbs().Faces[0]
        # Cut along its old seam : the boolean leaves the cut 2 pcurves, IsClosed took it for a seam
        half = Part.makeTorus(5, 2).common(Part.makeBox(16, 8, 6, V(-8, 0, -3)))
        cut = next(f for f in half.Faces if isinstance(f.Surface, Part.Toroid))

        def half_torus(p):
            if p.y >= 0:
                return abs(math.hypot(math.hypot(p.x, p.y) - 5, p.z) - 2)
            return min(math.hypot(p.y, abs(math.hypot(p.x - s, p.z) - 2)) for s in (5, -5))

        def rectangle(p):
            dx = max(0.0, -p.x, p.x - 10)
            dy = max(0.0, -p.y, p.y - 10)
            return math.sqrt(dx * dx + dy * dy + (p.z - 10) ** 2)

        def side(p):
            return math.hypot(math.hypot(p.x, p.y) - 5, max(0.0, -p.z, p.z - 10))

        for name, face, distance in (
            ("box top", top, rectangle),
            ("cylinder side", cylinder, side),
            ("sphere", sphere, lambda p: abs(p.Length - 5)),
            ("torus", torus, lambda p: abs(math.hypot(math.hypot(p.x, p.y) - 5, p.z) - 2)),
            ("NURBS sphere", nurbs, lambda p: abs(p.Length - 5)),
            ("half torus", cut, half_torus),
        ):
            with self.subTest(face=name):
                self.check_near(name, face, lambda m: m.getNodesByFace, distance)

    def test_nodes_by_edge_shapes(self):
        line = Part.makeLine(V(0, 0, 0), V(10, 0, 0))
        circle = Part.makeCircle(5, V(0, 0, 10), V(0, 0, 1))
        arc = Part.makeCircle(5, V(0, 0, 0), V(0, 0, 1), 0, 120)

        def segment(p):
            return math.hypot(math.hypot(p.y, p.z), max(0.0, -p.x, p.x - 10))

        def on_arc(p):
            if wedge_gap(p.x, p.y, 120) == 0.0:
                return math.hypot(math.hypot(p.x, p.y) - 5, p.z)
            ends = (
                V(5, 0, 0),
                V(5 * math.cos(math.radians(120)), 5 * math.sin(math.radians(120)), 0),
            )
            return min((p - e).Length for e in ends)

        for name, edge, distance in (
            ("segment", line, segment),
            ("circle", circle, lambda p: math.hypot(math.hypot(p.x, p.y) - 5, p.z - 10)),
            ("arc 120", arc, on_arc),
        ):
            with self.subTest(edge=name):
                self.check_near(name, edge, lambda m: m.getNodesByEdge, distance)

    def test_nodes_on_a_face_by_a_reflex_corner(self):
        # The classifier of ExtPF called OUT nodes along an edge extended past the reflex corner
        ell = [(0, 0), (10, 0), (10, 4), (4, 4), (4, 10), (0, 10)]
        floor = [(2, 2), (8, 2), (8, 4), (4, 4), (4, 8), (2, 8)]
        pocket = Part.makeBox(10, 10, 10)
        pocket = pocket.cut(Part.makeBox(6, 2, 5, V(2, 2, 6))).cut(
            Part.makeBox(2, 6, 5, V(2, 2, 6))
        )
        for name, solid, polygon, z in (
            ("L pocket floor", pocket.removeSplitter().Solids[0], floor, 6.0),
            ("L extrusion top", prism(ell, 5).Solids[0], ell, 5.0),
        ):
            with self.subTest(face=name):
                face = next(
                    f
                    for f in solid.Faces
                    if abs(f.BoundBox.ZLength) < 1e-9 and abs(f.BoundBox.ZMin - z) < 1e-9
                )
                tol = solid.getTolerance(1)
                mesh = Fem.FemMesh()
                on_face = {}
                for r in (5e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 0.1):
                    for k in range(360):
                        x = 4 + r * math.cos(math.radians(k + 0.5))
                        y = 4 + r * math.sin(math.radians(k + 0.5))
                        # Farther than two tolerances from the outline, on the face or off it
                        if outline_gap(x, y, polygon) > 2.0 * tol:
                            on_face[mesh.addNode(x, y, z)] = polygon_gap(x, y, polygon) == 0.0
                found = set(mesh.getNodesByFace(face))
                expected = {n for n, on in on_face.items() if on}
                self.assertEqual((len(expected - found), len(found - expected)), (0, 0))

    def test_nodes_on_a_needle_face(self):
        # A cone of 0.0057 degrees : by the apex a foot ON for FClass2d lies 100 mm from its edge
        side = next(
            f for f in Part.makeCone(0.01, 0, 100).Faces if isinstance(f.Surface, Part.Cone)
        )
        tol = side.Tolerance
        mesh = Fem.FemMesh()
        expected = {}
        for h in (1e-7, 3e-7, 1e-6, 1e-5, 1e-4):
            for i in range(24):
                a = 2 * math.pi * (i + 0.5) / 24
                way = V(math.cos(a), math.sin(a), 0)
                foot = V(0, 0, 100 - h) + way * (NEEDLE_SLOPE * h)
                normal = (way + V(0, 0, NEEDLE_SLOPE)).normalize()
                for f in (0.0,) + TOL_FRACTIONS + tuple(-f for f in TOL_FRACTIONS):
                    p = foot + normal * (f * tol)
                    d = needle_side_gap(p)
                    if TRUTH_IN * tol < d < 2.0 * tol:
                        continue
                    expected[mesh.addNode(p.x, p.y, p.z)] = (d <= TRUTH_IN * tol, p)
        wrong = self.wrong_nodes(expected, set(mesh.getNodesByFace(side)))
        self.assertEqual(wrong[:5], [], f"{len(wrong)} wrong of {len(expected)} nodes")

    def test_nodes_by_a_sliver_corner(self):
        # By a torus sliver corner FClass2d and the face classifier call OUT a band 1e-5 wide inside
        a = 8.95
        sliver = Part.makeTorus(7, 2).common(Part.makeBox(12 - a, 12, 6, V(a, 0, -3)))
        cut = next(
            f
            for f in sliver.Faces
            if isinstance(f.Surface, Part.Plane) and abs(f.BoundBox.XMax - a) < 1e-9
        )
        tol = cut.Tolerance
        mesh = Fem.FemMesh()
        expected = {}
        for y in (1e-5, 3e-5, 1e-4, 3e-4, 1e-3):
            top = math.sqrt(4 - (math.hypot(a, y) - 7) ** 2)
            for below in (2e-6, 5e-6, 1e-5, 3e-5, 1e-4):
                for f in (0.0,) + TOL_FRACTIONS + tuple(-f for f in TOL_FRACTIONS):
                    d = abs(f) * tol
                    if TRUTH_IN * tol < d < 2.0 * tol:
                        continue
                    p = V(a + f * tol, y, top - below)
                    expected[mesh.addNode(p.x, p.y, p.z)] = (d <= TRUTH_IN * tol, p)
        wrong = self.wrong_nodes(expected, set(mesh.getNodesByFace(cut)))
        self.assertEqual(wrong[:3], [], f"{len(wrong)} wrong of {len(expected)} nodes")

    def test_nodes_on_a_kinked_end_face(self):
        # A roof end face, its ridge a B-spline folded C0, goes the old way of DistShapeShape
        zc = 2 + 2 / math.tan(math.radians(20))
        face = kinked_section([(-2, 0), (2, 0), (2, 2), (0, zc), (-2, 2)], 3)
        tol = face.Tolerance
        rng = random.Random(1966)
        mesh = Fem.FemMesh()
        points = {}
        for r in (1e-7, 3e-7, 1e-6, 1e-5):
            for _ in range(60):
                way = V(rng.gauss(0, 1), rng.gauss(0, 1), rng.gauss(0, 1)).normalize()
                p = V(0, 0, zc) + way * r
                points[mesh.addNode(p.x, p.y, p.z)] = p
        found = set(mesh.getNodesByFace(face))
        kept = {n for n, p in points.items() if face.distToShape(Part.Vertex(p))[0] < tol}
        self.assertEqual((len(found - kept), len(kept - found)), (0, 0))
        # The old way keeps the loose vertices too
        corner = next(v for v in face.Vertexes if (v.Point - V(2, 0, 0)).Length < 1e-9)
        corner.fixTolerance(1e-4)
        mesh = Fem.FemMesh()
        node = mesh.addNode(2 + 5e-5, 0.0, -5e-5)
        self.assertEqual(mesh.getNodesByFace(face), [node])
        # And the loose edges, and a node right at the tolerance of an ordinary vertex
        side = next(e for e in face.Edges if e.BoundBox.XMax < -1.9 and e.BoundBox.ZLength > 1)
        side.fixTolerance(1e-4, Part.Edge)
        tip = next(v for v in face.Vertexes if (v.Point - V(2, 0, 2)).Length < 1e-9)
        mesh = Fem.FemMesh()
        nodes = [mesh.addNode(-2 - 5e-5, 5e-5, 1.0), mesh.addNode(2.0, tip.Tolerance, 2.0)]
        self.assertEqual(mesh.getNodesByFace(face), nodes)

    def test_nodes_by_the_tolerance_itself(self):
        # Square off a face or edge, far from its boundary : under the tolerance on, over it off
        cylinder = Part.makeCylinder(5, 10)
        sphere = Part.makeSphere(5)
        top = next(f for f in Part.makeBox(10, 10, 10).Faces if f.BoundBox.ZMin > 9.9)
        side = next(f for f in cylinder.Faces if isinstance(f.Surface, Part.Cylinder))
        segment = Part.makeLine(V(0, 0, 0), V(10, 0, 0))
        circle = Part.makeCircle(5, V(0, 0, 0), V(0, 0, 1))
        both = (1.0, -1.0)
        # Past an end or a corner the vertex is the closest point, only outward counts
        cases = (
            ("box top", top, V(5, 5, 10), V(0, 0, 1), both),
            ("cylinder side", side, V(5, 0, 5), V(1, 0, 0), both),
            ("sphere", sphere.Faces[0], V(0, 5, 0), V(0, 1, 0), both),
            ("segment", segment, V(5, 0, 0), V(0, 0.6, 0.8), both),
            ("circle", circle, V(0, -5, 0), V(0, -0.8, 0.6), both),
            ("segment end", segment, V(10, 0, 0), V(1, 0, 0), (1.0,)),
            ("box top corner", top, V(10, 10, 10), V(1, 1, 0).normalize(), (1.0,)),
        )
        for name, shape, at, way, sides in cases:
            tol = shape.Tolerance
            mesh = Fem.FemMesh()
            expected = {}
            for f in (0.8, 0.95, 1.05, 1.2):
                for side_of in sides:
                    p = at + way * (side_of * f * tol)
                    expected[mesh.addNode(p.x, p.y, p.z)] = (f < 1.0, p)
            search = mesh.getNodesByFace if shape.ShapeType == "Face" else mesh.getNodesByEdge
            with self.subTest(shape=name):
                self.assertEqual(self.wrong_nodes(expected, set(search(shape))), [])

    def test_nodes_by_a_circle_start(self):
        # OCCT puts a foot within 1e-7 of an edge end onto that end, here by a circle seam vertex
        for radius in (5.0, 50.0):
            disc = Part.makeCylinder(radius, 1e-3)
            rim = next(e for e in disc.Edges if e.isClosed() and e.BoundBox.ZMax < 1e-9)
            base = next(
                f
                for f in disc.Faces
                if isinstance(f.Surface, Part.Plane) and f.BoundBox.ZMax < 1e-9
            )
            for name, shape, search in (
                ("rim", rim, lambda m: m.getNodesByEdge),
                ("base", base, lambda m: m.getNodesByFace),
            ):
                tol = shape.Tolerance
                mesh = Fem.FemMesh()
                expected = {}
                for p, d in by_circle_start(radius, tol):
                    expected[mesh.addNode(p.x, p.y, p.z)] = (d <= TRUTH_IN * tol, p)
                wrong = self.wrong_nodes(expected, set(search(mesh)(shape)))
                with self.subTest(shape=name, radius=radius):
                    self.assertEqual(wrong[:3], [], f"{name} : {len(wrong)} wrong")

    def test_nodes_on_a_tiny_flat_patch(self):
        # A flat degree 1 B-spline half a micrometre wide : asked for minima only, ExtPS found none
        poles = [
            [
                V(0.10222828622785718, 0.006946620435553719, 0.0519764567317198),
                V(0.10228666029150135, 0.007179338654011325, 0.052109124052234),
            ],
            [
                V(0.10266707227168204, 0.007247087586275569, 0.0512563253225764),
                V(0.10272544633532621, 0.007479805804734063, 0.05138899264309149),
            ],
        ]
        surface = Part.BSplineSurface()
        surface.buildFromPolesMultsKnots(
            poles, [2, 2], [2, 2], [0.0, 1.0], [0.0, 1.0], False, False, 1, 1
        )
        face = surface.toShape()
        mesh = Fem.FemMesh()
        on = [
            mesh.addNode(*surface.value(i / 10, j / 10)) for i in range(1, 10) for j in range(1, 10)
        ]
        normal = surface.normal(0.5, 0.5)
        off = [
            mesh.addNode(*(surface.value(i / 10, j / 10) + normal * (side * 3.0 * face.Tolerance)))
            for i in range(1, 10)
            for j in range(1, 10)
            for side in (-1.0, 1.0)
        ]
        found = set(mesh.getNodesByFace(face))
        self.assertEqual(
            (len([n for n in on if n not in found]), len([n for n in off if n in found])), (0, 0)
        )

    def test_nodes_by_a_nurbs_cone_apex(self):
        # Below a NURBS apex ExtPS stops 0.03 radian off the foot, the NURBS holds the cone to 3e-15
        cone = Part.makeCone(8.0, 0, 10, V(0, 0, 0), V(0, 0, 1), 240).toNurbs()
        plate = Part.makeBox(40, 40, 1, V(-20, -20, -1))
        pillar = Part.makeBox(2, 2, 21, V(15, 15, -1))
        solid = cone.fuse(plate).fuse(pillar).removeSplitter().Solids[0]
        side = next(
            f
            for f in solid.Faces
            if isinstance(f.Surface, Part.BSplineSurface) and f.BoundBox.ZMax > 9.9 and f.Area > 50
        )
        tol = side.Tolerance
        lean = math.atan(0.8)
        mesh = Fem.FemMesh()
        expected = {}
        for h in (3e-6, 5e-6, 1e-5, 2e-5, 5e-5):
            for k in range(40):
                a = math.radians(240) * (k + 0.5) / 40
                on = V(0.8 * h * math.cos(a), 0.8 * h * math.sin(a), 10 - h)
                normal = V(
                    math.cos(lean) * math.cos(a), math.cos(lean) * math.sin(a), math.sin(lean)
                )
                for offset in (TRUTH_IN, -TRUTH_IN, 3.0):
                    p = on + normal * (offset * tol)
                    expected[mesh.addNode(p.x, p.y, p.z)] = (offset < 1.0, p)
        wrong = self.wrong_nodes(expected, set(mesh.getNodesByFace(side)))
        self.assertEqual(wrong[:5], [], f"{len(wrong)} of {len(expected)} nodes wrong")

    def test_nodes_on_a_scaled_loft(self):
        # A loft side scaled to 0.01 : ExtPS weighed squared distances against parameter tolerances
        ring = [
            (3.426116916096332, 0.0),
            (3.2471762947944187, 3.2471762947944183),
            (0.0, 4.1792466914851545),
            (-2.1669653396734665, 2.166965339673467),
            (-4.361684345084626, 0.0),
            (-2.7992349784125046, -2.7992349784125037),
            (0.0, -5.3961679817519705),
            (3.5057733857761306, -3.5057733857761324),
        ]
        curve = Part.BSplineCurve()
        curve.interpolate([V(x, y, 0) for x, y in ring], PeriodicFlag=True)
        top = Part.Wire(
            Part.makeCircle(
                3.7112040632007672, V(-0.648154959832437, -0.7230073253323477, 7.94241285392831)
            )
        )
        loft = Part.makeLoft([Part.Wire(curve.toShape()), top], True).Solids[0]
        loft.scale(0.01)
        side = next(f for f in loft.Faces if isinstance(f.Surface, Part.BSplineSurface))
        surface = side.Surface
        tol = side.Tolerance
        u0, u1, v0, v1 = side.ParameterRange
        mesh = Fem.FemMesh()
        expected = {}
        # On the face by its own parameters, the truth reads the surface and nothing else
        for i in range(1, 60):
            for j in range(1, 12):
                u = u0 + (u1 - u0) * i / 60
                v = v0 + (v1 - v0) * j / 12
                on = surface.value(u, v)
                normal = surface.normal(u, v)
                for offset in (TRUTH_IN, -TRUTH_IN, 3.0):
                    p = on + normal * (offset * tol)
                    expected[mesh.addNode(p.x, p.y, p.z)] = (offset < 1.0, p)
        wrong = self.wrong_nodes(expected, set(mesh.getNodesByFace(side)))
        self.assertEqual(wrong[:5], [], f"{len(wrong)} of {len(expected)} nodes wrong")

    def test_more_nodes_than_the_search_lists(self):
        # Over 64 x cores^2 nodes one thread overflows its list, whatever the number of threads
        top = next(f for f in Part.makeBox(10, 10, 10).Faces if f.BoundBox.ZMin > 9.9)
        count = 64 * (os.cpu_count() or 1) ** 2 + 64
        side = math.ceil(math.sqrt(count))
        mesh = Fem.FemMesh()
        on = set()
        for i in range(count):
            on.add(mesh.addNode(0.5 + 9 * (i % side) / side, 0.5 + 9 * (i // side) / side, 10))
        for i in range(500):
            mesh.addNode(5, 5, 20 + i)
        self.assertEqual(set(mesh.getNodesByFace(top)), on)

    def test_nodes_by_a_circle_below_the_tolerance(self):
        # ExtPC gives up on the axis of a circle, one under the tolerance still holds its centre
        ring = Part.makeCircle(5e-8)
        mesh = Fem.FemMesh()
        centre = mesh.addNode(0, 0, 0)
        mesh.addNode(0, 0, 2e-7)
        self.assertEqual(mesh.getNodesByEdge(ring), [centre])

    def test_nodes_by_a_face_of_any_orientation(self):
        # An Internal or External face has Internal edges too : a disc read that way had no boundary
        tol = Part.Face(Part.Wire(Part.makeCircle(1))).Tolerance
        for orientation in ("Forward", "Reversed", "Internal", "External"):
            disc = Part.Face(Part.Wire(Part.makeCircle(1)))
            disc.Orientation = orientation
            mesh = Fem.FemMesh()
            on = [mesh.addNode(0.3, 0.2, 0), mesh.addNode(1 + 0.5 * tol, 0, 0)]
            mesh.addNode(0.8, 0.8, 0)
            mesh.addNode(1 + 3 * tol, 0, 0)
            with self.subTest(orientation=orientation):
                self.assertEqual(mesh.getNodesByFace(disc), on)

    def check_searches(self, name, mesh, shape):
        solid = shape.Solids[0]
        with self.subTest(shape=name, element="solid"):
            # Pins the current result : nodes up to 1e-3 inside the solid are still left out
            expected = closer_than(mesh, solid, solid.getTolerance(1))
            self.assertEqual(set(mesh.getNodesBySolid(solid)), expected)
        # Every node DistShapeShape keeps is kept, none past 100 tolerances, its bound by a pole
        for index, face in enumerate(shape.Faces, 1):
            with self.subTest(shape=name, element=f"Face{index}"):
                found = set(mesh.getNodesByFace(face))
                self.assertEqual(closer_than(mesh, face, face.Tolerance) - found, set())
                self.assertEqual(found - closer_than(mesh, face, 100 * face.Tolerance), set())
        for index, edge in enumerate(shape.Edges, 1):
            with self.subTest(shape=name, element=f"Edge{index}"):
                found = set(mesh.getNodesByEdge(edge))
                self.assertEqual(closer_than(mesh, edge, edge.Tolerance) - found, set())
                self.assertEqual(found - closer_than(mesh, edge, 100 * edge.Tolerance), set())
