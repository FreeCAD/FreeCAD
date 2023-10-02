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


import unittest
import os
import shutil
import stat
import tempfile
import time
from zipfile import ZipFile
import FreeCAD

from addonmanager_git import GitManager, NoGitFound, GitFailed


class TestGit(unittest.TestCase):

    MODULE = "test_git"  # file name without extension

    def setUp(self):
        """Set up the test case: called by the unit test system"""
        self.cwd = os.getcwd()
        test_data_dir = os.path.join(
            FreeCAD.getHomePath(), "Mod", "AddonManager", "AddonManagerTest", "data"
        )
        git_repo_zip = os.path.join(test_data_dir, "test_repo.zip")
        self.test_dir = os.path.join(
            tempfile.gettempdir(), "FreeCADTesting", "AddonManagerTests", "Git"
        )
        os.makedirs(self.test_dir, exist_ok=True)
        self.test_repo_remote = os.path.join(self.test_dir, "TEST_REPO_REMOTE")
        if os.path.exists(self.test_repo_remote):
            # Make sure any old copy that got left around is deleted
            self._rmdir(self.test_repo_remote)

        if not os.path.exists(git_repo_zip):
            self.skipTest("Can't find test repo")
            return

        with ZipFile(git_repo_zip, "r") as zip_repo:
            zip_repo.extractall(self.test_repo_remote)
        self.test_repo_remote = os.path.join(self.test_repo_remote, "test_repo")

        try:
            self.git = GitManager()
        except NoGitFound:
            self.skipTest("No git found")

    def tearDown(self):
        """Clean up after the test"""
        os.chdir(self.cwd)
        # self._rmdir(self.test_dir)
        os.rename(self.test_dir, self.test_dir + ".old." + str(time.time()))

    def test_clone(self):
        """Test git clone"""
        checkout_dir = self._clone_test_repo()
        self.assertTrue(os.path.exists(checkout_dir))
        self.assertTrue(os.path.exists(os.path.join(checkout_dir, ".git")))
        self.assertEqual(os.getcwd(), self.cwd, "We should be left in the same CWD we started")

    def test_checkout(self):
        """Test git checkout"""
        checkout_dir = self._clone_test_repo()

        self.git.checkout(checkout_dir, "HEAD~1")
        status = self.git.status(checkout_dir).strip()
        expected_status = "## HEAD (no branch)"
        self.assertEqual(status, expected_status)

        self.assertEqual(os.getcwd(), self.cwd, "We should be left in the same CWD we started")

    def test_update(self):
        """Test using git to update the local repo"""
        checkout_dir = self._clone_test_repo()

        self.git.reset(checkout_dir, ["--hard", "HEAD~1"])
        self.assertTrue(self.git.update_available(checkout_dir))
        self.git.update(checkout_dir)
        self.assertFalse(self.git.update_available(checkout_dir))
        self.assertEqual(os.getcwd(), self.cwd, "We should be left in the same CWD we started")

    def test_tag_and_branch(self):
        """Test checking the currently checked-out tag"""
        checkout_dir = self._clone_test_repo()

        expected_tag = "TestTag"
        self.git.checkout(checkout_dir, expected_tag)
        found_tag = self.git.current_tag(checkout_dir)
        self.assertEqual(found_tag, expected_tag)
        self.assertFalse(self.git.update_available(checkout_dir))

        expected_branch = "TestBranch"
        self.git.checkout(checkout_dir, expected_branch)
        found_branch = self.git.current_branch(checkout_dir)
        self.assertEqual(found_branch, expected_branch)
        self.assertFalse(self.git.update_available(checkout_dir))

        expected_branch = "main"
        self.git.checkout(checkout_dir, expected_branch)
        found_branch = self.git.current_branch(checkout_dir)
        self.assertEqual(found_branch, expected_branch)
        self.assertFalse(self.git.update_available(checkout_dir))

        self.assertEqual(os.getcwd(), self.cwd, "We should be left in the same CWD we started")

    def test_get_remote(self):
        """Test getting the remote location"""
        checkout_dir = self._clone_test_repo()
        expected_remote = self.test_repo_remote
        returned_remote = self.git.get_remote(checkout_dir)
        self.assertEqual(expected_remote, returned_remote)
        self.assertEqual(os.getcwd(), self.cwd, "We should be left in the same CWD we started")

    def test_repair(self):
        """Test the repair feature (and some exception throwing)"""
        checkout_dir = self._clone_test_repo()
        remote = self.git.get_remote(checkout_dir)
        git_dir = os.path.join(checkout_dir, ".git")
        self.assertTrue(os.path.exists(git_dir))
        self._rmdir(git_dir)

        # Make sure that we've truly broken the install
        with self.assertRaises(GitFailed):
            self.git.status(checkout_dir)

        self.git.repair(remote, checkout_dir)
        status = self.git.status(checkout_dir)
        self.assertEqual(status, "## main...origin/main\n")
        self.assertEqual(os.getcwd(), self.cwd, "We should be left in the same CWD we started")

    def _rmdir(self, path):
        try:
            shutil.rmtree(path, onerror=self._remove_readonly)
        except Exception as e:
            print(e)

    def _remove_readonly(self, func, path, _) -> None:
        """Remove a read-only file."""

        os.chmod(path, stat.S_IWRITE)
        func(path)

    def _clone_test_repo(self) -> str:
        checkout_dir = os.path.join(self.test_dir, "test_repo")
        try:
            # Git won't clone to an existing directory, so make sure to remove it first
            if os.path.exists(checkout_dir):
                self._rmdir(checkout_dir)
            self.git.clone(self.test_repo_remote, checkout_dir)
        except GitFailed as e:
            self.fail(str(e))
        return checkout_dir
