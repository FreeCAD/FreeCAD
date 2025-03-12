import logging
import subprocess
import sys
import unittest
from unittest.mock import MagicMock, patch

try:
    import FreeCAD
    import FreeCADGui
except ImportError:
    try:
        from PySide6 import QtCore, QtWidgets
    except ImportError:
        from PySide2 import QtCore, QtWidgets

sys.path.append(
    "../.."
)  # So that when run standalone, the Addon Manager classes imported below are available

from addonmanager_python_deps_gui import (
    PythonPackageManager,
    call_pip,
    PipFailed,
    python_package_updates_are_available,
    parse_pip_list_output,
)
from AddonManagerTest.gui.gui_mocks import DialogInteractor, DialogWatcher


class TestPythonPackageManager(unittest.TestCase):

    def setUp(self) -> None:
        self.manager = PythonPackageManager([])

    def tearDown(self) -> None:
        if self.manager.worker_thread:
            self.manager.worker_thread.terminate()
            self.manager.worker_thread.wait()

    @patch("addonmanager_python_deps_gui.PythonPackageManager._create_list_from_pip")
    def test_show(self, patched_create_list_from_pip):
        dialog_watcher = DialogWatcher("Manage Python Dependencies")
        self.manager.show()
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")


class TestPythonDepsStandaloneFunctions(unittest.TestCase):

    @patch("addonmanager_utilities.run_interruptable_subprocess")
    def test_call_pip(self, mock_run_subprocess: MagicMock):
        call_pip(["arg1", "arg2", "arg3"])
        mock_run_subprocess.assert_called()
        args = mock_run_subprocess.call_args[0][0]
        self.assertTrue("pip" in args)

    @patch("addonmanager_python_deps_gui.get_python_exe")
    def test_call_pip_no_python(self, mock_get_python_exe: MagicMock):
        mock_get_python_exe.return_value = None
        with self.assertRaises(PipFailed):
            call_pip(["arg1", "arg2", "arg3"])

    @patch("addonmanager_utilities.run_interruptable_subprocess")
    def test_call_pip_exception_raised(self, mock_run_subprocess: MagicMock):
        mock_run_subprocess.side_effect = subprocess.CalledProcessError(
            -1, "dummy_command", "Fake contents of stdout", "Fake contents of stderr"
        )
        with self.assertRaises(PipFailed):
            call_pip(["arg1", "arg2", "arg3"])

    @patch("addonmanager_utilities.run_interruptable_subprocess")
    def test_call_pip_splits_results(self, mock_run_subprocess: MagicMock):
        result_mock = MagicMock()
        result_mock.stdout = "\n".join(["Value 1", "Value 2", "Value 3"])
        mock_run_subprocess.return_value = result_mock
        result = call_pip(["arg1", "arg2", "arg3"])
        self.assertEqual(len(result), 3)

    @patch("addonmanager_python_deps_gui.call_pip")
    def test_python_package_updates_are_available(self, mock_call_pip: MagicMock):
        mock_call_pip.return_value = "Some result"
        result = python_package_updates_are_available()
        self.assertEqual(result, True)

    @patch("addonmanager_python_deps_gui.call_pip")
    def test_python_package_updates_are_available_no_results(self, mock_call_pip: MagicMock):
        """An empty string is an indication that no updates are available"""
        mock_call_pip.return_value = ""
        result = python_package_updates_are_available()
        self.assertEqual(result, False)

    @patch("addonmanager_python_deps_gui.call_pip")
    def test_python_package_updates_are_available_pip_failure(self, mock_call_pip: MagicMock):
        logging.disable()
        mock_call_pip.side_effect = PipFailed("Test error message")
        logging.disable()  # A logging error message is expected here, but not desirable during test runs
        result = python_package_updates_are_available()
        self.assertEqual(result, False)
        logging.disable(logging.NOTSET)

    def test_parse_pip_list_output_no_input(self):
        results_dict = parse_pip_list_output("", "")
        self.assertEqual(len(results_dict), 0)

    def test_parse_pip_list_output_all_packages_no_updates(self):
        results_dict = parse_pip_list_output(
            ["Package    Version", "---------- -------", "gitdb      4.0.9", "setuptools 41.2.0"],
            [],
        )
        self.assertEqual(len(results_dict), 2)
        self.assertTrue("gitdb" in results_dict)
        self.assertTrue("setuptools" in results_dict)
        self.assertEqual(results_dict["gitdb"]["installed_version"], "4.0.9")
        self.assertEqual(results_dict["gitdb"]["available_version"], "")
        self.assertEqual(results_dict["setuptools"]["installed_version"], "41.2.0")
        self.assertEqual(results_dict["setuptools"]["available_version"], "")

    def test_parse_pip_list_output_all_packages_with_updates(self):
        results_dict = parse_pip_list_output(
            [],
            [
                "Package    Version Latest Type",
                "---------- ------- ------ -----",
                "pip        21.0.1  22.1.2 wheel",
                "setuptools 41.2.0  63.2.0 wheel",
            ],
        )
        self.assertEqual(len(results_dict), 2)
        self.assertTrue("pip" in results_dict)
        self.assertTrue("setuptools" in results_dict)
        self.assertEqual(results_dict["pip"]["installed_version"], "21.0.1")
        self.assertEqual(results_dict["pip"]["available_version"], "22.1.2")
        self.assertEqual(results_dict["setuptools"]["installed_version"], "41.2.0")
        self.assertEqual(results_dict["setuptools"]["available_version"], "63.2.0")


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    QtCore.QTimer.singleShot(0, unittest.main)
    app.exec()
