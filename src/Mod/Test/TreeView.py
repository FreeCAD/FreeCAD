
# TreeView test module

import time
import unittest
import FreeCAD

from PySide import QtCore, QtGui
import FreeCADGui


class TreeViewTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def getTreeViewDocItem(self, docName):
        mainWnd = FreeCADGui.getMainWindow()
        treeDock = mainWnd.findChild(QtGui.QDockWidget, "Tree View")
        if treeDock is None:
            treeDock = mainWnd.findChild(QtGui.QDockWidget, "Combo View")
            if not treeDock is None:
                tabWidget = treeDock.findChild(QtGui.QTabWidget)
                if not tabWidget is None:
                    tabWidget.setCurrentIndex(0)

        self.assertTrue(not treeDock is None, "No Tree View docks available")

        treeView = treeDock.findChild(QtGui.QTreeWidget)
        self.assertTrue(not treeView is None, "No Tree View widget found")

        appNode = treeView.topLevelItem(0)
        self.assertTrue(not appNode is None, "No Application top level node")

        docNode = next((appNode.child(i) for i in range(appNode.childCount())
                                         if appNode.child(i).text(0).startswith(docName)), None)
        self.assertTrue(not docNode is None, "No test Document node")

        return docNode

    def waitForTreeViewSync(self):
        start = time.time()
        while time.time() < start + 0.5:
            FreeCADGui.updateGui()

    def testMoveTransposableItems(self):
        # Makes sense only if Gui is shown
        if not FreeCAD.GuiUp:
            return

        FreeCAD.TestEnvironment = True

        doc = FreeCAD.newDocument("TreeViewTest1")
        box = doc.addObject("Part::Box", "Box")
        cyl = doc.addObject("Part::Cylinder", "Cylinder")
        sph = doc.addObject("Part::Sphere", "Sphere")
        doc.recompute()

        viewBox = doc.getObject("Box").ViewObject
        viewCyl = doc.getObject("Cylinder").ViewObject
        viewSph = doc.getObject("Sphere").ViewObject

        self.waitForTreeViewSync()
        docNode = self.getTreeViewDocItem("TreeViewTest1")

        nodeNames = [ docNode.child(i).text(0) for i in range(docNode.childCount()) ]
        self.assertTrue(nodeNames == [ "Box", "Cylinder", "Sphere" ],
                        "Unexpected initial tree view order: " + str(nodeNames))

        baseRank = viewBox.TreeRank
        self.assertTrue([ viewBox.TreeRank, viewCyl.TreeRank, viewSph.TreeRank ]
                        == [ baseRank, baseRank + 1, baseRank + 2 ],
                        "Unexpected initial state of tree ranks")

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(box)
        self.waitForTreeViewSync()
        
        FreeCADGui.runCommand("Std_GroupMoveDown")
        self.waitForTreeViewSync()

        FreeCADGui.runCommand("Std_GroupMoveDown")
        self.waitForTreeViewSync()

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(sph)
        self.waitForTreeViewSync()

        FreeCADGui.runCommand("Std_GroupMoveUp")
        self.waitForTreeViewSync()

        nodeNames = [ docNode.child(i).text(0) for i in range(docNode.childCount()) ]
        self.assertTrue(nodeNames == [ "Sphere", "Cylinder", "Box" ],
                        "Unexpected final tree view order: " + str(nodeNames))
        self.assertTrue([ viewBox.TreeRank, viewCyl.TreeRank, viewSph.TreeRank ]
                        == [ baseRank + 2, baseRank + 1, baseRank ],
                        "Unexpected final state of tree ranks")

        del FreeCAD.TestEnvironment


    def testMoveUnmovableItems(self):
        # Makes sense only if Gui is shown
        if not FreeCAD.GuiUp:
            return

        FreeCAD.TestEnvironment = True

        doc = FreeCAD.newDocument("TreeViewTest2")
        box = doc.addObject("Part::Box", "Box")
        cyl = doc.addObject("Part::Cylinder", "Cylinder")
        doc.recompute()

        viewBox = doc.getObject("Box").ViewObject
        viewCyl = doc.getObject("Cylinder").ViewObject

        self.waitForTreeViewSync()
        docNode = self.getTreeViewDocItem("TreeViewTest2")

        nodeNames = [ docNode.child(i).text(0) for i in range(docNode.childCount()) ]
        self.assertTrue(nodeNames == [ "Box", "Cylinder" ],
                        "Unexpected initial tree view order: " + str(nodeNames))

        baseRank = viewBox.TreeRank
        self.assertTrue([ viewBox.TreeRank, viewCyl.TreeRank ] == [ baseRank, baseRank + 1 ],
                        "Unexpected initial state of tree ranks")

        cut = doc.addObject("Part::Cut", "Cut")
        cut.Base = box
        cut.Tool = cyl
        self.waitForTreeViewSync()

        nodeNames = [ docNode.child(i).text(0) for i in range(docNode.childCount()) ]
        self.assertTrue(nodeNames == [ "Cut" ],
                        "Unexpected tree view order after cut: " + str(nodeNames))

        cutNode = docNode.child(0)
        cutNode.setExpanded(True)

        nodeNames = [ cutNode.child(i).text(0) for i in range(cutNode.childCount()) ]
        self.assertTrue(nodeNames == [ "Box", "Cylinder" ],
                        "Unexpected tree view order of cut components: " + str(nodeNames))
        self.assertTrue([ viewBox.TreeRank, viewCyl.TreeRank ] == [ baseRank, baseRank + 1 ],
                        "Unexpected state of tree ranks after cut")

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(box)
        self.waitForTreeViewSync()

        FreeCADGui.runCommand("Std_GroupMoveDown")
        self.waitForTreeViewSync()

        nodeNames = [ cutNode.child(i).text(0) for i in range(cutNode.childCount()) ]
        self.assertTrue(nodeNames == [ "Box", "Cylinder" ],
                        "Unexpected tree view order of cut components after move down: " + str(nodeNames))
        self.assertTrue([ viewBox.TreeRank, viewCyl.TreeRank ] == [ baseRank, baseRank + 1 ],
                        "Unexpected state of tree ranks after move down")

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(cyl)
        self.waitForTreeViewSync()

        FreeCADGui.runCommand("Std_GroupMoveUp")
        self.waitForTreeViewSync()

        nodeNames = [ cutNode.child(i).text(0) for i in range(cutNode.childCount()) ]
        self.assertTrue(nodeNames == [ "Box", "Cylinder" ],
                        "Unexpected tree view order of cut components after move up: " + str(nodeNames))
        self.assertTrue([ viewBox.TreeRank, viewCyl.TreeRank ] == [ baseRank, baseRank + 1 ],
                        "Unexpected state of tree ranks after move up")
