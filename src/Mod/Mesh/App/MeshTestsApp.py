#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#  Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>
#  LGPL

import os
import sys
import io
import FreeCAD, unittest, Mesh
import MeshEnums
from FreeCAD import Base
import time, tempfile, math

# http://python-kurs.eu/threads.php
try:
    import _thread as thread
except Exception:
    import thread

from os.path import join

# ---------------------------------------------------------------------------
# define the functions to test the FreeCAD mesh module
# ---------------------------------------------------------------------------


class MeshTopoTestCases(unittest.TestCase):
    def setUp(self):
        # set up a planar face with 18 triangles
        self.planarMesh = []
        for x in range(3):
            for y in range(3):
                self.planarMesh.append([0.0 + x, 0.0 + y, 0.0000])
                self.planarMesh.append([1.0 + x, 1.0 + y, 0.0000])
                self.planarMesh.append([0.0 + x, 1.0 + y, 0.0000])
                self.planarMesh.append([0.0 + x, 0.0 + y, 0.0000])
                self.planarMesh.append([1.0 + x, 0.0 + y, 0.0000])
                self.planarMesh.append([1.0 + x, 1.0 + y, 0.0000])

    def testCollapseFacetsSingle(self):
        for i in range(18):
            planarMeshObject = Mesh.Mesh(self.planarMesh)
            planarMeshObject.collapseFacets([i])

    def testCollapseFacetsMultible(self):
        planarMeshObject = Mesh.Mesh(self.planarMesh)
        planarMeshObject.collapseFacets(range(7))

    def testCollapseFacetsAll(self):
        planarMeshObject = Mesh.Mesh(self.planarMesh)
        planarMeshObject.collapseFacets(range(18))

    # fmt: off
    def testCorruptedFacet(self):
        v = FreeCAD.Vector
        mesh = Mesh.Mesh()
        mesh.addFacet(
        v(1.0e1, -1.0e1, 1.0e1),
        v(1.0e1, +1.0e1, 1.0e1),
        v(0.0e0, 0.0e0, 1.0e1))

        mesh.addFacet(
        v(-1.0e1, -1.0e1, 1.0e1),
        v(-1.0e1, +1.0e1, 1.0e1),
        v(0e0, 0.0e0, 1.0e1))

        mesh.addFacet(
        v(+1.0e1, +1.0e1, 1.0e1),
        v(-1.0e1, +1.0e1, 1.0e1),
        v(.0e0, 0.0e0, 1.0e1))

        mesh.addFacet(
        v(+1.0e1, -1.0e1, 1.0e1),
        v(-1.0e1, -1.0e1, 1.0e1),
        v(.0e0, 0.0e0, 1.0e1))

        mesh.addFacet(
        v(-1.0e1, +1.0e1, 1.0e1),
        v(+1.0e1, +1.0e1, 1.0e1),
        v(+1.0e1, +1.0e1, 1.0e1))

        mesh.addFacet(
        v(+1.0e1, +1.0e1, 1.0e1),
        v(+1.0e1, 00.0e1, 1.0e1),
        v(+1.0e1, -1.0e1, 1.0e1))

        self.assertEqual(mesh.CountFacets, 6)
        mesh.fixIndices()
        self.assertEqual(mesh.CountFacets, 5)


# fmt: on


