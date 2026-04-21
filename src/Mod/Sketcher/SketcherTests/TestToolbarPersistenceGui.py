# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD

try:
    import FreeCADGui
except ImportError:
    FreeCADGui = None

from PySide import QtCore, QtGui


def is_gui_available():
    if FreeCADGui is None:
        return False

    try:
        return FreeCADGui.getMainWindow() is not None
    except (AttributeError, RuntimeError):
        return False


class TestToolbarPersistenceGui(unittest.TestCase):
    def setUp(self):
        if not is_gui_available():
            self.skipTest("GUI not available")

        self._modified_toolbars = {}
        self._ensure_workbenches("PartWorkbench", "PartDesignWorkbench", "SketcherWorkbench")
        self._set_per_workbench_layout_preference(True)

        self.doc = FreeCAD.newDocument("TestToolbarPersistenceGui")
        FreeCADGui.activateView("Gui::View3DInventor", True)
        self.pump(200)

        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.doc.recompute()
        self.pump(200)

    def tearDown(self):
        if not is_gui_available():
            return

        try:
            self._restore_toolbars()
        finally:
            gui_doc = FreeCADGui.ActiveDocument
            if gui_doc is not None:
                try:
                    gui_doc.resetEdit()
                except Exception:
                    # resetEdit() can legitimately fail when nothing is currently in edit mode.
                    pass

            if hasattr(self, "doc") and self.doc is not None:
                if self.doc.Name in FreeCAD.listDocuments():
                    FreeCAD.closeDocument(self.doc.Name)

    def _ensure_workbenches(self, *names):
        workbenches = FreeCADGui.listWorkbenches()
        missing = [name for name in names if name not in workbenches]
        if missing:
            self.skipTest(f"Required workbenches are unavailable: {', '.join(missing)}")

    def _set_per_workbench_layout_preference(self, enabled):
        params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/MainWindow")
        saved = {
            "RememberToolbarLayoutByWorkbench": params.GetBool(
                "RememberToolbarLayoutByWorkbench", False
            )
        }
        self.addCleanup(self._restore_param_values, params, saved)
        params.SetBool("RememberToolbarLayoutByWorkbench", enabled)

    def _restore_param_values(self, params, values):
        for key, value in values.items():
            if isinstance(value, bool):
                params.SetBool(key, value)
            else:
                params.SetString(key, value)

    def pump(self, timeout_ms=120):
        loop = QtCore.QEventLoop()
        QtCore.QTimer.singleShot(timeout_ms, loop.quit)
        loop.exec_()

    def wait_until(self, predicate, description, timeout_ms=6000, step_ms=120):
        remaining = timeout_ms
        while remaining > 0:
            if predicate():
                return True
            self.pump(step_ms)
            remaining -= step_ms

        if predicate():
            return True

        self.fail(f"Timed out waiting for {description}")
        return False

    def main_window(self):
        return FreeCADGui.getMainWindow()

    def normalized_action_text(self, action):
        return str(action.text()).replace("&", "")

    def menu_action_texts(self, menu):
        return [
            text
            for text in (self.normalized_action_text(action) for action in menu.actions())
            if text
        ]

    def menu_section_texts(self, menu):
        return [
            text
            for text in (
                self.normalized_action_text(action)
                for action in menu.actions()
                if action.isSeparator()
            )
            if text
        ]

    def find_action_by_whats_this(self, whats_this):
        for action in self.main_window().findChildren(QtGui.QAction):
            if str(action.whatsThis()) == whats_this:
                return action
        return None

    def toolbar_menu(self):
        action = self.find_action_by_whats_this("Std_ToolBarMenu")
        self.assertIsNotNone(action, "Could not find Std_ToolBarMenu action")
        menu = action.menu()
        self.assertIsNotNone(menu, "Std_ToolBarMenu action should own a menu")
        return menu

    def capture_popup_menu(self, popup):
        popup.clear()
        popup.aboutToShow.emit()
        self.pump(120)
        texts = self.menu_action_texts(popup)
        sections = self.menu_section_texts(popup)
        return sections, texts

    def prepare_popup_menu(self, popup):
        popup.clear()
        popup.aboutToShow.emit()
        self.pump(120)
        return popup

    def trigger_menu_action(self, popup, action_text):
        menu = self.prepare_popup_menu(popup)
        for action in menu.actions():
            if self.normalized_action_text(action) == action_text:
                action.trigger()
                self.pump(250)
                return

        self.fail(f"Menu action '{action_text}' was not found")

    def capture_status_bar_context_menu(self):
        status_bar = self.main_window().statusBar()
        self.assertIsNotNone(status_bar, "Main window should provide a status bar")

        result = {}
        local_pos = status_bar.rect().center()
        global_pos = status_bar.mapToGlobal(local_pos)
        QtGui.QCursor.setPos(global_pos)

        def capture():
            popup = QtGui.QApplication.activePopupWidget()
            if popup is None:
                return

            result["texts"] = self.menu_action_texts(popup)
            result["sections"] = self.menu_section_texts(popup)
            popup.hide()

        QtCore.QTimer.singleShot(150, capture)
        event = QtGui.QMouseEvent(
            QtCore.QEvent.MouseButtonRelease,
            local_pos,
            global_pos,
            QtCore.Qt.RightButton,
            QtCore.Qt.RightButton,
            QtCore.Qt.NoModifier,
        )
        QtGui.QApplication.sendEvent(status_bar, event)
        self.assertIn("texts", result, "Status bar context menu did not open")
        self.pump(120)
        return result["sections"], result["texts"]

    def toolbar_key(self, toolbar):
        key = toolbar.property("PersistenceKey")
        if key:
            return str(key)
        return str(toolbar.objectName())

    def toolbar_tier(self, toolbar):
        tier = toolbar.property("Tier")
        if tier:
            return str(tier)
        return ""

    def toolbar_tier_label(self, toolbar):
        labels = {
            "recommended": QtGui.QApplication.translate("MainWindow", "Recommended"),
            "secondary": QtGui.QApplication.translate("MainWindow", "Secondary"),
            "advanced": QtGui.QApplication.translate("MainWindow", "Advanced"),
            "contextual": QtGui.QApplication.translate("MainWindow", "Contextual"),
        }
        return labels.get(self.toolbar_tier(toolbar), "")

    def toolbar_menu_label(self, toolbar):
        base_label = self.normalized_action_text(toolbar.toggleViewAction())
        if self.toolbar_tier(toolbar) == "recommended":
            return base_label

        tier_label = self.toolbar_tier_label(toolbar)
        if not tier_label:
            return base_label
        return f"{base_label} ({tier_label})"

    def toolbar_area_value(self, area):
        return int(getattr(area, "value", area))

    def toolbar_area_enum(self, value):
        mapping = {
            self.toolbar_area_value(QtCore.Qt.LeftToolBarArea): QtCore.Qt.LeftToolBarArea,
            self.toolbar_area_value(QtCore.Qt.RightToolBarArea): QtCore.Qt.RightToolBarArea,
            self.toolbar_area_value(QtCore.Qt.TopToolBarArea): QtCore.Qt.TopToolBarArea,
            self.toolbar_area_value(QtCore.Qt.BottomToolBarArea): QtCore.Qt.BottomToolBarArea,
            self.toolbar_area_value(QtCore.Qt.NoToolBarArea): QtCore.Qt.NoToolBarArea,
        }
        return mapping[value]

    def alternative_toolbar_area(self, toolbar):
        current_area = self.toolbar_area_value(self.main_window().toolBarArea(toolbar))
        for area in (
            QtCore.Qt.RightToolBarArea,
            QtCore.Qt.LeftToolBarArea,
            QtCore.Qt.BottomToolBarArea,
            QtCore.Qt.TopToolBarArea,
        ):
            if self.toolbar_area_value(area) != current_area:
                return area

        return QtCore.Qt.TopToolBarArea

    def toolbar_area_name(self, area):
        mapping = {
            self.toolbar_area_value(QtCore.Qt.LeftToolBarArea): "Left",
            self.toolbar_area_value(QtCore.Qt.RightToolBarArea): "Right",
            self.toolbar_area_value(QtCore.Qt.TopToolBarArea): "Top",
            self.toolbar_area_value(QtCore.Qt.BottomToolBarArea): "Bottom",
        }
        return mapping[self.toolbar_area_value(area)]

    def backup_bool_param(self, params, key):
        existing = key in {str(name) for name in params.GetBools()}
        value = params.GetBool(key) if existing else False

        def restore():
            if existing:
                params.SetBool(key, value)
            else:
                params.RemBool(key)

        self.addCleanup(restore)

    def backup_group(self, params, group_name, backup_name):
        had_group = params.HasGroup(group_name)
        params.RemGroup(backup_name)
        if had_group:
            params.GetGroup(group_name).CopyTo(params.GetGroup(backup_name))

        def restore():
            params.RemGroup(group_name)
            if had_group:
                params.GetGroup(backup_name).CopyTo(params.GetGroup(group_name))
            params.RemGroup(backup_name)

        self.addCleanup(restore)

    def all_toolbars(self):
        return list(self.main_window().findChildren(QtGui.QToolBar))

    def toolbars_for_prefix(self, prefix, active_only=False):
        items = []
        for toolbar in self.all_toolbars():
            key = self.toolbar_key(toolbar)
            if not key.startswith(prefix):
                continue
            if active_only and not toolbar.toggleViewAction().isVisible():
                continue
            items.append(toolbar)

        items.sort(key=lambda toolbar: self.toolbar_key(toolbar))
        return items

    def toolbar_by_key(self, key):
        for toolbar in self.all_toolbars():
            if self.toolbar_key(toolbar) == key:
                return toolbar
        return None

    def wait_for_toolbar(self, key):
        self.wait_until(lambda: self.toolbar_by_key(key) is not None, f"toolbar {key}")
        return self.toolbar_by_key(key)

    def activate_workbench(self, name, prefix=None):
        FreeCADGui.activateWorkbench(name)
        self.pump(350)
        if prefix:
            self.wait_until(
                lambda: len(self.toolbars_for_prefix(prefix, active_only=True)) > 0,
                f"{name} toolbars with prefix {prefix}",
            )
        self.pump(250)

    def choose_toolbar(self, prefix, exclude=None):
        exclude = exclude or set()
        items = [
            toolbar
            for toolbar in self.toolbars_for_prefix(prefix, active_only=True)
            if self.toolbar_key(toolbar) not in exclude
        ]
        self.assertTrue(items, f"No active toolbar found for prefix {prefix}")
        return items[0]

    def record_toolbar_state(self, toolbar, workbench, context=None):
        key = self.toolbar_key(toolbar)
        if key in self._modified_toolbars:
            return key

        self._modified_toolbars[key] = {
            "workbench": workbench,
            "context": context,
            "area": self.toolbar_area_value(self.main_window().toolBarArea(toolbar)),
            "visible": toolbar.isVisible(),
        }
        return key

    def restore_toolbar_state(self, key, state):
        toolbar = self.wait_for_toolbar(key)
        self.assertIsNotNone(toolbar, f"Expected toolbar {key} to exist during restore")
        toolbar.show()
        self.main_window().addToolBar(self.toolbar_area_enum(state["area"]), toolbar)
        self.pump(200)
        if state["visible"]:
            toolbar.show()
        else:
            toolbar.hide()
        self.pump(150)

    def move_toolbar(self, key, area):
        toolbar = self.wait_for_toolbar(key)
        self.assertIsNotNone(toolbar, f"Expected toolbar {key} to exist")
        toolbar.show()
        self.main_window().addToolBar(area, toolbar)
        self.pump(250)
        actual_area = self.main_window().toolBarArea(toolbar)
        self.assertEqual(
            self.toolbar_area_value(actual_area),
            self.toolbar_area_value(area),
            f"Toolbar {key} should be in area {self.toolbar_area_value(area)}",
        )

    def show_toolbar(self, key):
        toolbar = self.wait_for_toolbar(key)
        self.assertIsNotNone(toolbar, f"Expected toolbar {key} to exist")
        toolbar.show()
        self.pump(200)

    def hide_toolbar(self, key):
        toolbar = self.wait_for_toolbar(key)
        self.assertIsNotNone(toolbar, f"Expected toolbar {key} to exist")
        toolbar.hide()
        self.pump(200)
        self.assertFalse(toolbar.isVisible(), f"Toolbar {key} should be hidden")

    def assert_toolbar_area(self, key, expected_area):
        toolbar = self.wait_for_toolbar(key)
        self.assertIsNotNone(toolbar, f"Expected toolbar {key} to exist")
        actual_area = self.main_window().toolBarArea(toolbar)
        self.assertEqual(
            self.toolbar_area_value(actual_area),
            self.toolbar_area_value(expected_area),
            f"Toolbar {key} should restore to area {self.toolbar_area_value(expected_area)}",
        )

    def assert_toolbar_visibility(self, key, expected_visible):
        toolbar = self.wait_for_toolbar(key)
        self.assertIsNotNone(toolbar, f"Expected toolbar {key} to exist")
        self.wait_until(
            lambda: toolbar.toggleViewAction().isVisible(),
            f"toolbar {key} to become active",
        )
        self.wait_until(
            lambda: toolbar.isVisible() == expected_visible,
            f"toolbar {key} visibility to become {expected_visible}",
        )

    def enter_sketch_edit(self):
        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        ok = FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)
        self.assertTrue(ok, "Failed to enter Sketcher edit mode")
        self.wait_until(
            lambda: len(self.toolbars_for_prefix("ctx:SketcherWorkbench:edit:", active_only=True))
            > 0,
            "Sketcher contextual edit toolbars",
            timeout_ms=8000,
        )

    def leave_sketch_edit(self):
        gui_doc = FreeCADGui.ActiveDocument
        if gui_doc is None:
            return
        gui_doc.resetEdit()
        self.wait_until(
            lambda: len(self.toolbars_for_prefix("ctx:SketcherWorkbench:edit:", active_only=True))
            == 0,
            "Sketcher contextual toolbars to hide",
            timeout_ms=8000,
        )
        self.pump(200)

    def _restore_toolbars(self):
        if not self._modified_toolbars:
            return

        restored_context = False
        try:
            for key, state in self._modified_toolbars.items():
                if state["context"] is not None:
                    continue
                self.activate_workbench(state["workbench"], f"wb:{state['workbench']}:")
                self.restore_toolbar_state(key, state)

            contextual = [
                (key, state)
                for key, state in self._modified_toolbars.items()
                if state["context"] == "edit"
            ]
            if contextual:
                self.enter_sketch_edit()
                restored_context = True
                for key, state in contextual:
                    self.restore_toolbar_state(key, state)
        finally:
            if restored_context:
                self.leave_sketch_edit()

    def test_toolbar_layout_persists_across_workbench_and_edit_switches(self):
        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")
        part_toolbar = self.choose_toolbar("wb:PartWorkbench:")
        part_key = self.record_toolbar_state(part_toolbar, "PartWorkbench")
        self.move_toolbar(part_key, QtCore.Qt.LeftToolBarArea)

        self.activate_workbench("PartDesignWorkbench", "wb:PartDesignWorkbench:")
        pd_toolbar = self.choose_toolbar("wb:PartDesignWorkbench:")
        pd_key = self.record_toolbar_state(pd_toolbar, "PartDesignWorkbench")
        self.move_toolbar(pd_key, QtCore.Qt.BottomToolBarArea)

        pd_hidden_toolbar = self.choose_toolbar(
            "wb:PartDesignWorkbench:",
            exclude={pd_key},
        )
        pd_hidden_key = self.record_toolbar_state(pd_hidden_toolbar, "PartDesignWorkbench")
        self.show_toolbar(pd_hidden_key)
        self.hide_toolbar(pd_hidden_key)

        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        sketch_toolbar = self.choose_toolbar("wb:SketcherWorkbench:")
        sketch_key = self.record_toolbar_state(sketch_toolbar, "SketcherWorkbench")
        self.move_toolbar(sketch_key, QtCore.Qt.RightToolBarArea)

        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")
        self.assert_toolbar_area(part_key, QtCore.Qt.LeftToolBarArea)

        self.activate_workbench("PartDesignWorkbench", "wb:PartDesignWorkbench:")
        self.assert_toolbar_area(pd_key, QtCore.Qt.BottomToolBarArea)
        self.assert_toolbar_visibility(pd_hidden_key, False)

        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        self.assert_toolbar_area(sketch_key, QtCore.Qt.RightToolBarArea)

        self.enter_sketch_edit()
        edit_toolbar = self.choose_toolbar("ctx:SketcherWorkbench:edit:")
        edit_key = self.record_toolbar_state(
            edit_toolbar,
            "SketcherWorkbench",
            context="edit",
        )
        self.move_toolbar(edit_key, QtCore.Qt.LeftToolBarArea)

        edit_hidden_toolbar = self.choose_toolbar(
            "ctx:SketcherWorkbench:edit:",
            exclude={edit_key},
        )
        edit_hidden_key = self.record_toolbar_state(
            edit_hidden_toolbar,
            "SketcherWorkbench",
            context="edit",
        )
        self.show_toolbar(edit_hidden_key)
        self.hide_toolbar(edit_hidden_key)

        self.leave_sketch_edit()

        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")
        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        self.assert_toolbar_area(sketch_key, QtCore.Qt.RightToolBarArea)

        self.enter_sketch_edit()
        self.assert_toolbar_area(edit_key, QtCore.Qt.LeftToolBarArea)
        self.assert_toolbar_visibility(edit_hidden_key, False)
        self.leave_sketch_edit()

    def test_unsaved_scope_falls_back_to_recommended_toolbars(self):
        visibility_group = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/Toolbars")
        scoped_keys = (
            "wb:SketcherWorkbench:Sketcher",
            "ctx:SketcherWorkbench:edit:Edit Mode",
            "ctx:SketcherWorkbench:edit:Geometries",
            "ctx:SketcherWorkbench:edit:Constraints",
            "ctx:SketcherWorkbench:edit:Sketcher Tools",
            "ctx:SketcherWorkbench:edit:B-Spline Tools",
            "ctx:SketcherWorkbench:edit:Visual Helpers",
        )
        for key in ("shared:View",) + scoped_keys:
            self.backup_bool_param(visibility_group, key)

        layout_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/WorkbenchLayouts")
        self.backup_group(
            layout_params,
            "SketcherWorkbench",
            "__ToolbarUnsavedScopeBackup__SketcherWorkbench",
        )
        self.backup_group(
            layout_params,
            "ctx:SketcherWorkbench:edit",
            "__ToolbarUnsavedScopeBackup__SketcherEdit",
        )

        visibility_group.SetBool("shared:View", True)
        for key in scoped_keys:
            visibility_group.SetBool(key, False)

        layout_params.RemGroup("SketcherWorkbench")
        layout_params.RemGroup("ctx:SketcherWorkbench:edit")

        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")
        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        self.assert_toolbar_visibility("shared:View", True)
        self.assert_toolbar_visibility("wb:SketcherWorkbench:Sketcher", True)

        self.enter_sketch_edit()
        self.assert_toolbar_visibility("shared:View", True)
        self.assert_toolbar_visibility("ctx:SketcherWorkbench:edit:Geometries", True)
        self.leave_sketch_edit()

    def test_toolbar_menu_groups_and_reset_actions(self):
        shared_label = QtGui.QApplication.translate("MainWindow", "Shared Toolbars")
        workbench_label = QtGui.QApplication.translate("MainWindow", "Workbench Toolbars")
        contextual_label = QtGui.QApplication.translate("MainWindow", "Contextual Toolbars")
        reset_workbench_label = QtGui.QApplication.translate(
            "MainWindow", "Reset Current Workbench Layout"
        )
        reset_contextual_label = QtGui.QApplication.translate(
            "MainWindow", "Reset Current Contextual Layout"
        )
        show_recommended_only_label = QtGui.QApplication.translate(
            "MainWindow", "Show Recommended Only"
        )
        recommended_reset_workbench_label = QtGui.QApplication.translate(
            "MainWindow", "Reset To Recommended Workbench Layout"
        )
        recommended_reset_contextual_label = QtGui.QApplication.translate(
            "MainWindow", "Reset To Recommended Contextual Layout"
        )

        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")

        sections, texts = self.capture_popup_menu(self.toolbar_menu())
        self.assertIn(
            shared_label, sections, "Main toolbar menu should expose shared toolbar group"
        )
        self.assertIn(
            workbench_label,
            sections,
            "Main toolbar menu should expose workbench toolbar group in workbench mode",
        )
        self.assertIn(
            sketcher_toolbar_label,
            texts,
            "Main toolbar menu should expose recommended tier label for workbench toolbars",
        )
        self.assertIn(
            clipboard_toolbar_label,
            texts,
            "Main toolbar menu should expose secondary tier label for shared toolbars",
        )
        self.assertIn(
            macro_toolbar_label,
            texts,
            "Main toolbar menu should expose advanced tier label for shared toolbars",
        )
        self.assertIn(
            show_recommended_only_label,
            texts,
            "Main toolbar menu should expose show recommended only action in workbench mode",
        )
        self.assertIn(
            reset_workbench_label,
            texts,
            "Main toolbar menu should expose workbench layout reset in workbench mode",
        )
        self.assertIn(
            recommended_reset_workbench_label,
            texts,
            "Main toolbar menu should expose recommended workbench reset in workbench mode",
        )

        self.enter_sketch_edit()
        sections, texts = self.capture_popup_menu(self.toolbar_menu())
        self.assertIn(shared_label, sections, "Main toolbar menu should keep shared toolbar group")
        self.assertIn(
            contextual_label,
            sections,
            "Main toolbar menu should expose contextual toolbar group during edit mode",
        )
        self.assertIn(
            contextual_toolbar_label,
            texts,
            "Main toolbar menu should expose contextual tier label during edit mode",
        )
        self.assertIn(
            show_recommended_only_label,
            texts,
            "Main toolbar menu should expose show recommended only action during edit mode",
        )
        self.assertIn(
            reset_contextual_label,
            texts,
            "Main toolbar menu should expose contextual layout reset during edit mode",
        )
        self.assertIn(
            recommended_reset_contextual_label,
            texts,
            "Main toolbar menu should expose recommended contextual reset during edit mode",
        )

        self.leave_sketch_edit()
        _, texts = self.capture_status_bar_context_menu()
        self.assertIn(
            reset_workbench_label,
            texts,
            "Workbench runtime context menu should expose workbench layout reset",
        )
        self.assertNotIn(
            reset_contextual_label,
            texts,
            "Workbench runtime context menu should not expose contextual reset outside edit mode",
        )
        self.assertIn(
            recommended_reset_workbench_label,
            texts,
            "Workbench runtime context menu should expose recommended workbench reset",
        )
        self.assertIn(
            show_recommended_only_label,
            texts,
            "Workbench runtime context menu should expose show recommended only action",
        )
        self.assertNotIn(
            recommended_reset_contextual_label,
            texts,
            "Workbench runtime context menu should not expose recommended contextual reset outside edit mode",
        )

        self.enter_sketch_edit()
        _, texts = self.capture_status_bar_context_menu()
        self.assertIn(
            reset_contextual_label,
            texts,
            "Contextual runtime context menu should expose contextual layout reset",
        )
        self.assertNotIn(
            reset_workbench_label,
            texts,
            "Contextual runtime context menu should not expose workbench reset label",
        )
        self.assertIn(
            recommended_reset_contextual_label,
            texts,
            "Contextual runtime context menu should expose recommended contextual reset",
        )
        self.assertIn(
            show_recommended_only_label,
            texts,
            "Contextual runtime context menu should expose show recommended only action",
        )
        self.assertNotIn(
            recommended_reset_workbench_label,
            texts,
            "Contextual runtime context menu should not expose recommended workbench reset",
        )
        self.leave_sketch_edit()

    def test_toolbar_tier_metadata_is_exposed(self):
        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")

        self.assertEqual(
            self.toolbar_tier(self.wait_for_toolbar("wb:SketcherWorkbench:Sketcher")),
            "recommended",
        )
        self.assertEqual(
            self.toolbar_tier(self.wait_for_toolbar("shared:Clipboard")),
            "secondary",
        )
        self.assertEqual(
            self.toolbar_tier(self.wait_for_toolbar("shared:Macro")),
            "advanced",
        )

        self.enter_sketch_edit()
        self.assertEqual(
            self.toolbar_tier(self.wait_for_toolbar("ctx:SketcherWorkbench:edit:Geometries")),
            "contextual",
        )
        self.leave_sketch_edit()

    def test_recommended_toolbar_reset_restores_tier_defaults(self):
        recommended_reset_workbench_label = QtGui.QApplication.translate(
            "MainWindow", "Reset To Recommended Workbench Layout"
        )
        recommended_reset_contextual_label = QtGui.QApplication.translate(
            "MainWindow", "Reset To Recommended Contextual Layout"
        )

        visibility_group = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/Toolbars")
        for key in (
            "shared:View",
            "shared:Clipboard",
            "shared:Macro",
            "ctx:SketcherWorkbench:edit:Geometries",
        ):
            self.backup_bool_param(visibility_group, key)

        layout_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/WorkbenchLayouts")
        self.backup_group(
            layout_params,
            "SketcherWorkbench",
            "__ToolbarRecommendedResetBackup__SketcherWorkbench",
        )
        self.backup_group(
            layout_params,
            "ctx:SketcherWorkbench:edit",
            "__ToolbarRecommendedResetBackup__SketcherEdit",
        )

        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        for key in ("shared:View", "shared:Clipboard", "shared:Macro"):
            self.record_toolbar_state(self.wait_for_toolbar(key), "SketcherWorkbench")

        self.show_toolbar("shared:Clipboard")
        self.show_toolbar("shared:Macro")
        self.hide_toolbar("shared:View")

        self.trigger_menu_action(self.toolbar_menu(), recommended_reset_workbench_label)
        self.assert_toolbar_visibility("shared:View", True)
        self.assert_toolbar_visibility("shared:Clipboard", False)
        self.assert_toolbar_visibility("shared:Macro", False)

        self.enter_sketch_edit()
        self.record_toolbar_state(
            self.wait_for_toolbar("ctx:SketcherWorkbench:edit:Geometries"),
            "SketcherWorkbench",
            context="edit",
        )
        self.hide_toolbar("ctx:SketcherWorkbench:edit:Geometries")

        self.trigger_menu_action(self.toolbar_menu(), recommended_reset_contextual_label)
        self.assert_toolbar_visibility("ctx:SketcherWorkbench:edit:Geometries", True)
        self.leave_sketch_edit()

    def test_show_recommended_only_preserves_layout(self):
        show_recommended_only_label = QtGui.QApplication.translate(
            "MainWindow", "Show Recommended Only"
        )

        visibility_group = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/Toolbars")
        for key in (
            "shared:View",
            "shared:Clipboard",
            "shared:Macro",
            "ctx:SketcherWorkbench:edit:Geometries",
        ):
            self.backup_bool_param(visibility_group, key)

        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")

        view_toolbar = self.wait_for_toolbar("shared:View")
        self.record_toolbar_state(view_toolbar, "SketcherWorkbench")
        self.move_toolbar("shared:View", QtCore.Qt.RightToolBarArea)

        for key in ("shared:Clipboard", "shared:Macro"):
            self.record_toolbar_state(self.wait_for_toolbar(key), "SketcherWorkbench")

        self.show_toolbar("shared:Clipboard")
        self.show_toolbar("shared:Macro")
        self.hide_toolbar("shared:View")

        self.trigger_menu_action(self.toolbar_menu(), show_recommended_only_label)
        self.assert_toolbar_visibility("shared:View", True)
        self.assert_toolbar_visibility("shared:Clipboard", False)
        self.assert_toolbar_visibility("shared:Macro", False)
        self.assert_toolbar_area("shared:View", QtCore.Qt.RightToolBarArea)

        self.enter_sketch_edit()
        contextual_key = "ctx:SketcherWorkbench:edit:Geometries"
        self.record_toolbar_state(
            self.wait_for_toolbar(contextual_key),
            "SketcherWorkbench",
            context="edit",
        )
        self.move_toolbar(contextual_key, QtCore.Qt.LeftToolBarArea)
        self.hide_toolbar(contextual_key)

        self.trigger_menu_action(self.toolbar_menu(), show_recommended_only_label)
        self.assert_toolbar_visibility(contextual_key, True)
        self.assert_toolbar_area(contextual_key, QtCore.Qt.LeftToolBarArea)
        self.leave_sketch_edit()

    def test_legacy_toolbar_names_restore_with_scoped_keys(self):
        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        toolbar = self.wait_for_toolbar("wb:SketcherWorkbench:Sketcher")
        key = self.record_toolbar_state(toolbar, "SketcherWorkbench")
        legacy_name = str(toolbar.objectName())
        self.assertNotEqual(key, legacy_name, "Test requires a scoped toolbar persistence key")

        target_area = self.alternative_toolbar_area(toolbar)
        target_area_name = self.toolbar_area_name(target_area)
        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")

        visibility_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/Toolbars")
        self.backup_bool_param(visibility_params, key)
        self.backup_bool_param(visibility_params, legacy_name)
        visibility_params.RemBool(key)
        visibility_params.SetBool(legacy_name, False)

        layout_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/WorkbenchLayouts")
        self.backup_group(
            layout_params,
            "SketcherWorkbench",
            "__ToolbarMigrationBackup__SketcherWorkbench",
        )
        workbench_layout = layout_params.GetGroup("SketcherWorkbench")
        workbench_layout.Clear()
        workbench_layout.SetBool("Saved", True)
        for area_name in ("Top", "Left", "Right", "Bottom"):
            workbench_layout.SetString(
                area_name, legacy_name if area_name == target_area_name else ""
            )

        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        self.assert_toolbar_visibility(key, False)
        self.assert_toolbar_area(key, target_area)
