# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ---------------------------------------------------------------------------
#    Copyright (c) 2026 Chris Jones github.com/ipatch                      *
#                                                                          *
#    This file is part of the FreeCAD CAx development system.              *
#                                                                          *
#    This program is free software; you can redistribute it and/or modify  *
#    it under the terms of the GNU Lesser General Public License (LGPL)    *
#    as published by the Free Software Foundation; either version 2 of     *
#    the License, or (at your option) any later version.                   *
#    for detail see the LICENCE text file.                                 *
#                                                                          *
#    FreeCAD is distributed in the hope that it will be useful,            *
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#    GNU Library General Public License for more details.                  *
#                                                                          *
#    You should have received a copy of the GNU Library General Public     *
#    License along with FreeCAD; if not, write to the Free Software        *
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307   *
#    USA                                                                   *
#                                                                          *
# ---------------------------------------------------------------------------

"""Tests for SuppressibleExtension persistence (issue #24587).

When a PartDesign feature is suppressed, the tree view should show a red slash
overlay on the icon AND strikethrough text on the label. Previously, the
strikethrough was lost after save/close/reopen because it was only applied via
a transient signal. The fix in Tree.cpp queries the Suppressed state in
DocumentObjectItem::testStatus() so the strikethrough is re-applied on load.

These tests verify that the underlying Suppressed property and extension
persist correctly across save/reload cycles.
"""

import os
import tempfile
import unittest

import FreeCAD
import FreeCADGui
import Part
import Sketcher
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
        """Suppressed property must persist after save/close/reopen (issue #24587)."""
        self._createBodyWithPadAndFillet()

        self.Fillet.Suppressed = True
        self.Doc.recompute()
        volumeSuppressed = self.Body.Shape.Volume

        # Save, close, reopen
        tmpdir = tempfile.mkdtemp(prefix="freecad_test_suppressed_")
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

        # Cleanup temp file
        os.remove(filepath)
        os.rmdir(tmpdir)

    def testUnsuppressedPersistsAfterReload(self):
        """A feature suppressed then unsuppressed should stay unsuppressed after reload."""
        self._createBodyWithPadAndFillet()

        self.Fillet.Suppressed = True
        self.Doc.recompute()
        self.Fillet.Suppressed = False
        self.Doc.recompute()
        volumeActive = self.Body.Shape.Volume

        tmpdir = tempfile.mkdtemp(prefix="freecad_test_suppressed_")
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

        os.remove(filepath)
        os.rmdir(tmpdir)

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

        tmpdir = tempfile.mkdtemp(prefix="freecad_test_suppressed_")
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

        os.remove(filepath)
        os.rmdir(tmpdir)


def _findTreeWidget():
    """Find the main tree widget (QTreeWidget) in the FreeCAD GUI."""
    mw = FreeCADGui.getMainWindow()
    trees = mw.findChildren(QtGui.QTreeWidget)
    for tree in trees:
        if tree.objectName() == "Tree view":
            return tree
    # Fallback: check header text
    for tree in trees:
        if tree.headerItem().text(0) == "Labels & Attributes":
            return tree
    if trees:
        return trees[0]
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

        strikeOut = _getTreeItemStrikeOut(tree, "Box")
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
        strikeOut = _getTreeItemStrikeOut(tree, "Box")
        self.assertIsNotNone(strikeOut, "Could not find 'Box' tree item")
        self.assertFalse(
            strikeOut,
            "Unsuppressed feature should not have strikethrough font",
        )

    def testStrikethroughPersistsAfterReload(self):
        """Issue #24587: strikethrough must persist after save/close/reopen."""
        self._createBodyWithBox()

        self.Box.Suppressed = True
        self.Doc.recompute()
        QtGui.QApplication.processEvents()

        # Save
        tmpdir = tempfile.mkdtemp(prefix="freecad_test_strikethrough_")
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

        # Verify the tree item has strikethrough
        tree = _findTreeWidget()
        self.assertIsNotNone(tree, "Could not find tree widget after reload")

        strikeOut = _getTreeItemStrikeOut(tree, "Box")
        self.assertIsNotNone(strikeOut, "Could not find 'Box' tree item after reload")
        self.assertTrue(
            strikeOut,
            "Suppressed feature should have strikethrough font after file reload (issue #24587)",
        )

        # Cleanup
        os.remove(filepath)
        os.rmdir(tmpdir)