class MeshSplitTestCases(unittest.TestCase):
    def setUp(self):
        self.mesh = Mesh.createBox(1.0, 1.0, 1.0)

    def testSplitFacetOnOneEdge(self):
        p1 = self.mesh.Points[0].Vector
        p2 = self.mesh.Points[1].Vector
        p3 = self.mesh.Points[2].Vector
        self.mesh.splitFacet(0, p1, (p2 + p3) / 2)
        self.assertFalse(self.mesh.hasNonManifolds())
        self.assertFalse(self.mesh.hasInvalidNeighbourhood())
        self.assertFalse(self.mesh.hasPointsOutOfRange())
        self.assertFalse(self.mesh.hasFacetsOutOfRange())
        self.assertFalse(self.mesh.hasCorruptedFacets())
        self.assertTrue(self.mesh.isSolid())

    def testSplitFacetOnTwoEdges_21(self):
        p1 = self.mesh.Points[0].Vector
        p2 = self.mesh.Points[1].Vector
        p3 = self.mesh.Points[2].Vector
        self.mesh.splitFacet(0, (p1 + p3) / 2, (p2 + p3) / 2)
        self.assertFalse(self.mesh.hasNonManifolds())
        self.assertFalse(self.mesh.hasInvalidNeighbourhood())
        self.assertFalse(self.mesh.hasPointsOutOfRange())
        self.assertFalse(self.mesh.hasFacetsOutOfRange())
        self.assertFalse(self.mesh.hasCorruptedFacets())
        self.assertTrue(self.mesh.isSolid())

    def testSplitFacetOnTwoEdges_12(self):
        p1 = self.mesh.Points[0].Vector
        p2 = self.mesh.Points[1].Vector
        p3 = self.mesh.Points[2].Vector
        self.mesh.splitFacet(0, (p2 + p3) / 2, (p1 + p3) / 2)
        self.assertFalse(self.mesh.hasNonManifolds())
        self.assertFalse(self.mesh.hasInvalidNeighbourhood())
        self.assertFalse(self.mesh.hasPointsOutOfRange())
        self.assertFalse(self.mesh.hasFacetsOutOfRange())
        self.assertFalse(self.mesh.hasCorruptedFacets())
        self.assertTrue(self.mesh.isSolid())

    def testSplitFacetOnTwoEdges_01(self):
        p1 = self.mesh.Points[0].Vector
        p2 = self.mesh.Points[1].Vector
        p3 = self.mesh.Points[2].Vector
        self.mesh.splitFacet(0, (p1 + p2) / 2, (p2 + p3) / 2)
        self.assertFalse(self.mesh.hasNonManifolds())
        self.assertFalse(self.mesh.hasInvalidNeighbourhood())
        self.assertFalse(self.mesh.hasPointsOutOfRange())
        self.assertFalse(self.mesh.hasFacetsOutOfRange())
        self.assertFalse(self.mesh.hasCorruptedFacets())
        self.assertTrue(self.mesh.isSolid())

    def testSplitFacetOnTwoEdges_10(self):
        p1 = self.mesh.Points[0].Vector
        p2 = self.mesh.Points[1].Vector
        p3 = self.mesh.Points[2].Vector
        self.mesh.splitFacet(0, (p2 + p3) / 2, (p1 + p2) / 2)
        self.assertFalse(self.mesh.hasNonManifolds())
        self.assertFalse(self.mesh.hasInvalidNeighbourhood())
        self.assertFalse(self.mesh.hasPointsOutOfRange())
        self.assertFalse(self.mesh.hasFacetsOutOfRange())
        self.assertFalse(self.mesh.hasCorruptedFacets())
        self.assertTrue(self.mesh.isSolid())

    def testSplitFacetOnTwoEdges_02(self):
        p1 = self.mesh.Points[0].Vector
        p2 = self.mesh.Points[1].Vector
        p3 = self.mesh.Points[2].Vector
        self.mesh.splitFacet(0, (p1 + p2) / 2, (p1 + p3) / 2)
        self.assertFalse(self.mesh.hasNonManifolds())
        self.assertFalse(self.mesh.hasInvalidNeighbourhood())
        self.assertFalse(self.mesh.hasPointsOutOfRange())
        self.assertFalse(self.mesh.hasFacetsOutOfRange())
        self.assertFalse(self.mesh.hasCorruptedFacets())
        self.assertTrue(self.mesh.isSolid())

    def testSplitFacetOnTwoEdges_20(self):
        p1 = self.mesh.Points[0].Vector
        p2 = self.mesh.Points[1].Vector
        p3 = self.mesh.Points[2].Vector
        self.mesh.splitFacet(0, (p1 + p3) / 2, (p1 + p2) / 2)
        self.assertFalse(self.mesh.hasNonManifolds())
        self.assertFalse(self.mesh.hasInvalidNeighbourhood())
        self.assertFalse(self.mesh.hasPointsOutOfRange())
        self.assertFalse(self.mesh.hasFacetsOutOfRange())
        self.assertFalse(self.mesh.hasCorruptedFacets())
        self.assertTrue(self.mesh.isSolid())

    def testSplitFacetOnTwoEdges_5teps(self):
        Vec3d = FreeCAD.Vector
        for i in range(5):
            f = self.mesh.Facets[0]
            p1 = Vec3d(f.Points[0])
            p2 = Vec3d(f.Points[1])
            p3 = Vec3d(f.Points[2])
            self.mesh.splitFacet(0, (p1 + p3) / 2, (p2 + p3) / 2)

        self.assertFalse(self.mesh.hasNonManifolds())
        self.assertFalse(self.mesh.hasInvalidNeighbourhood())
        self.assertFalse(self.mesh.hasPointsOutOfRange())
        self.assertFalse(self.mesh.hasFacetsOutOfRange())
        self.assertFalse(self.mesh.hasCorruptedFacets())
        self.assertTrue(self.mesh.isSolid())

    def testFindNearest(self):
        self.assertEqual(len(self.mesh.nearestFacetOnRay((-2, 2, -6), (0, 0, 1))), 0)
        self.assertEqual(len(self.mesh.nearestFacetOnRay((0.5, 0.5, 0.5), (0, 0, 1))), 1)
        self.assertEqual(
            len(self.mesh.nearestFacetOnRay((0.5, 0.5, 0.5), (0, 0, 1), -math.pi / 2)), 0
        )
        self.assertEqual(
            len(self.mesh.nearestFacetOnRay((0.2, 0.1, 0.2), (0, 0, 1))),
            len(self.mesh.nearestFacetOnRay((0.2, 0.1, 0.2), (0, 0, -1))),
        )
        self.assertEqual(
            len(self.mesh.nearestFacetOnRay((0.2, 0.1, 0.2), (0, 0, 1), math.pi / 2)),
            len(self.mesh.nearestFacetOnRay((0.2, 0.1, 0.2), (0, 0, -1), math.pi / 2)),
        )
        # Apply placement to mesh
        plm = Base.Placement(Base.Vector(1, 2, 3), Base.Rotation(1, 1, 1, 1))
        pnt = Base.Vector(0.5, 0.5, 0.5)
        vec = Base.Vector(0.0, 0.0, 1.0)

        self.mesh.Placement = plm
        self.assertEqual(len(self.mesh.nearestFacetOnRay(pnt, vec)), 0)

        # Apply the placement on the ray as well
        pnt = plm.multVec(pnt)
        vec = plm.Rotation.multVec(vec)
        self.assertEqual(len(self.mesh.nearestFacetOnRay(pnt, vec)), 1)

    def testForaminate(self):
        class FilterAngle:
            def __init__(self, mesh, vec, limit):
                self.myMesh = mesh
                self.vec = vec
                self.limit = limit

            def check_angle(self, item):
                angle = self.myMesh.Facets[item].Normal.getAngle(self.vec)
                return angle < self.limit

        results = self.mesh.foraminate((0.0, 0.0, 0.0), (0, 1, 1))
        filtered_result = list(
            filter(
                FilterAngle(self.mesh, FreeCAD.Vector(0, 1, 1), math.pi / 2).check_angle,
                results.keys(),
            )
        )

        self.assertEqual(
            filtered_result, list(self.mesh.foraminate((0.0, 0.0, 0.0), (0, 1, 1), math.pi / 2))
        )

    def testForaminatePlacement(self):
        pnt = Base.Vector(0.0, 0.0, 0.0)
        vec = Base.Vector(0.0, 1.0, 1.0)
        results = self.mesh.foraminate(pnt, vec)
        self.assertEqual(len(results), 4)

        # Apply placement to mesh
        plm = Base.Placement(Base.Vector(1, 2, 3), Base.Rotation(1, 1, 1, 1))
        self.mesh.Placement = plm
        self.assertEqual(len(self.mesh.foraminate(pnt, vec)), 0)

        # Apply the placement on the ray as well
        pnt = plm.multVec(pnt)
        vec = plm.Rotation.multVec(vec)
        results2 = self.mesh.foraminate(pnt, vec)
        self.assertEqual(len(results2), 4)
        self.assertEqual(list(results), list(results2))


