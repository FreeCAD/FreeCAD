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

"""Contains the unit test class for addonmanager_uninstaller.py non-GUI functionality."""

import functools
import os
from stat import S_IREAD, S_IRGRP, S_IROTH, S_IWUSR
import tempfile
import unittest

import FreeCAD

from addonmanager_uninstaller import AddonUninstaller, MacroUninstaller

from Addon import Addon
from AddonManagerTest.app.mocks import MockAddon, MockMacro


class TestAddonUninstaller(unittest.TestCase):
    """Test class for addonmanager_uninstaller.py non-GUI functionality"""

    MODULE = "test_uninstaller"  # file name without extension

    def setUp(self):
        """Initialize data needed for all tests"""
        self.test_data_dir = os.path.join(
            FreeCAD.getHomePath(), "Mod", "AddonManager", "AddonManagerTest", "data"
        )
        self.mock_addon = MockAddon()
        self.signals_caught = []
        self.test_object = AddonUninstaller(self.mock_addon)

        self.test_object.finished.connect(functools.partial(self.catch_signal, "finished"))
        self.test_object.success.connect(functools.partial(self.catch_signal, "success"))
        self.test_object.failure.connect(functools.partial(self.catch_signal, "failure"))

    def tearDown(self):
        """Finalize the test."""

    def catch_signal(self, signal_name, *_):
        """Internal use: used to catch and log any emitted signals. Not called directly."""
        self.signals_caught.append(signal_name)

    def setup_dummy_installation(self, temp_dir) -> str:
        """Set up a dummy Addon in temp_dir"""
        toplevel_path = os.path.join(temp_dir, self.mock_addon.name)
        os.makedirs(toplevel_path)
        with open(os.path.join(toplevel_path, "README.md"), "w") as f:
            f.write("## Mock Addon ##\n\nFile created by the unit test code.")
        self.test_object.installation_path = temp_dir
        return toplevel_path

    def create_fake_macro(self, macro_directory, fake_macro_name, digest):
        """Create an FCMacro file and matching digest entry for later removal"""
        os.makedirs(macro_directory, exist_ok=True)
        fake_file_installed = os.path.join(macro_directory, fake_macro_name)
        with open(digest, "a", encoding="utf-8") as f:
            f.write("# The following files were created outside this installation:\n")
            f.write(fake_file_installed + "\n")
        with open(fake_file_installed, "w", encoding="utf-8") as f:
            f.write("# Fake macro data for unit testing")

    def test_uninstall_normal(self):
        """Test the integrated uninstall function under normal circumstances"""

        with tempfile.TemporaryDirectory() as temp_dir:
            toplevel_path = self.setup_dummy_installation(temp_dir)
            self.test_object.run()
            self.assertTrue(os.path.exists(temp_dir))
            self.assertFalse(os.path.exists(toplevel_path))
            self.assertNotIn("failure", self.signals_caught)
            self.assertIn("success", self.signals_caught)
            self.assertIn("finished", self.signals_caught)

    def test_uninstall_no_name(self):
        """Test the integrated uninstall function for an addon without a name"""

        with tempfile.TemporaryDirectory() as temp_dir:
            toplevel_path = self.setup_dummy_installation(temp_dir)
            self.mock_addon.name = None
            result = self.test_object.run()
            self.assertTrue(os.path.exists(temp_dir))
            self.assertIn("failure", self.signals_caught)
            self.assertNotIn("success", self.signals_caught)
            self.assertIn("finished", self.signals_caught)

    def test_uninstall_dangerous_name(self):
        """Test the integrated uninstall function for an addon with a dangerous name"""

        with tempfile.TemporaryDirectory() as temp_dir:
            toplevel_path = self.setup_dummy_installation(temp_dir)
            self.mock_addon.name = "./"
            result = self.test_object.run()
            self.assertTrue(os.path.exists(temp_dir))
            self.assertIn("failure", self.signals_caught)
            self.assertNotIn("success", self.signals_caught)
            self.assertIn("finished", self.signals_caught)

    def test_uninstall_unmatching_name(self):
        """Test the integrated uninstall function for an addon with a name that isn't installed"""

        with tempfile.TemporaryDirectory() as temp_dir:
            toplevel_path = self.setup_dummy_installation(temp_dir)
            self.mock_addon.name += "Nonexistent"
            result = self.test_object.run()
            self.assertTrue(os.path.exists(temp_dir))
            self.assertIn("failure", self.signals_caught)
            self.assertNotIn("success", self.signals_caught)
            self.assertIn("finished", self.signals_caught)

    def test_uninstall_addon_with_macros(self):
        """Tests that the uninstaller removes the macro files"""
        with tempfile.TemporaryDirectory() as temp_dir:
            toplevel_path = self.setup_dummy_installation(temp_dir)
            macro_directory = os.path.join(temp_dir, "Macros")
            self.create_fake_macro(
                macro_directory,
                "FakeMacro.FCMacro",
                os.path.join(toplevel_path, "AM_INSTALLATION_DIGEST.txt"),
            )
            result = self.test_object.run()
            self.assertNotIn("failure", self.signals_caught)
            self.assertIn("success", self.signals_caught)
            self.assertIn("finished", self.signals_caught)
            self.assertFalse(os.path.exists(os.path.join(macro_directory, "FakeMacro.FCMacro")))
            self.assertTrue(os.path.exists(macro_directory))

    def test_uninstall_calls_script(self):
        """Tests that the main uninstaller run function calls the uninstall.py script"""

        class Interceptor:
            def __init__(self):
                self.called = False
                self.args = []

            def func(self, *args):
                self.called = True
                self.args = args

        interceptor = Interceptor()
        with tempfile.TemporaryDirectory() as temp_dir:
            toplevel_path = self.setup_dummy_installation(temp_dir)
            self.test_object.run_uninstall_script = interceptor.func
            result = self.test_object.run()
            self.assertTrue(interceptor.called, "Failed to call uninstall script")

    def test_remove_extra_files_no_digest(self):
        """Tests that a lack of digest file is not an error, and nothing gets removed"""
        with tempfile.TemporaryDirectory() as temp_dir:
            self.test_object.remove_extra_files(temp_dir)  # Shouldn't throw
            self.assertTrue(os.path.exists(temp_dir))

    def test_remove_extra_files_empty_digest(self):
        """Test that an empty digest file is not an error, and nothing gets removed"""
        with tempfile.TemporaryDirectory() as temp_dir:
            with open("AM_INSTALLATION_DIGEST.txt", "w", encoding="utf-8") as f:
                f.write("")
            self.test_object.remove_extra_files(temp_dir)  # Shouldn't throw
            self.assertTrue(os.path.exists(temp_dir))

    def test_remove_extra_files_comment_only_digest(self):
        """Test that a digest file that contains only comment lines is not an error, and nothing
        gets removed"""
        with tempfile.TemporaryDirectory() as temp_dir:
            with open("AM_INSTALLATION_DIGEST.txt", "w", encoding="utf-8") as f:
                f.write("# Fake digest file for unit testing")
            self.test_object.remove_extra_files(temp_dir)  # Shouldn't throw
            self.assertTrue(os.path.exists(temp_dir))

    def test_remove_extra_files_repeated_files(self):
        """Test that a digest with the same file repeated removes it once, but doesn't error on
        later requests to remove it."""
        with tempfile.TemporaryDirectory() as temp_dir:
            toplevel_path = self.setup_dummy_installation(temp_dir)
            macro_directory = os.path.join(temp_dir, "Macros")
            self.create_fake_macro(
                macro_directory,
                "FakeMacro.FCMacro",
                os.path.join(toplevel_path, "AM_INSTALLATION_DIGEST.txt"),
            )
            self.create_fake_macro(
                macro_directory,
                "FakeMacro.FCMacro",
                os.path.join(toplevel_path, "AM_INSTALLATION_DIGEST.txt"),
            )
            self.create_fake_macro(
                macro_directory,
                "FakeMacro.FCMacro",
                os.path.join(toplevel_path, "AM_INSTALLATION_DIGEST.txt"),
            )
            self.test_object.remove_extra_files(toplevel_path)  # Shouldn't throw
            self.assertFalse(os.path.exists(os.path.join(macro_directory, "FakeMacro.FCMacro")))

    def test_remove_extra_files_normal_case(self):
        """Test that a digest that is a "normal" case removes the requested files"""
        with tempfile.TemporaryDirectory() as temp_dir:
            toplevel_path = self.setup_dummy_installation(temp_dir)
            macro_directory = os.path.join(temp_dir, "Macros")
            self.create_fake_macro(
                macro_directory,
                "FakeMacro1.FCMacro",
                os.path.join(toplevel_path, "AM_INSTALLATION_DIGEST.txt"),
            )
            self.create_fake_macro(
                macro_directory,
                "FakeMacro2.FCMacro",
                os.path.join(toplevel_path, "AM_INSTALLATION_DIGEST.txt"),
            )
            self.create_fake_macro(
                macro_directory,
                "FakeMacro3.FCMacro",
                os.path.join(toplevel_path, "AM_INSTALLATION_DIGEST.txt"),
            )

            # Make sure the setup worked as expected, otherwise the test is meaningless
            self.assertTrue(os.path.exists(os.path.join(macro_directory, "FakeMacro1.FCMacro")))
            self.assertTrue(os.path.exists(os.path.join(macro_directory, "FakeMacro2.FCMacro")))
            self.assertTrue(os.path.exists(os.path.join(macro_directory, "FakeMacro3.FCMacro")))

            self.test_object.remove_extra_files(toplevel_path)  # Shouldn't throw

            self.assertFalse(os.path.exists(os.path.join(macro_directory, "FakeMacro1.FCMacro")))
            self.assertFalse(os.path.exists(os.path.join(macro_directory, "FakeMacro2.FCMacro")))
            self.assertFalse(os.path.exists(os.path.join(macro_directory, "FakeMacro3.FCMacro")))

    def test_runs_uninstaller_script_successful(self):
        """Tests that the uninstall.py script is called"""
        with tempfile.TemporaryDirectory() as temp_dir:
            toplevel_path = self.setup_dummy_installation(temp_dir)
            with open(os.path.join(toplevel_path, "uninstall.py"), "w", encoding="utf-8") as f:
                double_escaped = temp_dir.replace("\\", "\\\\")
                f.write(
                    f"""# Mock uninstaller script
import os
path = '{double_escaped}'
with open(os.path.join(path,"RAN_UNINSTALLER.txt"),"w",encoding="utf-8") as f:
    f.write("File created by uninstall.py from unit tests")
"""
                )
            self.test_object.run_uninstall_script(toplevel_path)  # The exception does not leak out
            self.assertTrue(os.path.exists(os.path.join(temp_dir, "RAN_UNINSTALLER.txt")))

    def test_runs_uninstaller_script_failure(self):
        """Tests that exceptions in the uninstall.py script do not leak out"""
        with tempfile.TemporaryDirectory() as temp_dir:
            toplevel_path = self.setup_dummy_installation(temp_dir)
            with open(os.path.join(toplevel_path, "uninstall.py"), "w", encoding="utf-8") as f:
                f.write(
                    f"""# Mock uninstaller script
raise RuntimeError("Fake exception for unit testing")
"""
                )
            self.test_object.run_uninstall_script(toplevel_path)  # The exception does not leak out


