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


class TestToolbarBreakRestoreGui(unittest.TestCase):
    def setUp(self):
        if not is_gui_available():
            self.skipTest("GUI not available")

        self._toolbar_states = {}
        self._ensure_workbench("SketcherWorkbench")
        self._set_per_workbench_layout_preference(True)

        layout_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/WorkbenchLayouts")
        self._backup_group(layout_params, "SketcherWorkbench")
        self._backup_group(layout_params, "ctx:SketcherWorkbench:edit")
        layout_params.RemGroup("SketcherWorkbench")
        layout_params.RemGroup("ctx:SketcherWorkbench:edit")

        toolbar_params = FreeCAD.ParamGet("User parameter:BaseApp/MainWindow/Toolbars")
        self._backup_bool_param(toolbar_params, "shared:View")
        self._backup_bool_param(toolbar_params, "ctx:SketcherWorkbench:edit:Constraints")

        self.doc = FreeCAD.newDocument("TestToolbarBreakRestoreGui")
        FreeCADGui.activateView("Gui::View3DInventor", True)
        self.pump(200)

        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.doc.recompute()
        self.pump(200)

    def tearDown(self):
        if not is_gui_available():
            return

        try:
            self._restore_toolbar_states()
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

    def _ensure_workbench(self, name):
        if name not in FreeCADGui.listWorkbenches():
            self.skipTest(f"Required workbench is unavailable: {name}")

    def _set_per_workbench_layout_preference(self, enabled):
        params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/MainWindow")
        existed = "RememberToolbarLayoutByWorkbench" in {str(name) for name in params.GetBools()}
        value = params.GetBool("RememberToolbarLayoutByWorkbench", False)

        def restore():
            if existed:
                params.SetBool("RememberToolbarLayoutByWorkbench", value)
            else:
                params.RemBool("RememberToolbarLayoutByWorkbench")

        self.addCleanup(restore)
        params.SetBool("RememberToolbarLayoutByWorkbench", enabled)

    def _backup_bool_param(self, params, key):
        existing = key in {str(name) for name in params.GetBools()}
        value = params.GetBool(key) if existing else False

        def restore():
            if existing:
                params.SetBool(key, value)
            else:
                params.RemBool(key)

        self.addCleanup(restore)

    def _backup_group(self, params, group_name):
        backup_name = f"__ToolbarBreakRestoreBackup__{group_name.replace(':', '_')}"
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

    def pump(self, timeout_ms=120):
        loop = QtCore.QEventLoop()
        QtCore.QTimer.singleShot(timeout_ms, loop.quit)
        loop.exec_()

    def wait_until(self, predicate, description, timeout_ms=6000, step_ms=120):
        remaining = timeout_ms
        while remaining > 0:
            if predicate():
                return
            self.pump(step_ms)
            remaining -= step_ms

        if predicate():
            return

        self.fail(f"Timed out waiting for {description}")

    def main_window(self):
        return FreeCADGui.getMainWindow()

    def toolbar_key(self, toolbar):
        key = toolbar.property("PersistenceKey")
        if key:
            return str(key)
        return str(toolbar.objectName())

    def all_toolbars(self):
        return list(self.main_window().findChildren(QtGui.QToolBar))

    def toolbar_by_key(self, key):
        for toolbar in self.all_toolbars():
            if self.toolbar_key(toolbar) == key:
                return toolbar
        return None

    def wait_for_toolbar(self, key):
        self.wait_until(lambda: self.toolbar_by_key(key) is not None, f"toolbar {key}")
        return self.toolbar_by_key(key)

    def activate_workbench(self, name):
        FreeCADGui.activateWorkbench(name)
        self.pump(350)

    def contextual_toolbar_action_is_visible(self):
        toolbar = self.toolbar_by_key("ctx:SketcherWorkbench:edit:Constraints")
        return toolbar is not None and toolbar.toggleViewAction().isVisible()

    def enter_sketch_edit(self):
        self.activate_workbench("SketcherWorkbench")
        ok = FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)
        self.assertTrue(ok, "Failed to enter Sketcher edit mode")
        self.wait_until(
            self.contextual_toolbar_action_is_visible,
            "Sketcher contextual toolbar to become active",
            timeout_ms=8000,
        )
        self.pump(250)

    def leave_sketch_edit(self):
        gui_doc = FreeCADGui.ActiveDocument
        if gui_doc is None:
            return
        gui_doc.resetEdit()
        self.wait_until(
            lambda: not self.contextual_toolbar_action_is_visible(),
            "Sketcher contextual toolbar to hide",
            timeout_ms=8000,
        )
        self.pump(250)

    def record_toolbar_state(self, toolbar):
        key = self.toolbar_key(toolbar)
        if key in self._toolbar_states:
            return

        self._toolbar_states[key] = {
            "area": self.main_window().toolBarArea(toolbar),
            "break": self.main_window().toolBarBreak(toolbar),
            "visible": toolbar.isVisible(),
        }

    def _restore_toolbar_states(self):
        for key, state in self._toolbar_states.items():
            toolbar = self.toolbar_by_key(key)
            if toolbar is None:
                continue

            toolbar.show()
            self.main_window().addToolBar(state["area"], toolbar)
            if state["break"]:
                self.main_window().insertToolBarBreak(toolbar)
            else:
                self.main_window().removeToolBarBreak(toolbar)
            toolbar.setVisible(state["visible"])
            self.pump(100)

    def show_toolbar(self, toolbar):
        toolbar.show()
        self.pump(200)
        self.assertTrue(
            toolbar.isVisible(),
            f"Toolbar {self.toolbar_key(toolbar)} should be visible",
        )

    def test_contextual_restore_removes_stale_toolbar_break(self):
        self.activate_workbench("SketcherWorkbench")
        view_toolbar = self.wait_for_toolbar("shared:View")
        self.record_toolbar_state(view_toolbar)

        self.enter_sketch_edit()
        contextual_toolbar = self.wait_for_toolbar("ctx:SketcherWorkbench:edit:Constraints")
        self.record_toolbar_state(contextual_toolbar)

        self.show_toolbar(view_toolbar)
        self.show_toolbar(contextual_toolbar)
        self.main_window().addToolBar(QtCore.Qt.TopToolBarArea, view_toolbar)
        self.main_window().addToolBar(QtCore.Qt.TopToolBarArea, contextual_toolbar)
        self.main_window().removeToolBarBreak(contextual_toolbar)
        self.pump(250)
        self.assertFalse(self.main_window().toolBarBreak(contextual_toolbar))

        self.leave_sketch_edit()
        self.main_window().insertToolBarBreak(contextual_toolbar)
        self.pump(250)
        self.assertTrue(self.main_window().toolBarBreak(contextual_toolbar))

        self.enter_sketch_edit()
        self.assertFalse(self.main_window().toolBarBreak(contextual_toolbar))
        self.leave_sketch_edit()