class MeshGeoTestCases(unittest.TestCase):
    def setUp(self):
        # set up a planar face with 2 triangles
        self.planarMesh = []

    def testIntersection(self):
        self.planarMesh.append([0.9961, 1.5413, 4.3943])
        self.planarMesh.append([9.4796, 10.024, -3.0937])
        self.planarMesh.append([1.4308, 11.3841, 2.6829])
        self.planarMesh.append([2.6493, 2.2536, 3.0679])
        self.planarMesh.append([13.1126, 0.4857, -4.4417])
        self.planarMesh.append([10.2410, 8.9040, -3.5002])
        planarMeshObject = Mesh.Mesh(self.planarMesh)
        f1 = planarMeshObject.Facets[0]
        f2 = planarMeshObject.Facets[1]
        res = f1.intersect(f2)
        self.assertTrue(len(res) == 0)

    def testIntersection2(self):
        self.planarMesh.append([-16.097176, -29.891157, 15.987688])
        self.planarMesh.append([-16.176304, -29.859991, 15.947966])
        self.planarMesh.append([-16.071451, -29.900553, 15.912505])
        self.planarMesh.append([-16.092241, -29.893408, 16.020439])
        self.planarMesh.append([-16.007210, -29.926180, 15.967641])
        self.planarMesh.append([-16.064457, -29.904951, 16.090832])
        planarMeshObject = Mesh.Mesh(self.planarMesh)
        f1 = planarMeshObject.Facets[0]
        f2 = planarMeshObject.Facets[1]
        # does definitely NOT intersect
        res = f1.intersect(f2)
        self.assertTrue(len(res) == 0)

    def testIntersectionOfTransformedMesh(self):
        self.planarMesh.append([0.0, 10.0, 10.0])
        self.planarMesh.append([10.0, 0.0, 10.0])
        self.planarMesh.append([10.0, 10.0, 10.0])
        self.planarMesh.append([6.0, 8.0, 10.0])
        self.planarMesh.append([16.0, 8.0, 10.0])
        self.planarMesh.append([6.0, 18.0, 10.0])
        planarMeshObject = Mesh.Mesh(self.planarMesh)

        mat = Base.Matrix()
        mat.rotateX(1.0)
        mat.rotateY(1.0)
        mat.rotateZ(1.0)
        planarMeshObject.transformGeometry(mat)

        f1 = planarMeshObject.Facets[0]
        f2 = planarMeshObject.Facets[1]
        res = f1.intersect(f2)
        self.assertEqual(len(res), 2)

    def testIntersectionOfParallelTriangles(self):
        self.planarMesh.append([0.0, 10.0, 10.0])
        self.planarMesh.append([10.0, 0.0, 10.0])
        self.planarMesh.append([10.0, 10.0, 10.0])
        self.planarMesh.append([6.0, 8.0, 10.1])
        self.planarMesh.append([16.0, 8.0, 10.1])
        self.planarMesh.append([6.0, 18.0, 10.1])
        planarMeshObject = Mesh.Mesh(self.planarMesh)

        mat = Base.Matrix()
        mat.rotateX(1.0)
        mat.rotateY(1.0)
        mat.rotateZ(1.0)
        planarMeshObject.transformGeometry(mat)

        f1 = planarMeshObject.Facets[0]
        f2 = planarMeshObject.Facets[1]
        res = f1.intersect(f2)
        self.assertTrue(len(res) == 0)

    def testIntersectionOnEdge(self):
        self.planarMesh.append([5.0, -1.9371663331985474, 0.49737977981567383])
        self.planarMesh.append([4.0, -1.9371663331985474, 0.49737977981567383])
        self.planarMesh.append([5.0, -1.9842294454574585, 0.25066646933555603])
        self.planarMesh.append([4.6488823890686035, -1.7827962636947632, 0.4577442705631256])
        self.planarMesh.append([4.524135112762451, -2.0620131492614746, 0.5294350385665894])
        self.planarMesh.append([4.6488823890686035, -1.8261089324951172, 0.23069120943546295])
        planarMeshObject = Mesh.Mesh(self.planarMesh)
        f1 = planarMeshObject.Facets[0]
        f2 = planarMeshObject.Facets[1]
        res = f1.intersect(f2)
        self.assertEqual(len(res), 2)

    def testIntersectionCoplanar(self):
        self.planarMesh.append([0.0, 10.0, 10.0])
        self.planarMesh.append([10.0, 0.0, 10.0])
        self.planarMesh.append([10.0, 10.0, 10.0])
        self.planarMesh.append([6.0, 8.0, 10.0])
        self.planarMesh.append([16.0, 8.0, 10.0])
        self.planarMesh.append([6.0, 18.0, 10.0])
        planarMeshObject = Mesh.Mesh(self.planarMesh)
        f1 = planarMeshObject.Facets[0]
        f2 = planarMeshObject.Facets[1]
        res = f1.intersect(f2)
        self.assertTrue(len(res) == 2)

    def testIntersectionOverlap(self):
        self.planarMesh.append([0.0, 0.0, 0.0])
        self.planarMesh.append([5.0, 0.0, 0.0])
        self.planarMesh.append([8.0, 5.0, 0.0])
        self.planarMesh.append([4.0, 0.0, 0.0])
        self.planarMesh.append([10.0, 0.0, 0.0])
        self.planarMesh.append([9.0, 5.0, 0.0])
        planarMeshObject = Mesh.Mesh(self.planarMesh)
        f1 = planarMeshObject.Facets[0]
        f2 = planarMeshObject.Facets[1]
        res = f1.intersect(f2)
        self.assertTrue(len(res) == 2)

    def testIntersectionOfIntersectingEdges(self):
        self.planarMesh.append([0.0, 10.0, 10.0])
        self.planarMesh.append([10.0, 0.0, 10.0])
        self.planarMesh.append([10.0, 10.0, 10.0])
        self.planarMesh.append([6.0, 8.0, 10.0])
        self.planarMesh.append([16.0, 8.0, 10.0])
        self.planarMesh.append([6.0, 18.0, 10.0])
        planarMeshObject = Mesh.Mesh(self.planarMesh)

        edge1 = planarMeshObject.Facets[0].getEdge(2)
        edge2 = planarMeshObject.Facets[1].getEdge(2)
        res = edge1.intersectWithEdge(edge2)
        self.assertTrue(len(res) == 1)
        self.assertEqual(res[0][0], 6.0)
        self.assertEqual(res[0][1], 10.0)
        self.assertEqual(res[0][2], 10.0)

    def testIntersectionOfParallelEdges(self):
        self.planarMesh.append([0.0, 10.0, 10.0])
        self.planarMesh.append([10.0, 0.0, 10.0])
        self.planarMesh.append([10.0, 10.0, 10.0])
        self.planarMesh.append([6.0, 8.0, 10.0])
        self.planarMesh.append([16.0, 8.0, 10.0])
        self.planarMesh.append([6.0, 18.0, 10.0])
        planarMeshObject = Mesh.Mesh(self.planarMesh)

        edge1 = planarMeshObject.Facets[0].getEdge(2)
        edge2 = planarMeshObject.Facets[1].getEdge(0)
        res = edge1.intersectWithEdge(edge2)
        self.assertTrue(len(res) == 0)

    def testIntersectionOfCollinearEdges(self):
        self.planarMesh.append([0.0, 0.0, 0.0])
        self.planarMesh.append([6.0, 0.0, 0.0])
        self.planarMesh.append([3.0, 4.0, 0.0])
        self.planarMesh.append([7.0, 0.0, 0.0])
        self.planarMesh.append([13.0, 0.0, 0.0])
        self.planarMesh.append([10.0, 4.0, 0.0])
        planarMeshObject = Mesh.Mesh(self.planarMesh)

        edge1 = planarMeshObject.Facets[0].getEdge(0)
        edge2 = planarMeshObject.Facets[1].getEdge(0)
        res = edge1.intersectWithEdge(edge2)
        self.assertTrue(len(res) == 0)

    def testIntersectionOfWarpedEdges(self):
        self.planarMesh.append([0.0, 0.0, 0.0])
        self.planarMesh.append([6.0, 0.0, 0.0])
        self.planarMesh.append([3.0, 4.0, 0.0])
        self.planarMesh.append([2.0, 2.0, 1.0])
        self.planarMesh.append([8.0, 2.0, 1.0])
        self.planarMesh.append([5.0, 6.0, 1.0])
        planarMeshObject = Mesh.Mesh(self.planarMesh)

        edge1 = planarMeshObject.Facets[0].getEdge(1)
        edge2 = planarMeshObject.Facets[1].getEdge(0)
        res = edge1.intersectWithEdge(edge2)
        self.assertTrue(len(res) == 0)

    def testSelfIntersection(self):
        s = b"""solid Simple
facet normal 0.0e0 0.0e0 1.0e1
    outer loop
        vertex 0.0e1 0.0e1 1.0e1
        vertex 0.0e1 +1.0e1 1.0e1
        vertex +1.0e1 0.0e1 1.0e1
    endloop
endfacet
facet normal 0.0e0 0.0e0 1.0e1
    outer loop
        vertex 0.0e1 +1.0e1 1.0e1
        vertex +1.0e1 0.0e1 1.0e1
        vertex 1.0e1 1.0e1 1.0e1
    endloop
endfacet
facet normal 0.0e0 0.0e0 1.0e1
    outer loop
        vertex 0.0e1 0.0e1 1.0e1
        vertex 0.0e1 +1.0e1 1.0e1
        vertex -1.0e1 1.0e1 1.0e1
    endloop
endfacet
facet normal 0.0e0 0.0e0 1.0e1
    outer loop
        vertex 0.0e1 0.0e1 1.0e1
        vertex +1.0e1 0.0e1 1.0e1
        vertex +1.0e1 -1.0e1 1.0e1
    endloop
endfacet
facet normal 0.0e0 0.0e0 1.0e1
    outer loop
        vertex 0.6e1 0.8e1 1.0e1
        vertex +1.6e1 0.8e1 1.0e1
        vertex +0.6e1 1.8e1 1.0e1
    endloop
endfacet
endsolid Simple"""
        mesh = Mesh.Mesh()
        data = io.BytesIO(s)
        mesh.read(Stream=data, Format="AST")
        self.assertTrue(mesh.hasSelfIntersections())


