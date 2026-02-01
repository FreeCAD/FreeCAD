# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD Arch workbench.
# You can find the full license text in the LICENSE file in the root directory.

import Arch
import Draft
import ArchCoveringGui
import FreeCAD as App
import FreeCADGui as Gui
from draftutils import params
from bimtests import TestArchBaseGui


class TestArchCoveringGui(TestArchBaseGui.TestArchBaseGui):
    """GUI-side tests for Arch Covering Task Panel logic and workflows."""

    def setUp(self):
        super().setUp()
        self.box = self.document.addObject("Part::Box", "BaseBox")
        self.document.recompute()
        self.panel = None
        # Isolate tests by saving and restoring the global parameter
        self.original_joint_width = params.get_param_arch("CoveringJoint", ret_default=False)

    def tearDown(self):
        # Restore the global parameter to prevent test pollution
        params.set_param_arch("CoveringJoint", self.original_joint_width)
        # Ensure any open task panel is closed and the template is removed
        if self.panel:
            self.panel.reject()
        self.pump_gui_events()
        super().tearDown()

    def test_template_expression_transfer(self):
        """Verify that expressions on the template object are transferred to real objects."""
        self.printTestMessage("expression transfer from template...")
        # Open panel in creation mode (no obj passed)
        self.panel = ArchCoveringGui.ArchCoveringTaskPanel()

        # Set an expression on the template property (simulating user entering f(x) in UI)
        expression = "100mm + 200mm"
        self.panel.template.buffer.setExpression("TileLength", expression)

        # Assign a target face
        self.panel.selection_list = [(self.box, ["Face6"])]

        # Accept (create the real object)
        self.panel.accept()
        self.pump_gui_events()

        # Assert
        # Internal name may vary (e.g. Covering001) due to template creation.
        # Find by type instead.
        coverings = [o for o in self.document.Objects if Draft.get_type(o) == "Covering"]
        self.assertEqual(len(coverings), 1, "Expected exactly one Covering object to be created")
        new_obj = coverings[0]

        # Verify the expression by inspecting the ExpressionEngine
        found_expr = False
        if hasattr(new_obj, "ExpressionEngine"):
            for path, expr_val in new_obj.ExpressionEngine:
                if path == "TileLength":
                    # FreeCAD normalizes expressions (e.g. adding spaces).
                    # We compare stripped strings to avoid false negatives.
                    self.assertEqual(expr_val.replace(" ", ""), expression.replace(" ", ""))
                    found_expr = True
                    break

        self.assertTrue(found_expr, "Expression was not found on the generated object.")

    def test_selection_ui_labels(self):
        """Test the smart labeling logic for the 'Base' field."""
        self.printTestMessage("selection UI labels...")
        self.panel = ArchCoveringGui.ArchCoveringTaskPanel()
        # TODO: this test needs to be reworked to be locale-independent

        # Scenario A: single face
        self.panel.selection_list = [(self.box, ["Face1"])]
        self.panel._updateSelectionUI()
        self.assertEqual(self.panel.le_selection.text(), "BaseBox.Face1")

        # Scenario B: multiple faces of same object
        self.panel.selection_list = [(self.box, ["Face1"]), (self.box, ["Face2"])]
        self.panel._updateSelectionUI()
        self.assertIn("(2 faces)", self.panel.le_selection.text())

        # Scenario C: multiple distinct objects
        box2 = self.document.addObject("Part::Box", "SecondBox")
        self.panel.selection_list = [self.box, box2]
        self.panel._updateSelectionUI()
        self.assertEqual(self.panel.le_selection.text(), "2 objects selected")

    def test_batch_creation_logic(self):
        """Verify that selecting multiple faces creates multiple covering objects."""
        self.printTestMessage("batch creation logic...")
        initial_count = len(self.document.Objects)

        # Provide a selection list with 3 targets
        selection = [(self.box, ["Face1"]), (self.box, ["Face2"]), (self.box, ["Face3"])]
        self.panel = ArchCoveringGui.ArchCoveringTaskPanel(selection=selection)

        # Manipulate the widget to ensure the binding and the underlying template object are updated
        # correctly.
        target_width = 450.0
        self.panel.sb_width.setProperty("rawValue", target_width)
        # Let the Qt event loop process the change and update the template via binding
        self.pump_gui_events()

        self.panel.accept()
        self.pump_gui_events()

        # Verify 3 new objects created + 1 (the initial box) - 1 (the template was deleted)
        self.assertEqual(len(self.document.Objects), initial_count + 3)

        # Safe filtering using Draft utility to avoid 'PrimitivePy' attribute errors
        coverings = [o for o in self.document.Objects if Draft.get_type(o) == "Covering"]
        self.assertEqual(len(coverings), 3)
        for c in coverings:
            self.assertAlmostEqual(c.TileWidth.Value, target_width, places=5)

    def test_continue_mode_workflow(self):
        """Test the soft-reset behavior of Continue mode."""
        self.printTestMessage("continue mode workflow...")
        self.panel = ArchCoveringGui.ArchCoveringTaskPanel(selection=[(self.box, ["Face6"])])
        self.panel.chk_continue.setChecked(True)

        # Accept first operation (should return False to keep dialog open)
        result = self.panel.accept()
        self.assertFalse(result, "Panel should not close when Continue is checked.")

        # Verify state reset
        self.assertEqual(len(self.panel.selection_list), 0)
        self.assertEqual(self.panel.le_selection.text(), "No selection")
        self.assertTrue(self.panel.isPicking(), "Picking should be re-armed automatically.")

        # Ensure template still exists for the next round
        self.assertIsNotNone(self.document.getObject(self.panel.template.buffer.Name))

    def test_mode_switching_ux(self):
        """Verify that thickness is disabled when entering pattern modes."""
        self.printTestMessage("mode switching UX...")
        self.panel = ArchCoveringGui.ArchCoveringTaskPanel()

        # Initial state: Solid Tiles (index 0)
        self.panel.combo_mode.setCurrentIndex(0)
        self.assertTrue(self.panel.sb_thick.isEnabled())

        # Switch to Parametric Pattern (index 1)
        self.panel.combo_mode.setCurrentIndex(1)
        self.assertFalse(self.panel.sb_thick.isEnabled())
        self.assertEqual(self.panel.sb_thick.property("rawValue"), 0.0)

        # Switch back to Solid Tiles
        self.panel.combo_mode.setCurrentIndex(0)
        self.assertTrue(self.panel.sb_thick.isEnabled())

        # Should restore the previous default thickness
        self.assertGreater(self.panel.sb_thick.property("rawValue"), 0.0)

    def test_cleanup_removes_template(self):
        """Ensure the template object is deleted on close/reject."""
        self.printTestMessage("template cleanup on reject...")
        self.panel = ArchCoveringGui.ArchCoveringTaskPanel()
        template_name = self.panel.template.buffer.Name
        self.assertIsNotNone(self.document.getObject(template_name))

        # Close the panel
        self.panel.reject()
        self.pump_gui_events()

        # Verify the object is gone from the document
        self.assertIsNone(
            self.document.getObject(template_name), "Phantom object was not cleaned up."
        )

    def test_self_dependency_filter(self):
        """Verify that a covering cannot be assigned as its own base."""
        self.printTestMessage("self-dependency filter...")
        covering = Arch.makeCovering(self.box)
        self.document.recompute()

        # Open in Edit Mode
        self.panel = ArchCoveringGui.ArchCoveringTaskPanel(obj=covering)
        self.panel.setPicking(True)

        # Simulate selecting the covering itself
        # This calls _onSelectionChanged internally via the observer
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(covering)
        self.pump_gui_events()

        # Assert: The internal selection tracker should still be None or the original
        self.assertNotEqual(
            self.panel.selected_obj, covering, "The panel allowed selecting itself as a base."
        )

    def test_texture_mapping_math(self):
        """
        Verify texture vector math isolation (logic test).
        """
        self.printTestMessage("texture mapping math...")

        # Setup: Create a covering on the XY plane
        # 1000x1000 face, 0 rotation
        base = (self.box, ["Face6"])  # Face6 is Top (Z=1000)
        covering = Arch.makeCovering(base)
        covering.TileLength = 200.0
        covering.TileWidth = 200.0
        covering.JointWidth = 0.0
        covering.Rotation = 0.0
        self.document.recompute()

        # Access the ViewProvider Proxy
        vp = covering.ViewObject.Proxy

        # Case A: "Shaded" Mode (Global Coordinates)
        # Expected: Direction U should align with X (1,0,0) scaled by tile size
        # Period = 200.0. Vector = (1/200, 0, 0) = (0.005, 0, 0)
        res_shaded = vp._compute_texture_mapping(covering, "Shaded")
        self.assertIsNotNone(res_shaded, "Mapping should succeed for Shaded mode")

        dir_u, dir_v, s_off, t_off = res_shaded

        self.assertAlmostEqual(dir_u.x, 0.005, places=5)
        self.assertAlmostEqual(dir_u.y, 0.0, places=5)
        self.assertAlmostEqual(dir_v.y, 0.005, places=5)

        # Case B: Rotated Covering
        # Rotate the covering 45 degrees around Z.
        # Note: We rotate the Covering, but the Base (Box) remains static (0 deg).
        covering.Placement.Rotation = App.Rotation(App.Vector(0, 0, 1), 45)
        self.document.recompute()

        # In "Shaded" (Global) mode, the texture follows the Base geometry.
        # Since the Box didn't rotate, the UV vectors should still be (1,0,0) global.
        res_global = vp._compute_texture_mapping(covering, "Shaded")
        self.assertAlmostEqual(
            res_global[0].x,
            0.005,
            places=5,
            msg="Shaded mode should track the static Base geometry (Global)",
        )

        # In "Flat Lines" (Local) mode, the logic transforms Global vectors into Local space.
        # Global U=(1,0,0). Object is Rotated 45. Local X is 45 deg from Global X.
        # Inverse Rotation (-45) is applied to Global U.
        # Vector (1,0,0) rotated by -45 deg -> (0.707, -0.707, 0)
        # Scaled by 0.005 -> (0.003535, -0.003535, 0)
        res_local = vp._compute_texture_mapping(covering, "Flat Lines")

        # Check X component
        self.assertAlmostEqual(
            res_local[0].x,
            0.003535,
            places=5,
            msg="Flat Lines mode should transform vectors into rotated Local space",
        )

    def test_texture_scenegraph_structure(self):
        """
        Verify scene graph node injection (integration test).

        This test executes the full `updateTexture` workflow and inspects the Coin3D nodes to ensure
        the texture elements were actually inserted.
        """
        self.printTestMessage("texture scene graph structure...")
        import pivy.coin as coin
        from draftutils import gui_utils
        import tempfile
        import os

        # Setup
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        self.document.recompute()

        # Minimal 1x1 PNG to prevent "division by zero" errors in gui_utils and "could not be read"
        # warnings in Coin3D.
        minimal_png = (
            b"\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00\x00\x1f\x15\xc4\x89"
            b"\x00\x00\x00\nIDATx\x9cc\x00\x01\x00\x00\x05\x00\x01\r\n-\xb4\x00\x00\x00\x00IEND\xaeB`\x82"
        )

        # Create a temporary file
        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as tmp:
            tmp.write(minimal_png)
            tmp_path = tmp.name

        try:
            # Configure for texture
            covering.ViewObject.TextureImage = tmp_path
            covering.ViewObject.TextureScale = App.Vector(1, 1, 0)

            # Access Proxy
            vp = covering.ViewObject.Proxy

            # Note: We skip mocking the cache here to test the fallback path as well.
            # Since the file is now a valid PNG, gui_utils.load_texture will succeed,
            # populating the cache and returning a valid image.

            # Trigger Update
            vp.updateTexture(covering)

            # Traverse Scene Graph
            root = covering.ViewObject.RootNode
            flat_root = None

            # Find FlatRoot inside the Switch
            switch = gui_utils.find_coin_node(root, coin.SoSwitch)
            if switch:
                for i in range(switch.getNumChildren()):
                    child = switch.getChild(i)
                    if child.getName().getString() == "FlatRoot":
                        flat_root = child
                        break

            self.assertIsNotNone(flat_root, "Could not find FlatRoot node in scene graph")

            # Verify Children
            has_texture = False
            has_coords = False
            has_transform = False

            for i in range(flat_root.getNumChildren()):
                node = flat_root.getChild(i)
                if isinstance(node, coin.SoTexture2):
                    has_texture = True
                    # FIX: Use getString() instead of decode('utf-8')
                    # Also normalize paths to ensure cross-platform compatibility
                    node_path = os.path.normpath(node.filename.getValue().getString())
                    expected_path = os.path.normpath(tmp_path)
                    if node_path == expected_path:
                        pass
                elif isinstance(node, coin.SoTextureCoordinatePlane):
                    has_coords = True
                elif isinstance(node, coin.SoTexture2Transform):
                    has_transform = True

            self.assertTrue(has_texture, "SoTexture2 node is missing")
            self.assertTrue(has_coords, "SoTextureCoordinatePlane node is missing")
            self.assertTrue(has_transform, "SoTexture2Transform node is missing")

        finally:
            # Cleanup temp file
            if os.path.exists(tmp_path):
                os.remove(tmp_path)
