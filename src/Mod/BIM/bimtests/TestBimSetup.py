# SPDX-License-Identifier: LGPL-2.1-or-later

"""Tests for IfcOpenShell health reporting in BIM Setup."""

from pathlib import Path
import sys
import unittest
from unittest.mock import Mock, call, patch

import FreeCADGui

if not hasattr(FreeCADGui, "addCommand"):
    FreeCADGui.addCommand = Mock()

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bimcommands"))

from BimSetup import BIM_Setup
from nativeifc import backend


def _status(available, *, version="", error=""):
    return backend.BackendStatus(available=available, version=version, error=error)


class TestBimSetup(unittest.TestCase):
    def setUp(self):
        self.setup = BIM_Setup()

    def test_status_refreshes_all_capability_profiles(self):
        healthy = _status(True, version="0.8.5")
        with (
            patch.object(backend, "invalidate") as invalidate,
            patch.object(backend, "get_status", return_value=healthy) as get_status,
        ):
            statuses = self.setup.getIfcOpenShellStatus(refresh=True)

        invalidate.assert_called_once_with()
        self.assertEqual(len(statuses), 3)
        self.assertEqual(
            get_status.call_args_list,
            [
                call(capability=backend.IMPORT),
                call(capability=backend.MULTICORE_IMPORT),
                call(capability=backend.EXPORT),
            ],
        )

    def test_health_text_reports_partial_installation(self):
        statuses = {
            "Single-process import": _status(True, version="0.8.5"),
            "Multicore import": _status(False, error="missing geometry iterator"),
            "NativeIFC export": _status(False, error="missing API"),
        }

        text = self.setup.formatIfcOpenShellStatus(statuses)

        self.assertIn("IfcOpenShell 0.8.5", text)
        self.assertIn("Single-process import: available", text)
        self.assertIn("missing geometry iterator", text)
        self.assertIn('href="#install"', text)

    def test_healthy_installation_does_not_open_updater(self):
        statuses = {"operation": _status(True)}
        with (
            patch.object(self.setup, "getIfcOpenShellStatus", return_value=statuses),
            patch("BimSetup.FreeCADGui.runCommand", create=True) as run_command,
        ):
            result = self.setup.getIfcOpenShell()

        self.assertIs(result, statuses)
        run_command.assert_not_called()

    def test_updater_result_is_revalidated(self):
        unavailable = {"operation": _status(False)}
        healthy = {"operation": _status(True)}
        with (
            patch.object(
                self.setup,
                "getIfcOpenShellStatus",
                side_effect=(unavailable, healthy),
            ) as get_status,
            patch("BimSetup.FreeCADGui.runCommand", create=True) as run_command,
        ):
            result = self.setup.getIfcOpenShell()

        run_command.assert_called_once_with("IFC_UpdateIOS", 1)
        self.assertEqual(get_status.call_args_list, [call(), call(refresh=True)])
        self.assertIs(result, healthy)


if __name__ == "__main__":
    unittest.main()
