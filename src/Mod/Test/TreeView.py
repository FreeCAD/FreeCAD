
# TreeView test module

import os
import time
import tempfile
import unittest
import FreeCAD

from PySide import QtCore, QtGui
import FreeCADGui


class TreeViewTestCase(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def getTreeWidget(self):
        mainWnd = FreeCADGui.getMainWindow()
        treeDock = mainWnd.findChild(QtGui.QDockWidget, "Tree View")
        if treeDock is None:
            treeDock = mainWnd.findChild(QtGui.QDockWidget, "Combo View")
            if not treeDock is None:
                tabWidget = treeDock.findChild(QtGui.QTabWidget, "combiTab")
                if not tabWidget is None:
                    tabWidget.setCurrentIndex(0)
        self.assertTrue(not treeDock is None, "No Tree View docks available")

        treeView = treeDock.findChild(QtGui.QTreeWidget)
        self.assertTrue(not treeView is None, "No Tree View widget found")

        return treeView

    def waitForTreeViewSync(self):
        start = time.time()
        while time.time() < start + 0.5:
            FreeCADGui.updateGui()

    def selectDocItem(self, docItem):

        treeView = self.getTreeWidget()

        if docItem.TypeId == "App::Document":
            appNode = treeView.topLevelItem(0)
            self.assertTrue(not appNode is None, "No Application top level node")

            docNode = next((appNode.child(i) for i in range(appNode.childCount())
                                             if appNode.child(i).text(0) == docItem.Label), None)
            self.assertTrue(not docNode is None, "No test Document node")
            treeView.setCurrentItem(docNode)
        else:
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.Selection.addSelection(docItem)
            self.waitForTreeViewSync()

    def trySwapOuterTreeViewItems(self, docItem, transposable):

        self.selectDocItem(docItem)
        treeView = self.getTreeWidget()

        selected = treeView.selectedItems()
        self.assertTrue(len(selected) == 1,
                        "Unexpected count of selected items: " + str(len(selected)))
        selected = selected[0]

        originalState = [ selected.child(i).text(0) for i in range(selected.childCount()) ]
        self.assertTrue(len(originalState) >= 1,
                        "No children found in item " + selected.text(0))

        targetState = originalState.copy()
        if transposable:
            targetState[0], targetState[-1] = targetState[-1], targetState[0]

        treeView.setCurrentItem(selected.child(0))
        self.waitForTreeViewSync()

        # One move down attempt more to test boundary behaviour
        for i in range(len(originalState)):
            FreeCADGui.runCommand("Std_GroupMoveDown")
            self.waitForTreeViewSync()

        treeView.setCurrentItem(selected.child(len(originalState) - 2 if len(originalState) > 1 else 0))
        self.waitForTreeViewSync()

        # One move up attempt more to test boundary behaviour
        for i in range(len(originalState) - 1):
            FreeCADGui.runCommand("Std_GroupMoveUp")
            self.waitForTreeViewSync()

        finalState = [ selected.child(i).text(0) for i in range(selected.childCount()) ]
        self.assertTrue(targetState == finalState,
                        "Unexpected final state: %s\nExpected: %s" % (finalState, targetState))

    def getChildrenOf(self, docItem):

        self.selectDocItem(docItem)
        treeView = self.getTreeWidget()

        selected = treeView.selectedItems()
        self.assertTrue(len(selected) == 1,
                        "Unexpected count of selected items: " + str(len(selected)))
        selected = selected[0]

        return [ selected.child(i).text(0) for i in range(selected.childCount()) ]


    def testMoveTransposableItems(self):
        # Makes sense only if Gui is shown
        if not FreeCAD.GuiUp:
            return

        FreeCAD.TestEnvironment = True

        doc = FreeCAD.newDocument("TreeViewTest1")
        FreeCAD.setActiveDocument(doc.Name)

        box = doc.addObject("Part::Box", "Box")
        cyl = doc.addObject("Part::Cylinder", "Cylinder")
        sph = doc.addObject("Part::Sphere", "Sphere")
        con = doc.addObject("Part::Cone", "Cone")
        doc.recompute()

        self.trySwapOuterTreeViewItems(doc, True)

        grp = doc.addObject("App::DocumentObjectGroup", "Group")
        grp.addObjects([ box, cyl, sph, con ])
        doc.recompute()

        self.trySwapOuterTreeViewItems(grp, True)

        del FreeCAD.TestEnvironment


    def testMoveUnmovableItems(self):
        # Makes sense only if Gui is shown
        if not FreeCAD.GuiUp:
            return

        FreeCAD.TestEnvironment = True

        doc = FreeCAD.newDocument("TreeViewTest2")
        FreeCAD.setActiveDocument(doc.Name)

        sph = doc.addObject("Part::Sphere", "Sphere")
        con = doc.addObject("Part::Cone", "Cone")
        doc.recompute()

        cut = doc.addObject("Part::Cut", "Cut")
        cut.Base = sph
        cut.Tool = con
        doc.recompute()

        self.trySwapOuterTreeViewItems(cut, False)

        bdy = doc.addObject("PartDesign::Body", "Body")
        box = doc.addObject("PartDesign::AdditiveBox", "Box")
        bdy.addObject(box)
        doc.recompute()

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(bdy)
        self.waitForTreeViewSync()

        treeView = self.getTreeWidget()
        treeView.selectedItems()[0].setExpanded(True)
        self.waitForTreeViewSync()

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(doc.Name, bdy.Name, box.Name + ".Face6")
        self.waitForTreeViewSync()

        cha = bdy.newObject("PartDesign::Chamfer", "Chamfer")
        cha.Base = (box, ["Face6"])
        doc.recompute()

        cyl = doc.addObject("PartDesign::SubtractiveCylinder", "Cylinder")
        bdy.addObject(cyl)
        doc.recompute()

        self.trySwapOuterTreeViewItems(bdy, False)

        del FreeCAD.TestEnvironment


    def testItemOrderSaveAndRestore(self):
        # Makes sense only if Gui is shown
        if not FreeCAD.GuiUp:
            return

        FreeCAD.TestEnvironment = True

        doc = FreeCAD.newDocument("TreeViewTest3")
        FreeCAD.setActiveDocument(doc.Name)

        grp = doc.addObject("App::DocumentObjectGroup", "Group")
        box = doc.addObject("Part::Box", "Box")
        cyl = doc.addObject("Part::Cylinder", "Cylinder")
        sph = doc.addObject("Part::Sphere", "Sphere")
        con = doc.addObject("Part::Cone", "Cone")
        doc.recompute()

        origOrder = self.getChildrenOf(doc)
        self.assertTrue(origOrder == ["Group", "Box", "Cylinder", "Sphere", "Cone"])

        origOrderFile = tempfile.gettempdir() + os.sep + "TreeViewTest3_1.fcstd"
        doc.saveAs(origOrderFile)

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(con)
        FreeCADGui.Selection.addSelection(cyl)
        self.waitForTreeViewSync()

        FreeCADGui.runCommand("Std_GroupMoveUp")
        self.waitForTreeViewSync()

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(grp)
        FreeCADGui.Selection.addSelection(box)
        self.waitForTreeViewSync()

        FreeCADGui.runCommand("Std_GroupMoveDown")
        self.waitForTreeViewSync()

        newOrder = self.getChildrenOf(doc)
        self.assertTrue(newOrder == ["Cylinder", "Cone", "Sphere", "Group", "Box"])

        newOrderFile = tempfile.gettempdir() + os.sep + "TreeViewTest3_2.fcstd"
        doc.saveAs(newOrderFile)

        FreeCAD.closeDocument(doc.Name)
        self.waitForTreeViewSync()

        doc = FreeCAD.open(origOrderFile)
        order = self.getChildrenOf(doc)
        self.assertTrue(order == origOrder)

        doc = FreeCAD.open(newOrderFile)
        order = self.getChildrenOf(doc)
        self.assertTrue(order == newOrder)

        del FreeCAD.TestEnvironment
