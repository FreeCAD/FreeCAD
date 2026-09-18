# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors
# SPDX-FileNotice: Part of the FreeCAD project.

"""Tests of the Python wrappers for the Crash Reporter. Note that most CrashReporter testing
happens on the C++ side, so this file contains only the tests necessary to prove that the
Python bindings correctly interface with the C++ code. They are not end-to-end tests of the
crash reporting system."""

import inspect
import types
import unittest

import FreeCAD as App


class TestCrashReporter(unittest.TestCase):

    def testGetCrashReportsReturnsAList(self):
        """Never none, never some other type: just a list"""
        self.assertIsInstance(App.getCrashReports(), list)

    def testGetLastCrashReportReturnsReportOrNone(self):
        """Always returns either a CrashReport object or None, no other type"""
        report = App.getLastCrashReport()
        self.assertIsInstance(report, (App.CrashReport, type(None)))

    def testNoConstruction(self):
        """There is intentionally no way for Python to create a CrashReport (which would almost
        certainly be pure trash), so make sure someone doesn't accidentally flip the switch to
        make a constructor for these types."""
        for dont_make_one_of_these in [App.CrashReport, App.CrashFrame]:
            # With Constructor=False, FreeCAD raises a RuntimeError, not a TypeError, here
            with self.assertRaises(RuntimeError):
                _ = dont_make_one_of_these()

    def testCrashReportAPICheck(self):
        """The API should not be inadvertently changed. If you add something to it, make sure you
        *also* add it to this list. If you remove something... well, don't do that :), you'll probably
        break some Addons."""
        crash_report_attributes = (
            "path_to_raw_report_file",
            "fault_address",
            "thread_id",
            "timestamp",
            "process_id",
            "fault_code",
            "fault_name",
            "partial_write",
            "capture_was_signal_safe",
            "build_id",
            "minidump_path",
            "os",
            "os_version",
            "architecture",
            "freecad_version",
            "symbolicated",
            "stack_frames",
        )

        self.assertEqual(
            set(crash_report_attributes),
            {n for n in dir(App.CrashReport) if not n.startswith("_")},
        )

        # We can also make sure it's really a binding, and not something else
        for attr in crash_report_attributes:
            with self.subTest(attribute=attr):
                self.assertIsInstance(
                    inspect.getattr_static(App.CrashReport, attr),
                    types.GetSetDescriptorType,
                )

    def testCrashFrameAPICheck(self):
        """Same reasoning as testCrashReportAPICheck, above"""
        crash_frame_attributes = (
            "address",
            "module_offset",
            "module",
            "symbol",
            "file",
            "line",
            "is_inline",
        )

        self.assertEqual(
            set(crash_frame_attributes),
            {n for n in dir(App.CrashFrame) if not n.startswith("_")},
        )

        # We can also make sure it's really a binding, and not something else
        for attr in crash_frame_attributes:
            with self.subTest(attribute=attr):
                self.assertIsInstance(
                    inspect.getattr_static(App.CrashFrame, attr),
                    types.GetSetDescriptorType,
                )

    def testMethodsDontTakeArguments(self):
        """None of the functions in this API take any arguments."""

        # Noting for the record that if this ever *doesn't* fail, clearCrashReports will end up
        # clearing the caller's *real* crash report directory. So don't go changing this API
        # without updating the tests!!
        functions = [App.getLastCrashReport, App.getCrashReports, App.clearCrashReports]
        for func in functions:
            with self.subTest(func=func):
                with self.assertRaises(TypeError):
                    func(42)