class PivyTestCases(unittest.TestCase):
    def setUp(self):
        # set up a planar face with 2 triangles
        self.planarMesh = []
        FreeCAD.newDocument("MeshTest")

    def testRayPick(self):
        if not FreeCAD.GuiUp:
            return
        self.planarMesh.append([-16.097176, -29.891157, 15.987688])
        self.planarMesh.append([-16.176304, -29.859991, 15.947966])
        self.planarMesh.append([-16.071451, -29.900553, 15.912505])
        self.planarMesh.append([-16.092241, -29.893408, 16.020439])
        self.planarMesh.append([-16.007210, -29.926180, 15.967641])
        self.planarMesh.append([-16.064457, -29.904951, 16.090832])
        planarMeshObject = Mesh.Mesh(self.planarMesh)

        from pivy import coin
        import FreeCADGui

        Mesh.show(planarMeshObject)
        view = FreeCADGui.ActiveDocument.ActiveView.getViewer()
        rp = coin.SoRayPickAction(view.getSoRenderManager().getViewportRegion())
        rp.setRay(coin.SbVec3f(-16.05, 16.0, 16.0), coin.SbVec3f(0, -1, 0))
        rp.apply(view.getSoRenderManager().getSceneGraph())
        pp = rp.getPickedPoint()
        self.assertTrue(pp is not None)
        det = pp.getDetail()
        self.assertTrue(det.getTypeId() == coin.SoFaceDetail.getClassTypeId())
        det = coin.cast(det, det.getTypeId().getName().getString())
        self.assertTrue(det.getFaceIndex() == 1)

    def testPrimitiveCount(self):
        if not FreeCAD.GuiUp:
            return
        self.planarMesh.append([-16.097176, -29.891157, 15.987688])
        self.planarMesh.append([-16.176304, -29.859991, 15.947966])
        self.planarMesh.append([-16.071451, -29.900553, 15.912505])
        self.planarMesh.append([-16.092241, -29.893408, 16.020439])
        self.planarMesh.append([-16.007210, -29.926180, 15.967641])
        self.planarMesh.append([-16.064457, -29.904951, 16.090832])
        planarMeshObject = Mesh.Mesh(self.planarMesh)

        from pivy import coin
        import FreeCADGui

        Mesh.show(planarMeshObject)
        view = FreeCADGui.ActiveDocument.ActiveView
        view.setAxisCross(False)
        pc = coin.SoGetPrimitiveCountAction()
        pc.apply(view.getSceneGraph())
        self.assertTrue(pc.getTriangleCount() == 2)
        # self.assertTrue(pc.getPointCount() == 6)

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument("MeshTest")


