# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest
import FreeCAD
import FreeCADGui
import Mesh
from pivy import coin


class PivyTestCases(unittest.TestCase):
    def setUp(self):
        self.planarMesh = []
        FreeCAD.newDocument("MeshTest")

    def testRayPick(self):
        self.planarMesh.append([-16.097176, -29.891157, 15.987688])
        self.planarMesh.append([-16.176304, -29.859991, 15.947966])
        self.planarMesh.append([-16.071451, -29.900553, 15.912505])
        self.planarMesh.append([-16.092241, -29.893408, 16.020439])
        self.planarMesh.append([-16.007210, -29.926180, 15.967641])
        self.planarMesh.append([-16.064457, -29.904951, 16.090832])
        planarMeshObject = Mesh.Mesh(self.planarMesh)

        Mesh.show(planarMeshObject)
        view = FreeCADGui.ActiveDocument.ActiveView.getViewer()
        rp = coin.SoRayPickAction(view.getSoRenderManager().getViewportRegion())
        rp.setRay(coin.SbVec3f(-16.05, 16.0, 16.0), coin.SbVec3f(0, -1, 0))
        rp.apply(view.getSoRenderManager().getSceneGraph())
        pp = rp.getPickedPoint()
        self.assertIsNotNone(pp)
        det = pp.getDetail()
        self.assertEqual(det.getTypeId(), coin.SoFaceDetail.getClassTypeId())
        det = coin.cast(det, det.getTypeId().getName().getString())
        self.assertEqual(det.getFaceIndex(), 1)

    def testPrimitiveCount(self):
        self.planarMesh.append([-16.097176, -29.891157, 15.987688])
        self.planarMesh.append([-16.176304, -29.859991, 15.947966])
        self.planarMesh.append([-16.071451, -29.900553, 15.912505])
        self.planarMesh.append([-16.092241, -29.893408, 16.020439])
        self.planarMesh.append([-16.007210, -29.926180, 15.967641])
        self.planarMesh.append([-16.064457, -29.904951, 16.090832])
        planarMeshObject = Mesh.Mesh(self.planarMesh)

        view = FreeCADGui.ActiveDocument.ActiveView
        view.setAxisCross(False)
        pc = coin.SoGetPrimitiveCountAction()
        pc.apply(view.getSceneGraph())
        pre_mesh_tc = pc.getTriangleCount()
        Mesh.show(planarMeshObject)
        pc.apply(view.getSceneGraph())
        mesh_tc = pc.getTriangleCount() - pre_mesh_tc
        self.assertEqual(mesh_tc, 2)

    def tearDown(self):
        FreeCAD.closeDocument("MeshTest")
