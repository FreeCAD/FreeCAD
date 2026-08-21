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
        mass_marker = part.ensureMassMarker()
        gravity = assembly.ensureGravity()
        simulation_parameters = assembly.ensureSimulationParameters()
        animation_parameters = assembly.ensureAnimationParameters()

        assembly.addPart(part)
        part.addMarker(marker)
        self.document.recompute()
        self.Gui.updateGui()

        assembly_children = assembly.ViewObject.claimChildren()
        assembly_children_3d = assembly.ViewObject.claimChildren3D()
        part_children = part.ViewObject.claimChildren()
        part_children_3d = part.ViewObject.claimChildren3D()

        self.assertIn(gravity, assembly_children)
        self.assertIn(simulation_parameters, assembly_children)
        self.assertIn(animation_parameters, assembly_children)
        self.assertIn(part, assembly_children_3d)
        self.assertIn(mass_marker, part_children)
        self.assertIn(part.getMarkersFolder(), part_children)
        self.assertLess(part_children.index(mass_marker), part_children.index(part.getMarkersFolder()))
        self.assertNotIn(marker, part_children)
        self.assertEqual(part.getMarkersFolder().Group, [marker])
        self.assertIn(marker, part_children_3d)
        self.assertIn(mass_marker, part_children_3d)
        self.assertIsNotNone(part.ViewObject.getChildRoot())

    def test_marker_always_uses_axis_triad_representation(self):
        assembly = self.document.addObject("MbDFEM::MbDAssembly", "Assembly")
        part = self.document.addObject("MbDFEM::MbDPart", "Part")
        marker = self.document.addObject("MbDFEM::MbDMarker", "Marker")

        assembly.addPart(part)
        part.addMarker(marker)
        self.document.recompute()
        self.Gui.updateGui()

        self.assertNotIn("AxisTriad", assembly.ViewObject.PropertiesList)
        self.assertNotIn("AxisTriad", part.ViewObject.PropertiesList)
        self.assertNotIn("AxisTriad", marker.ViewObject.PropertiesList)
        self.assertTrue(marker.ViewObject.Visibility)

    def test_marker_command_uses_part_local_face_placement(self):
        import InitGui
        import Part

        class SelectionCandidate:
            pass

        part = self.document.addObject("MbDFEM::MbDPart", "Part")
        part.Shape = Part.makeCylinder(2, 10)
        part.Placement = App.Placement(
            App.Vector(10, 20, 30),
            App.Rotation(App.Vector(0, 0, 1), 90),
        )
        self.document.recompute()

        face_index, face = next(
            (index, face)
            for index, face in enumerate(part.Shape.Faces, start=1)
            if getattr(face.Surface, "TypeId", None) == "Part::GeomCylinder"
        )
        transformed_face = face.copy().transformShape(part.getGlobalPlacement().toMatrix(), True)
        selected = SelectionCandidate()
        selected.Object = part
        selected.SubElementNames = [f"Face{face_index}"]
        selected.SubObjects = [transformed_face]

        command = InitGui.CreateMbDMarkerCommand()
        expected_local = command._reference_placement(face)
        selected_part, sub_name, selected_element = command._selection_from_candidate(selected)
        actual_local = command._reference_placement(selected_element)

        self.assertIs(selected_part, part)
        self.assertEqual(sub_name, f"Face{face_index}")
        self.assertLess((actual_local.Base - expected_local.Base).Length, 1e-7)
        for axis in (App.Vector(1, 0, 0), App.Vector(0, 1, 0), App.Vector(0, 0, 1)):
            actual_axis = actual_local.Rotation.multVec(axis)
            expected_axis = expected_local.Rotation.multVec(axis)
            self.assertLess((actual_axis - expected_axis).Length, 1e-7)

    def test_assembly_drop_normalizes_part_feature_shape_placement(self):
        import Part

        assembly = self.document.addObject("MbDFEM::MbDAssembly", "Assembly")
        source = self.document.addObject("Part::Feature", "Source")
        source.Shape = Part.makeCylinder(8, 20)
        source.Placement = App.Placement(
            App.Vector(10, 0, 0),
            App.Rotation(App.Vector(0, 1, 0), 40),
        )
        self.document.recompute()
        self.assertGreater(abs(source.Shape.Placement.Rotation.Angle), 1e-9)

        assembly.ViewObject.dropObject(source)
        self.document.recompute()

        part = next(
            obj for obj in self.document.Objects if obj.TypeId == "MbDFEM::MbDPart"
        )
        self.assertTrue(part.Placement.isSame(source.Placement, 1e-9))
        self.assertLess(part.Shape.Placement.Base.Length, 1e-9)
        self.assertLess(abs(part.Shape.Placement.Rotation.Angle), 1e-9)
        marker = part.getMassMarker()
        self.assertIsNotNone(marker)
        self.assertLess((marker.Placement.Base - App.Vector(0, 0, 10)).Length, 1e-9)

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

    def test_simulation_parameters_selection_opens_task_panel(self):
        import FreeCADMbDSimulationPanel

        assembly = self.document.addObject("MbDFEM::MbDAssembly", "Assembly")
        simulation_parameters = assembly.ensureSimulationParameters()
        self.document.recompute()
        self.Gui.updateGui()

        self.assertEqual(
            simulation_parameters.ViewObject.TypeId,
            "MbDFEMGui::ViewProviderMbDSimulationParameters",
        )
        self.assertIs(
            FreeCADMbDSimulationPanel.owning_assembly(simulation_parameters),
            assembly,
        )

        self.assertTrue(simulation_parameters.ViewObject.doubleClicked())
        dialog = self.Gui.Control.activeDialog()

        try:
            self.assertIsInstance(dialog, FreeCADMbDSimulationPanel.SimulationTaskPanel)
            self.assertIs(dialog.parameters, simulation_parameters)
            self.assertIs(dialog.assembly, assembly)
        finally:
            if dialog is not None:
                self.Gui.Control.closeDialog()

    def test_mass_marker_double_click_opens_task_panel(self):
        import FreeCADMbDMassMarkerPanel

        part = self.document.addObject("MbDFEM::MbDPart", "Part")
        mass_marker = part.ensureMassMarker()
        self.document.recompute()
        self.Gui.updateGui()

        self.assertTrue(mass_marker.ViewObject.doubleClicked())
        dialog = self.Gui.Control.activeDialog()

        try:
            self.assertIsInstance(dialog, FreeCADMbDMassMarkerPanel.MassMarkerTaskPanel)
            self.assertIs(dialog.marker, mass_marker)
        finally:
            if dialog is not None:
                self.Gui.Control.closeDialog()

        observer = FreeCADMbDSimulationPanel.SimulationParametersSelectionObserver()
        observer.addSelection(
            self.document.Name,
            assembly.Name,
            f"{simulation_parameters.Name}.",
            None,
        )
        dialog = self.Gui.Control.activeDialog()
        try:
            self.assertIsInstance(dialog, FreeCADMbDSimulationPanel.SimulationTaskPanel)
        finally:
            if dialog is not None:
                self.Gui.Control.closeDialog()


if __name__ == "__main__":
    sys.exit(unittest.main())
