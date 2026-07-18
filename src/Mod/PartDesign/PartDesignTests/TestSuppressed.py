# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Chris Jones github.com/ipatch
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

"""Tests for SuppressibleExtension persistence (github issue #24587).

When a PartDesign feature is suppressed, the tree view should show a red slash
overlay on the icon AND strikethrough text on the label. Previously, the
strikethrough was lost after save/close/reopen because it was only applied via
a transient signal. The fix in Tree.cpp queries the Suppressed state in
DocumentObjectItem::testStatus() so the strikethrough is re-applied on load.

These tests verify that the underlying Suppressed property and extension
persist correctly across save/reload cycles.

To run the gui tests as github ci runs them, useful for running gui tests locally
use the below command, ie.

xvfb-run \
/path/to/local/freecad/install/bin/FreeCAD -t TestPartDesignGui.TestSuppressedStrikethrough
"""

import os
import tempfile
import time
import unittest

import FreeCAD
import FreeCADGui
import TestSketcherApp

from PySide import QtGui


class TestSuppressed(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestSuppressed")

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

    def _createBodyWithPadAndFillet(self):
        """Create a Body with a Pad (10x10x10 box) and a Fillet."""
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.PadSketch = self.Doc.addObject("Sketcher::SketchObject", "SketchPad")
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 10
        self.Doc.recompute()
        self.Fillet = self.Doc.addObject("PartDesign::Fillet", "Fillet")
        self.Body.addObject(self.Fillet)
        self.Fillet.Base = (self.Pad, ["Edge1"])
        self.Fillet.Radius = 1.0
        self.Doc.recompute()

    def testSuppressibleExtensionExists(self):
        """PartDesign features should have SuppressibleExtension."""
        self._createBodyWithPadAndFillet()
        self.assertTrue(
            self.Pad.hasExtension("App::SuppressibleExtension"),
            "Pad should have SuppressibleExtension",
        )
        self.assertTrue(
            self.Fillet.hasExtension("App::SuppressibleExtension"),
            "Fillet should have SuppressibleExtension",
        )

    def testSuppressChangesShape(self):
        """Suppressing a feature should change the body shape."""
        self._createBodyWithPadAndFillet()
        volumeWithFillet = self.Body.Shape.Volume

        self.Fillet.Suppressed = True
        self.Doc.recompute()
        self.assertTrue(self.Fillet.Suppressed)

        volumeWithoutFillet = self.Body.Shape.Volume
        self.assertNotAlmostEqual(
            volumeWithFillet,
            volumeWithoutFillet,
            places=2,
            msg="Suppressing fillet should change body volume",
        )

        # Unsuppress should restore
        self.Fillet.Suppressed = False
        self.Doc.recompute()
        self.assertFalse(self.Fillet.Suppressed)
        self.assertAlmostEqual(
            self.Body.Shape.Volume,
            volumeWithFillet,
            places=2,
            msg="Unsuppressing should restore original volume",
        )

    def testSuppressedPersistsAfterReload(self):
        """Suppressed property must persist after save/close/reopen (github issue #24587)."""
        self._createBodyWithPadAndFillet()

        self.Fillet.Suppressed = True
        self.Doc.recompute()
        volumeSuppressed = self.Body.Shape.Volume

        # Save, close, reopen
        with tempfile.TemporaryDirectory(prefix="freecad_test_suppressed_") as tmpdir:
            filepath = os.path.join(tmpdir, "test_suppressed.FCStd")
            self.Doc.saveAs(filepath)
            FreeCAD.closeDocument(self.Doc.Name)
            self.Doc = FreeCAD.openDocument(filepath)

        filletReloaded = self.Doc.getObject("Fillet")
        bodyReloaded = self.Doc.getObject("Body")

        self.assertIsNotNone(filletReloaded)
        self.assertIsNotNone(bodyReloaded)
        self.assertTrue(
            filletReloaded.Suppressed,
            "Fillet.Suppressed should be True after reload",
        )
        self.assertTrue(
            filletReloaded.hasExtension("App::SuppressibleExtension"),
            "SuppressibleExtension should persist after reload",
        )
        self.assertAlmostEqual(
            bodyReloaded.Shape.Volume,
            volumeSuppressed,
            places=2,
            msg="Body volume should reflect suppressed state after reload",
        )

    def testUnsuppressedPersistsAfterReload(self):
        """A feature suppressed then unsuppressed should stay unsuppressed after reload."""
        self._createBodyWithPadAndFillet()

        self.Fillet.Suppressed = True
        self.Doc.recompute()
        self.Fillet.Suppressed = False
        self.Doc.recompute()
        volumeActive = self.Body.Shape.Volume

        with tempfile.TemporaryDirectory(prefix="freecad_test_suppressed_") as tmpdir:
            filepath = os.path.join(tmpdir, "test_unsuppressed.FCStd")
            self.Doc.saveAs(filepath)
            FreeCAD.closeDocument(self.Doc.Name)
            self.Doc = FreeCAD.openDocument(filepath)

        filletReloaded = self.Doc.getObject("Fillet")
        bodyReloaded = self.Doc.getObject("Body")

        self.assertFalse(
            filletReloaded.Suppressed,
            "Fillet.Suppressed should be False after reload",
        )
        self.assertAlmostEqual(
            bodyReloaded.Shape.Volume,
            volumeActive,
            places=2,
            msg="Body volume should reflect active fillet after reload",
        )

    def testMultipleSuppressedFeaturesReload(self):
        """Multiple suppressed features should all persist after reload."""
        self._createBodyWithPadAndFillet()

        # Add a Chamfer too
        self.Chamfer = self.Doc.addObject("PartDesign::Chamfer", "Chamfer")
        self.Body.addObject(self.Chamfer)
        self.Chamfer.Base = (self.Fillet, ["Edge2"])
        self.Chamfer.Size = 0.5
        self.Doc.recompute()

        self.Fillet.Suppressed = True
        self.Chamfer.Suppressed = True
        self.Doc.recompute()

        with tempfile.TemporaryDirectory(prefix="freecad_test_suppressed_") as tmpdir:
            filepath = os.path.join(tmpdir, "test_multi_suppressed.FCStd")
            self.Doc.saveAs(filepath)
            FreeCAD.closeDocument(self.Doc.Name)
            self.Doc = FreeCAD.openDocument(filepath)

        self.assertTrue(
            self.Doc.getObject("Fillet").Suppressed,
            "Fillet should remain suppressed after reload",
        )
        self.assertTrue(
            self.Doc.getObject("Chamfer").Suppressed,
            "Chamfer should remain suppressed after reload",
        )


def _findTreeWidget():
    """Find the main tree widget (QTreeWidget) in the FreeCAD GUI."""
    mw = FreeCADGui.getMainWindow()
    if mw is None:
        return None
    trees = mw.findChildren(QtGui.QTreeWidget)
    # under xvfb, objectName may be empty - pick the tree with content
    for tree in trees:
        if tree.topLevelItemCount() > 0:
            return tree
    # no tree has items yet, return the last one (usually the model tree)
    if trees:
        return trees[-1]
    return None


def _getTreeItemStrikeOut(tree, label, parent=None):
    """Recursively find a QTreeWidgetItem by label and return its strikeOut state.

    Returns the font's strikeOut() bool, or None if item not found.
    Reads the font immediately to avoid C++ object lifetime issues.
    """
    if parent is None:
        for i in range(tree.topLevelItemCount()):
            result = _getTreeItemStrikeOut(tree, label, tree.topLevelItem(i))
            if result is not None:
                return result
    else:
        if parent.text(0) == label:
            # Read font data immediately before the item can be invalidated
            return parent.font(0).strikeOut()
        for i in range(parent.childCount()):
            result = _getTreeItemStrikeOut(tree, label, parent.child(i))
            if result is not None:
                return result
    return None


class TestSuppressedStrikethrough(unittest.TestCase):
    """Test that suppressed features show strikethrough in the tree after reload."""

    def setUp(self):
        if not hasattr(FreeCADGui, "getMainWindow") or FreeCADGui.getMainWindow() is None:
            self.skipTest("Requires GUI")
        self.Doc = FreeCAD.newDocument("TestSuppressedStrikethrough")
        FreeCADGui.activateView("Gui::View3DInventor", True)

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

    def _createBodyWithBox(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        FreeCADGui.activeView().setActiveObject("pdbody", self.Body)
        self.Box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        self.Body.addObject(self.Box)
        self.Box.Length = 10.0
        self.Box.Width = 10.0
        self.Box.Height = 10.0
        self.Doc.recompute()
        QtGui.QApplication.processEvents()

    def testSuppressedShowsStrikethrough(self):
        """Suppressing a feature should immediately show strikethrough in tree."""
        self._createBodyWithBox()

        self.Box.Suppressed = True
        self.Doc.recompute()
        QtGui.QApplication.processEvents()

        tree = _findTreeWidget()
        self.assertIsNotNone(tree, "Could not find tree widget")

        # retry up to 50 times with 50ms naps
        strikeOut = None
        for _ in range(50):
            QtGui.QApplication.processEvents()
            strikeOut = _getTreeItemStrikeOut(tree, "Box")
            if strikeOut is not None:
                break
            time.sleep(0.05)

        self.assertIsNotNone(strikeOut, "Could not find 'Box' tree item")
        self.assertTrue(
            strikeOut,
            "Suppressed feature should have strikethrough font in tree",
        )

    def testUnsuppressedNoStrikethrough(self):
        """Unsuppressing a feature should remove strikethrough."""
        self._createBodyWithBox()

        self.Box.Suppressed = True
        self.Doc.recompute()
        QtGui.QApplication.processEvents()

        self.Box.Suppressed = False
        self.Doc.recompute()
        QtGui.QApplication.processEvents()

        tree = _findTreeWidget()

        strikeOut = None
        for _ in range(50):
            QtGui.QApplication.processEvents()
            strikeOut = _getTreeItemStrikeOut(tree, "Box")
            if strikeOut is not None:
                break
            time.sleep(0.05)

        self.assertIsNotNone(strikeOut, "Could not find 'Box' tree item")
        self.assertFalse(
            strikeOut,
            "Unsuppressed feature should not have strikethrough font",
        )

    def testStrikethroughPersistsAfterReload(self):
        """github issue #24587: strikethrough must persist after save/close/reopen."""
        self._createBodyWithBox()

        self.Box.Suppressed = True
        self.Doc.recompute()
        QtGui.QApplication.processEvents()

        # Save
        with tempfile.TemporaryDirectory(prefix="freecad_test_suppressed_") as tmpdir:
            filepath = os.path.join(tmpdir, "test_strikethrough.FCStd")
            self.Doc.saveAs(filepath)

            # Close
            FreeCAD.closeDocument(self.Doc.Name)
            QtGui.QApplication.processEvents()

            # Reopen
            self.Doc = FreeCAD.openDocument(filepath)
            QtGui.QApplication.processEvents()

        # Verify the property persisted
        boxReloaded = self.Doc.getObject("Box")
        self.assertIsNotNone(boxReloaded, "Box should exist in reloaded doc")
        self.assertTrue(boxReloaded.Suppressed, "Box.Suppressed should be True after reload")

        # wait & verify for the tree to populate - it's possible the ci operates slower than my local m1
        tree = _findTreeWidget()
        self.assertIsNotNone(tree, "could not find tree widget after reload")

        strikeOut = None
        for _ in range(50):
            QtGui.QApplication.processEvents()
            strikeOut = _getTreeItemStrikeOut(tree, "Box")
            if strikeOut is not None:
                break
            time.sleep(0.05)

        self.assertIsNotNone(strikeOut, "Could not find 'Box' tree item after reload")
        self.assertTrue(
            strikeOut,
            "Suppressed feature should have strikethrough font after file reload "
            "(github issue #24587)",
        )
