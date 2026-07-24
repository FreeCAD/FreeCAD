# SPDX-License-Identifier: LGPL-2.1-or-later
"""**************************************************************************
*                                                                          *
*   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
*                                                                          *
*   This file is part of FreeCAD.                                          *
*                                                                          *
*   FreeCAD is free software: you can redistribute it and/or modify it     *
*   under the terms of the GNU Lesser General Public License as            *
*   published by the Free Software Foundation, either version 2.1 of the   *
*   License, or (at your option) any later version.                        *
*                                                                          *
*   FreeCAD is distributed in the hope that it will be useful, but         *
*   WITHOUT ANY WARRANTY; without even the implied warranty of             *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
*   Lesser General Public License for more details.                        *
*                                                                          *
*   You should have received a copy of the GNU Lesser General Public       *
*   License along with FreeCAD. If not, see                                *
*   <https://www.gnu.org/licenses/>.                                       *
*                                                                          *
***************************************************************************/"""

import os
import tempfile
import threading
import time
import unittest
import zipfile

import FreeCAD
import FreeCADGui
from PySide6 import QtCore, QtWidgets

# ---------------------------------------------------------------------------
# define the functions to test the FreeCAD Gui Document code
# ---------------------------------------------------------------------------


class TestGuiDocument(unittest.TestCase):
    def setUp(self):
        # Create a new document
        self.doc = FreeCAD.newDocument("TestDoc")

    def tearDown(self):
        # Close the document
        if self.doc is not None and self.doc.Name in FreeCAD.listDocuments():
            FreeCAD.closeDocument(self.doc.Name)

    def _findAutoSaver(self):
        app = QtWidgets.QApplication.instance()
        self.assertIsNotNone(app)

        for child in app.children():
            meta_object = child.metaObject() if hasattr(child, "metaObject") else None
            if meta_object and meta_object.className() == "Gui::AutoSaver":
                return child

        raise self.failureException("Gui::AutoSaver was not found in the QApplication object tree")

    def _recoveryArchive(self):
        return os.path.join(self.doc.TransientDir, "fc_recovery_file.fcstd")

    def _removeRecoveryArchive(self):
        archive = self._recoveryArchive()
        if os.path.exists(archive):
            os.remove(archive)

    def _invokeAutoSaverFlush(self):
        invoked = QtCore.QMetaObject.invokeMethod(
            self._findAutoSaver(),
            "flushPendingSave",
            QtCore.Qt.ConnectionType.DirectConnection,
            QtCore.Q_ARG(str, self.doc.Name),
        )
        self.assertTrue(invoked)

    def _processEventsUntil(self, predicate, timeout=3.0):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            QtWidgets.QApplication.processEvents(QtCore.QEventLoop.ProcessEventsFlag.AllEvents, 50)
            if predicate():
                return True
            time.sleep(0.01)

        return predicate()

    def _assertRecoveryArchiveContains(self, expected_label=None):
        archive = self._recoveryArchive()
        self.assertTrue(os.path.isfile(archive))

        with zipfile.ZipFile(archive) as recovery:
            self.assertIn("Document.xml", recovery.namelist())
            self.assertIn("GuiDocument.xml", recovery.namelist())

            if expected_label is not None:
                document_xml = recovery.read("Document.xml").decode("utf-8", errors="replace")
                self.assertIn(expected_label, document_xml)

    def _writeArchiveWithoutGuiDocument(self, source_path, target_path):
        with zipfile.ZipFile(source_path, "r") as source:
            self.assertIn("Document.xml", source.namelist())
            self.assertIn("GuiDocument.xml", source.namelist())

            with zipfile.ZipFile(target_path, "w") as target:
                for item in source.infolist():
                    if item.filename == "GuiDocument.xml":
                        continue

                    target.writestr(item, source.read(item.filename))

        with zipfile.ZipFile(target_path, "r") as target:
            self.assertIn("Document.xml", target.namelist())
            self.assertNotIn("GuiDocument.xml", target.namelist())

    def testGetTreeRootObject(self):
        # Create objects at the root level
        group1 = self.doc.addObject("App::DocumentObjectGroup", "Group1")
        group2 = self.doc.addObject("App::DocumentObjectGroup", "Group2")
        obj1 = self.doc.addObject("App::FeaturePython", "RootObject1")
        part1 = self.doc.addObject("App::Part", "Part1")

        # Create App::Parts and groups with objects in them
        part1_obj = part1.newObject("App::FeaturePython", "Part1_Object")
        group3 = group2.newObject("App::DocumentObjectGroup", "Group1")
        group1_obj = group3.newObject("App::FeaturePython", "Group1_Object")

        # Fetch the root objects using getTreeRootObjects
        root_objects = FreeCADGui.getDocument("TestDoc").TreeRootObjects

        # Check if the new function returns the correct root objects
        expected_root_objects = [group1, group2, obj1, part1]
        self.assertEqual(set(root_objects), set(expected_root_objects))

    def testIssue30418(self):
        class ViewProvider:
            def __init__(self, vobj):
                vobj.Proxy = self

            def attach(self, vobj):
                self.ViewObject = vobj
                self.Object = vobj.Object

            def setEdit(self, vobj, mode):
                return True

            def unsetEdit(self, vobj, mode):
                obj = vobj.Object
                doc = obj.Document
                doc.removeObject(obj.Name)
                return True

        gui = FreeCADGui.getDocument(self.doc)

        self.doc.openTransaction("Add object")
        obj = self.doc.addObject("App::FeaturePython", "Object")
        ViewProvider(obj.ViewObject)
        self.doc.commitTransaction()
        gui.setEdit(obj, 0)
        self.doc.undo()

        self.assertTrue(True)

    def testViewObjectRequiresMainThread(self):
        obj = self.doc.addObject("App::FeaturePython", "ThreadGuard")

        result = {}

        def access_view_object():
            try:
                _ = obj.ViewObject
            except Exception as exc:  # pragma: no cover - exercised through assertion below
                result["exception"] = exc

        worker = threading.Thread(target=access_view_object)
        worker.start()
        worker.join()

        self.assertIn("exception", result)
        self.assertIsInstance(result["exception"], RuntimeError)
        self.assertIn("main thread", str(result["exception"]).lower())

    def testRefreshFallsBackToSyncForFeaturePython(self):
        params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Document")
        old_async = params.GetBool("EnableAsyncRecompute", True)

        class RefreshProxy:
            def __init__(self):
                self.executed_thread_id = None

            def execute(self, obj):
                self.executed_thread_id = threading.get_ident()
                time.sleep(0.05)

        proxy = RefreshProxy()
        obj = self.doc.addObject("App::FeaturePython", "PythonFeature")
        obj.Proxy = proxy
        obj.touch()

        try:
            params.SetBool("EnableAsyncRecompute", True)

            start = time.monotonic()
            FreeCADGui.runCommand("Std_Refresh", 0)
            elapsed = time.monotonic() - start
        finally:
            params.SetBool("EnableAsyncRecompute", old_async)

        self.assertEqual(proxy.executed_thread_id, threading.get_ident())
        self.assertGreaterEqual(elapsed, 0.04)

    def testRecoverySnapshotIncludesGuiDocument(self):
        self.doc.addObject("App::FeaturePython", "RecoveryGuiObject")

        self.assertTrue(FreeCAD.writeRecoverySnapshotToTransientDir(self.doc))

        self._assertRecoveryArchiveContains()

    def testRestoreWithoutGuiDocumentFitsViewAndRestoresVisibility(self):
        try:
            from pivy import coin  # noqa: F401
        except ImportError as exc:
            raise unittest.SkipTest(f"Coin bindings are unavailable: {exc}") from exc

        try:
            import Part  # noqa: F401
        except ImportError as exc:
            raise unittest.SkipTest(f"Part workbench objects are unavailable: {exc}") from exc

        visible_box = self.doc.addObject("Part::Box", "VisibleBox")
        hidden_cylinder = self.doc.addObject("Part::Cylinder", "HiddenCylinder")

        visible_box.Length = 25
        visible_box.Width = 20
        visible_box.Height = 15
        visible_box.Placement.Base = FreeCAD.Vector(80, 35, 45)
        visible_box.Visibility = True
        visible_box.ViewObject.Visibility = True

        hidden_cylinder.Radius = 5
        hidden_cylinder.Height = 20
        hidden_cylinder.Placement.Base = FreeCAD.Vector(120, 50, 55)
        hidden_cylinder.Visibility = False
        hidden_cylinder.ViewObject.Visibility = False

        self.doc.recompute()

        with tempfile.TemporaryDirectory() as temp_dir:
            source_path = os.path.join(temp_dir, "with_gui_document.FCStd")
            target_path = os.path.join(temp_dir, "without_gui_document.FCStd")

            self.doc.saveAs(source_path)
            self._writeArchiveWithoutGuiDocument(source_path, target_path)

            FreeCAD.closeDocument(self.doc.Name)
            self.doc = None

            restored_doc = FreeCAD.openDocument(target_path)
            self.doc = restored_doc
            try:
                FreeCAD.setActiveDocument(restored_doc.Name)

                view = FreeCADGui.getDocument(restored_doc.Name).ActiveView
                self.assertIsNotNone(view)

                self.assertTrue(restored_doc.VisibleBox.ViewObject.Visibility)
                self.assertFalse(restored_doc.HiddenCylinder.Visibility)
                self.assertFalse(restored_doc.HiddenCylinder.ViewObject.Visibility)

                def cameraIsFitted():
                    position = view.getCameraNode().position.getValue().getValue()
                    return sum(abs(component) for component in position) > 10.0

                self.assertTrue(self._processEventsUntil(cameraIsFitted), view.getCamera())
            finally:
                if self.doc is not None and self.doc.Name in FreeCAD.listDocuments():
                    FreeCAD.closeDocument(self.doc.Name)
                self.doc = None

    def testAutoSaverFlushWritesRecoverySnapshot(self):
        obj = self.doc.addObject("App::FeaturePython", "AutoSaveGuiObject")
        obj.Label = "AutoSaveImmediate"
        self._removeRecoveryArchive()

        self._invokeAutoSaverFlush()

        self._assertRecoveryArchiveContains(expected_label="AutoSaveImmediate")

    def testAutoSaverRetriesWhenDocumentBecomesStable(self):
        obj = self.doc.addObject("App::FeaturePython", "AutoSaveBlockedObject")
        self._removeRecoveryArchive()

        self.doc.openTransaction("AutoSaveBlocked")
        obj.Label = "AutoSaveRetried"
        self._invokeAutoSaverFlush()

        self.assertFalse(os.path.exists(self._recoveryArchive()))

        self.doc.abortTransaction()

        self.assertTrue(self._processEventsUntil(lambda: os.path.exists(self._recoveryArchive())))
        self._assertRecoveryArchiveContains()

    def testAutoSaverStableSignalDoesNotBypassTimeout(self):
        obj = self.doc.addObject("App::FeaturePython", "AutoSaveStableObject")
        self._removeRecoveryArchive()

        self.doc.openTransaction("AutoSaveStable")
        obj.Label = "AutoSaveStillPending"
        self.doc.commitTransaction()

        QtWidgets.QApplication.processEvents(QtCore.QEventLoop.ProcessEventsFlag.AllEvents, 50)
        self.assertFalse(os.path.exists(self._recoveryArchive()))

    def testAutoSaverCoalescesBlockedFlushesToLatestCommittedState(self):
        obj = self.doc.addObject("App::FeaturePython", "AutoSaveChurnObject")
        self._removeRecoveryArchive()

        self.doc.openTransaction("AutoSaveChurn")
        for index in range(8):
            obj.Label = f"AutoSaveBurst{index}"
            self._invokeAutoSaverFlush()

        self.assertFalse(os.path.exists(self._recoveryArchive()))

        self.doc.commitTransaction()

        self.assertTrue(self._processEventsUntil(lambda: os.path.exists(self._recoveryArchive())))
        self._assertRecoveryArchiveContains(expected_label="AutoSaveBurst7")
