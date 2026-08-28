# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD
from pivy import coin
from PySide import QtCore
from SketcherTests.GuiTestCase import FreeCADGui, SketcherGuiTestCase


class TestGeometryCreationModeGui(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()

        FreeCADGui.activateWorkbench("SketcherWorkbench")
        self.doc = FreeCAD.newDocument("TestGeometryCreationModeGui")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.doc.recompute()
        main_window = FreeCADGui.getMainWindow()
        main_window.show()
        main_window.raise_()
        main_window.activateWindow()
        FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)
        self.pump(200)

    def scene_node(self, name):
        view = FreeCADGui.ActiveDocument.ActiveView
        scene_root = view.getViewer().getSoRenderManager().getSceneGraph()
        search = coin.SoSearchAction()
        search.setName(name)
        search.setSearchingAll(True)
        search.apply(scene_root)
        path = search.getPath()
        self.assertIsNotNone(path, f"Expected scene node {name}")
        return path.getTail()

    def material_rgb(self, material):
        color = material.diffuseColor.getValues()[0].getValue()
        return tuple(int(component * 255.0 + 0.5) for component in color)

    def preference_rgb(self, name, default):
        parameters = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
        color = parameters.GetUnsigned(name, default)
        return ((color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF)

    def preview_matches(self, style, material, pattern, color):
        return (
            style.linePattern.getValue() == pattern
            and self.material_rgb(material) == color
        )

    def test_construction_toggle_updates_active_preview_immediately(self):
        view = FreeCADGui.ActiveDocument.ActiveView
        view.viewTop()
        view.fitAll()
        self.pump(100)
        viewport = view.graphicsView().viewport()
        viewport.setFocus()
        center = viewport.rect().center()
        start = self.clamp_to_widget(
            viewport, center + QtCore.QPoint(-120, -80)
        )
        end = self.clamp_to_widget(
            viewport, center + QtCore.QPoint(120, -80)
        )

        FreeCADGui.runCommand("Sketcher_CreatePolyline")
        self.pump(200)
        self.move(viewport, start)
        self.click(viewport, start)
        self.move(viewport, end)

        coordinates = self.scene_node("EditCurvesCoordinate")
        preview_style = self.scene_node("EditCurvesDrawStyle")
        preview_material = self.scene_node("EditCurvesMaterials")
        normal_style = self.scene_node("CurvesDrawStyle")
        construction_style = self.scene_node("CurvesConstructionDrawStyle")
        normal_pattern = normal_style.linePattern.getValue()
        construction_pattern = construction_style.linePattern.getValue()
        normal_color = self.preference_rgb("EditedEdgeColor", 0xFFFFFFFF)
        construction_color = self.preference_rgb("ConstructionColor", 0x0000DCFF)

        self.assertGreaterEqual(
            coordinates.point.getNum(),
            2,
            "Expected an active polyline preview",
        )
        self.assertTrue(
            self.preview_matches(
                preview_style,
                preview_material,
                normal_pattern,
                normal_color,
            ),
            "Expected a normal-geometry preview before toggling",
        )

        FreeCADGui.Selection.clearSelection()
        FreeCADGui.runCommand("Sketcher_ToggleConstruction")

        self.assertTrue(
            self.wait_until(
                lambda: self.preview_matches(
                    preview_style,
                    preview_material,
                    construction_pattern,
                    construction_color,
                )
            ),
            "Construction mode should update the active preview without mouse movement",
        )

        FreeCADGui.runCommand("Sketcher_ToggleConstruction")

        self.assertTrue(
            self.wait_until(
                lambda: self.preview_matches(
                    preview_style,
                    preview_material,
                    normal_pattern,
                    normal_color,
                )
            ),
            "Normal mode should restore the active preview without mouse movement",
        )
