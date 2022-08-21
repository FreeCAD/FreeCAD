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

""" Wrapper around git executable to simplify calling git commands from Python. """

# pylint: disable=too-few-public-methods

import os
import platform
import shutil
import subprocess
from typing import List
import time
import FreeCAD

translate = FreeCAD.Qt.translate


def initialize_git() -> object:
    """If git is enabled, locate the git executable if necessary and return a new
    GitManager object. The executable location is saved in user preferences for reuse,
    and git can be disabled by setting the disableGit parameter in the Addons
    preference group. Returns None if for any of those reasons we aren't using git."""

    git_manager = None
    pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
    disable_git = pref.GetBool("disableGit", False)
    if not disable_git:
        try:
            git_manager = GitManager()
        except NoGitFound:
            pass
    return git_manager


class NoGitFound(RuntimeError):
    """Could not locate the git executable on this system."""


class GitFailed(RuntimeError):
    """The call to git returned an error of some kind"""


class GitManager:
    """A class to manage access to git: mostly just provides a simple wrapper around the basic
    command-line calls. Provides optional asynchronous access to clone and update."""

    def __init__(self):
        self.git_exe = None
        self._find_git()
        if not self.git_exe:
            raise NoGitFound()

    def clone(self, remote, local_path, args: List[str] = None):
        """Clones the remote to the local path"""
        final_args = ["clone", "--recurse-submodules"]
        if args:
            final_args.extend(args)
        final_args.extend([remote, local_path])
        self._synchronous_call_git(final_args)

    def async_clone(self, remote, local_path, progress_monitor, args: List[str] = None):
        """Clones the remote to the local path, sending periodic progress updates
        to the passed progress_monitor. Returns a handle that can be used to
        cancel the job."""

    def checkout(self, local_path, spec, args: List[str] = None):
        """Checks out a specific git revision, tag, or branch. Any valid argument to
        git checkout can be submitted."""
        old_dir = os.getcwd()
        os.chdir(local_path)
        final_args = ["checkout"]
        if args:
            final_args.extend(args)
        final_args.append(spec)
        self._synchronous_call_git(final_args)
        os.chdir(old_dir)

    def update(self, local_path):
        """Fetches and pulls the local_path from its remote"""
        old_dir = os.getcwd()
        os.chdir(local_path)
        self._synchronous_call_git(["fetch"])
        try:
            self._synchronous_call_git(["pull"])
            self._synchronous_call_git(["submodule", "update", "--init", "--recursive"])
        except GitFailed as e:
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Basic git update failed with the following message:",
                )
                + str(e)
                + "\n"
            )
            FreeCAD.Console.PrintWarning(
                translate(
                    "AddonsInstaller",
                    "Backing up the original directory and re-cloning",
                )
                + "...\n"
            )
            remote = self.get_remote(local_path)
            with open(os.path.join(local_path, "ADDON_DISABLED"), "w") as f:
                f.write(
                    "This is a backup of an addon that failed to update cleanly so was re-cloned. "
                    + "It was disabled by the Addon Manager's git update facility and can be safely "
                    + "deleted if the addon is working properly."
                )
            os.chdir("..")
            os.rename(local_path, local_path + ".backup" + str(time.time()))
            self.clone(remote, local_path)
        os.chdir(old_dir)

    def status(self, local_path) -> str:
        """Gets the v1 porcelain status"""
        old_dir = os.getcwd()
        os.chdir(local_path)
        try:
            status = self._synchronous_call_git(["status", "-sb", "--porcelain"])
        except GitFailed as e:
            os.chdir(old_dir)
            raise e

        os.chdir(old_dir)
        return status

    def reset(self, local_path, args: List[str] = None):
        """Executes the git reset command"""
        old_dir = os.getcwd()
        os.chdir(local_path)
        final_args = ["reset"]
        if args:
            final_args.extend(args)
        try:
            self._synchronous_call_git(final_args)
        except GitFailed as e:
            os.chdir(old_dir)
            raise e
        os.chdir(old_dir)

    def async_fetch_and_update(self, local_path, progress_monitor, args=None):
        """Same as fetch_and_update, but asynchronous"""

    def update_available(self, local_path) -> bool:
        """Returns True if an update is available from the remote, or false if not"""
        old_dir = os.getcwd()
        os.chdir(local_path)
        try:
            self._synchronous_call_git(["fetch"])
            status = self._synchronous_call_git(["status", "-sb", "--porcelain"])
        except GitFailed as e:
            os.chdir(old_dir)
            raise e
        os.chdir(old_dir)
        return "behind" in status

    def current_tag(self, local_path) -> str:
        """Get the name of the currently checked-out tag if HEAD is detached"""
        old_dir = os.getcwd()
        os.chdir(local_path)
        try:
            tag = self._synchronous_call_git(["describe", "--tags"]).strip()
        except GitFailed as e:
            os.chdir(old_dir)
            raise e
        os.chdir(old_dir)
        return tag

    def current_branch(self, local_path) -> str:
        """Get the name of the current branch"""
        old_dir = os.getcwd()
        os.chdir(local_path)
        try:
            branch = self._synchronous_call_git(["branch", "--show-current"]).strip()
        except GitFailed as e:
            os.chdir(old_dir)
            raise e
        os.chdir(old_dir)
        return branch

    def repair(self, remote, local_path):
        """Assumes that local_path is supposed to be a local clone of the given remote, and
        ensures that it is. Note that any local changes in local_path will be destroyed. This
        is achieved by archiving the old path, cloning an entirely new copy, and then deleting
        the old directory."""

        original_cwd = os.getcwd()

        # Make sure we are not currently in that directory, otherwise on Windows the rename
        # will fail. To guarantee we aren't in it, change to it, then shift up one.
        os.chdir(local_path)
        os.chdir("..")
        backup_path = local_path + ".backup" + str(time.time())
        os.rename(local_path, backup_path)
        try:
            self.clone(remote, local_path)
        except GitFailed as e:
            FreeCAD.Console.PrintError(
                translate(
                    "AddonsInstaller", "Failed to clone {} into {} using git"
                ).format(remote, local_path)
            )
            os.chdir(original_cwd)
            raise e
        os.chdir(original_cwd)

    def get_remote(self, local_path) -> str:
        """Get the repository that this local path is set to fetch from"""
        old_dir = os.getcwd()
        os.chdir(local_path)
        try:
            response = self._synchronous_call_git(["remote", "-v", "show"])
        except GitFailed as e:
            os.chdir(old_dir)
            raise e
        lines = response.split("\n")
        result = "(unknown remote)"
        for line in lines:
            if line.endswith("(fetch)"):

                # The line looks like:
                # origin  https://some/sort/of/path (fetch)

                segments = line.split()
                if len(segments) == 3:
                    result = segments[1]
                    break
                else:
                    FreeCAD.Console.PrintWarning(
                        "Error parsing the results from git remote -v show:\n"
                    )
                    FreeCAD.Console.PrintWarning(line + "\n")
        os.chdir(old_dir)
        return result

    def _find_git(self):
        # Find git. In preference order
        #   A) The value of the GitExecutable user preference
        #   B) The executable located in the same bin directory as FreeCAD and called "git"
        #   C) The result of an shutil search for your system's "git" executable
        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
        git_exe = prefs.GetString("GitExecutable", "Not set")
        if not git_exe or git_exe == "Not set" or not os.path.exists(git_exe):
            fc_dir = FreeCAD.getHomePath()
            git_exe = os.path.join(fc_dir, "bin", "git")
            if "Windows" in platform.system():
                git_exe += ".exe"

        if not git_exe or not os.path.exists(git_exe):
            git_exe = shutil.which("git")

        if not git_exe or not os.path.exists(git_exe):
            return

        prefs.SetString("GitExecutable", git_exe)
        self.git_exe = git_exe

    def _synchronous_call_git(self, args: List[str]) -> str:
        """Calls git and returns its output."""
        final_args = [self.git_exe]
        final_args.extend(args)
        try:
            proc = subprocess.run(
                final_args,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                shell=True,
                check=True,
            )
        except subprocess.CalledProcessError as e:
            raise GitFailed(str(e)) from e

        if proc.returncode != 0:
            raise GitFailed(
                f"Git returned a non-zero exit status: {proc.returncode}\n"
                + f"Called with: {' '.join(final_args)}\n\n"
                + f"Returned stderr:\n{proc.stderr.decode()}"
            )

        return proc.stdout.decode()
