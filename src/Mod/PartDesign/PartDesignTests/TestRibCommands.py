# SPDX-License-Identifier: LGPL-2.1-or-later
"""Selection, task-panel and command regressions for the isolated Rib tool."""

import unittest
import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtCore, QtWidgets


class TestRibReferenceSelection(unittest.TestCase):
    def setUp(self):
        from PartDesignTests.TestRib import TestRib

        Gui.activateWorkbench("PartDesignWorkbench")
        self.fixture = TestRib("testSideProfileRib")
        self.fixture.setUp()
        self.fixture.testSideProfileRib()
        self.doc = self.fixture.doc
        self.rib = self.doc.Rib
        self.point = self.doc.Body.newObject("PartDesign::Point", "TargetPoint")
        self.point.Placement.Base = App.Vector(16, 12, 0)
        self.plane = self.doc.Body.newObject("PartDesign::Plane", "DirectionPlane")
        self.plane.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 180)
        self.doc.Body.Tip = self.rib
        self.doc.recompute()
        Gui.activeDocument().activeView().setActiveObject("pdbody", self.doc.Body)
        Gui.activeDocument().setEdit(self.rib.Name)

    def tearDown(self):
        Gui.Selection.clearSelection()
        Gui.activeDocument().resetEdit()
        Gui.Control.closeDialog()
        self.fixture.tearDown()

    def widget(self, kind, name):
        widget = Gui.getMainWindow().findChild(kind, name)
        self.assertIsNotNone(widget, name)
        return widget

    def testOwnViewProviderAndNoPadPanel(self):
        self.assertEqual(self.rib.ViewObject.TypeId, "PartDesignGui::ViewProviderRib")
        self.assertIsNotNone(self.widget(QtWidgets.QWidget, "ribParametersPanel"))
        window = Gui.getMainWindow()
        self.assertIsNone(window.findChild(QtWidgets.QCheckBox, "thinMode"))
        self.assertIsNone(window.findChild(QtWidgets.QWidget, "thinPropertiesPanel"))

    def testCreateFromSketchAndSelectedEdge(self):
        # Exercise the real command and unchanged shared selection utility.
        Gui.activeDocument().resetEdit()
        Gui.Control.closeDialog()
        self.doc.removeObject(self.rib.Name)
        self.doc.Body.Tip = self.doc.Base
        self.doc.recompute()
        for sub in ("", "Edge1"):
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(self.doc.Profile, sub)
            Gui.runCommand("PartDesign_Rib", 0)
            self.doc.recompute()
            created = self.doc.Body.Tip
            self.assertEqual(created.TypeId, "PartDesign::Rib")
            self.assertEqual(created.Profile[0], self.doc.Profile)
            self.assertTrue(created.AutoDirection)
            self.assertEqual(created.Extension, "Tangent")
            self.assertEqual(created.Type, "UpToShape")
            self.fixture.valid(created)
            self.assertIsNotNone(self.widget(QtWidgets.QWidget, "ribParametersPanel"))
            Gui.activeDocument().resetEdit()
            Gui.Control.closeDialog()
            self.doc.removeObject(created.Name)
            self.doc.Body.Tip = self.doc.Base
            self.doc.recompute()

    def testTowardPointAndFace(self):
        self.widget(QtWidgets.QComboBox, "ribDirectionMode").setCurrentIndex(1)
        top = next(
            i + 1
            for i, f in enumerate(self.doc.Base.Shape.Faces)
            if f.normalAt(0, 0).z > 0.99 and abs(f.CenterOfMass.z) < 1e-7
        )
        for obj, sub in ((self.point, ""), (self.doc.Base, "Face%d" % top)):
            button = self.widget(QtWidgets.QToolButton, "ribSelectDirection")
            button.click()
            Gui.Selection.addSelection(self.doc.Name, obj.Name, sub)
            self.assertFalse(button.isChecked())
            self.assertEqual(self.rib.ReferenceAxis[0], obj)
            self.assertIn(
                obj.Label, self.widget(QtWidgets.QLineEdit, "ribDirectionReference").text()
            )
            self.doc.recompute()
            self.fixture.valid(self.rib)

    def testAdvancedCollapsedByDefault(self):
        advanced = self.widget(QtWidgets.QWidget, "ribAdvancedParameters")
        contents = self.widget(QtWidgets.QWidget, "ribAdvancedPanel")
        self.assertTrue(advanced.isVisible())
        self.assertFalse(contents.isVisible())
        self.assertTrue(
            QtCore.QMetaObject.invokeMethod(advanced, "showHide", QtCore.Qt.DirectConnection)
        )
        loop = QtCore.QEventLoop()
        QtCore.QTimer.singleShot(500, loop.quit)
        loop.exec()
        self.assertTrue(contents.isVisible())

    def testParallelDatumPlane(self):
        self.widget(QtWidgets.QComboBox, "ribDirectionMode").setCurrentIndex(2)
        button = self.widget(QtWidgets.QToolButton, "ribSelectDirection")
        button.click()
        # A point has no intrinsic axis and must not be accepted in this mode.
        Gui.Selection.addSelection(self.doc.Name, self.point.Name)
        self.assertTrue(button.isChecked())
        Gui.Selection.addSelection(self.doc.Name, self.plane.Name)
        self.assertFalse(button.isChecked())
        self.assertEqual(self.rib.ReferenceAxis[0], self.plane)
        self.doc.recompute()
        self.fixture.valid(self.rib)

    def testDraftTowardDatumPoint(self):
        self.widget(QtWidgets.QComboBox, "ribPullMode").setCurrentIndex(2)
        button = self.widget(QtWidgets.QToolButton, "ribSelectPullDirection")
        button.click()
        Gui.Selection.addSelection(self.doc.Name, self.point.Name)
        self.assertFalse(button.isChecked())
        self.assertEqual(self.rib.DraftPullDirection[0], self.point)
        self.rib.TaperAngle = 0.5
        self.doc.recompute()
        self.fixture.valid(self.rib)

    def testDesignerVisibilityAndExtentMapping(self):
        extent = self.widget(QtWidgets.QComboBox, "ribExtent")
        length = self.widget(QtWidgets.QAbstractSpinBox, "ribLength")
        reversedBox = self.widget(QtWidgets.QCheckBox, "ribReversed")
        second = self.widget(QtWidgets.QAbstractSpinBox, "ribThickness2")
        self.assertEqual(
            [extent.itemData(i) for i in range(extent.count())], ["UpToShape", "Length"]
        )
        self.assertTrue(length.isHidden())
        self.assertTrue(reversedBox.isHidden())
        extent.setCurrentIndex(1)
        self.assertEqual(self.rib.Type, "Length")
        self.assertFalse(length.isHidden())
        self.assertFalse(reversedBox.isHidden())
        self.widget(QtWidgets.QComboBox, "ribPlacement").setCurrentIndex(3)
        self.assertFalse(second.isHidden())
        self.widget(QtWidgets.QComboBox, "ribPlacement").setCurrentIndex(2)
        self.assertTrue(second.isHidden())
        extent.setCurrentIndex(0)
        self.assertEqual(self.rib.Type, "UpToShape")
        self.assertTrue(length.isHidden())

    def testDesignerQuantityBindings(self):
        thickness = self.widget(QtWidgets.QAbstractSpinBox, "ribThickness")
        draft = self.widget(QtWidgets.QAbstractSpinBox, "ribDraftAngle")
        thickness.setProperty("rawValue", 3.0)
        self.assertAlmostEqual(self.rib.ThinThickness.Value, 3.0)
        for angle in (-0.5, 0.5, 0.0):
            draft.setProperty("rawValue", angle)
            self.assertAlmostEqual(self.rib.TaperAngle.Value, angle)
            self.fixture.valid(self.rib)

    def testDesignerReferenceClear(self):
        mode = self.widget(QtWidgets.QComboBox, "ribDirectionMode")
        mode.setCurrentIndex(1)
        self.widget(QtWidgets.QToolButton, "ribSelectDirection").click()
        Gui.Selection.addSelection(self.doc.Name, self.point.Name)
        self.assertEqual(self.rib.ReferenceAxis[0], self.point)
        self.widget(QtWidgets.QToolButton, "ribClearDirection").click()
        self.assertEqual(mode.currentIndex(), 0)
        self.assertTrue(self.rib.AutoDirection)
        self.assertFalse(self.rib.ReferenceAxis)
        self.assertEqual(self.widget(QtWidgets.QLineEdit, "ribDirectionReference").text(), "")
        self.fixture.valid(self.rib)

    def testDesignerRetranslationPreservesValues(self):
        panel = self.widget(QtWidgets.QWidget, "ribParametersPanel")
        while panel and panel.metaObject().className() != "PartDesignGui::TaskRibParameters":
            panel = panel.parentWidget()
        self.assertIsNotNone(panel)
        extent = self.widget(QtWidgets.QComboBox, "ribExtent")
        extent.setCurrentIndex(0)
        thickness = self.rib.ThinThickness.Value
        volume = self.rib.Shape.Volume
        QtCore.QCoreApplication.sendEvent(panel, QtCore.QEvent(QtCore.QEvent.LanguageChange))
        self.assertEqual(extent.currentData(), "UpToShape")
        self.assertEqual(self.rib.Type, "UpToShape")
        self.assertEqual(self.rib.ThinThickness.Value, thickness)
        self.assertAlmostEqual(self.rib.Shape.Volume, volume)
        self.assertEqual(self.widget(QtWidgets.QComboBox, "ribDirectionMode").count(), 5)
        self.assertEqual(self.widget(QtWidgets.QComboBox, "ribPullMode").count(), 6)
