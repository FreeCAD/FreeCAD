# SPDX-License-Identifier: LGPL-2.1-or-later

# Test for FreeCAD issue https://github.com/FreeCAD/FreeCAD/issues/21516
# Reading ViewObject.VisualLayerList from Python used to raise
# "PropertyVisualLayerList has no python counterpart".

import FreeCAD

from SketcherTests.GuiTestCase import SketcherGuiTestCase


class TestVisualLayerListGui(SketcherGuiTestCase):
    def setUp(self):
        super().setUp()
        self.doc = FreeCAD.newDocument("TestVisualLayerList")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.doc.recompute()

    def testVisualLayerListIsReadableFromPython(self):
        layers = self.sketch.ViewObject.VisualLayerList

        self.assertIsInstance(layers, list)
        # ViewProviderSketch creates three default layers: normal, dashed and hidden.
        self.assertEqual(len(layers), 3)
        for layer in layers:
            self.assertIsInstance(layer, dict)
            self.assertEqual(set(layer.keys()), {"LinePattern", "LineWidth", "Visible"})
            self.assertIsInstance(layer["LinePattern"], int)
            self.assertIsInstance(layer["LineWidth"], float)
            self.assertIsInstance(layer["Visible"], bool)

    def testVisualLayerListDefaultValues(self):
        layers = self.sketch.ViewObject.VisualLayerList

        expected = [
            {"LinePattern": 0xFFFF, "LineWidth": 3.0, "Visible": True},
            {"LinePattern": 0x7E7E, "LineWidth": 3.0, "Visible": True},
            {"LinePattern": 0xFFFF, "LineWidth": 3.0, "Visible": False},
        ]
        self.assertEqual(layers, expected)
