# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import subprocess
import sys
import tempfile
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
        if os.environ.get("FREECAD_TOOLBAR_RESTART_PHASE") == "read":
            main_window_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow")
            shared_layout = self.layout_group_text(
                main_window_params, "SharedToolBarLayout", "Left"
            )
            self.assertIn("shared:::View", shared_layout)

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
        if os.environ.get("FREECAD_TOOLBAR_RESTART_PHASE"):
            # Restart workers use an isolated configuration and must leave this setting
            # enabled for the next process to exercise the real startup path.
            params.SetBool("RememberToolbarLayoutByWorkbench", enabled)
            return

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

    def _run_restart_phase(self, config_path, phase):
        command = [
            sys.executable,
            "--user-cfg",
            config_path,
            "--run-test",
            "TestSketcherGui.TestToolbarPersistenceGui.test_toolbar_layout_survives_process_restart",
        ]
        environment = os.environ.copy()
        environment["FREECAD_TOOLBAR_RESTART_PHASE"] = phase
        try:
            result = subprocess.run(
                command,
                env=environment,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=120,
                check=False,
            )
        except subprocess.TimeoutExpired as error:
            output = error.stdout or error.stderr or ""
            self.fail(f"FreeCAD restart phase {phase} timed out:\n{output[-6000:]}")
        if result.returncode:
            self.fail(
                f"FreeCAD restart phase {phase} failed with exit code "
                f"{result.returncode}:\n{result.stdout[-6000:]}"
            )

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
        texts = []
        for action in menu.actions():
            text = self.normalized_action_text(action)
            if text:
                texts.append(text)
            if action.menu() is not None:
                texts.extend(self.menu_action_texts(action.menu()))
        return texts

    def find_menu_action(self, menu, action_text):
        for action in menu.actions():
            if self.normalized_action_text(action) == action_text:
                return action
            if action.menu() is not None:
                nested = self.find_menu_action(action.menu(), action_text)
                if nested is not None:
                    return nested
        return None

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
        action = self.find_menu_action(menu, action_text)
        if action is not None:
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
        # Send a complete right-button click so the toolbar manager's event filter
        # receives the release event on the status bar.  A release-only event is
        # platform-dependent and can bypass that filter under the offscreen backend.
        for event_type in (QtCore.QEvent.MouseButtonPress, QtCore.QEvent.MouseButtonRelease):
            event = QtGui.QMouseEvent(
                event_type,
                local_pos,
                global_pos,
                QtCore.Qt.RightButton,
                QtCore.Qt.RightButton,
                QtCore.Qt.NoModifier,
            )
            QtGui.QApplication.sendEvent(status_bar, event)
        self.pump(250)
        self.assertIn("texts", result, "Status bar context menu did not open")
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
        if self.toolbar_tier(toolbar) in {"recommended", "contextual"}:
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

    def layout_group_text(self, params, group_name, area=None):
        layout = params.GetGroup(group_name).GetGroup("Layout")
        identities = []
        scope_names = {0: "legacy", 1: "shared", 2: "wb", 3: "ctx"}
        areas = (area,) if area else ("Top", "Left", "Right", "Bottom")
        for area_name in areas:
            area_group = layout.GetGroup(area_name)
            for index in range(area_group.GetInt("Count")):
                entry = area_group.GetGroup(str(index))
                if entry.GetBool("Break"):
                    continue
                identities.append(
                    ":".join(
                        (
                            scope_names[entry.GetInt("Scope")],
                            entry.GetString("Workbench"),
                            entry.GetString("Context"),
                            entry.GetString("Toolbar"),
                        )
                    )
                )
        return ",".join(identities)

    def layout_group_signature(self, params, group_name):
        group = params.GetGroup(group_name)
        layout = group.GetGroup("Layout")
        areas = []
        for area_name in ("Top", "Left", "Right", "Bottom"):
            area_group = layout.GetGroup(area_name)
            entries = []
            for index in range(area_group.GetInt("Count")):
                entry = area_group.GetGroup(str(index))
                if entry.GetBool("Break"):
                    entries.append("Break")
                else:
                    entries.append(
                        (
                            entry.GetInt("Scope"),
                            entry.GetString("Workbench"),
                            entry.GetString("Context"),
                            entry.GetString("Toolbar"),
                            entry.GetInt("SharedPrefix"),
                        )
                    )
            areas.append((area_name, tuple(entries)))
        return group.GetBool("Saved"), tuple(areas)

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
        self.wait_until(
            lambda: self.toolbar_area_value(self.main_window().toolBarArea(toolbar))
            == self.toolbar_area_value(expected_area),
            f"toolbar {key} area to become {self.toolbar_area_value(expected_area)}",
        )
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

    def test_toolbar_layout_survives_process_restart(self):
        phase = os.environ.get("FREECAD_TOOLBAR_RESTART_PHASE")
        if phase:
            self._run_restart_worker(phase)
            return

        with tempfile.TemporaryDirectory(prefix="freecad-toolbar-restart-") as directory:
            config_path = os.path.join(directory, "user.cfg")
            self._run_restart_phase(config_path, "write")
            self._run_restart_phase(config_path, "read")

    def _run_restart_worker(self, phase):
        if phase == "read":
            main_window_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow")
            shared_layout = self.layout_group_text(
                main_window_params, "SharedToolBarLayout", "Left"
            )
            self.assertIn("shared:::View", shared_layout)

        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")
        if phase == "write":
            toolbar = self.wait_for_toolbar("shared:View")
            toolbar.show()
            self.main_window().addToolBar(QtCore.Qt.LeftToolBarArea, toolbar)
            self.pump(300)
            self.assert_toolbar_area("shared:View", QtCore.Qt.LeftToolBarArea)
            self.activate_workbench("PartDesignWorkbench", "wb:PartDesignWorkbench:")
            main_window_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow")
            shared_layout = self.layout_group_text(main_window_params, "SharedToolBarLayout")
            self.assertIn("shared:::View", shared_layout)
            FreeCAD.saveParameter()
        elif phase == "read":
            main_window_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow")
            shared_layout = self.layout_group_text(
                main_window_params, "SharedToolBarLayout", "Left"
            )
            self.assertIn("shared:::View", shared_layout)
            self.assert_toolbar_area("shared:View", QtCore.Qt.LeftToolBarArea)
        else:
            self.fail(f"Unknown toolbar restart phase: {phase}")

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

    def test_scoped_layout_switch_saves_only_current_scope_toolbars(self):
        layout_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/WorkbenchLayouts")
        self.backup_group(
            layout_params,
            "SketcherWorkbench",
            "__ToolbarScopeIsolationBackup__SketcherWorkbench",
        )
        self.backup_group(
            layout_params,
            "ctx:SketcherWorkbench:edit",
            "__ToolbarScopeIsolationBackup__SketcherEdit",
        )
        layout_params.RemGroup("SketcherWorkbench")
        layout_params.RemGroup("ctx:SketcherWorkbench:edit")

        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        self.wait_for_toolbar("wb:SketcherWorkbench:Sketcher")
        for _ in range(2):
            self.enter_sketch_edit()
            self.wait_for_toolbar("ctx:SketcherWorkbench:edit:Geometries")
            self.leave_sketch_edit()

        workbench_layout = self.layout_group_text(layout_params, "SketcherWorkbench")
        contextual_layout = self.layout_group_text(layout_params, "ctx:SketcherWorkbench:edit")
        self.assertNotIn("ctx:SketcherWorkbench:edit:", workbench_layout)
        self.assertNotIn("wb:SketcherWorkbench:", contextual_layout)
        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")

    def test_shared_toolbar_layout_is_global_across_workbenches(self):
        layout_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/WorkbenchLayouts")
        main_window_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow")
        self.backup_group(
            layout_params,
            "PartWorkbench",
            "__ToolbarSharedIsolationBackup__PartWorkbench",
        )
        self.backup_group(
            layout_params,
            "PartDesignWorkbench",
            "__ToolbarSharedIsolationBackup__PartDesignWorkbench",
        )
        self.backup_group(
            main_window_params,
            "SharedToolBarLayout",
            "__ToolbarSharedIsolationBackup__SharedToolBarLayout",
        )
        layout_params.RemGroup("PartWorkbench")
        layout_params.RemGroup("PartDesignWorkbench")
        main_window_params.RemGroup("SharedToolBarLayout")

        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")
        shared_key = self.record_toolbar_state(
            self.wait_for_toolbar("shared:View"),
            "PartWorkbench",
        )
        part_key = self.record_toolbar_state(
            self.choose_toolbar("wb:PartWorkbench:"),
            "PartWorkbench",
        )
        self.move_toolbar(shared_key, QtCore.Qt.RightToolBarArea)
        self.move_toolbar(part_key, QtCore.Qt.LeftToolBarArea)

        self.activate_workbench("PartDesignWorkbench", "wb:PartDesignWorkbench:")
        self.assert_toolbar_area(shared_key, QtCore.Qt.RightToolBarArea)
        pd_key = self.record_toolbar_state(
            self.choose_toolbar("wb:PartDesignWorkbench:"),
            "PartDesignWorkbench",
        )
        self.move_toolbar(pd_key, QtCore.Qt.TopToolBarArea)

        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")
        self.assert_toolbar_area(shared_key, QtCore.Qt.RightToolBarArea)
        self.assert_toolbar_area(part_key, QtCore.Qt.LeftToolBarArea)
        self.assertNotIn("shared:", self.layout_group_text(layout_params, "PartWorkbench"))
        self.assertIn(
            "shared:::View", self.layout_group_text(main_window_params, "SharedToolBarLayout")
        )

        self.activate_workbench("PartDesignWorkbench", "wb:PartDesignWorkbench:")
        self.assert_toolbar_area(shared_key, QtCore.Qt.RightToolBarArea)
        self.assert_toolbar_area(pd_key, QtCore.Qt.TopToolBarArea)

    def test_auto_arrange_workbench_keeps_shared_and_inactive_scopes(self):
        layout_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/WorkbenchLayouts")
        main_window_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow")
        self.backup_group(
            layout_params,
            "PartWorkbench",
            "__ToolbarAutoArrangeIsolationBackup__PartWorkbench",
        )
        self.backup_group(
            layout_params,
            "PartDesignWorkbench",
            "__ToolbarAutoArrangeIsolationBackup__PartDesignWorkbench",
        )
        self.backup_group(
            main_window_params,
            "SharedToolBarLayout",
            "__ToolbarAutoArrangeIsolationBackup__SharedToolBarLayout",
        )
        layout_params.RemGroup("PartWorkbench")
        layout_params.RemGroup("PartDesignWorkbench")
        main_window_params.RemGroup("SharedToolBarLayout")

        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")
        shared_key = self.record_toolbar_state(
            self.wait_for_toolbar("shared:View"),
            "PartWorkbench",
        )
        part_key = self.record_toolbar_state(
            self.choose_toolbar("wb:PartWorkbench:"),
            "PartWorkbench",
        )
        self.move_toolbar(shared_key, QtCore.Qt.RightToolBarArea)
        self.move_toolbar(part_key, QtCore.Qt.LeftToolBarArea)

        self.activate_workbench("PartDesignWorkbench", "wb:PartDesignWorkbench:")
        self.assert_toolbar_area(shared_key, QtCore.Qt.RightToolBarArea)
        inactive_layout_before = self.layout_group_signature(layout_params, "PartWorkbench")
        pd_key = self.record_toolbar_state(
            self.choose_toolbar("wb:PartDesignWorkbench:"),
            "PartDesignWorkbench",
        )
        self.move_toolbar(pd_key, QtCore.Qt.LeftToolBarArea)
        self.hide_toolbar(pd_key)
        shared_layout_before = self.layout_group_signature(
            main_window_params, "SharedToolBarLayout"
        )
        shared_toolbar = self.wait_for_toolbar(shared_key)
        shared_toolbar_before = (
            self.toolbar_area_value(self.main_window().toolBarArea(shared_toolbar)),
            shared_toolbar.geometry(),
            self.main_window().toolBarBreak(shared_toolbar),
        )
        self.trigger_menu_action(
            self.toolbar_menu(),
            QtGui.QApplication.translate("MainWindow", "Auto-arrange Toolbar Layout"),
        )
        self.assert_toolbar_area(pd_key, QtCore.Qt.TopToolBarArea)
        self.assert_toolbar_visibility(pd_key, False)
        self.assert_toolbar_area(shared_key, QtCore.Qt.RightToolBarArea)
        self.assertEqual(
            (
                self.toolbar_area_value(self.main_window().toolBarArea(shared_toolbar)),
                shared_toolbar.geometry(),
                self.main_window().toolBarBreak(shared_toolbar),
            ),
            shared_toolbar_before,
            "Auto-arrange must not move or re-row shared toolbars",
        )
        self.assertEqual(
            self.layout_group_signature(main_window_params, "SharedToolBarLayout"),
            shared_layout_before,
            "Auto-arrange must not rewrite the shared toolbar layout",
        )
        self.assertEqual(
            self.layout_group_signature(layout_params, "PartWorkbench"),
            inactive_layout_before,
            "Auto-arrange must not rewrite an inactive workbench layout",
        )

        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")
        self.assert_toolbar_area(shared_key, QtCore.Qt.RightToolBarArea)
        self.assert_toolbar_area(part_key, QtCore.Qt.LeftToolBarArea)

    def test_auto_arrange_contextual_scope_preserves_workbench_and_visibility(self):
        layout_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/WorkbenchLayouts")
        self.backup_group(
            layout_params,
            "SketcherWorkbench",
            "__ToolbarContextualAutoArrangeBackup__SketcherWorkbench",
        )
        self.backup_group(
            layout_params,
            "ctx:SketcherWorkbench:edit",
            "__ToolbarContextualAutoArrangeBackup__SketcherEdit",
        )
        layout_params.RemGroup("SketcherWorkbench")
        layout_params.RemGroup("ctx:SketcherWorkbench:edit")

        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        workbench_key = self.record_toolbar_state(
            self.wait_for_toolbar("wb:SketcherWorkbench:Sketcher"),
            "SketcherWorkbench",
        )
        self.move_toolbar(workbench_key, QtCore.Qt.RightToolBarArea)
        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")
        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        self.assert_toolbar_area(workbench_key, QtCore.Qt.RightToolBarArea)

        self.enter_sketch_edit()
        contextual_key = "ctx:SketcherWorkbench:edit:Geometries"
        contextual_toolbar = self.wait_for_toolbar(contextual_key)
        self.record_toolbar_state(contextual_toolbar, "SketcherWorkbench", context="edit")
        self.move_toolbar(contextual_key, QtCore.Qt.LeftToolBarArea)
        self.hide_toolbar(contextual_key)
        main_window_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow")
        shared_layout_before = self.layout_group_signature(
            main_window_params, "SharedToolBarLayout"
        )
        shared_toolbar = self.wait_for_toolbar("shared:View")
        shared_toolbar_before = (
            self.toolbar_area_value(self.main_window().toolBarArea(shared_toolbar)),
            shared_toolbar.geometry(),
            self.main_window().toolBarBreak(shared_toolbar),
        )

        try:
            self.trigger_menu_action(
                self.toolbar_menu(),
                QtGui.QApplication.translate("MainWindow", "Auto-arrange Toolbar Layout"),
            )
            self.assert_toolbar_area(workbench_key, QtCore.Qt.RightToolBarArea)
            self.assert_toolbar_area(contextual_key, QtCore.Qt.TopToolBarArea)
            self.assert_toolbar_visibility(contextual_key, False)
            self.assertEqual(
                (
                    self.toolbar_area_value(self.main_window().toolBarArea(shared_toolbar)),
                    shared_toolbar.geometry(),
                    self.main_window().toolBarBreak(shared_toolbar),
                ),
                shared_toolbar_before,
                "Contextual auto-arrange must not move or re-row shared toolbars",
            )
            self.assertEqual(
                self.layout_group_signature(main_window_params, "SharedToolBarLayout"),
                shared_layout_before,
                "Contextual auto-arrange must not rewrite the shared toolbar layout",
            )
        finally:
            self.leave_sketch_edit()

    def test_custom_toolbar_tier_is_loaded_from_preferences(self):
        workbench_params = FreeCAD.ParamGet("User parameter:BaseApp/Workbench")
        self.backup_group(
            workbench_params,
            "SketcherWorkbench",
            "__ToolbarCustomTierBackup__SketcherWorkbench",
        )

        toolbar_group = workbench_params.GetGroup("SketcherWorkbench").GetGroup("Toolbar")
        toolbar_group.Clear()
        custom_toolbar = toolbar_group.GetGroup("Custom_1")
        custom_toolbar.SetString("Name", "Custom Tier Test")
        custom_toolbar.SetBool("Active", True)
        custom_toolbar.SetString("Tier", "advanced")
        custom_toolbar.SetString("Std_Undo", "Gui")

        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")
        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")

        toolbar = self.wait_for_toolbar("wb:SketcherWorkbench:Custom Tier Test")
        self.assertEqual(self.toolbar_tier(toolbar), "advanced")

    def test_toolbar_menu_groups_and_recovery_action_structure(self):
        shared_label = QtGui.QApplication.translate("MainWindow", "Shared Toolbars")
        workbench_label = QtGui.QApplication.translate("MainWindow", "Workbench Toolbars")
        contextual_label = QtGui.QApplication.translate("MainWindow", "Contextual Toolbars")
        show_recommended_label = QtGui.QApplication.translate(
            "MainWindow", "Show Recommended Toolbars Only"
        )
        toolbar_layout_label = QtGui.QApplication.translate("MainWindow", "Toolbar Layout")
        auto_arrange_label = QtGui.QApplication.translate(
            "MainWindow", "Auto-arrange Toolbar Layout"
        )
        removed_labels = (
            "Reset Current Workbench Layout",
            "Reset Current Contextual Layout",
            "Reset To Recommended Workbench Layout",
            "Reset To Recommended Contextual Layout",
        )

        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        sketcher_toolbar_label = self.toolbar_menu_label(
            self.wait_for_toolbar("wb:SketcherWorkbench:Sketcher")
        )
        clipboard_toolbar_label = self.toolbar_menu_label(self.wait_for_toolbar("shared:Clipboard"))
        macro_toolbar_label = self.toolbar_menu_label(self.wait_for_toolbar("shared:Macro"))

        toolbar_menu = self.toolbar_menu()
        sections, texts = self.capture_popup_menu(toolbar_menu)
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
        self.assertIn(show_recommended_label, texts)
        self.assertIn(toolbar_layout_label, texts)
        self.assertIn(auto_arrange_label, texts)
        root_actions = self.menu_action_texts(toolbar_menu)
        self.assertTrue(
            any(
                self.normalized_action_text(action) == show_recommended_label
                for action in toolbar_menu.actions()
            ),
            "Recommended visibility should remain a primary toolbar-menu action",
        )
        layout_action = self.find_menu_action(toolbar_menu, toolbar_layout_label)
        self.assertIsNotNone(layout_action)
        self.assertIsNotNone(layout_action.menu())
        auto_arrange_action = self.find_menu_action(toolbar_menu, auto_arrange_label)
        self.assertIsNotNone(auto_arrange_action)
        self.assertEqual(
            auto_arrange_action.parent(),
            layout_action.menu(),
            "Auto-arrange should be nested under Toolbar Layout",
        )
        for removed_label in removed_labels:
            self.assertNotIn(removed_label, root_actions)
        self.assertFalse(
            any(
                self.normalized_action_text(action) == auto_arrange_label
                for action in toolbar_menu.actions()
            ),
            "Auto-arrange should not be a primary toolbar-menu action",
        )

        self.enter_sketch_edit()
        contextual_toolbar_label = self.toolbar_menu_label(
            self.wait_for_toolbar("ctx:SketcherWorkbench:edit:Geometries")
        )
        toolbar_menu = self.toolbar_menu()
        sections, texts = self.capture_popup_menu(toolbar_menu)
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
        self.assertIn(show_recommended_label, texts)
        self.assertIn(toolbar_layout_label, texts)
        self.assertIn(auto_arrange_label, texts)
        for removed_label in removed_labels:
            self.assertNotIn(removed_label, texts)

        self.leave_sketch_edit()
        _, texts = self.capture_status_bar_context_menu()
        self.assertIn(show_recommended_label, texts)
        self.assertIn(toolbar_layout_label, texts)
        self.assertIn(auto_arrange_label, texts)
        for removed_label in removed_labels:
            self.assertNotIn(removed_label, texts)

        self.enter_sketch_edit()
        _, texts = self.capture_status_bar_context_menu()
        self.assertIn(show_recommended_label, texts)
        self.assertIn(toolbar_layout_label, texts)
        self.assertIn(auto_arrange_label, texts)
        for removed_label in removed_labels:
            self.assertNotIn(removed_label, texts)
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

    def test_auto_arrange_layout_breaks_rows_to_fit_narrow_window(self):
        layout_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/WorkbenchLayouts")
        self.backup_group(
            layout_params,
            "PartDesignWorkbench",
            "__ToolbarAutoArrangeRowsBackup__PartDesignWorkbench",
        )
        layout_params.RemGroup("PartDesignWorkbench")

        self.activate_workbench("PartDesignWorkbench", "wb:PartDesignWorkbench:")
        scoped_toolbars = self.toolbars_for_prefix("wb:PartDesignWorkbench:")
        self.assertGreaterEqual(
            len(scoped_toolbars),
            2,
            "PartDesign should expose multiple toolbars",
        )
        selected = sorted(
            scoped_toolbars,
            key=lambda toolbar: toolbar.sizeHint().width(),
            reverse=True,
        )[:2]
        visibility_group = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/Toolbars")
        for toolbar in scoped_toolbars:
            key = self.record_toolbar_state(toolbar, "PartDesignWorkbench")
            self.backup_bool_param(visibility_group, key)
            if toolbar in selected:
                self.show_toolbar(key)
            else:
                self.hide_toolbar(key)

        shared_toolbar = self.wait_for_toolbar("shared:View")
        shared_key = self.record_toolbar_state(shared_toolbar, "PartDesignWorkbench")
        self.backup_bool_param(visibility_group, shared_key)
        self.backup_group(
            FreeCAD.ParamGet("User parameter:BaseApp/MainWindow"),
            "SharedToolBarLayout",
            "__ToolbarAutoArrangeRowsBackup__SharedToolBarLayout",
        )
        self.move_toolbar(shared_key, QtCore.Qt.RightToolBarArea)
        self.hide_toolbar(shared_key)
        shared_layout_before = self.layout_group_signature(
            FreeCAD.ParamGet("User parameter:BaseApp/MainWindow"), "SharedToolBarLayout"
        )

        original_size = self.main_window().size()
        self.addCleanup(self.main_window().resize, original_size)
        original_minimum_width = self.main_window().minimumWidth()
        self.addCleanup(self.main_window().setMinimumWidth, original_minimum_width)
        self.main_window().setMinimumWidth(1)
        self.main_window().resize(300, original_size.height())
        self.pump(250)

        self.trigger_menu_action(
            self.toolbar_menu(),
            QtGui.QApplication.translate("MainWindow", "Auto-arrange Toolbar Layout"),
        )

        self.assertTrue(
            any(
                self.main_window().toolBarBreak(toolbar)
                for toolbar in selected
                if toolbar.isVisible()
            ),
            "Auto-arrange should break scoped toolbars into usable rows on a narrow window",
        )
        self.assertGreater(
            len({toolbar.geometry().y() for toolbar in selected if toolbar.isVisible()}),
            1,
            "Visible scoped toolbars should occupy more than one row at this width",
        )
        self.assert_toolbar_area(shared_key, QtCore.Qt.RightToolBarArea)
        self.assertFalse(shared_toolbar.isVisible())
        self.assertEqual(
            self.layout_group_signature(
                FreeCAD.ParamGet("User parameter:BaseApp/MainWindow"), "SharedToolBarLayout"
            ),
            shared_layout_before,
            "Auto-arrange must leave the shared layout unchanged on narrow windows",
        )

    def test_show_recommended_only_preserves_layout(self):
        show_recommended_only_label = QtGui.QApplication.translate(
            "MainWindow", "Show Recommended Toolbars Only"
        )

        visibility_group = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/Toolbars")
        for key in (
            "shared:View",
            "shared:Clipboard",
            "shared:Macro",
            "wb:SketcherWorkbench:Sketcher",
            "ctx:SketcherWorkbench:edit:Geometries",
        ):
            self.backup_bool_param(visibility_group, key)

        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")

        view_toolbar = self.wait_for_toolbar("shared:View")
        self.record_toolbar_state(view_toolbar, "SketcherWorkbench")
        self.move_toolbar("shared:View", QtCore.Qt.RightToolBarArea)
        workbench_key = self.record_toolbar_state(
            self.wait_for_toolbar("wb:SketcherWorkbench:Sketcher"), "SketcherWorkbench"
        )
        self.move_toolbar(workbench_key, QtCore.Qt.LeftToolBarArea)

        for key in ("shared:Clipboard", "shared:Macro"):
            self.record_toolbar_state(self.wait_for_toolbar(key), "SketcherWorkbench")

        self.show_toolbar("shared:Clipboard")
        self.show_toolbar("shared:Macro")
        self.hide_toolbar("shared:View")
        self.hide_toolbar(workbench_key)
        main_window_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow")
        workbench_layouts = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/WorkbenchLayouts")
        shared_layout_before = self.layout_group_signature(
            main_window_params, "SharedToolBarLayout"
        )
        workbench_layout_before = self.layout_group_signature(
            workbench_layouts, "SketcherWorkbench"
        )

        self.trigger_menu_action(self.toolbar_menu(), show_recommended_only_label)
        self.assert_toolbar_visibility("shared:View", True)
        self.assert_toolbar_visibility("shared:Clipboard", False)
        self.assert_toolbar_visibility("shared:Macro", False)
        self.assert_toolbar_area("shared:View", QtCore.Qt.RightToolBarArea)
        self.assert_toolbar_visibility(workbench_key, True)
        self.assert_toolbar_area(workbench_key, QtCore.Qt.LeftToolBarArea)
        self.assertEqual(
            self.layout_group_signature(main_window_params, "SharedToolBarLayout"),
            shared_layout_before,
        )
        self.assertEqual(
            self.layout_group_signature(workbench_layouts, "SketcherWorkbench"),
            workbench_layout_before,
        )

        self.enter_sketch_edit()
        contextual_key = "ctx:SketcherWorkbench:edit:Geometries"
        self.record_toolbar_state(
            self.wait_for_toolbar(contextual_key),
            "SketcherWorkbench",
            context="edit",
        )
        self.move_toolbar(contextual_key, QtCore.Qt.LeftToolBarArea)
        self.hide_toolbar(contextual_key)
        contextual_layout_before = self.layout_group_signature(
            workbench_layouts, "ctx:SketcherWorkbench:edit"
        )
        shared_context_layout_before = self.layout_group_signature(
            main_window_params, "SharedToolBarLayout"
        )

        self.trigger_menu_action(self.toolbar_menu(), show_recommended_only_label)
        self.assert_toolbar_visibility(contextual_key, True)
        self.assert_toolbar_area(contextual_key, QtCore.Qt.LeftToolBarArea)
        self.assert_toolbar_area(workbench_key, QtCore.Qt.LeftToolBarArea)
        self.assertEqual(
            self.layout_group_signature(main_window_params, "SharedToolBarLayout"),
            shared_context_layout_before,
        )
        self.assertEqual(
            self.layout_group_signature(workbench_layouts, "ctx:SketcherWorkbench:edit"),
            contextual_layout_before,
        )
        self.leave_sketch_edit()

    def test_legacy_toolbar_names_restore_with_scoped_keys(self):
        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        toolbar = self.wait_for_toolbar("wb:SketcherWorkbench:Sketcher")
        key = self.record_toolbar_state(toolbar, "SketcherWorkbench")
        legacy_name = str(toolbar.objectName())
        self.assertNotEqual(key, legacy_name, "Test requires a scoped toolbar persistence key")

        self.activate_workbench("PartWorkbench", "wb:PartWorkbench:")

        visibility_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/Toolbars")
        self.backup_bool_param(visibility_params, key)
        self.backup_bool_param(visibility_params, legacy_name)
        visibility_params.RemBool(key)
        visibility_params.SetBool(legacy_name, False)

        self.activate_workbench("SketcherWorkbench", "wb:SketcherWorkbench:")
        self.assert_toolbar_visibility(key, False)