# Threads


def loadFile(name):
    # lock.acquire()
    mesh = Mesh.Mesh()
    # FreeCAD.Console.PrintMessage("Create mesh instance\n")
    # lock.release()
    mesh.read(name)
    # FreeCAD.Console.PrintMessage("Mesh loaded successfully.\n")


def createMesh(r, s):
    # FreeCAD.Console.PrintMessage("Create sphere (%s,%s)...\n"%(r,s))
    mesh = Mesh.createSphere(r, s)
    # FreeCAD.Console.PrintMessage("... destroy sphere\n")


class LoadMeshInThreadsCases(unittest.TestCase):
    def setUp(self):
        pass

    def testSphereMesh(self):
        for i in range(6, 8):
            thread.start_new(createMesh, (10.0, (i + 1) * 20))
        time.sleep(10)

    def testLoadMesh(self):
        mesh = Mesh.createSphere(10.0, 100)  # a fine sphere
        name = tempfile.gettempdir() + os.sep + "mesh.stl"
        mesh.write(name)
        # FreeCAD.Console.PrintMessage("Write mesh to %s\n"%(name))
        # lock=thread.allocate_lock()
        for i in range(2):
            thread.start_new(loadFile, (name,))
        time.sleep(1)

    def tearDown(self):
        pass


