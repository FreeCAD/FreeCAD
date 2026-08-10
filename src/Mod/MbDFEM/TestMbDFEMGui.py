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

    def test_solve_command_resolves_active_assembly_without_module_global_helper(self):
        import InitGui

        assembly = self.document.addObject("MbDFEM::MbDAssembly", "Assembly")
        self.document.recompute()
        self.Gui.Selection.clearSelection()
        self.Gui.Selection.addSelection(assembly)
        command = InitGui.SolveMbDAssemblyCommand()

        self.assertIs(command.activeAssembly(), assembly)
        self.assertNotIn("_active_mbdfem_assembly", InitGui.SolveMbDAssemblyCommand.Activated.__code__.co_names)
        self.assertNotIn("_active_mbd_assembly", InitGui.SolveMbDAssemblyCommand.Activated.__code__.co_names)

    def test_animation_parameters_selection_opens_task_panel(self):
        import FreeCADMbDAnimationPanel

        assembly = self.document.addObject("MbDFEM::MbDAssembly", "Assembly")
        animation_parameters = assembly.ensureAnimationParameters()
        self.document.recompute()
        self.Gui.updateGui()

        self.assertEqual(
            animation_parameters.ViewObject.TypeId,
            "MbDFEMGui::ViewProviderMbDAnimationParameters",
        )
        self.assertIs(
            FreeCADMbDAnimationPanel.owning_assembly(animation_parameters),
            assembly,
        )

        self.assertTrue(animation_parameters.ViewObject.doubleClicked())
        dialog = self.Gui.Control.activeDialog()

        try:
            self.assertIsInstance(dialog, FreeCADMbDAnimationPanel.AnimationTaskPanel)
            self.assertIs(dialog.animation_parameters, animation_parameters)
            self.assertIs(dialog.assembly, assembly)
        finally:
            if dialog is not None:
                self.Gui.Control.closeDialog()

        observer = FreeCADMbDAnimationPanel.AnimationParametersSelectionObserver()
        observer.addSelection(
            self.document.Name,
            assembly.Name,
            f"{animation_parameters.Name}.",
            None,
        )
        dialog = self.Gui.Control.activeDialog()
        try:
            self.assertIsInstance(dialog, FreeCADMbDAnimationPanel.AnimationTaskPanel)
        finally:
            if dialog is not None:
                self.Gui.Control.closeDialog()


if __name__ == "__main__":
    sys.exit(unittest.main())
