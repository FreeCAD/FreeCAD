# SPDX-License-Identifier: LGPL-2.1-or-later
"""Rib command and task-panel smoke test."""

import unittest
import math

import FreeCAD as App
import FreeCADGui as Gui
import Part
from PySide import QtWidgets

from PartDesignTests import TestRib


class TestRibReferenceSelection(unittest.TestCase):
    def testSweepAngleFromVerticalProfile(self):
        fixture = TestRib.TestRib()
        fixture.setUp()
        try:
            Gui.activateWorkbench("PartDesignWorkbench")
            fixture.makeRib("L", "Line")
            fixture.profile.delGeometry(0)
            fixture.profile.addGeometry(
                Part.LineSegment(App.Vector(20, 14, 0), App.Vector(20, 27, 0)), False
            )
            fixture.rib.ExtendType = "Off"
            fixture.rib.Direction = App.Vector(-1, 0, 0)
            fixture.assertRib()
            original = fixture.rib.Direction
            self.assertTrue(Gui.activeDocument().setEdit(fixture.rib.Name))
            angle = Gui.getMainWindow().findChild(QtWidgets.QWidget, "ribSweepAngle")
            self.assertIsNotNone(angle)
            self.assertTrue(angle.isEnabled())
            self.assertLess((fixture.rib.Direction - original).Length, 1e-10)
            angle.setProperty("rawValue", 190.0)
            Gui.updateGui()
            expected = App.Vector(math.cos(math.radians(-170)), 0, math.sin(math.radians(-170)))
            self.assertLess((fixture.rib.Direction - expected).Length, 1e-10)
            fixture.assertRib()
            if App.ParamGet("User parameter:BaseApp/Preferences/Gui/Gizmos").GetBool(
                "EnableGizmos", True
            ):
                from pivy import coin

                search = coin.SoSearchAction()
                search.setName("RibSweepAngleDragger")
                search.setSearchingAll(True)
                search.apply(Gui.activeDocument().activeView().getSceneGraph())
                self.assertIsNotNone(search.getPath())
                container = search.getPath().getTail()
                self.assertTrue(container.getField("visible").getValue())
                for index in range(search.getPath().getLength()):
                    node = search.getPath().getNode(index)
                    if str(node.getTypeId().getName()) == "GizmoContainer":
                        self.assertTrue(node.getField("visible").getValue())
                normal = container.getField("rotation").getValue().multVec(coin.SbVec3f(0, 0, 1))
                self.assertAlmostEqual(normal[0], 0, places=5)
                self.assertAlmostEqual(normal[1], -1, places=5)
                self.assertAlmostEqual(normal[2], 0, places=5)
                dragger = container.getPart("dragger", False)
                pointer = dragger.getField("rotation").getValue().multVec(coin.SbVec3f(1, 0, 0))
                self.assertAlmostEqual(pointer[0], math.cos(math.radians(-170)), places=5)
                self.assertAlmostEqual(pointer[1], math.sin(math.radians(-170)), places=5)
                continuous = Gui.getMainWindow().findChild(QtWidgets.QWidget, "ribSweepDragAngle")
                self.assertIsNotNone(continuous)
                # Exercise the values delivered by a continuing rotation drag.
                # The internal angle must not be reset when the display wraps.
                dragger.getField("isActive").setValue(True)
                try:
                    for degrees in (359, 360, 361, 719, 720, 721, 0, -1, -360, -361):
                        continuous.setProperty("rawValue", float(degrees))
                        self.assertAlmostEqual(angle.property("rawValue"), degrees % 360)
                        self.assertAlmostEqual(continuous.property("rawValue"), degrees)
                        self.assertTrue(container.getField("visible").getValue())
                finally:
                    dragger.getField("isActive").setValue(False)
                angle.setProperty("rawValue", 360.0)
                self.assertEqual(angle.property("rawValue"), 0.0)
                angle.setProperty("rawValue", 190.0)
                fixture.assertRib()
        finally:
            Gui.Selection.clearSelection()
            Gui.activeDocument().resetEdit()
            Gui.Control.closeDialog()
            fixture.tearDown()

    def testCommandFromProfile(self):
        fixture = TestRib.TestRib()
        fixture.setUp()
        try:
            Gui.activateWorkbench("PartDesignWorkbench")
            fixture.makeRib("L", "Line")
            fixture.doc.removeObject(fixture.rib.Name)
            fixture.doc.recompute()
            Gui.activeDocument().activeView().setActiveObject("pdbody", fixture.body)
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(fixture.profile)
            Gui.runCommand("PartDesign_Rib")
            Gui.updateGui()
            rib = next(obj for obj in fixture.body.Group if obj.TypeId == "PartDesign::Rib")
            self.assertEqual(rib.Profile[0], fixture.profile)
            self.assertNotIn("Invalid", rib.State, rib.getStatusString())
            self.assertIsNotNone(Gui.Control.activeDialog())
            window = Gui.getMainWindow()
            self.assertIsNotNone(window.findChild(QtWidgets.QWidget, "ribParametersPanel"))
            self.assertIsNone(window.findChild(QtWidgets.QWidget, "ribAdvancedPanel"))
        finally:
            Gui.Selection.clearSelection()
            Gui.activeDocument().resetEdit()
            Gui.Control.closeDialog()
            fixture.tearDown()
