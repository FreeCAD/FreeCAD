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

"""Contains the unit test class for addonmanager_installer.py non-GUI functionality."""

import unittest
from unittest.mock import Mock
import os
import shutil
import tempfile
from zipfile import ZipFile
import sys

sys.path.append("../../")  # So the IDE can find the imports below

import FreeCAD
from addonmanager_installer import InstallationMethod, AddonInstaller, MacroInstaller
from addonmanager_git import GitManager, initialize_git
from addonmanager_metadata import MetadataReader
from Addon import Addon
from AddonManagerTest.app.mocks import MockAddon, MockMacro


class TestAddonInstaller(unittest.TestCase):
    """Test class for addonmanager_installer.py non-GUI functionality"""

    MODULE = "test_installer"  # file name without extension

    def setUp(self):
        """Initialize data needed for all tests"""
        # self.start_time = time.perf_counter()
        self.test_data_dir = os.path.join(
            FreeCAD.getHomePath(), "Mod", "AddonManager", "AddonManagerTest", "data"
        )
        self.real_addon = Addon(
            "TestAddon",
            "https://github.com/FreeCAD/FreeCAD-addons",
            Addon.Status.NOT_INSTALLED,
            "master",
        )
        self.mock_addon = MockAddon()

    def tearDown(self):
        """Finalize the test."""
        # end_time = time.perf_counter()
        # print(f"Test '{self.id()}' ran in {end_time-self.start_time:.4f} seconds")

    def test_validate_object(self):
        """An object is valid if it has a name, url, and branch attribute."""

        AddonInstaller._validate_object(self.real_addon)  # Won't raise
        AddonInstaller._validate_object(self.mock_addon)  # Won't raise

        class NoName:
            def __init__(self):
                self.url = "https://github.com/FreeCAD/FreeCAD-addons"
                self.branch = "master"

        no_name = NoName()
        with self.assertRaises(RuntimeError):
            AddonInstaller._validate_object(no_name)

        class NoUrl:
            def __init__(self):
                self.name = "TestAddon"
                self.branch = "master"

        no_url = NoUrl()
        with self.assertRaises(RuntimeError):
            AddonInstaller._validate_object(no_url)

        class NoBranch:
            def __init__(self):
                self.name = "TestAddon"
                self.url = "https://github.com/FreeCAD/FreeCAD-addons"

        no_branch = NoBranch()
        with self.assertRaises(RuntimeError):
            AddonInstaller._validate_object(no_branch)

    def test_update_metadata(self):
        """If a metadata file exists in the installation location, it should be
        loaded."""
        addon = Mock()
        addon.name = "MockAddon"
        installer = AddonInstaller(addon, [])
        installer._update_metadata()  # Does nothing, but should not crash

        installer = AddonInstaller(self.real_addon, [])
        with tempfile.TemporaryDirectory() as temp_dir:
            installer.installation_path = temp_dir
            installer._update_metadata()
            addon_dir = os.path.join(temp_dir, self.real_addon.name)
            os.mkdir(addon_dir)
            shutil.copy(
                os.path.join(self.test_data_dir, "good_package.xml"),
                os.path.join(addon_dir, "package.xml"),
            )
            good_metadata = MetadataReader.from_file(os.path.join(addon_dir, "package.xml"))
            installer._update_metadata()
            self.assertEqual(self.real_addon.installed_version, good_metadata.version)

    def test_finalize_zip_installation_non_github(self):
        """Ensure that zip files are correctly extracted."""
        with tempfile.TemporaryDirectory() as temp_dir:
            test_simple_repo = os.path.join(self.test_data_dir, "test_simple_repo.zip")
            non_gh_mock = MockAddon()
            non_gh_mock.url = test_simple_repo
            non_gh_mock.name = "NonGitHubMock"
            installer = AddonInstaller(non_gh_mock, [])
            installer.installation_path = temp_dir
            installer._finalize_zip_installation(test_simple_repo)
            expected_location = os.path.join(temp_dir, non_gh_mock.name, "README")
            self.assertTrue(os.path.isfile(expected_location), "Non-GitHub zip extraction failed")

    def test_finalize_zip_installation_github(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            test_github_style_repo = os.path.join(self.test_data_dir, "test_github_style_repo.zip")
            self.mock_addon.url = test_github_style_repo
            self.mock_addon.branch = "master"
            installer = AddonInstaller(self.mock_addon, [])
            installer.installation_path = temp_dir
            installer._finalize_zip_installation(test_github_style_repo)
            expected_location = os.path.join(temp_dir, self.mock_addon.name, "README")
            self.assertTrue(os.path.isfile(expected_location), "GitHub zip extraction failed")

    def test_code_in_branch_subdirectory_true(self):
        """When there is a subdirectory with the branch name in it, find it"""
        installer = AddonInstaller(self.mock_addon, [])
        with tempfile.TemporaryDirectory() as temp_dir:
            os.mkdir(os.path.join(temp_dir, f"{self.mock_addon.name}-{self.mock_addon.branch}"))
            result = installer._code_in_branch_subdirectory(temp_dir)
            self.assertTrue(result, "Failed to find ZIP subdirectory")

    def test_code_in_branch_subdirectory_false(self):
        """When there is not a subdirectory with the branch name in it, don't find
        one"""
        installer = AddonInstaller(self.mock_addon, [])
        with tempfile.TemporaryDirectory() as temp_dir:
            result = installer._code_in_branch_subdirectory(temp_dir)
            self.assertFalse(result, "Found ZIP subdirectory when there was none")

    def test_code_in_branch_subdirectory_more_than_one(self):
        """When there are multiple subdirectories, never find a branch subdirectory"""
        installer = AddonInstaller(self.mock_addon, [])
        with tempfile.TemporaryDirectory() as temp_dir:
            os.mkdir(os.path.join(temp_dir, f"{self.mock_addon.name}-{self.mock_addon.branch}"))
            os.mkdir(os.path.join(temp_dir, "AnotherSubdir"))
            result = installer._code_in_branch_subdirectory(temp_dir)
            self.assertFalse(result, "Found ZIP subdirectory when there were multiple subdirs")

    def test_move_code_out_of_subdirectory(self):
        """All files are moved out and the subdirectory is deleted"""
        installer = AddonInstaller(self.mock_addon, [])
        with tempfile.TemporaryDirectory() as temp_dir:
            subdir = os.path.join(temp_dir, f"{self.mock_addon.name}-{self.mock_addon.branch}")
            os.mkdir(subdir)
            with open(os.path.join(subdir, "README.txt"), "w", encoding="utf-8") as f:
                f.write("# Test file for unit testing")
            with open(os.path.join(subdir, "AnotherFile.txt"), "w", encoding="utf-8") as f:
                f.write("# Test file for unit testing")
            installer._move_code_out_of_subdirectory(temp_dir)
            self.assertTrue(os.path.isfile(os.path.join(temp_dir, "README.txt")))
            self.assertTrue(os.path.isfile(os.path.join(temp_dir, "AnotherFile.txt")))
            self.assertFalse(os.path.isdir(subdir))

    def test_install_by_git(self):
        """Test using git to install. Depends on there being a local git
        installation: the test is skipped if there is no local git."""
        git_manager = initialize_git()
        if not git_manager:
            self.skipTest("git not found, skipping git installer tests")
            return

        # Our test git repo has to be in a zipfile, otherwise it cannot itself be
        # stored in git, since it has a .git subdirectory.
        with tempfile.TemporaryDirectory() as temp_dir:
            git_repo_zip = os.path.join(self.test_data_dir, "test_repo.zip")
            with ZipFile(git_repo_zip, "r") as zip_repo:
                zip_repo.extractall(temp_dir)

            mock_addon = MockAddon()
            mock_addon.url = os.path.join(temp_dir, "test_repo")
            mock_addon.branch = "main"
            installer = AddonInstaller(mock_addon, [])
            installer.installation_path = os.path.join(temp_dir, "installed_addon")
            installer._install_by_git()

            self.assertTrue(os.path.exists(installer.installation_path))
            addon_name_dir = os.path.join(installer.installation_path, mock_addon.name)
            self.assertTrue(os.path.exists(addon_name_dir))
            readme = os.path.join(addon_name_dir, "README.md")
            self.assertTrue(os.path.exists(readme))

    def test_install_by_copy(self):
        """Test using a simple filesystem copy to install an addon."""
        with tempfile.TemporaryDirectory() as temp_dir:
            git_repo_zip = os.path.join(self.test_data_dir, "test_repo.zip")
            with ZipFile(git_repo_zip, "r") as zip_repo:
                zip_repo.extractall(temp_dir)

            mock_addon = MockAddon()
            mock_addon.url = os.path.join(temp_dir, "test_repo")
            mock_addon.branch = "main"
            installer = AddonInstaller(mock_addon, [])
            installer.addon_to_install = mock_addon
            installer.installation_path = os.path.join(temp_dir, "installed_addon")
            installer._install_by_copy()

            self.assertTrue(os.path.exists(installer.installation_path))
            addon_name_dir = os.path.join(installer.installation_path, mock_addon.name)
            self.assertTrue(os.path.exists(addon_name_dir))
            readme = os.path.join(addon_name_dir, "README.md")
            self.assertTrue(os.path.exists(readme))

    def test_determine_install_method_local_path(self):
        """Test which install methods are accepted for a local path"""

        with tempfile.TemporaryDirectory() as temp_dir:
            installer = AddonInstaller(self.mock_addon, [])
            method = installer._determine_install_method(temp_dir, InstallationMethod.COPY)
            self.assertEqual(method, InstallationMethod.COPY)
            git_manager = initialize_git()
            if git_manager:
                method = installer._determine_install_method(temp_dir, InstallationMethod.GIT)
                self.assertEqual(method, InstallationMethod.GIT)
            method = installer._determine_install_method(temp_dir, InstallationMethod.ZIP)
            self.assertIsNone(method)
            method = installer._determine_install_method(temp_dir, InstallationMethod.ANY)
            self.assertEqual(method, InstallationMethod.COPY)

    def test_determine_install_method_file_url(self):
        """Test which install methods are accepted for a file:// url"""

        with tempfile.TemporaryDirectory() as temp_dir:
            installer = AddonInstaller(self.mock_addon, [])
            temp_dir = "file://" + temp_dir.replace(os.path.sep, "/")
            method = installer._determine_install_method(temp_dir, InstallationMethod.COPY)
            self.assertEqual(method, InstallationMethod.COPY)
            git_manager = initialize_git()
            if git_manager:
                method = installer._determine_install_method(temp_dir, InstallationMethod.GIT)
                self.assertEqual(method, InstallationMethod.GIT)
            method = installer._determine_install_method(temp_dir, InstallationMethod.ZIP)
            self.assertIsNone(method)
            method = installer._determine_install_method(temp_dir, InstallationMethod.ANY)
            self.assertEqual(method, InstallationMethod.COPY)

    def test_determine_install_method_local_zip(self):
        """Test which install methods are accepted for a local path to a zipfile"""

        with tempfile.TemporaryDirectory() as temp_dir:
            installer = AddonInstaller(self.mock_addon, [])
            temp_file = os.path.join(temp_dir, "dummy.zip")
            method = installer._determine_install_method(temp_file, InstallationMethod.COPY)
            self.assertEqual(method, InstallationMethod.ZIP)
            method = installer._determine_install_method(temp_file, InstallationMethod.GIT)
            self.assertIsNone(method)
            method = installer._determine_install_method(temp_file, InstallationMethod.ZIP)
            self.assertEqual(method, InstallationMethod.ZIP)
            method = installer._determine_install_method(temp_file, InstallationMethod.ANY)
            self.assertEqual(method, InstallationMethod.ZIP)

    def test_determine_install_method_remote_zip(self):
        """Test which install methods are accepted for a remote path to a zipfile"""

        installer = AddonInstaller(self.mock_addon, [])

        temp_file = "https://freecad.org/dummy.zip"  # Doesn't have to actually exist!

        method = installer._determine_install_method(temp_file, InstallationMethod.COPY)
        self.assertIsNone(method)
        method = installer._determine_install_method(temp_file, InstallationMethod.GIT)
        self.assertIsNone(method)
        method = installer._determine_install_method(temp_file, InstallationMethod.ZIP)
        self.assertEqual(method, InstallationMethod.ZIP)
        method = installer._determine_install_method(temp_file, InstallationMethod.ANY)
        self.assertEqual(method, InstallationMethod.ZIP)

    def test_determine_install_method_https_known_sites_copy(self):
        """Test which install methods are accepted for an https GitHub URL"""

        installer = AddonInstaller(self.mock_addon, [])
        installer.git_manager = True

        for site in ["github.org", "gitlab.org", "framagit.org", "salsa.debian.org"]:
            with self.subTest(site=site):
                temp_file = f"https://{site}/dummy/dummy"  # Doesn't have to actually exist!
                method = installer._determine_install_method(temp_file, InstallationMethod.COPY)
                self.assertIsNone(method, f"Allowed copying from {site} URL")

    def test_determine_install_method_https_known_sites_git(self):
        """Test which install methods are accepted for an https GitHub URL"""

        installer = AddonInstaller(self.mock_addon, [])
        installer.git_manager = True

        for site in ["github.org", "gitlab.org", "framagit.org", "salsa.debian.org"]:
            with self.subTest(site=site):
                temp_file = f"https://{site}/dummy/dummy"  # Doesn't have to actually exist!
                method = installer._determine_install_method(temp_file, InstallationMethod.GIT)
                self.assertEqual(
                    method,
                    InstallationMethod.GIT,
                    f"Failed to allow git access to {site} URL",
                )

    def test_determine_install_method_https_known_sites_zip(self):
        """Test which install methods are accepted for an https GitHub URL"""

        installer = AddonInstaller(self.mock_addon, [])
        installer.git_manager = True

        for site in ["github.org", "gitlab.org", "framagit.org", "salsa.debian.org"]:
            with self.subTest(site=site):
                temp_file = f"https://{site}/dummy/dummy"  # Doesn't have to actually exist!
                method = installer._determine_install_method(temp_file, InstallationMethod.ZIP)
                self.assertEqual(
                    method,
                    InstallationMethod.ZIP,
                    f"Failed to allow zip access to {site} URL",
                )

    def test_determine_install_method_https_known_sites_any_gm(self):
        """Test which install methods are accepted for an https GitHub URL"""

        installer = AddonInstaller(self.mock_addon, [])
        installer.git_manager = True

        for site in ["github.org", "gitlab.org", "framagit.org", "salsa.debian.org"]:
            with self.subTest(site=site):
                temp_file = f"https://{site}/dummy/dummy"  # Doesn't have to actually exist!
                method = installer._determine_install_method(temp_file, InstallationMethod.ANY)
                self.assertEqual(
                    method,
                    InstallationMethod.GIT,
                    f"Failed to allow git access to {site} URL",
                )

    def test_determine_install_method_https_known_sites_any_no_gm(self):
        """Test which install methods are accepted for an https GitHub URL"""

        installer = AddonInstaller(self.mock_addon, [])
        installer.git_manager = None

        for site in ["github.org", "gitlab.org", "framagit.org", "salsa.debian.org"]:
            with self.subTest(site=site):
                temp_file = f"https://{site}/dummy/dummy"  # Doesn't have to actually exist!
                method = installer._determine_install_method(temp_file, InstallationMethod.ANY)
                self.assertEqual(
                    method,
                    InstallationMethod.ZIP,
                    f"Failed to allow zip access to {site} URL",
                )

    def test_fcmacro_copying(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            mock_addon = MockAddon()
            mock_addon.url = os.path.join(self.test_data_dir, "test_addon_with_fcmacro.zip")
            installer = AddonInstaller(mock_addon, [])
            installer.installation_path = temp_dir
            installer.macro_installation_path = os.path.join(temp_dir, "Macros")
            installer.run()
            self.assertTrue(
                os.path.exists(os.path.join(temp_dir, "Macros", "TestMacro.FCMacro")),
                "FCMacro file was not copied to macro installation location",
            )


class TestMacroInstaller(unittest.TestCase):
    MODULE = "test_installer"  # file name without extension

    def setUp(self):
        """Set up the mock objects"""

        self.mock = MockAddon()
        self.mock.macro = MockMacro()

    def test_installation(self):
        """Test the wrapper around the macro installer"""

        # Note that this doesn't test the underlying Macro object's install function,
        # it only tests whether that function is called appropriately by the
        # MacroInstaller wrapper.
        with tempfile.TemporaryDirectory() as temp_dir:
            installer = MacroInstaller(self.mock)
            installer.installation_path = temp_dir
            installation_succeeded = installer.run()
            self.assertTrue(installation_succeeded)
            self.assertTrue(os.path.exists(os.path.join(temp_dir, self.mock.macro.filename)))
