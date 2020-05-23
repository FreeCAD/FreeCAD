from PySide import QtCore
from PySide import QtGui
from PySide.QtTest import QTest

import FreeCAD
import FreeCADGui
import GuiTestSupport
import ObjectsFem


class TestGmshTaskPanel(GuiTestSupport.TaskPanelTest):

    def openTaskPanel(self, mesh):
        vDoc = mesh.ViewObject.Document
        vDoc.setEdit(mesh.Name)

    def closeTaskPanel(self, mesh):
        vDoc = mesh.ViewObject.Document
        vDoc.resetEdit()

    def testCubeDefaultMesh(self):
        cube = self.doc.addObject("Part::Box")
        self.doc.recompute()
        mesh = ObjectsFem.makeMeshGmsh(self.doc, "FEMMeshGmsh")
        mesh.Part = cube
        self.openTaskPanel(mesh)
        self.assertEqual(mesh.FemMesh.NodeCount, 0)
        self.clickButton(QtGui.QDialogButtonBox.Apply)
        self.clickButton(QtGui.QDialogButtonBox.Ok)
        self.assertNotEqual(mesh.FemMesh.NodeCount, 0)

    def testCubeNoMesh(self):
        cube = self.doc.addObject("Part::Box")
        self.doc.recompute()
        mesh = ObjectsFem.makeMeshGmsh(self.doc, "FEMMeshGmsh")
        mesh.Part = cube
        self.openTaskPanel(mesh)
        self.assertEqual(mesh.FemMesh.NodeCount, 0)
        self.clickButton(QtGui.QDialogButtonBox.Ok)
        self.assertEqual(mesh.FemMesh.NodeCount, 0)

    def testCubeMeshResolution(self):
        cube = self.doc.addObject("Part::Box")
        self.doc.recompute()

        meshMany = ObjectsFem.makeMeshGmsh(self.doc, "FEMMeshGmsh")
        meshMany.Part = cube
        self.openTaskPanel(meshMany)
        self.loadTaskPanel("GmshMesh")
        maxIf = self.getChild("if_max")
        minIf = self.getChild("if_min")
        maxIf.setText("")
        minIf.setText("")
        QTest.keyClicks(maxIf, "1.5")
        QTest.keyClicks(minIf, "0.5")
        self.clickButton(QtGui.QDialogButtonBox.Apply)
        self.clickButton(QtGui.QDialogButtonBox.Ok)

        meshLess = ObjectsFem.makeMeshGmsh(self.doc, "FEMMeshGmsh")
        meshLess.Part = cube
        self.openTaskPanel(meshLess)
        self.loadTaskPanel("GmshMesh")
        maxIf = self.getChild("if_max")
        minIf = self.getChild("if_min")
        maxIf.setText("")
        minIf.setText("")
        QTest.keyClicks(maxIf, "2")
        QTest.keyClicks(minIf, "1.5")
        self.clickButton(QtGui.QDialogButtonBox.Apply)
        self.clickButton(QtGui.QDialogButtonBox.Ok)

        self.assertGreater(
            meshMany.FemMesh.NodeCount,
            meshLess.FemMesh.NodeCount)
