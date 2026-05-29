# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD BIM workbench.
# You can find the full license text in the LICENSE file in the root directory.

import Arch
import Draft
import ArchCoveringGui
import FreeCAD as App
import FreeCADGui as Gui
from FreeCAD import Qt
from draftutils import params
from bimtests import TestArchBaseGui

translate = Qt.translate


class TestArchCoveringGui(TestArchBaseGui.TestArchBaseGui):
    """GUI-side tests for Arch Covering Task Panel logic and workflows."""

    # These are all params that _save_user_preferences() may write. Saving and restoring these using
    # the hardcoded defaults (ret_default=True) ensures every test starts from a clean, known state
    # regardless of what values prior tests or sessions stored.
    _COVERING_PARAMS = [
        "CoveringLength",
        "CoveringWidth",
        "CoveringThickness",
        "CoveringJoint",
        "CoveringRotation",
        "CoveringFinishMode",
        "CoveringAlignment",
    ]

    def setUp(self):
        super().setUp()
        self.box = self.document.addObject("Part::Box", "BaseBox")
        self.document.recompute()
        self.panel = None
        # Snapshot the hardcoded defaults and immediately write them into the param store so that
        # every test begins with the same known values, even if a prior test (or a prior run of the
        # suite) left corrupted values behind.
        self._saved_params = {}
        for k in self._COVERING_PARAMS:
            self._saved_params[k] = params.get_param_arch(k, ret_default=False)
            params.set_param_arch(k, params.get_param_arch(k, ret_default=True))

    def tearDown(self):
        # Restore the param store to its pre-test state so the test does not permanently alter the
        # user's preferences.
        for k, v in self._saved_params.items():
            params.set_param_arch(k, v)
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
                    # Compare stripped strings to avoid false negatives.
                    self.assertEqual(expr_val.replace(" ", ""), expression.replace(" ", ""))
                    found_expr = True
                    break

        self.assertTrue(found_expr, "Expression was not found on the generated object.")

    def test_selection_ui_labels(self):
        """Test the smart labeling logic for the 'Base' field."""
        self.printTestMessage("selection UI labels...")
        self.panel = ArchCoveringGui.ArchCoveringTaskPanel()

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

        # 3 coverings created; box already in initial_count; template created then deleted within
        # accept()
        self.assertEqual(len(self.document.Objects), initial_count + 3)

        # Filter out non-Covering objects
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
        self.assertEqual(self.panel.le_selection.text(), translate("Arch", "No selection"))
        self.assertTrue(self.panel.isPicking(), "Picking should be re-armed automatically.")

        # Ensure template still exists for the next round
        self.assertIsNotNone(self.document.getObject(self.panel.template.buffer.Name))

    def test_mode_switching_ux(self):
        """Verify mode-dependent widget enable/disable state."""
        self.printTestMessage("mode switching UX...")
        self.panel = ArchCoveringGui.ArchCoveringTaskPanel()

        # Thickness is always enabled regardless of mode
        self.panel.combo_mode.setCurrentIndex(0)
        self.assertTrue(self.panel.sb_thick.isEnabled())

        self.panel.combo_mode.setCurrentIndex(1)
        self.assertTrue(self.panel.sb_thick.isEnabled())

        self.panel.combo_mode.setCurrentIndex(2)
        self.assertTrue(self.panel.sb_thick.isEnabled())

        self.panel.combo_mode.setCurrentIndex(3)
        self.assertTrue(self.panel.sb_thick.isEnabled())

        # Visuals (texture) widget is disabled only in Hatch Pattern mode (index 3)
        self.panel.combo_mode.setCurrentIndex(0)
        self.assertTrue(self.panel.vis_widget.isEnabled())

        self.panel.combo_mode.setCurrentIndex(3)
        self.assertFalse(self.panel.vis_widget.isEnabled())

        self.panel.combo_mode.setCurrentIndex(0)
        self.assertTrue(self.panel.vis_widget.isEnabled())

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

        # Assert: the self-dependency filter should have cleared the selection
        self.assertIsNone(self.panel.selected_obj, "The panel allowed selecting itself as a base.")

    def test_texture_mapping_math(self):
        """
        Verify texture vector math isolation (logic test).
        """
        self.printTestMessage("texture mapping math...")

        # Setup: Create a covering on the XY plane
        # 1000x1000 face, 0 rotation
        base = (self.box, ["Face6"])  # Face6 is Top (Z=1000)
        covering = Arch.makeCovering(base)
        # Recompute first with default dimensions so execute() runs warning-free, then assign the
        # test-specific values. _compute_texture_mapping reads properties directly without
        # recomputing, so JointWidth=0 (chosen for clean period math: period = TileLength) is safe
        # here but would trigger JOINT_TOO_SMALL if execute() saw it.
        self.document.recompute()
        covering.TileLength = 200.0
        covering.TileWidth = 200.0
        covering.JointWidth = 0.0
        covering.Rotation = 0.0

        # Access the ViewProvider Proxy
        vp = covering.ViewObject.Proxy

        # Case A: shaded mode (global coordinates)
        # Period = TileLength + JointWidth = 200 + 0 = 200.
        # The texture scale must be 1/period in both U and V, and U ⊥ V.
        # We do not assert specific axis directions because getFaceFrame is edge-aligned and the
        # direction depends on face geometry, not world axes.
        period = covering.TileLength + covering.JointWidth  # = 200.0
        expected_scale = 1.0 / period  # = 0.005

        res_shaded = vp._compute_texture_mapping(covering)
        self.assertIsNotNone(res_shaded, "Mapping should succeed")

        dir_u, dir_v, s_off, t_off = res_shaded

        self.assertAlmostEqual(
            dir_u.Length, expected_scale, places=5, msg="dir_u magnitude should be 1/period"
        )
        self.assertAlmostEqual(
            dir_v.Length, expected_scale, places=5, msg="dir_v magnitude should be 1/period"
        )
        self.assertAlmostEqual(
            dir_u.dot(dir_v), 0.0, places=5, msg="dir_u and dir_v must be perpendicular"
        )

        # Case B: rotated covering
        # Rotate the covering 45 degrees around Z.
        # Note: the Covering is rotated, but the Base (Box) remains static (0 deg). Note:
        # covering.Placement.Rotation = ... is a no-op in Python because Placement returns a copy.
        # The full Placement object must be assigned. This is done after recompute(), because
        # execute() resets Placement from the geometry engine result. _compute_texture_mapping reads
        # Placement directly and does not trigger a recompute, so the value is stable.
        self.document.recompute()
        covering.Placement = App.Placement(
            covering.Placement.Base, App.Rotation(App.Vector(0, 0, 1), 45)
        )

        # The texture follows the Base geometry, not the covering's Placement.
        # Since the Box didn't rotate, the UV vectors must be unchanged from Case A.
        res_global = vp._compute_texture_mapping(covering)
        self.assertAlmostEqual(
            res_global[0].Length,
            expected_scale,
            places=5,
            msg="UV scale must be invariant to object rotation",
        )
        self.assertTrue(
            res_global[0].isEqual(res_shaded[0], 1e-6),
            msg="UV vectors must be invariant to object rotation",
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
            # Configure for texture. TextureImage and TextureScale live on the DocumentObject (not
            # the ViewObject) so they are saved in the FCStd file.
            covering.TextureImage = tmp_path
            covering.TextureScale = App.Vector(1, 1, 0)

            # Access Proxy
            vp = covering.ViewObject.Proxy

            # Skip mocking the cache here to test the fallback path as well. Since the file is now a
            # valid PNG, gui_utils.load_texture will succeed, populating the cache and returning a
            # valid image.

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

            # Verify children
            has_texture = False
            has_coords = False
            has_transform = False

            for i in range(flat_root.getNumChildren()):
                node = flat_root.getChild(i)
                if isinstance(node, coin.SoTexture2):
                    has_texture = True
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

    def test_edit_mode_accept(self):
        """Verify that accepting in edit mode writes panel values back to the covering."""
        self.printTestMessage("edit mode accept...")
        covering = Arch.makeCovering(self.box)
        covering.TileWidth = 300.0
        self.document.recompute()

        self.panel = ArchCoveringGui.ArchCoveringTaskPanel(obj=covering)

        # Simulate the user changing TileWidth in the spinbox
        new_width = 450.0
        self.panel.sb_width.setProperty("rawValue", new_width)
        self.pump_gui_events()

        result = self.panel.accept()
        self.pump_gui_events()

        self.assertTrue(result, "Edit mode accept should return True.")
        self.assertAlmostEqual(
            covering.TileWidth.Value,
            new_width,
            places=3,
            msg="Covering TileWidth should reflect the value set in the panel.",
        )
        # accept() cleans up the panel; prevent tearDown from calling reject() again.
        self.panel = None

    def test_preference_saving_on_accept(self):
        """Verify that accepting a creation saves the current values to user preferences."""
        self.printTestMessage("preference saving on accept...")
        self.panel = ArchCoveringGui.ArchCoveringTaskPanel(selection=[(self.box, ["Face6"])])

        # Set a distinctive TileLength in the spinbox
        test_length = 333.0
        self.panel.sb_length.setProperty("rawValue", test_length)
        self.pump_gui_events()

        self.panel.accept()
        self.pump_gui_events()

        saved = params.get_param_arch("CoveringLength")
        self.assertAlmostEqual(
            saved,
            test_length,
            places=3,
            msg="CoveringLength param should be updated after accept.",
        )
        self.panel = None