class PolynomialFitCases(unittest.TestCase):
    def setUp(self):
        pass

    def testFitGood(self):
        # symmetric
        v = []
        v.append(FreeCAD.Vector(0, 0, 0.0))
        v.append(FreeCAD.Vector(1, 0, 0.5))
        v.append(FreeCAD.Vector(2, 0, 0.0))
        v.append(FreeCAD.Vector(0, 1, 0.5))
        v.append(FreeCAD.Vector(1, 1, 1.0))
        v.append(FreeCAD.Vector(2, 1, 0.5))
        v.append(FreeCAD.Vector(0, 2, 0.0))
        v.append(FreeCAD.Vector(1, 2, 0.5))
        v.append(FreeCAD.Vector(2, 2, 0.0))
        d = Mesh.polynomialFit(v)
        c = d["Coefficients"]
        # print ("Polynomial: f(x,y)=%f*x^2%+f*y^2%+f*x*y%+f*x%+f*y%+f" % (c[0],c[1],c[2],c[3],c[4],c[5]))
        for i in d["Residuals"]:
            self.assertTrue(math.fabs(i) < 0.0001, "Too high residual %f" % math.fabs(i))

    def testFitExact(self):
        # symmetric
        v = []
        v.append(FreeCAD.Vector(0, 0, 0.0))
        v.append(FreeCAD.Vector(1, 0, 0.0))
        v.append(FreeCAD.Vector(2, 0, 0.0))
        v.append(FreeCAD.Vector(0, 1, 0.0))
        v.append(FreeCAD.Vector(1, 1, 1.0))
        v.append(FreeCAD.Vector(2, 1, 0.0))
        d = Mesh.polynomialFit(v)
        c = d["Coefficients"]
        # print ("Polynomial: f(x,y)=%f*x^2%+f*y^2%+f*x*y%+f*x%+f*y%+f" % (c[0],c[1],c[2],c[3],c[4],c[5]))
        for i in d["Residuals"]:
            self.assertTrue(math.fabs(i) < 0.0001, "Too high residual %f" % math.fabs(i))

    def testFitBad(self):
        # symmetric
        v = []
        v.append(FreeCAD.Vector(0, 0, 0.0))
        v.append(FreeCAD.Vector(1, 0, 0.0))
        v.append(FreeCAD.Vector(2, 0, 0.0))
        v.append(FreeCAD.Vector(0, 1, 0.0))
        v.append(FreeCAD.Vector(1, 1, 1.0))
        v.append(FreeCAD.Vector(2, 1, 0.0))
        v.append(FreeCAD.Vector(0, 2, 0.0))
        v.append(FreeCAD.Vector(1, 2, 0.0))
        v.append(FreeCAD.Vector(2, 2, 0.0))
        d = Mesh.polynomialFit(v)
        c = d["Coefficients"]
        # print ("Polynomial: f(x,y)=%f*x^2%+f*y^2%+f*x*y%+f*x%+f*y%+f" % (c[0],c[1],c[2],c[3],c[4],c[5]))
        for i in d["Residuals"]:
            self.assertFalse(math.fabs(i) < 0.0001, "Residual %f must be higher" % math.fabs(i))

    def tearDown(self):
        pass


