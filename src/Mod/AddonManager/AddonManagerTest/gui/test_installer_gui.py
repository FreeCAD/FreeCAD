# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
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
# ***************************************************************************

import os
import tempfile
import unittest
import FreeCAD

from PySide import QtCore, QtWidgets

from addonmanager_installer_gui import AddonInstallerGUI, MacroInstallerGUI

from AddonManagerTest.gui.gui_mocks import DialogWatcher, DialogInteractor
from AddonManagerTest.app.mocks import MockAddon

translate = FreeCAD.Qt.translate


class TestInstallerGui(unittest.TestCase):

    MODULE = "test_installer_gui"  # file name without extension

    def setUp(self):
        self.addon_to_install = MockAddon()
        self.installer_gui = AddonInstallerGUI(self.addon_to_install)
        self.finalized_thread = False

    def tearDown(self):
        pass

    def test_success_dialog(self):
        # Pop the modal dialog and verify that it opens, and responds to an OK click
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Success"),
            QtWidgets.QDialogButtonBox.Ok,
        )
        self.installer_gui._installation_succeeded()
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_failure_dialog(self):
        # Pop the modal dialog and verify that it opens, and responds to a Cancel click
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Installation Failed"),
            QtWidgets.QDialogButtonBox.Cancel,
        )
        self.installer_gui._installation_failed(
            self.addon_to_install, "Test of installation failure"
        )
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_no_python_dialog(self):
        # Pop the modal dialog and verify that it opens, and responds to a No click
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Cannot execute Python"),
            QtWidgets.QDialogButtonBox.No,
        )
        self.installer_gui._report_no_python_exe()
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_no_pip_dialog(self):
        # Pop the modal dialog and verify that it opens, and responds to a No click
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Cannot execute pip"),
            QtWidgets.QDialogButtonBox.No,
        )
        self.installer_gui._report_no_pip("pip not actually run, this was a test")
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_dependency_failure_dialog(self):
        # Pop the modal dialog and verify that it opens, and responds to a No click
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Package installation failed"),
            QtWidgets.QDialogButtonBox.No,
        )
        self.installer_gui._report_dependency_failure(
            "Unit test", "Nothing really failed, this is a test of the dialog box"
        )
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_install(self):
        # Run the installation code and make sure it puts the directory in place
        with tempfile.TemporaryDirectory() as temp_dir:
            self.installer_gui.installer.installation_path = temp_dir
            self.installer_gui.install()  # This does not block
            self.installer_gui.installer.success.disconnect(
                self.installer_gui._installation_succeeded
            )
            self.installer_gui.installer.failure.disconnect(self.installer_gui._installation_failed)
            while not self.installer_gui.worker_thread.isFinished():
                QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 100)
            self.assertTrue(
                os.path.exists(os.path.join(temp_dir, "MockAddon")),
                "Installed directory not found",
            )

    def test_handle_disallowed_python(self):
        disallowed_packages = ["disallowed_package_name"]
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Missing Requirement"),
            QtWidgets.QDialogButtonBox.Cancel,
        )
        self.installer_gui._handle_disallowed_python(disallowed_packages)
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_handle_disallowed_python_long_list(self):
        """A separate test for when there are MANY packages, which takes a separate code path."""
        disallowed_packages = []
        for i in range(50):
            disallowed_packages.append(f"disallowed_package_name_{i}")
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Missing Requirement"),
            QtWidgets.QDialogButtonBox.Cancel,
        )
        self.installer_gui._handle_disallowed_python(disallowed_packages)
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_report_missing_workbenches_single(self):
        """Test only missing one workbench"""
        wbs = ["OneMissingWorkbench"]
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Missing Requirement"),
            QtWidgets.QDialogButtonBox.Cancel,
        )
        self.installer_gui._report_missing_workbenches(wbs)
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_report_missing_workbenches_multiple(self):
        """Test only missing one workbench"""
        wbs = ["FirstMissingWorkbench", "SecondMissingWorkbench"]
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Missing Requirement"),
            QtWidgets.QDialogButtonBox.Cancel,
        )
        self.installer_gui._report_missing_workbenches(wbs)
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_resolve_dependencies_then_install(self):
        class MissingDependenciesMock:
            def __init__(self):
                self.external_addons = ["addon_1", "addon_2"]
                self.python_requires = ["py_req_1", "py_req_2"]
                self.python_optional = ["py_opt_1", "py_opt_2"]

        missing = MissingDependenciesMock()
        dialog_watcher = DialogWatcher(
            translate("DependencyResolutionDialog", "Resolve Dependencies"),
            QtWidgets.QDialogButtonBox.Cancel,
        )
        self.installer_gui._resolve_dependencies_then_install(missing)
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_check_python_version_bad(self):
        class MissingDependenciesMock:
            def __init__(self):
                self.python_min_version = {"major": 3, "minor": 9999}

        missing = MissingDependenciesMock()
        dialog_watcher = DialogWatcher(
            translate("AddonsInstaller", "Incompatible Python version"),
            QtWidgets.QDialogButtonBox.Cancel,
        )
        stop_installing = self.installer_gui._check_python_version(missing)
        self.assertTrue(dialog_watcher.dialog_found, "Failed to find the expected dialog box")
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")
        self.assertTrue(stop_installing, "Failed to halt installation on bad Python version")

    def test_check_python_version_good(self):
        class MissingDependenciesMock:
            def __init__(self):
                self.python_min_version = {"major": 3, "minor": 0}

        missing = MissingDependenciesMock()
        stop_installing = self.installer_gui._check_python_version(missing)
        self.assertFalse(stop_installing, "Failed to continue installation on good Python version")

    def test_clean_up_optional(self):
        class MissingDependenciesMock:
            def __init__(self):
                self.python_optional = [
                    "allowed_packages_1",
                    "allowed_packages_2",
                    "disallowed_package",
                ]

        allowed_packages = ["allowed_packages_1", "allowed_packages_2"]
        missing = MissingDependenciesMock()
        self.installer_gui.installer.allowed_packages = set(allowed_packages)
        self.installer_gui._clean_up_optional(missing)
        self.assertTrue("allowed_packages_1" in missing.python_optional)
        self.assertTrue("allowed_packages_2" in missing.python_optional)
        self.assertFalse("disallowed_package" in missing.python_optional)

    def intercept_run_dependency_installer(self, addons, python_requires, python_optional):
        self.assertEqual(python_requires, ["py_req_1", "py_req_2"])
        self.assertEqual(python_optional, ["py_opt_1", "py_opt_2"])
        self.assertEqual(addons[0].name, "addon_1")
        self.assertEqual(addons[1].name, "addon_2")

    def test_dependency_dialog_yes_clicked(self):
        class DialogMock:
            class ListWidgetMock:
                class ListWidgetItemMock:
                    def __init__(self, name):
                        self.name = name

                    def text(self):
                        return self.name

                    def checkState(self):
                        return QtCore.Qt.Checked

                def __init__(self, items):
                    self.list = []
                    for item in items:
                        self.list.append(DialogMock.ListWidgetMock.ListWidgetItemMock(item))

                def count(self):
                    return len(self.list)

                def item(self, i):
                    return self.list[i]

            def __init__(self):
                self.listWidgetAddons = DialogMock.ListWidgetMock(["addon_1", "addon_2"])
                self.listWidgetPythonRequired = DialogMock.ListWidgetMock(["py_req_1", "py_req_2"])
                self.listWidgetPythonOptional = DialogMock.ListWidgetMock(["py_opt_1", "py_opt_2"])

        class AddonMock:
            def __init__(self, name):
                self.name = name

        self.installer_gui.dependency_dialog = DialogMock()
        self.installer_gui.addons = [AddonMock("addon_1"), AddonMock("addon_2")]
        self.installer_gui._run_dependency_installer = self.intercept_run_dependency_installer
        self.installer_gui._dependency_dialog_yes_clicked()