class TestMacroUninstaller(unittest.TestCase):
    """Test class for addonmanager_uninstaller.py non-GUI functionality"""

    MODULE = "test_uninstaller"  # file name without extension

    def setUp(self):
        self.mock_addon = MockAddon()
        self.mock_addon.macro = MockMacro()
        self.test_object = MacroUninstaller(self.mock_addon)
        self.signals_caught = []

        self.test_object.finished.connect(functools.partial(self.catch_signal, "finished"))
        self.test_object.success.connect(functools.partial(self.catch_signal, "success"))
        self.test_object.failure.connect(functools.partial(self.catch_signal, "failure"))

    def tearDown(self):
        pass

    def catch_signal(self, signal_name, *_):
        """Internal use: used to catch and log any emitted signals. Not called directly."""
        self.signals_caught.append(signal_name)

    def test_remove_simple_macro(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            self.test_object.installation_location = temp_dir
            self.mock_addon.macro.install(temp_dir)
            # Make sure the setup worked, otherwise the test is meaningless
            self.assertTrue(os.path.exists(os.path.join(temp_dir, self.mock_addon.macro.filename)))
            self.test_object.run()
            self.assertFalse(os.path.exists(os.path.join(temp_dir, self.mock_addon.macro.filename)))
            self.assertNotIn("failure", self.signals_caught)
            self.assertIn("success", self.signals_caught)
            self.assertIn("finished", self.signals_caught)

    def test_remove_macro_with_icon(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            self.test_object.installation_location = temp_dir
            self.mock_addon.macro.icon = "mock_icon_test.svg"
            self.mock_addon.macro.install(temp_dir)
            # Make sure the setup worked, otherwise the test is meaningless
            self.assertTrue(os.path.exists(os.path.join(temp_dir, self.mock_addon.macro.filename)))
            self.assertTrue(os.path.exists(os.path.join(temp_dir, self.mock_addon.macro.icon)))
            self.test_object.run()
            self.assertFalse(os.path.exists(os.path.join(temp_dir, self.mock_addon.macro.filename)))
            self.assertFalse(os.path.exists(os.path.join(temp_dir, self.mock_addon.macro.icon)))
            self.assertNotIn("failure", self.signals_caught)
            self.assertIn("success", self.signals_caught)
            self.assertIn("finished", self.signals_caught)

    def test_remove_macro_with_xpm_data(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            self.test_object.installation_location = temp_dir
            self.mock_addon.macro.xpm = "/*Fake XPM data*/"
            self.mock_addon.macro.install(temp_dir)
            # Make sure the setup worked, otherwise the test is meaningless
            self.assertTrue(os.path.exists(os.path.join(temp_dir, self.mock_addon.macro.filename)))
            self.assertTrue(os.path.exists(os.path.join(temp_dir, "MockMacro_icon.xpm")))
            self.test_object.run()
            self.assertFalse(os.path.exists(os.path.join(temp_dir, self.mock_addon.macro.filename)))
            self.assertFalse(os.path.exists(os.path.join(temp_dir, "MockMacro_icon.xpm")))
            self.assertNotIn("failure", self.signals_caught)
            self.assertIn("success", self.signals_caught)
            self.assertIn("finished", self.signals_caught)

    def test_remove_macro_with_files(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            self.test_object.installation_location = temp_dir
            self.mock_addon.macro.other_files = [
                "test_file_1.txt",
                "test_file_2.FCMacro",
                "subdir/test_file_3.txt",
            ]
            self.mock_addon.macro.install(temp_dir)
            # Make sure the setup worked, otherwise the test is meaningless
            for f in self.mock_addon.macro.other_files:
                self.assertTrue(
                    os.path.exists(os.path.join(temp_dir, f)),
                    f"Expected {f} to exist, and it does not",
                )
            self.test_object.run()
            for f in self.mock_addon.macro.other_files:
                self.assertFalse(
                    os.path.exists(os.path.join(temp_dir, f)),
                    f"Expected {f} to be removed, and it was not",
                )
            self.assertFalse(
                os.path.exists(os.path.join(temp_dir, "subdir")),
                "Failed to remove empty subdirectory",
            )
            self.assertNotIn("failure", self.signals_caught)
            self.assertIn("success", self.signals_caught)
            self.assertIn("finished", self.signals_caught)

    def test_remove_nonexistent_macro(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            self.test_object.installation_location = temp_dir
            # Don't run the installer:

            self.assertFalse(os.path.exists(os.path.join(temp_dir, self.mock_addon.macro.filename)))
            self.test_object.run()  # Should not raise an exception
            self.assertFalse(os.path.exists(os.path.join(temp_dir, self.mock_addon.macro.filename)))
            self.assertNotIn("failure", self.signals_caught)
            self.assertIn("success", self.signals_caught)
            self.assertIn("finished", self.signals_caught)

    def test_remove_write_protected_macro(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            self.test_object.installation_location = temp_dir
            self.mock_addon.macro.install(temp_dir)
            # Make sure the setup worked, otherwise the test is meaningless
            f = os.path.join(temp_dir, self.mock_addon.macro.filename)
            self.assertTrue(os.path.exists(f))
            os.chmod(f, S_IREAD | S_IRGRP | S_IROTH)
            self.test_object.run()

            if os.path.exists(f):
                os.chmod(f, S_IWUSR | S_IREAD)
                self.assertNotIn("success", self.signals_caught)
                self.assertIn("failure", self.signals_caught)
            else:
                # In some cases we managed to delete it anyway:
                self.assertIn("success", self.signals_caught)
                self.assertNotIn("failure", self.signals_caught)
            self.assertIn("finished", self.signals_caught)

    def test_cleanup_directories_multiple_empty(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            empty_directories = set(["empty1", "empty2", "empty3"])
            full_paths = set()
            for directory in empty_directories:
                full_path = os.path.join(temp_dir, directory)
                os.mkdir(full_path)
                full_paths.add(full_path)

            for directory in full_paths:
                self.assertTrue(directory, "Test code failed to create {directory}")
            self.test_object._cleanup_directories(full_paths)
            for directory in full_paths:
                self.assertFalse(os.path.exists(directory))

    def test_cleanup_directories_none(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            full_paths = set()
            self.test_object._cleanup_directories(full_paths)  # Shouldn't throw

    def test_cleanup_directories_not_empty(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            empty_directories = set(["empty1", "empty2", "empty3"])
            full_paths = set()
            for directory in empty_directories:
                full_path = os.path.join(temp_dir, directory)
                os.mkdir(full_path)
                full_paths.add(full_path)
                with open(os.path.join(full_path, "test.txt"), "w", encoding="utf-8") as f:
                    f.write("Unit test dummy data\n")

            for directory in full_paths:
                self.assertTrue(directory, "Test code failed to create {directory}")
            self.test_object._cleanup_directories(full_paths)
            for directory in full_paths:
                self.assertTrue(os.path.exists(directory))