class NastranReader(unittest.TestCase):
    def setUp(self):
        self.test_dir = join(FreeCAD.getHomePath(), "Mod", "Mesh", "App", "TestData")

    def testEightCharGRIDElement(self):
        m = Mesh.read(f"{self.test_dir}/NASTRAN_Test_GRID_CQUAD4.bdf")
        self.assertEqual(m.CountPoints, 10)
        self.assertEqual(m.CountFacets, 8)  # Quads split into two triangles

    def testDelimitedGRIDElement(self):
        m = Mesh.read(f"{self.test_dir}/NASTRAN_Test_Delimited_GRID_CQUAD4.bdf")
        self.assertEqual(m.CountPoints, 10)
        self.assertEqual(m.CountFacets, 8)  # Quads split into two triangles

    def testSixteenCharGRIDElement(self):
        m = Mesh.read(f"{self.test_dir}/NASTRAN_Test_GRIDSTAR_CQUAD4.bdf")
        self.assertEqual(m.CountPoints, 4)
        self.assertEqual(m.CountFacets, 2)  # Quads split into two triangles

    def testCTRIA3Element(self):
        m = Mesh.read(f"{self.test_dir}/NASTRAN_Test_GRID_CTRIA3.bdf")
        self.assertEqual(m.CountPoints, 3)
        self.assertEqual(m.CountFacets, 1)

    def tearDown(self):
        pass


