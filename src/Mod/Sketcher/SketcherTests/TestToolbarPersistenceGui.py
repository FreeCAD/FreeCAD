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

    def toolbar_key(self, toolbar):
        key = toolbar.property("PersistenceKey")
        if key:
            return str(key)
        return str(toolbar.objectName())

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
