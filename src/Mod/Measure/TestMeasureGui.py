# SPDX-License-Identifier: LGPL-2.1-or-later
"""GUI regressions for the core Measure tools."""

import unittest

import FreeCAD as App
import FreeCADGui as Gui
import MatGui
import Materials
import Part
from PySide import QtCore, QtWidgets


class TestMassPropertiesMaterial(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("MassPropertiesMaterialTest")
        self.doc.UndoMode = 1

    def tearDown(self):
        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()
        Gui.Selection.clearSelection()
        App.closeDocument(self.doc.Name)

    def material(self, name, density):
        value = Materials.Material()
        value.Name = name
        value.addPhysicalModel(Materials.UUIDs().Density)
        value.setPhysicalValue("Density", density)
        return value

    def feature(self, name, material_name, density):
        obj = self.doc.addObject("Part::Feature", name)
        obj.Shape = Part.makeBox(10, 10, 10)
        obj.ShapeMaterial = self.material(material_name, density)
        return obj

    def material_button(self):
        task = Gui.Control.activeTaskDialog()
        self.assertIsNotNone(task)
        for widget in task.getDialogContent():
            button = widget.findChild(QtWidgets.QPushButton, "materialButton")
            if button:
                return button
        self.fail("Material button not found")

    def test_material_selector_is_registered_widget(self):
        selector = Gui.UiLoader().createWidget("MatGui::MaterialSelector")
        self.assertIsNotNone(selector)
        self.assertEqual(selector.metaObject().className(), "MatGui::MaterialSelector")
        selector.deleteLater()

    def choose(self, uuid):
        errors = []

        def select_material():
            dialog = QtWidgets.QApplication.activeModalWidget()
            try:
                widget = dialog.findChild(QtWidgets.QWidget, "materialTreeWidget")
                self.assertIsNotNone(widget)
                picker = MatGui.MaterialTreeWidget(widget)
                picker.UUID = uuid
                buttons = dialog.findChild(QtWidgets.QDialogButtonBox)
                buttons.button(QtWidgets.QDialogButtonBox.Ok).click()
            except Exception as error:
                errors.append(error)
                if dialog:
                    dialog.reject()

        QtCore.QTimer.singleShot(100, select_material)
        self.material_button().click()
        self.assertEqual(errors, [])

    def test_mixed_materials_assignment_and_undo(self):
        first = self.feature("First", "Brass", "8500 kg/m^3")
        second = self.feature("Second", "Iron", "7874 kg/m^3")
        third = self.feature("Third", "Aluminium", "2700 kg/m^3")
        Gui.Selection.addSelection(first)
        Gui.Selection.addSelection(second)
        Gui.Selection.addSelection(third)
        Gui.runCommand("Std_MassProperties")
        Gui.updateGui()
        self.assertEqual(self.material_button().text(), "Brass, Iron, …")

        steel_uuid = "92589471-a6cb-4bbc-b748-d425a17dea7d"
        self.choose(steel_uuid)
        for obj in (first, second, third):
            self.assertEqual(obj.ShapeMaterial.UUID, steel_uuid)
        self.assertEqual(self.material_button().text(), "CalculiX-Steel")

        self.doc.undo()
        self.assertEqual(first.ShapeMaterial.Name, "Brass")
        self.assertEqual(second.ShapeMaterial.Name, "Iron")
        self.assertEqual(third.ShapeMaterial.Name, "Aluminium")

    def test_link_selection_edits_source_material(self):
        source = self.feature("Source", "Brass", "8500 kg/m^3")
        link = self.doc.addObject("App::Link", "Occurrence")
        link.setLink(source)
        Gui.Selection.addSelection(link)
        Gui.runCommand("Std_MassProperties")
        Gui.updateGui()
        self.assertEqual(self.material_button().text(), "Brass")

        iron_uuid = "1826c364-d26a-43fb-8f61-288281236836"
        self.choose(iron_uuid)
        self.assertEqual(source.ShapeMaterial.UUID, iron_uuid)
        self.assertEqual(self.material_button().text(), "Iron-Generic")
