# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2026 OpenAI                                             *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# **************************************************************************

"""GUI regression tests for BIM reload ownership helpers."""

import importlib
from pathlib import Path
import sys
import tempfile

import FreeCAD
import FreeCADGui

from bimtests.TestArchBaseGui import TestArchBaseGui

WORKBENCH_NAME = "BIMWorkbench"
APP_RUNTIME_NAME = "bim_reload_gui_test_app"
SESSION_RUNTIME_NAME = "reload_gui_test"


class _DummyWorkbenchManipulator:
    def modifyMenuBar(self):
        return []

    def modifyToolBars(self):
        return []


class _DocumentObserver:
    def __init__(self):
        self.created_documents = []

    def slotCreatedDocument(self, doc):
        self.created_documents.append(doc.Name)


class _SelectionObserver:
    def __init__(self):
        self.events = []

    def addSelection(self, document, obj, element, position):
        self.events.append((document, obj, element))


class _DummyGuiCommand:
    def __init__(self, menu_text):
        self.menu_text = menu_text

    def GetResources(self):
        return {
            "MenuText": self.menu_text,
            "ToolTip": self.menu_text,
            "Pixmap": "",
        }

    def Activated(self):
        return None


class TestArchReloadGui(TestArchBaseGui):
    def setUp(self):
        super().setUp()
        self.extra_documents = []
        self._activate_bim_workbench()

    def tearDown(self):
        try:
            FreeCADGui.Selection.clearSelection()
            FreeCADGui.stopPythonWorkbenchAutoReload(WORKBENCH_NAME)
            FreeCADGui.disposeSessionRuntime(
                SESSION_RUNTIME_NAME,
                workbench_name=WORKBENCH_NAME,
            )
            FreeCAD.disposeAppRuntime(APP_RUNTIME_NAME)
            for doc_name in reversed(self.extra_documents):
                try:
                    FreeCAD.closeDocument(doc_name)
                except Exception:
                    pass
        finally:
            super().tearDown()

    def _activate_bim_workbench(self):
        FreeCADGui.activateWorkbench(WORKBENCH_NAME)
        self.pump_gui_events()

    def _new_aux_document(self, suffix):
        doc = FreeCAD.newDocument(f"{self.doc_name}_{suffix}")
        self.extra_documents.append(doc.Name)
        self.pump_gui_events()
        return doc

    def _close_aux_document(self, doc):
        if doc is None:
            return
        doc_name = doc.Name
        try:
            FreeCAD.closeDocument(doc_name)
        finally:
            if doc_name in self.extra_documents:
                self.extra_documents.remove(doc_name)
        self.pump_gui_events()

    def _install_reload_resources(self, workbench_cleanup, session_cleanup):
        runtime = FreeCADGui.workbenchRuntime(WORKBENCH_NAME)
        runtime.addWorkbenchManipulator(
            _DummyWorkbenchManipulator(),
            key="reload_gui_test_manipulator",
        )
        runtime.own(
            "reload_gui_test_cleanup",
            object(),
            lambda: workbench_cleanup.append(runtime.generation),
        )

        session = FreeCADGui.sessionRuntime(
            SESSION_RUNTIME_NAME,
            workbench_name=WORKBENCH_NAME,
        )
        observer = _SelectionObserver()
        session.addSelectionObserver(observer, key="selection_observer")
        session.own(
            "reload_gui_test_cleanup",
            object(),
            lambda: session_cleanup.append("disposed"),
        )
        return runtime, session, observer

    def test_python_command_reregistration_replaces_existing_command(self):
        command_name = f"TestArchReloadGui_{self._testMethodName}"

        FreeCADGui.addCommand(command_name, _DummyGuiCommand("Initial label"))
        initial_info = FreeCADGui.Command.get(command_name).getInfo()
        self.assertEqual(initial_info["menuText"], "Initial label")

        FreeCADGui.addCommand(command_name, _DummyGuiCommand("Updated label"))
        updated_info = FreeCADGui.Command.get(command_name).getInfo()
        self.assertEqual(updated_info["menuText"], "Updated label")

    def test_generic_active_workbench_reload_commands_are_available(self):
        self.assertTrue(FreeCADGui.canReloadPythonWorkbench(WORKBENCH_NAME))
        self.assertTrue(FreeCADGui.isCommandActive("Std_ReloadActivePythonWorkbench"))
        self.assertTrue(FreeCADGui.isCommandActive("Std_ToggleActivePythonWorkbenchAutoReload"))

        reload_info = FreeCADGui.Command.get("Std_ReloadActivePythonWorkbench").getInfo()
        toggle_info = FreeCADGui.Command.get("Std_ToggleActivePythonWorkbenchAutoReload").getInfo()
        self.assertEqual(reload_info["menuText"], "Reload Active Python Workbench")
        self.assertEqual(
            toggle_info["menuText"],
            "Toggle Active Python Workbench Auto Reload",
        )

    def test_auto_reload_accepts_workbench_aliases(self):
        alias = getattr(FreeCADGui.getWorkbench(WORKBENCH_NAME), "MenuText", None)
        if not alias or alias == WORKBENCH_NAME:
            self.skipTest("Workbench does not expose a distinct alias")

        FreeCADGui.startPythonWorkbenchAutoReload(alias, debounce_ms=50)
        self.assertTrue(FreeCADGui.isPythonWorkbenchAutoReloadActive(alias))
        self.assertTrue(FreeCADGui.isPythonWorkbenchAutoReloadActive(WORKBENCH_NAME))
        self.assertTrue(FreeCADGui.stopPythonWorkbenchAutoReload(alias))
        self.assertFalse(FreeCADGui.isPythonWorkbenchAutoReloadActive(WORKBENCH_NAME))

    def test_reload_cleans_cyclic_selection_view_callbacks(self):
        import BimSelect

        runtime = FreeCADGui.workbenchRuntime(WORKBENCH_NAME)
        setup = runtime.getOwned(BimSelect.SETUP_RUNTIME_KEY)
        self.assertIsNotNone(setup)

        active_document = getattr(FreeCADGui, "ActiveDocument", None)
        self.assertIsNotNone(active_document)
        callback_count_before = setup.activeCallbackCount()
        setup.slotActivateDocument(active_document)
        callback_count_after_first_attach = setup.activeCallbackCount()
        self.assertGreaterEqual(callback_count_after_first_attach, callback_count_before)
        self.assertLessEqual(callback_count_after_first_attach, callback_count_before + 2)

        setup.slotActivateDocument(active_document)
        self.assertEqual(setup.activeCallbackCount(), callback_count_after_first_attach)

        FreeCADGui.reloadPythonWorkbench(WORKBENCH_NAME)
        self.pump_gui_events()

        self.assertEqual(setup.activeCallbackCount(), 0)

    def test_reload_rebinds_status_widget_controls(self):
        from PySide import QtGui

        statuswidget = (
            FreeCADGui.getMainWindow()
            .statusBar()
            .findChild(
                QtGui.QToolBar,
                "BIMStatusWidget",
            )
        )
        self.assertIsNotNone(statuswidget)
        old_lock_button = getattr(statuswidget, "lock_button", None)
        old_propertybuttons = getattr(statuswidget, "propertybuttons", None)
        self.assertIsNotNone(old_lock_button)
        self.assertIsNotNone(old_propertybuttons)

        FreeCADGui.reloadPythonWorkbench(WORKBENCH_NAME)
        self.pump_gui_events()

        statuswidget = (
            FreeCADGui.getMainWindow()
            .statusBar()
            .findChild(
                QtGui.QToolBar,
                "BIMStatusWidget",
            )
        )
        self.assertIsNotNone(statuswidget)
        self.assertIsNot(old_lock_button, getattr(statuswidget, "lock_button", None))
        self.assertIsNot(
            old_propertybuttons,
            getattr(statuswidget, "propertybuttons", None),
        )

    def test_reload_refreshes_nativeifc_document_observer(self):
        fake_ifcopenshell = None
        imported_ifcopenshell = None
        runtime = FreeCADGui.workbenchRuntime(WORKBENCH_NAME)

        try:
            with tempfile.TemporaryDirectory() as tmpdir:
                fake_ifcopenshell = Path(tmpdir) / "ifcopenshell.py"
                fake_ifcopenshell.write_text("version = '0.0'\n", encoding="utf-8")
                sys.path.insert(0, tmpdir)
                importlib.invalidate_caches()

                nativeifc = importlib.import_module("nativeifc")
                nativeifc.invalidate_ifcopenshell_cache()
                ifc_observer = importlib.import_module("nativeifc.ifc_observer")
                ifc_observer.add_observer()

                old_observer = runtime.getOwned(ifc_observer.OBSERVER_KEY)
                self.assertIsNotNone(old_observer)

                FreeCADGui.reloadPythonWorkbench(WORKBENCH_NAME)
                self.pump_gui_events()

                imported_ifcopenshell = importlib.import_module("nativeifc")
                imported_ifcopenshell.invalidate_ifcopenshell_cache()
                reloaded_ifc_observer = importlib.import_module("nativeifc.ifc_observer")
                reloaded_runtime = FreeCADGui.workbenchRuntime(WORKBENCH_NAME)
                new_observer = reloaded_runtime.getOwned(reloaded_ifc_observer.OBSERVER_KEY)
                self.assertIsNotNone(new_observer)
                self.assertIsNot(old_observer, new_observer)

                reloaded_ifc_observer.remove_observer()
        finally:
            if fake_ifcopenshell is not None and str(fake_ifcopenshell.parent) in sys.path:
                sys.path.remove(str(fake_ifcopenshell.parent))
            importlib.invalidate_caches()
            if imported_ifcopenshell is not None:
                imported_ifcopenshell.invalidate_ifcopenshell_cache()

    def test_reload_preserves_runtime_boundaries_without_leaks(self):
        app_cleanup = []
        workbench_cleanup = []
        session_cleanup = []

        app_runtime = FreeCAD.appRuntime(APP_RUNTIME_NAME)
        app_observer = _DocumentObserver()
        app_runtime.addDocumentObserver(app_observer, key="observer")
        app_runtime.own("cleanup", object(), lambda: app_cleanup.append("disposed"))

        aux_doc = self._new_aux_document("AppObserver")
        self.assertIn(aux_doc.Name, app_observer.created_documents)
        self._close_aux_document(aux_doc)
        self._activate_bim_workbench()

        runtime, _session, selection_observer = self._install_reload_resources(
            workbench_cleanup,
            session_cleanup,
        )
        initial_generation = runtime.generation

        box = self.document.addObject("Part::Box", "ReloadRuntimeBox")
        self.document.recompute()
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(box)
        self.pump_gui_events()
        self.assertEqual(len(selection_observer.events), 1)

        for reload_index in range(2):
            FreeCADGui.reloadPythonWorkbench(WORKBENCH_NAME)
            self.pump_gui_events()

            self.assertIsNotNone(FreeCAD.findAppRuntime(APP_RUNTIME_NAME))
            self.assertIsNone(
                FreeCADGui.findSessionRuntime(
                    SESSION_RUNTIME_NAME,
                    workbench_name=WORKBENCH_NAME,
                )
            )
            self.assertEqual(len(workbench_cleanup), reload_index + 1)
            self.assertEqual(len(session_cleanup), reload_index + 1)

            aux_doc = self._new_aux_document(f"Reload{reload_index}")
            self.assertIn(aux_doc.Name, app_observer.created_documents)
            self._close_aux_document(aux_doc)
            self._activate_bim_workbench()

            reloaded_runtime = FreeCADGui.findWorkbenchRuntime(WORKBENCH_NAME)
            self.assertIsNotNone(reloaded_runtime)
            self.assertGreater(reloaded_runtime.generation, initial_generation)
            initial_generation = reloaded_runtime.generation

            if reload_index == 0:
                _runtime, _session, selection_observer = self._install_reload_resources(
                    workbench_cleanup,
                    session_cleanup,
                )
                FreeCADGui.Selection.clearSelection()
                FreeCADGui.Selection.addSelection(box)
                self.pump_gui_events()
                self.assertEqual(len(selection_observer.events), 1)

        created_before_dispose = list(app_observer.created_documents)
        self.assertTrue(FreeCAD.disposeAppRuntime(APP_RUNTIME_NAME))
        self.assertEqual(app_cleanup, ["disposed"])
        self.assertIsNone(FreeCAD.findAppRuntime(APP_RUNTIME_NAME))

        aux_doc = self._new_aux_document("AfterDispose")
        self._close_aux_document(aux_doc)
        self.assertEqual(created_before_dispose, app_observer.created_documents)

    def test_reload_reimports_legacy_top_level_bim_modules(self):
        original_module = importlib.import_module("BimSelect")

        removed = FreeCADGui.reloadPythonWorkbench(WORKBENCH_NAME)
        self.pump_gui_events()

        self.assertIn("BimSelect", removed)
        reloaded_module = importlib.import_module("BimSelect")
        self.assertIsNot(original_module, reloaded_module)

    def test_watch_path_builder_excludes_only_requested_subtrees(self):
        watch_path_builder = FreeCADGui.reloadPythonWorkbench.__globals__[
            "_iter_watch_paths_for_base"
        ]
        excluded_dirs = FreeCADGui.reloadPythonWorkbench.__globals__[
            "_DIR_MOD_AUTO_RELOAD_EXCLUDED_DIRS"
        ]

        with tempfile.TemporaryDirectory() as tmpdir:
            base_path = Path(tmpdir)
            (base_path / "InitGui.py").write_text("# test\n", encoding="utf-8")
            (base_path / "bimcommands").mkdir()
            (base_path / "bimcommands" / "BimReloadWorkbench.py").write_text(
                "# test\n",
                encoding="utf-8",
            )
            (base_path / "Resources").mkdir()
            (base_path / "Resources" / "translations").mkdir()
            (base_path / "Resources" / "translations" / "BIM.ts").write_text(
                "<TS/>\n",
                encoding="utf-8",
            )
            (base_path / "__pycache__").mkdir()
            (base_path / "__pycache__" / "BimSelect.pyc").write_text("", encoding="utf-8")
            (base_path / "build").mkdir()
            (base_path / "build" / "scratch.py").write_text("# test\n", encoding="utf-8")
            (base_path / "icons").mkdir()
            (base_path / "icons" / "BIM.svg").write_text("<svg/>\n", encoding="utf-8")

            watched = set(
                watch_path_builder(
                    base_path,
                    {".py", ".svg"},
                    excluded_dirs,
                )
            )

            self.assertIn(str(base_path.resolve()), watched)
            self.assertIn(str((base_path / "InitGui.py").resolve()), watched)
            self.assertIn(str((base_path / "bimcommands").resolve()), watched)
            self.assertIn(
                str((base_path / "bimcommands" / "BimReloadWorkbench.py").resolve()), watched
            )
            self.assertIn(str((base_path / "Resources").resolve()), watched)
            self.assertIn(str((base_path / "icons" / "BIM.svg").resolve()), watched)
            self.assertNotIn(str((base_path / "Resources" / "translations").resolve()), watched)
            self.assertNotIn(
                str((base_path / "Resources" / "translations" / "BIM.ts").resolve()),
                watched,
            )
            self.assertNotIn(str((base_path / "__pycache__").resolve()), watched)
            self.assertNotIn(str((base_path / "build").resolve()), watched)
            self.assertNotIn(str((base_path / "build" / "scratch.py").resolve()), watched)