class MeshSubElement(unittest.TestCase):
    def setUp(self):
        self.mesh = Mesh.createBox(1.0, 1.0, 1.0)

    def testCenterOfGravity(self):
        c = self.mesh.CenterOfGravity
        self.assertEqual(c, Base.Vector(0.0, 0.0, 0.0))

    def testSubElements(self):
        types = self.mesh.getElementTypes()
        self.assertIn("Mesh", types)
        self.assertIn("Segment", types)

    def testCountSubElements(self):
        self.assertEqual(self.mesh.countSubElements("Mesh"), 1)
        self.assertEqual(self.mesh.countSubElements("Segment"), 0)

    def testFacesFromSubElement(self):
        element = self.mesh.getFacesFromSubElement("Mesh", 0)
        self.assertIsInstance(element, tuple)
        self.assertEqual(len(element), 2)
        self.assertEqual(len(element[0]), 8)
        self.assertEqual(len(element[1]), 12)

    def testSegmentSubElement(self):
        self.mesh.addSegment([0, 2, 4, 6, 8])
        self.assertEqual(self.mesh.countSegments(), 1)
        self.assertEqual(self.mesh.countSubElements("Segment"), 1)
        element = self.mesh.getFacesFromSubElement("Segment", 0)
        self.assertIsInstance(element, tuple)
        self.assertEqual(len(element), 2)
        self.assertEqual(len(element[0]), 7)
        self.assertEqual(len(element[1]), 5)
        segment = self.mesh.meshFromSegment(self.mesh.getSegment(0))
        self.assertEqual(segment.CountPoints, 7)
        self.assertEqual(segment.CountFacets, 5)

    def tearDown(self):
        pass


class MeshProperty(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("MeshTest")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def testMaterial(self):
        mesh = self.doc.addObject("Mesh::Feature", "Sphere")
        mesh.Mesh = Mesh.createBox(1.0, 1.0, 1.0)
        len1 = int(mesh.Mesh.CountFacets / 2)
        len2 = int(mesh.Mesh.CountFacets - len1)
        material = {"transparency": [0.2] * len1 + [0.8] * len2}
        material["binding"] = MeshEnums.Binding.PER_FACE
        material["ambientColor"] = [(1, 0, 0)] * (len1 + len2)
        material["diffuseColor"] = [(0, 1, 0)] * (len1 + len2)
        material["specularColor"] = [(0, 0, 1)] * (len1 + len2)
        material["emissiveColor"] = [(1, 1, 1)] * (len1 + len2)
        material["shininess"] = [0.3] * (len1 + len2)

        mesh.addProperty("Mesh::PropertyMaterial", "Material")
        mesh.Material = material

        TempPath = tempfile.gettempdir()
        SaveName = TempPath + os.sep + "mesh_with_material.FCStd"
        self.doc.saveAs(SaveName)
        FreeCAD.closeDocument(self.doc.Name)

        self.doc = FreeCAD.openDocument(SaveName)
        mesh2 = self.doc.Sphere
        material2 = mesh2.Material

        self.assertEqual(int(material2["binding"]), int(MeshEnums.Binding.PER_FACE))
        self.assertEqual(len(material2["ambientColor"]), len1 + len2)
        self.assertEqual(len(material2["diffuseColor"]), len1 + len2)
        self.assertEqual(len(material2["specularColor"]), len1 + len2)
        self.assertEqual(len(material2["emissiveColor"]), len1 + len2)
        self.assertEqual(len(material2["shininess"]), len1 + len2)
        self.assertEqual(len(material2["transparency"]), len1 + len2)
