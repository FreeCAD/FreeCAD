# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
Opt-in GUI autosave soak test.

This module is intentionally not registered in FreeCAD.__unit_test__, so it is
only run when requested explicitly.

Examples:
    FreeCAD src/Mod/Test/RunAutoSaverStress.py
    FreeCAD src/Mod/Test/RunAutoSaverStress.py documents=3 objects=150
    FreeCAD src/Mod/Test/RunAutoSaverStress.py iterations=25 burst=10 timeout=5.0
"""

import os
import statistics
import sys
import time
import traceback
import unittest
import zipfile

import FreeCAD
import FreeCADGui
from PySide6 import QtCore, QtWidgets


def _get_key_value_arg(names):
    for arg in sys.argv[2:]:
        for name in names:
            prefix = f"{name}="
            if arg.startswith(prefix):
                return arg[len(prefix) :]
    return None


def _get_int_arg(names, default):
    value = _get_key_value_arg(names)
    if value is None:
        return default
    try:
        return int(value)
    except ValueError as exc:
        raise ValueError(f"Expected integer value for one of {names}") from exc


def _get_float_arg(names, default):
    value = _get_key_value_arg(names)
    if value is None:
        return default
    try:
        return float(value)
    except ValueError as exc:
        raise ValueError(f"Expected float value for one of {names}") from exc


def _format_seconds(values):
    if not values:
        return "n/a"
    return f"min={min(values):.3f}s avg={statistics.mean(values):.3f}s " f"max={max(values):.3f}s"


def _format_bytes(values):
    if not values:
        return "n/a"
    return f"min={min(values)} avg={statistics.mean(values):.0f} " f"max={max(values)}"


class TestAutoSaverStress(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.document_count = _get_int_arg(("documents", "docs"), 2)
        cls.object_count = _get_int_arg(("objects",), 75)
        cls.iteration_count = _get_int_arg(("iterations",), 12)
        cls.burst_count = _get_int_arg(("burst",), 6)
        cls.wait_timeout = _get_float_arg(("timeout",), 5.0)

        for option_name, option_value in (
            ("documents", cls.document_count),
            ("objects", cls.object_count),
            ("iterations", cls.iteration_count),
            ("burst", cls.burst_count),
        ):
            if option_value < 1:
                raise ValueError(f"{option_name} must be greater than zero")
        if cls.wait_timeout <= 0:
            raise ValueError("timeout must be greater than zero")

    def setUp(self):
        self.autosaver = self._findAutoSaver()
        self.direct_flush_durations = []
        self.queued_flush_durations = []
        self.archive_sizes = []
        self.documents = []

        for index in range(self.document_count):
            document = FreeCAD.newDocument(f"AutoSaverStressDoc{index}")
            self.assertIsNotNone(FreeCADGui.getDocument(document.Name))
            objects = [
                document.addObject("App::FeaturePython", f"StressObject{obj_index}")
                for obj_index in range(self.object_count)
            ]
            self.documents.append((document, objects))

    def tearDown(self):
        for document, _objects in reversed(self.documents):
            if FreeCAD.getDocument(document.Name):
                FreeCAD.closeDocument(document.Name)

    def _findAutoSaver(self):
        app = QtWidgets.QApplication.instance()
        self.assertIsNotNone(app)

        for child in app.children():
            meta_object = child.metaObject() if hasattr(child, "metaObject") else None
            if meta_object and meta_object.className() == "Gui::AutoSaver":
                return child

        raise self.failureException("Gui::AutoSaver was not found in the QApplication object tree")

    def _recoveryArchive(self, document):
        return os.path.join(document.TransientDir, "fc_recovery_file.fcstd")

    def _removeRecoveryArchive(self, document):
        archive = self._recoveryArchive(document)
        if os.path.exists(archive):
            os.remove(archive)

    def _invokeAutoSaverFlush(self, document):
        invoked = QtCore.QMetaObject.invokeMethod(
            self.autosaver,
            "flushPendingSave",
            QtCore.Qt.ConnectionType.DirectConnection,
            QtCore.Q_ARG(str, document.Name),
        )
        self.assertTrue(invoked)

    def _processEventsUntil(self, predicate):
        deadline = time.monotonic() + self.wait_timeout
        while time.monotonic() < deadline:
            QtWidgets.QApplication.processEvents(QtCore.QEventLoop.ProcessEventsFlag.AllEvents, 50)
            if predicate():
                return True
            time.sleep(0.01)

        return predicate()

    def _assertRecoveryArchiveContains(self, document, expected_label):
        archive = self._recoveryArchive(document)
        self.assertTrue(os.path.isfile(archive), archive)

        with zipfile.ZipFile(archive) as recovery:
            self.assertIn("Document.xml", recovery.namelist())
            self.assertIn("GuiDocument.xml", recovery.namelist())
            document_xml = recovery.read("Document.xml").decode("utf-8", errors="replace")
            self.assertIn(expected_label, document_xml)

        self.archive_sizes.append(os.path.getsize(archive))

    def _mutateAllObjects(self, objects, label_prefix):
        for obj_index, obj in enumerate(objects):
            obj.Label = f"{label_prefix}-{obj_index}"

    def testSoak(self):
        for document_index, (document, objects) in enumerate(self.documents):
            for iteration in range(self.iteration_count):
                direct_label = f"direct-{document_index}-{iteration}"
                self._mutateAllObjects(objects, direct_label)
                self._removeRecoveryArchive(document)

                start = time.monotonic()
                self._invokeAutoSaverFlush(document)
                self.direct_flush_durations.append(time.monotonic() - start)
                self._assertRecoveryArchiveContains(
                    document,
                    f"{direct_label}-{self.object_count - 1}",
                )

                blocked_label = None
                self._removeRecoveryArchive(document)
                document.openTransaction(f"AutoSaverStress-{document_index}-{iteration}")
                rotating_object = objects[iteration % len(objects)]
                for burst in range(self.burst_count):
                    blocked_label = f"blocked-{document_index}-{iteration}-{burst}"
                    rotating_object.Label = blocked_label
                    self._invokeAutoSaverFlush(document)
                    self.assertFalse(os.path.exists(self._recoveryArchive(document)))

                start = time.monotonic()
                document.commitTransaction()
                self.assertTrue(
                    self._processEventsUntil(
                        lambda: os.path.exists(self._recoveryArchive(document))
                    )
                )
                self.queued_flush_durations.append(time.monotonic() - start)
                self._assertRecoveryArchiveContains(document, blocked_label)

        print(
            (
                "AutoSaver soak summary: "
                f"documents={self.document_count} "
                f"objects_per_document={self.object_count} "
                f"iterations={self.iteration_count} "
                f"burst={self.burst_count} "
                f"direct_flush={_format_seconds(self.direct_flush_durations)} "
                f"queued_flush={_format_seconds(self.queued_flush_durations)} "
                f"archive_bytes={_format_bytes(self.archive_sizes)}"
            ),
            flush=True,
        )


def _is_direct_script_run():
    if __name__ == "__main__":
        return True
    return len(sys.argv) > 1 and os.path.abspath(sys.argv[1]) == os.path.abspath(__file__)


def runDirect():
    case = TestAutoSaverStress("testSoak")
    try:
        TestAutoSaverStress.setUpClass()
        case.setUp()
        try:
            case.testSoak()
        finally:
            case.tearDown()
    except Exception:
        traceback.print_exc()
        return 1
    return 0


if _is_direct_script_run():
    sys.exit(runDirect())