class TestMacroInstallerGui(unittest.TestCase):
    class MockMacroAddon:
        class MockMacro:
            def __init__(self):
                self.install_called = False
                self.install_result = (
                    True  # External code can change to False to test failed install
                )
                self.name = "MockMacro"
                self.filename = "mock_macro_no_real_file.FCMacro"
                self.comment = "This is a mock macro for unit testing"
                self.icon = None
                self.xpm = None

            def install(self):
                self.install_called = True
                return self.install_result

        def __init__(self):
            self.macro = TestMacroInstallerGui.MockMacroAddon.MockMacro()
            self.name = self.macro.name
            self.display_name = self.macro.name

    class MockParameter:
        """Mock the parameter group to allow simplified behavior and introspection."""

        def __init__(self):
            self.params = {}
            self.groups = {}
            self.accessed_parameters = {}  # Dict is param name: default value

            types = ["Bool", "String", "Int", "UInt", "Float"]
            for t in types:
                setattr(self, f"Get{t}", self.get)
                setattr(self, f"Set{t}", self.set)
                setattr(self, f"Rem{t}", self.rem)

        def get(self, p, default=None):
            self.accessed_parameters[p] = default
            if p in self.params:
                return self.params[p]
            else:
                return default

        def set(self, p, value):
            self.params[p] = value

        def rem(self, p):
            if p in self.params:
                self.params.erase(p)

        def GetGroup(self, name):
            if name not in self.groups:
                self.groups[name] = TestMacroInstallerGui.MockParameter()
            return self.groups[name]

        def GetGroups(self):
            return self.groups.keys()

    class ToolbarIntercepter:
        def __init__(self):
            self.ask_for_toolbar_called = False
            self.install_macro_to_toolbar_called = False
            self.tb = None
            self.custom_group = TestMacroInstallerGui.MockParameter()
            self.custom_group.set("Name", "MockCustomToolbar")

        def _ask_for_toolbar(self, _):
            self.ask_for_toolbar_called = True
            return self.custom_group

        def _install_macro_to_toolbar(self, tb):
            self.install_macro_to_toolbar_called = True
            self.tb = tb

    class InstallerInterceptor:
        def __init__(self):
            self.ccc_called = False

        def _create_custom_command(
            self,
            toolbar,
            filename,
            menuText,
            tooltipText,
            whatsThisText,
            statustipText,
            pixmapText,
        ):
            self.ccc_called = True
            self.toolbar = toolbar
            self.filename = filename
            self.menuText = menuText
            self.tooltipText = tooltipText
            self.whatsThisText = whatsThisText
            self.statustipText = statustipText
            self.pixmapText = pixmapText

    def setUp(self):
        self.mock_macro = TestMacroInstallerGui.MockMacroAddon()
        self.installer = MacroInstallerGUI(self.mock_macro)
        self.installer.addon_params = TestMacroInstallerGui.MockParameter()
        self.installer.toolbar_params = TestMacroInstallerGui.MockParameter()

    def tearDown(self):
        pass

    def test_class_is_initialized(self):
        """Connecting to a signal does not throw"""
        self.installer.finished.connect(lambda: None)

    def test_ask_for_toolbar_no_dialog_default_exists(self):
        self.installer.addon_params.set("alwaysAskForToolbar", False)
        self.installer.addon_params.set("CustomToolbarName", "UnitTestCustomToolbar")
        utct = self.installer.toolbar_params.GetGroup("UnitTestCustomToolbar")
        utct.set("Name", "UnitTestCustomToolbar")
        utct.set("Active", True)
        result = self.installer._ask_for_toolbar([])
        self.assertIsNotNone(result)
        self.assertTrue(hasattr(result, "get"))
        name = result.get("Name")
        self.assertEqual(name, "UnitTestCustomToolbar")

    def test_ask_for_toolbar_with_dialog_cancelled(self):

        # First test: the user cancels the dialog
        self.installer.addon_params.set("alwaysAskForToolbar", True)
        dialog_watcher = DialogWatcher(
            translate("select_toolbar_dialog", "Select Toolbar"),
            QtWidgets.QDialogButtonBox.Cancel,
        )
        result = self.installer._ask_for_toolbar([])
        self.assertIsNone(result)

    def test_ask_for_toolbar_with_dialog_defaults(self):

        # Second test: the user leaves the dialog at all default values, so:
        #   - The checkbox "Ask every time" is unchecked
        #   - The selected toolbar option is "Create new toolbar", which triggers a search for
        # a new custom toolbar name by calling _create_new_custom_toolbar, which we mock.
        fake_custom_toolbar_group = TestMacroInstallerGui.MockParameter()
        fake_custom_toolbar_group.set("Name", "UnitTestCustomToolbar")
        self.installer._create_new_custom_toolbar = lambda: fake_custom_toolbar_group
        dialog_watcher = DialogWatcher(
            translate("select_toolbar_dialog", "Select Toolbar"),
            QtWidgets.QDialogButtonBox.Ok,
        )
        result = self.installer._ask_for_toolbar([])
        self.assertIsNotNone(result)
        self.assertTrue(hasattr(result, "get"))
        name = result.get("Name")
        self.assertEqual(name, "UnitTestCustomToolbar")
        self.assertIn("alwaysAskForToolbar", self.installer.addon_params.params)
        self.assertFalse(self.installer.addon_params.get("alwaysAskForToolbar", True))
        self.assertTrue(dialog_watcher.button_found, "Failed to find the expected button")

    def test_ask_for_toolbar_with_dialog_selection(self):

        # Third test: the user selects a custom toolbar in the dialog, and checks the box to always
        # ask.
        dialog_interactor = DialogInteractor(
            translate("select_toolbar_dialog", "Select Toolbar"),
            self.interactor_selection_option_and_checkbox,
        )
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UT_TB_1")
        ut_tb_2 = self.installer.toolbar_params.GetGroup("UT_TB_2")
        ut_tb_3 = self.installer.toolbar_params.GetGroup("UT_TB_3")
        ut_tb_1.set("Name", "UT_TB_1")
        ut_tb_2.set("Name", "UT_TB_2")
        ut_tb_3.set("Name", "UT_TB_3")
        result = self.installer._ask_for_toolbar(["UT_TB_1", "UT_TB_2", "UT_TB_3"])
        self.assertIsNotNone(result)
        self.assertTrue(hasattr(result, "get"))
        name = result.get("Name")
        self.assertEqual(name, "UT_TB_3")
        self.assertIn("alwaysAskForToolbar", self.installer.addon_params.params)
        self.assertTrue(self.installer.addon_params.get("alwaysAskForToolbar", False))

    def interactor_selection_option_and_checkbox(self, parent):

        boxes = parent.findChildren(QtWidgets.QComboBox)
        self.assertEqual(len(boxes), 1)  # Just to make sure...
        box = boxes[0]
        box.setCurrentIndex(box.count() - 2)  # Select the last thing but one

        checkboxes = parent.findChildren(QtWidgets.QCheckBox)
        self.assertEqual(len(checkboxes), 1)  # Just to make sure...
        checkbox = checkboxes[0]
        checkbox.setChecked(True)

        parent.accept()

    def test_macro_button_exists_no_command(self):
        # Test 1: No command for this macro
        self.installer._find_custom_command = lambda _: None
        button_exists = self.installer._macro_button_exists()
        self.assertFalse(button_exists)

    def test_macro_button_exists_true(self):
        # Test 2: Macro is in the list of buttons
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UnitTestCommand")
        ut_tb_1.set("UnitTestCommand", "FreeCAD")  # This is what the real thing looks like...
        self.installer._find_custom_command = lambda _: "UnitTestCommand"
        self.assertTrue(self.installer._macro_button_exists())

    def test_macro_button_exists_false(self):
        # Test 3: Macro is not in the list of buttons
        self.installer._find_custom_command = lambda _: "UnitTestCommand"
        self.assertFalse(self.installer._macro_button_exists())

    def test_ask_to_install_toolbar_button_disabled(self):
        self.installer.addon_params.SetBool("dontShowAddMacroButtonDialog", True)
        self.installer._ask_to_install_toolbar_button()
        # This should NOT block when dontShowAddMacroButtonDialog is True

    def test_ask_to_install_toolbar_button_enabled_no(self):
        self.installer.addon_params.SetBool("dontShowAddMacroButtonDialog", False)
        dialog_watcher = DialogWatcher(
            translate("toolbar_button", "Add button?"),
            QtWidgets.QDialogButtonBox.No,
        )
        # Note: that dialog does not use a QButtonBox, so we can really only test its
        # reject() signal, which is triggered by the DialogWatcher when it cannot find
        # the button. In this case, failure to find that button is NOT an error.
        self.installer._ask_to_install_toolbar_button()  # Blocks until killed by watcher
        self.assertTrue(dialog_watcher.dialog_found)

    def test_get_toolbar_with_name_found(self):
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UnitTestToolbar")
        ut_tb_1.set("Name", "Unit Test Toolbar")
        ut_tb_1.set("UnitTestParam", True)
        tb = self.installer._get_toolbar_with_name("Unit Test Toolbar")
        self.assertIsNotNone(tb)
        self.assertTrue(tb.get("UnitTestParam", False))

    def test_get_toolbar_with_name_not_found(self):
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UnitTestToolbar")
        ut_tb_1.set("Name", "Not the Unit Test Toolbar")
        tb = self.installer._get_toolbar_with_name("Unit Test Toolbar")
        self.assertIsNone(tb)

    def test_create_new_custom_toolbar_no_existing(self):
        tb = self.installer._create_new_custom_toolbar()
        self.assertEqual(tb.get("Name", ""), "Auto-Created Macro Toolbar")
        self.assertTrue(tb.get("Active", False), True)

    def test_create_new_custom_toolbar_one_existing(self):
        _ = self.installer._create_new_custom_toolbar()
        tb = self.installer._create_new_custom_toolbar()
        self.assertEqual(tb.get("Name", ""), "Auto-Created Macro Toolbar (2)")
        self.assertTrue(tb.get("Active", False), True)

    def test_check_for_toolbar_true(self):
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UT_TB_1")
        ut_tb_1.set("Name", "UT_TB_1")
        self.assertTrue(self.installer._check_for_toolbar("UT_TB_1"))

    def test_check_for_toolbar_false(self):
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UT_TB_1")
        ut_tb_1.set("Name", "UT_TB_1")
        self.assertFalse(self.installer._check_for_toolbar("Not UT_TB_1"))

    def test_install_toolbar_button_first_custom_toolbar(self):
        tbi = TestMacroInstallerGui.ToolbarIntercepter()
        self.installer._ask_for_toolbar = tbi._ask_for_toolbar
        self.installer._install_macro_to_toolbar = tbi._install_macro_to_toolbar
        self.installer._install_toolbar_button()
        self.assertTrue(tbi.install_macro_to_toolbar_called)
        self.assertFalse(tbi.ask_for_toolbar_called)
        self.assertTrue("Custom_1" in self.installer.toolbar_params.GetGroups())

    def test_install_toolbar_button_existing_custom_toolbar_1(self):
        # There is an existing custom toolbar, and we should use it
        tbi = TestMacroInstallerGui.ToolbarIntercepter()
        self.installer._ask_for_toolbar = tbi._ask_for_toolbar
        self.installer._install_macro_to_toolbar = tbi._install_macro_to_toolbar
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UT_TB_1")
        ut_tb_1.set("Name", "UT_TB_1")
        self.installer.addon_params.set("CustomToolbarName", "UT_TB_1")
        self.installer._install_toolbar_button()
        self.assertTrue(tbi.install_macro_to_toolbar_called)
        self.assertFalse(tbi.ask_for_toolbar_called)
        self.assertEqual(tbi.tb.get("Name", ""), "UT_TB_1")

    def test_install_toolbar_button_existing_custom_toolbar_2(self):
        # There are multiple existing custom toolbars, and we should use one of them
        tbi = TestMacroInstallerGui.ToolbarIntercepter()
        self.installer._ask_for_toolbar = tbi._ask_for_toolbar
        self.installer._install_macro_to_toolbar = tbi._install_macro_to_toolbar
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UT_TB_1")
        ut_tb_2 = self.installer.toolbar_params.GetGroup("UT_TB_2")
        ut_tb_3 = self.installer.toolbar_params.GetGroup("UT_TB_3")
        ut_tb_1.set("Name", "UT_TB_1")
        ut_tb_2.set("Name", "UT_TB_2")
        ut_tb_3.set("Name", "UT_TB_3")
        self.installer.addon_params.set("CustomToolbarName", "UT_TB_3")
        self.installer._install_toolbar_button()
        self.assertTrue(tbi.install_macro_to_toolbar_called)
        self.assertFalse(tbi.ask_for_toolbar_called)
        self.assertEqual(tbi.tb.get("Name", ""), "UT_TB_3")

    def test_install_toolbar_button_existing_custom_toolbar_3(self):
        # There are multiple existing custom toolbars, but none of them match
        tbi = TestMacroInstallerGui.ToolbarIntercepter()
        self.installer._ask_for_toolbar = tbi._ask_for_toolbar
        self.installer._install_macro_to_toolbar = tbi._install_macro_to_toolbar
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UT_TB_1")
        ut_tb_2 = self.installer.toolbar_params.GetGroup("UT_TB_2")
        ut_tb_3 = self.installer.toolbar_params.GetGroup("UT_TB_3")
        ut_tb_1.set("Name", "UT_TB_1")
        ut_tb_2.set("Name", "UT_TB_2")
        ut_tb_3.set("Name", "UT_TB_3")
        self.installer.addon_params.set("CustomToolbarName", "UT_TB_4")
        self.installer._install_toolbar_button()
        self.assertTrue(tbi.install_macro_to_toolbar_called)
        self.assertTrue(tbi.ask_for_toolbar_called)
        self.assertEqual(tbi.tb.get("Name", ""), "MockCustomToolbar")

    def test_install_toolbar_button_existing_custom_toolbar_4(self):
        # There are multiple existing custom toolbars, one of them matches, but we have set
        # "alwaysAskForToolbar" to True
        tbi = TestMacroInstallerGui.ToolbarIntercepter()
        self.installer._ask_for_toolbar = tbi._ask_for_toolbar
        self.installer._install_macro_to_toolbar = tbi._install_macro_to_toolbar
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UT_TB_1")
        ut_tb_2 = self.installer.toolbar_params.GetGroup("UT_TB_2")
        ut_tb_3 = self.installer.toolbar_params.GetGroup("UT_TB_3")
        ut_tb_1.set("Name", "UT_TB_1")
        ut_tb_2.set("Name", "UT_TB_2")
        ut_tb_3.set("Name", "UT_TB_3")
        self.installer.addon_params.set("CustomToolbarName", "UT_TB_3")
        self.installer.addon_params.set("alwaysAskForToolbar", True)
        self.installer._install_toolbar_button()
        self.assertTrue(tbi.install_macro_to_toolbar_called)
        self.assertTrue(tbi.ask_for_toolbar_called)
        self.assertEqual(tbi.tb.get("Name", ""), "MockCustomToolbar")

    def test_install_macro_to_toolbar_icon_abspath(self):
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UT_TB_1")
        ut_tb_1.set("Name", "UT_TB_1")
        ii = TestMacroInstallerGui.InstallerInterceptor()
        self.installer._create_custom_command = ii._create_custom_command
        with tempfile.NamedTemporaryFile() as ntf:
            self.mock_macro.macro.icon = ntf.name
            self.installer._install_macro_to_toolbar(ut_tb_1)
            self.assertTrue(ii.ccc_called)
            self.assertEqual(ii.pixmapText, ntf.name)

    def test_install_macro_to_toolbar_icon_relpath(self):
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UT_TB_1")
        ut_tb_1.set("Name", "UT_TB_1")
        ii = TestMacroInstallerGui.InstallerInterceptor()
        self.installer._create_custom_command = ii._create_custom_command
        with tempfile.TemporaryDirectory() as td:
            self.installer.macro_dir = td
            self.mock_macro.macro.icon = "RelativeIconPath.png"
            self.installer._install_macro_to_toolbar(ut_tb_1)
            self.assertTrue(ii.ccc_called)
            self.assertEqual(ii.pixmapText, os.path.join(td, "RelativeIconPath.png"))

    def test_install_macro_to_toolbar_xpm(self):
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UT_TB_1")
        ut_tb_1.set("Name", "UT_TB_1")
        ii = TestMacroInstallerGui.InstallerInterceptor()
        self.installer._create_custom_command = ii._create_custom_command
        with tempfile.TemporaryDirectory() as td:
            self.installer.macro_dir = td
            self.mock_macro.macro.xpm = "Not really xpm data, don't try to use it!"
            self.installer._install_macro_to_toolbar(ut_tb_1)
            self.assertTrue(ii.ccc_called)
            self.assertEqual(ii.pixmapText, os.path.join(td, "MockMacro_icon.xpm"))
            self.assertTrue(os.path.exists(os.path.join(td, "MockMacro_icon.xpm")))

    def test_install_macro_to_toolbar_no_icon(self):
        ut_tb_1 = self.installer.toolbar_params.GetGroup("UT_TB_1")
        ut_tb_1.set("Name", "UT_TB_1")
        ii = TestMacroInstallerGui.InstallerInterceptor()
        self.installer._create_custom_command = ii._create_custom_command
        with tempfile.TemporaryDirectory() as td:
            self.installer.macro_dir = td
            self.installer._install_macro_to_toolbar(ut_tb_1)
            self.assertTrue(ii.ccc_called)
            self.assertIsNone(ii.pixmapText)
