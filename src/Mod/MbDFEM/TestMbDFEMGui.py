# SPDX-License-Identifier: LGPL-2.1-or-later

import sys
import unittest

import FreeCAD as App
import MbDFEM  # noqa: F401


def _require_gui():
    if not App.GuiUp:
        raise unittest.SkipTest("GUI tests require FreeCAD GUI mode")

    try:
        import FreeCADGui as Gui
        import MbDFEMGui  # noqa: F401
    except Exception as exc:
        raise unittest.SkipTest("MbDFEMGui not available") from exc

    setup_without_gui = getattr(Gui, "setupWithoutGUI", None)
    if callable(setup_without_gui):
        setup_without_gui()

    return Gui


class MbDFEMGuiViewProviderTest(unittest.TestCase):
    def setUp(self):
        self.Gui = _require_gui()
        self.document = App.newDocument("MbDFEMGuiViewProviderTest")
        self.Gui.setActiveDocument(self.document.Name)

    def tearDown(self):
        if getattr(self, "document", None) is not None:
            self.Gui.Selection.clearSelection()
            if App.getDocument(self.document.Name):
                App.closeDocument(self.document.Name)

    def test_view_providers_expose_expected_children_and_child_root(self):
        assembly = self.document.addObject("MbDFEM::MbDAssembly", "Assembly")
        part = self.document.addObject("MbDFEM::MbDPart", "Part")
        marker = self.document.addObject("MbDFEM::MbDMarker", "Marker")
        gravity = assembly.ensureGravity()
        simulation_parameters = assembly.ensureSimulationParameters()
        animation_parameters = assembly.ensureAnimationParameters()

        assembly.addPart(part)
        part.addMarker(marker)
        self.document.recompute()
        self.Gui.updateGui()

        assembly_children = assembly.ViewObject.claimChildren()
        assembly_children_3d = assembly.ViewObject.claimChildren3D()
        part_children_3d = part.ViewObject.claimChildren3D()

        self.assertIn(gravity, assembly_children)
        self.assertIn(simulation_parameters, assembly_children)
        self.assertIn(animation_parameters, assembly_children)
        self.assertIn(part, assembly_children_3d)
        self.assertIn(marker, part_children_3d)
        self.assertIsNotNone(part.ViewObject.getChildRoot())

    def test_marker_always_uses_axis_triad_representation(self):
        assembly = self.document.addObject("MbDFEM::MbDAssembly", "Assembly")
        part = self.document.addObject("MbDFEM::MbDPart", "Part")
        marker = self.document.addObject("MbDFEM::MbDMarker", "Marker")

        assembly.addPart(part)
        part.addMarker(marker)
        self.document.recompute()
        self.Gui.updateGui()

        self.assertIn("AxisTriad", assembly.ViewObject.PropertiesList)
        self.assertIn("AxisTriad", part.ViewObject.PropertiesList)
        self.assertNotIn("AxisTriad", marker.ViewObject.PropertiesList)
        self.assertTrue(marker.ViewObject.Visibility)


if __name__ == "__main__":
    sys.exit(unittest.main())
