# SPDX-License-Identifier: LGPL-2.1-or-later

# **************************************************************************
#   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
#                                                                         *
#   This file is part of FreeCAD.                                         *
#                                                                         *
#   FreeCAD is free software: you can redistribute it and/or modify it    *
#   under the terms of the GNU Lesser General Public License as           *
#   published by the Free Software Foundation, either version 2.1 of the  *
#   License, or (at your option) any later version.                       *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful, but        *
#   WITHOUT ANY WARRANTY; without even the implied warranty of            *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
#   Lesser General Public License for more details.                       *
#                                                                         *
#   You should have received a copy of the GNU Lesser General Public      *
#   License along with FreeCAD. If not, see                               *
#   <https://www.gnu.org/licenses/>.                                      *
#                                                                         *
# **************************************************************************

import os
import tempfile
import unittest
import FreeCAD as App
import FreeCADGui
import ImportGui
from pivy import coin
from PySide import QtCore, QtWidgets


class ExportImportTest(unittest.TestCase):
    def setUp(self):
        TempPath = tempfile.gettempdir()
        self.fileName = TempPath + os.sep + "ColorPerFaceTest.step"
        self.doc = App.newDocument()

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def createImportedColoredBox(self, colors):
        part = self.doc.addObject("App::Part", "Part")
        box = part.newObject("Part::Box", "Box")
        self.doc.recompute()

        box.ViewObject.DiffuseColor = colors

        ImportGui.export([part], self.fileName)

        self.doc.clearDocument()
        ImportGui.insert(name=self.fileName, docName=self.doc.Name, merge=False, useLinkGroup=True)

        part_features = list(filter(lambda x: x.isDerivedFrom("Part::Feature"), self.doc.Objects))
        self.assertEqual(len(part_features), 1)
        return part_features[0]

    def testSaveLoadStepFile(self):
        """
        Create a STEP file with color per face
        """
        colors = [
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 0.0, 0.0, 1.0),
            (1.0, 1.0, 0.0, 1.0),
            (1.0, 1.0, 0.0, 1.0),
        ]
        feature = self.createImportedColoredBox(colors)

        self.assertEqual(len(feature.ViewObject.DiffuseColor), 6)
        self.assertEqual(feature.ViewObject.DiffuseColor, colors)
        self.assertEqual(feature.ViewObject.FaceAppearanceOverrides, (True,) * 6)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterialBinding.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(feature.ViewObject.RootNode)
        paths = sa.getPaths()

        bind = paths.get(1).getTail()
        self.assertEqual(bind.value.getValue(), bind.PER_PART)

        sa = coin.SoSearchAction()
        sa.setType(coin.SoMaterial.getClassTypeId())
        # We need an easier way to access nodes of a display mode
        sa.setInterest(coin.SoSearchAction.ALL)
        sa.apply(feature.ViewObject.RootNode)
        paths = sa.getPaths()

        mat = paths.get(1).getTail()
        self.assertEqual(mat.diffuseColor.getNum(), 6)

    def testAppearanceDialogPreservesFaceColors(self):
        if not App.GuiUp:
            self.skipTest("This test requires a graphical user interface (GUI).")

        feature = self.createImportedColoredBox(
            [
                (0.123, 0.234, 0.345, 1.0),
                (0.123, 0.234, 0.345, 1.0),
                (0.123, 0.234, 0.345, 1.0),
                (0.123, 0.234, 0.345, 1.0),
                (0.567, 0.678, 0.789, 1.0),
                (0.567, 0.678, 0.789, 1.0),
            ]
        )
        colors = list(feature.ViewObject.DiffuseColor)

        FreeCADGui.activateWorkbench("MaterialWorkbench")
        self.addCleanup(FreeCADGui.Selection.clearSelection)
        self.addCleanup(FreeCADGui.Control.closeDialog)
        FreeCADGui.Selection.addSelection(feature)
        FreeCADGui.runCommand("Std_SetAppearance")

        main_window = FreeCADGui.getMainWindow()
        appearance_button = main_window.findChild(QtWidgets.QPushButton, "buttonCustomAppearance")
        self.assertIsNotNone(appearance_button)

        dialog_state = {"color_dialog": False, "spinbox": False}

        def change_shininess_after_canceling_color_picker():
            dialog = QtWidgets.QApplication.activeModalWidget()
            if dialog is None:
                return

            color_button = dialog.findChild(QtWidgets.QPushButton, "diffuseColor")
            self.assertIsNotNone(color_button)

            def reject_color_dialog():
                color_dialog = QtWidgets.QApplication.activeModalWidget()
                if isinstance(color_dialog, QtWidgets.QColorDialog):
                    color_dialog.reject()
                    dialog_state["color_dialog"] = True

            QtCore.QTimer.singleShot(0, reject_color_dialog)
            color_button.click()

            spinbox = dialog.findChild(QtWidgets.QSpinBox, "shininess")
            if spinbox is not None:
                spinbox.setValue(50)
                dialog_state["spinbox"] = True
            dialog.reject()

        QtCore.QTimer.singleShot(0, change_shininess_after_canceling_color_picker)
        appearance_button.click()

        self.assertEqual(feature.ViewObject.DiffuseColor, colors)
        self.assertTrue(dialog_state["color_dialog"], "Color picker could not be found.")
        self.assertTrue(dialog_state["spinbox"], "Shininess control could not be found.")
        self.assertTrue(all(mat.Shininess == 0.5 for mat in feature.ViewObject.ShapeAppearance))

    def testMaterialSelectionPreservesOrReplacesFaceColors(self):
        if not App.GuiUp:
            self.skipTest("This test requires a graphical user interface (GUI).")

        feature = self.createImportedColoredBox(
            [
                (0.123, 0.234, 0.345, 1.0),
                (0.123, 0.234, 0.345, 1.0),
                (0.123, 0.234, 0.345, 1.0),
                (0.123, 0.234, 0.345, 1.0),
                (0.567, 0.678, 0.789, 1.0),
                (0.567, 0.678, 0.789, 1.0),
            ]
        )
        colors = list(feature.ViewObject.DiffuseColor)

        FreeCADGui.activateWorkbench("MaterialWorkbench")
        self.addCleanup(FreeCADGui.Selection.clearSelection)
        self.addCleanup(FreeCADGui.Control.closeDialog)
        FreeCADGui.Selection.addSelection(feature)
        FreeCADGui.runCommand("Std_SetAppearance")

        main_window = FreeCADGui.getMainWindow()
        material_widget = main_window.findChild(QtWidgets.QWidget, "widgetMaterial")
        replace_faces = main_window.findChild(QtWidgets.QCheckBox, "replaceFaceAppearances")
        self.assertIsNotNone(material_widget)
        self.assertIsNotNone(replace_faces)
        self.assertFalse(replace_faces.isChecked())

        import MatGui

        material_tree = MatGui.MaterialTreeWidget(material_widget)
        feature.ViewObject.FaceAppearanceOverrides = ()
        material_tree.UUID = "d1f317f0-5ffa-4798-8ab3-af2ff0b5182c"
        QtWidgets.QApplication.processEvents()
        self.assertEqual(feature.ViewObject.DiffuseColor, colors)

        feature.ViewObject.FaceAppearanceOverrides = (False, False, False, False, True, True)
        material_tree.UUID = "4151e19c-fd6a-4ca4-83d4-d5e17d76cb9c"
        QtWidgets.QApplication.processEvents()
        updated_colors = list(feature.ViewObject.DiffuseColor)
        self.assertNotEqual(updated_colors[:4], colors[:4])
        self.assertEqual(updated_colors[4:], colors[4:])

        replace_faces.setChecked(True)
        material_tree.UUID = "cddfa21f-0715-49dd-b35b-951c076fa52c"
        QtWidgets.QApplication.processEvents()
        self.assertEqual(len(feature.ViewObject.ShapeAppearance), 1)
