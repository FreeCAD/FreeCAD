# SPDX-License-Identifier: LGPL-2.1-or-later
"""GUI smoke checks for the Rib template."""

import unittest

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtWidgets

from PartDesignTests.TestRib import TestRib


class TestRibReferenceSelection(unittest.TestCase):
    def setUp(self):
        Gui.activateWorkbench("PartDesignWorkbench")
        self.fixture = TestRib()
        self.fixture.setUp()
        self.doc = self.fixture.doc
        self.rib = self.fixture.rib
        self.doc.recompute()
        Gui.activeDocument().activeView().setActiveObject("pdbody", self.fixture.body)

    def tearDown(self):
        Gui.Selection.clearSelection()
        Gui.activeDocument().resetEdit()
        Gui.Control.closeDialog()
        self.fixture.tearDown()

    def widget(self, name, widgetType=QtWidgets.QWidget, parent=None):
        if parent is None:
            parent = Gui.getMainWindow()
        widget = parent.findChild(widgetType, name)
        self.assertIsNotNone(widget, name)
        return widget

    def testTemplatePanel(self):
        self.assertEqual(self.rib.ViewObject.TypeId, "PartDesignGui::ViewProviderRib")
        Gui.activeDocument().setEdit(self.rib.Name)
        panel = self.widget("ribParametersPanel")
        advanced = self.widget("ribAdvancedParameters")
        self.widget("ribAdvancedPanel", parent=advanced)
        profile = self.widget("ribProfile", QtWidgets.QLineEdit, panel)
        self.assertEqual(profile.text(), self.fixture.profile.Label)
        self.assertTrue(profile.isReadOnly())
        for name in ("ribSelectProfile", "ribClearProfile"):
            self.widget(name, QtWidgets.QToolButton, panel)
        for name in ("ribExtension", "ribPlacement", "ribExtent"):
            self.widget(name, QtWidgets.QComboBox, panel)
        self.widget("ribLength", parent=panel)
        self.widget("ribThickness", parent=panel)
        self.widget("ribFilletRadius", parent=panel)
        self.widget("ribFilletRadiusLabel", QtWidgets.QLabel, panel)
        self.assertIsNone(advanced.findChild(QtWidgets.QWidget, "ribFilletRadius"))
        self.assertIsNone(panel.findChild(QtWidgets.QWidget, "ribThickness2"))
        self.widget("ribExtentDirection", QtWidgets.QLabel, advanced)
        for axis in "XYZ":
            self.widget("ribDirection" + axis, parent=advanced)
        for container in (panel, advanced):
            self.assertIsNone(container.findChild(QtWidgets.QCheckBox, "extendCheckBox"))


    def testEnumsLoadedWithoutMutation(self):
        # Opening an existing feature must use its saved values, not UI defaults.
        self.rib.ExtendType = "Off"
        self.rib.PlacementType = "Side B"
        self.rib.ExtentType = "Distance"
        self.rib.Distance = 12.5
        self.rib.Direction = App.Vector(-2, 3, -4)
        self.doc.recompute()
        Gui.activeDocument().setEdit(self.rib.Name)
        for name, prop, expected in (
            ("ribExtension", "ExtendType", "Off"),
            ("ribPlacement", "PlacementType", "Side B"),
            ("ribExtent", "ExtentType", "Distance"),
        ):
            with self.subTest(property=prop):
                combo = self.widget(name, QtWidgets.QComboBox)
                self.assertEqual(
                    [combo.itemData(index) for index in range(combo.count())],
                    self.rib.getEnumerationsOfProperty(prop),
                )
                self.assertEqual(combo.currentData(), expected)
                self.assertEqual(getattr(self.rib, prop), expected)
        self.assertEqual(self.rib.Distance, 12.5)
        self.assertEqual(self.widget("ribLength").property("rawValue"), 12.5)
        self.assertEqual(self.rib.Direction, App.Vector(-2, 3, -4))
        for axis, value in zip("XYZ", (-2, 3, -4)):
            self.assertEqual(self.widget("ribDirection" + axis).property("rawValue"), value)
        extension = self.widget("ribExtension", QtWidgets.QComboBox)
        extension.setCurrentIndex(extension.findData("C2"))
        self.assertEqual(self.rib.ExtendType, "C2")

    def testThicknessLoadedAndEdited(self):
        self.rib.Thickness = "3.5 mm"
        self.doc.recompute()
        Gui.activeDocument().setEdit(self.rib.Name)
        thickness = self.widget("ribThickness")
        self.assertEqual(thickness.property("rawValue"), 3.5)
        self.assertEqual(self.rib.Thickness.Value, 3.5)
        self.assertTrue(thickness.setProperty("rawValue", 4.0))
        self.assertEqual(self.rib.Thickness.Value, 4.0)

    def testDistanceVisibilityAndEdit(self):
        Gui.activeDocument().setEdit(self.rib.Name)
        extent = self.widget("ribExtent", QtWidgets.QComboBox)
        length = self.widget("ribLength")
        label = self.widget("ribLengthLabel", QtWidgets.QLabel)
        self.assertEqual(extent.currentData(), "Shape")
        self.assertTrue(length.isHidden())
        self.assertTrue(label.isHidden())
        extent.setCurrentIndex(extent.findData("Distance"))
        self.assertEqual(self.rib.ExtentType, "Distance")
        # Test explicit visibility independently of task-box collapse state.
        self.assertFalse(length.isHidden())
        self.assertFalse(label.isHidden())
        self.assertTrue(length.setProperty("rawValue", 7.5))
        self.assertEqual(self.rib.Distance, 7.5)
        extent.setCurrentIndex(extent.findData("Shape"))
        self.assertEqual(self.rib.ExtentType, "Shape")
        self.assertTrue(length.isHidden())
        self.assertTrue(label.isHidden())
        self.assertEqual(self.rib.Distance, 7.5)

    def testDirectionPreservedAndEdited(self):
        Gui.activeDocument().setEdit(self.rib.Name)
        self.assertEqual(self.rib.Direction, App.Vector(0, 0, -1))
        for axis, initial in zip("XYZ", (0, 0, -1)):
            self.assertEqual(self.widget("ribDirection" + axis).property("rawValue"), initial)
        for axis, component, value in zip("XYZ", "xyz", (-2, 3, -4)):
            field = self.widget("ribDirection" + axis)
            # rawValue's Qt property writer is QuantitySpinBox::setValue(double).
            self.assertTrue(field.setProperty("rawValue", value))
            self.assertEqual(getattr(self.rib.Direction, component), value)
        self.assertEqual(self.rib.Direction, App.Vector(-2, 3, -4))

    def testDraftAngleLoadedAndEdited(self):
        self.rib.DraftAngle = "-12.5 deg"
        self.doc.recompute()
        Gui.activeDocument().setEdit(self.rib.Name)
        angle = self.widget("ribDraftAngle", parent=self.widget("ribParametersPanel"))
        self.assertEqual(self.rib.getTypeIdOfProperty("DraftAngle"), "App::PropertyAngle")
        self.assertEqual(self.rib.DraftAngle.Value, -12.5)
        self.assertEqual(angle.property("rawValue"), -12.5)
        self.assertEqual(angle.property("minimum"), -89.0)
        self.assertEqual(angle.property("maximum"), 89.0)
        for value in (0.0, 15.0, -20.0):
            self.assertTrue(angle.setProperty("rawValue", value))
            self.assertEqual(self.rib.DraftAngle.Value, value)
        extent = self.widget("ribExtent", QtWidgets.QComboBox)
        for value in ("Distance", "Shape"):
            extent.setCurrentIndex(extent.findData(value))
            self.assertFalse(angle.isHidden())

    def testFilletRadiusLoadedAndEdited(self):
        self.rib.FilletRadius = 2.5
        self.doc.recompute()
        Gui.activeDocument().setEdit(self.rib.Name)
        panel = self.widget("ribParametersPanel")
        radius = self.widget("ribFilletRadius", parent=panel)
        label = self.widget("ribFilletRadiusLabel", QtWidgets.QLabel, panel)
        self.assertEqual(self.rib.getTypeIdOfProperty("FilletRadius"), "App::PropertyFloat")
        self.assertEqual(self.rib.FilletRadius, 2.5)
        self.assertEqual(radius.property("rawValue"), 2.5)
        self.assertEqual(radius.property("unit"), "mm")
        self.assertEqual(radius.property("minimum"), 0.0)
        self.assertEqual(radius.property("maximum"), 1e9)
        self.assertEqual(label.buddy(), radius)
        draft = self.widget("ribDraftAngle", parent=panel)
        self.assertEqual(
            panel.layout().getWidgetPosition(radius)[0],
            panel.layout().getWidgetPosition(draft)[0] + 1,
        )
        for value in (0.0, 1.5, 3.0):
            self.assertTrue(radius.setProperty("rawValue", value))
            self.assertEqual(self.rib.FilletRadius, value)
        extent = self.widget("ribExtent", QtWidgets.QComboBox)
        for value in ("Distance", "Shape"):
            extent.setCurrentIndex(extent.findData(value))
            self.assertFalse(radius.isHidden())
            self.assertFalse(label.isHidden())

    def testFilletRadiusExpressionLoaded(self):
        self.rib.setExpression("FilletRadius", "1 + 1.5")
        self.doc.recompute()
        Gui.activeDocument().setEdit(self.rib.Name)
        self.assertEqual(self.widget("ribFilletRadius").property("rawValue"), 2.5)
        self.assertEqual(self.rib.FilletRadius, 2.5)
        self.assertEqual(dict(self.rib.ExpressionEngine)["FilletRadius"], "1 + 1.5")

    def testCustomPullDirectionToggleAndEdit(self):
        Gui.activeDocument().setEdit(self.rib.Name)
        advanced = self.widget("ribAdvancedPanel")
        custom = self.widget("ribUseCustomPullDirection", QtWidgets.QCheckBox, advanced)
        self.assertFalse(custom.isChecked())
        self.assertFalse(self.rib.UseCustomPullDirection)
        for axis, value in zip("XYZ", (0, 0, 1)):
            field = self.widget("ribPullDirection" + axis, parent=advanced)
            self.assertFalse(field.isEnabled())
            self.assertEqual(field.property("rawValue"), value)
        custom.setChecked(True)
        self.assertTrue(self.rib.UseCustomPullDirection)
        for axis, component, value in zip("XYZ", "xyz", (-2, 3, -4)):
            field = self.widget("ribPullDirection" + axis, parent=advanced)
            self.assertTrue(field.isEnabled())
            self.assertTrue(field.setProperty("rawValue", value))
            self.assertEqual(getattr(self.rib.PullDirection, component), value)
        custom.setChecked(False)
        self.assertFalse(self.rib.UseCustomPullDirection)
        self.assertEqual(self.rib.PullDirection, App.Vector(-2, 3, -4))
        for axis in "XYZ":
            self.assertFalse(self.widget("ribPullDirection" + axis, parent=advanced).isEnabled())
        custom.setChecked(True)
        self.assertEqual(self.rib.PullDirection, App.Vector(-2, 3, -4))

    def testCustomPullDirectionLoadedWithoutMutation(self):
        self.rib.UseCustomPullDirection = True
        self.rib.PullDirection = App.Vector(2, -3, -4)
        self.doc.recompute()
        Gui.activeDocument().setEdit(self.rib.Name)
        self.assertTrue(self.widget("ribUseCustomPullDirection", QtWidgets.QCheckBox).isChecked())
        self.assertTrue(self.rib.UseCustomPullDirection)
        self.assertEqual(self.rib.PullDirection, App.Vector(2, -3, -4))
        for axis, value in zip("XYZ", (2, -3, -4)):
            field = self.widget("ribPullDirection" + axis)
            self.assertTrue(field.isEnabled())
            self.assertEqual(field.property("rawValue"), value)

    def testDraftExpressionsLoaded(self):
        self.rib.setExpression("DraftAngle", "5 deg + 2 deg")
        self.rib.setExpression("PullDirection.x", "1 + 1")
        self.rib.UseCustomPullDirection = True
        self.doc.recompute()
        Gui.activeDocument().setEdit(self.rib.Name)
        self.assertEqual(self.widget("ribDraftAngle").property("rawValue"), 7.0)
        self.assertEqual(self.widget("ribPullDirectionX").property("rawValue"), 2.0)
        self.assertEqual(self.rib.DraftAngle.Value, 7.0)
        self.assertEqual(self.rib.PullDirection.x, 2.0)
        self.assertEqual(len(self.rib.ExpressionEngine), 2)

    def testClearAndPickSketch(self):
        Gui.Selection.clearSelection()
        Gui.activeDocument().setEdit(self.rib.Name)
        clear = self.widget("ribClearProfile", QtWidgets.QToolButton)
        profile = self.widget("ribProfile", QtWidgets.QLineEdit)
        picker = self.widget("ribSelectProfile", QtWidgets.QToolButton)
        self.assertTrue(clear.isEnabled())
        clear.click()
        self.assertIsNone(self.rib.Profile)
        self.assertEqual(profile.text(), "")
        self.assertFalse(clear.isEnabled())
        picker.click()
        self.assertTrue(picker.isChecked())
        Gui.Selection.addSelection(self.fixture.profile)
        self.assertEqual(self.rib.Profile[0], self.fixture.profile)
        self.assertEqual(profile.text(), self.fixture.profile.Label)
        self.assertTrue(clear.isEnabled())
        self.assertFalse(picker.isChecked())
        self.assertEqual(Gui.Selection.getSelection(), [])

    def testCommandFromProfile(self):
        self.doc.removeObject(self.rib.Name)
        self.fixture.body.Tip = self.fixture.base
        self.doc.recompute()
        Gui.Selection.addSelection(self.fixture.profile, "Edge1")
        Gui.runCommand("PartDesign_Rib", 0)
        created = self.fixture.body.Tip
        self.assertEqual(created.TypeId, "PartDesign::Rib")
        self.assertEqual(created.Profile[0], self.fixture.profile)
        self.assertIn(
            "Cannot determine an in-plane rib sweep direction", created.getStatusString()
        )
        self.assertIsNotNone(
            Gui.getMainWindow().findChild(QtWidgets.QWidget, "ribParametersPanel")
        )
