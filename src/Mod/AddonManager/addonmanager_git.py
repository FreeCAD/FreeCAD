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
import FreeCAD


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
        self._synchronous_call_git(["pull"])
        self._synchronous_call_git(["submodule", "update", "--init", "--recursive"])
        os.chdir(old_dir)

    def status(self, local_path) -> str:
        """Gets the v1 porcelain status"""
        old_dir = os.getcwd()
        os.chdir(local_path)
        status = self._synchronous_call_git(["status", "-sb", "--porcelain"])
        os.chdir(old_dir)
        return status

    def reset(self, local_path, args: List[str] = None):
        """Executes the git reset command"""
        old_dir = os.getcwd()
        os.chdir(local_path)
        final_args = ["reset"]
        if args:
            final_args.extend(args)
        self._synchronous_call_git(final_args)
        os.chdir(old_dir)

    def async_fetch_and_update(self, local_path, progress_monitor, args=None):
        """Same as fetch_and_update, but asynchronous"""

    def update_available(self, local_path) -> bool:
        """Returns True if an update is available from the remote, or false if not"""
        old_dir = os.getcwd()
        os.chdir(local_path)
        self._synchronous_call_git(["fetch"])
        status = self._synchronous_call_git(["status", "-sb", "--porcelain"])
        os.chdir(old_dir)
        return "behind" in status

    def current_tag(self, local_path) -> str:
        """Get the name of the currently checked-out tag if HEAD is detached"""
        old_dir = os.getcwd()
        os.chdir(local_path)
        tag = self._synchronous_call_git(["describe", "--tags"]).strip()
        os.chdir(old_dir)
        return tag

    def current_branch(self, local_path) -> str:
        """Get the name of the current branch"""
        old_dir = os.getcwd()
        os.chdir(local_path)
        branch = self._synchronous_call_git(["branch", "--show-current"]).strip()
        os.chdir(old_dir)
        return branch

    def repair(self, remote, local_path):
        """Assumes that local_path is supposed to be a local clone of the given remote, and
        ensures that it is. Note that any local changes in local_path will be destroyed. This
        is achieved by archiving the old path, cloning an entirely new copy, and then deleting
        the old directory."""
        old_path = local_path + ".bak"
        os.rename(local_path, old_path)
        try:
            self.clone(remote, local_path)
        except GitFailed as e:
            shutil.rmtree(local_path)
            os.rename(old_path, local_path)
            raise e

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
