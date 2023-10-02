# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2023 FreeCAD Project Association                   *
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

"""Mock objects for use when testing the addon manager non-GUI code."""

# pylint: disable=too-few-public-methods,too-many-instance-attributes,missing-function-docstring

import os
from typing import Union, List
import xml.etree.ElementTree as ElemTree


class GitFailed(RuntimeError):
    pass


class MockConsole:
    """Spy for the FreeCAD.Console -- does NOT print anything out, just logs it."""

    def __init__(self):
        self.log = []
        self.messages = []
        self.warnings = []
        self.errors = []

    def PrintLog(self, data: str):
        self.log.append(data)

    def PrintMessage(self, data: str):
        self.messages.append(data)

    def PrintWarning(self, data: str):
        self.warnings.append(data)

    def PrintError(self, data: str):
        self.errors.append(data)

    def missing_newlines(self) -> int:
        """In most cases, all console entries should end with newlines: this is a
        convenience function for unit testing that is true."""
        counter = 0
        counter += self._count_missing_newlines(self.log)
        counter += self._count_missing_newlines(self.messages)
        counter += self._count_missing_newlines(self.warnings)
        counter += self._count_missing_newlines(self.errors)
        return counter

    @staticmethod
    def _count_missing_newlines(some_list) -> int:
        counter = 0
        for line in some_list:
            if line[-1] != "\n":
                counter += 1
        return counter


class MockAddon:
    """Minimal Addon class"""

    # pylint: disable=too-many-instance-attributes

    def __init__(
        self,
        name: str = None,
        url: str = None,
        status: object = None,
        branch: str = "main",
    ):
        test_dir = os.path.join(os.path.dirname(__file__), "..", "data")
        if name:
            self.name = name
            self.display_name = name
        else:
            self.name = "MockAddon"
            self.display_name = "Mock Addon"
        self.url = url if url else os.path.join(test_dir, "test_simple_repo.zip")
        self.branch = branch
        self.status = status
        self.macro = None
        self.update_status = None
        self.metadata = None
        self.icon_file = None
        self.last_updated = None
        self.requires = set()
        self.python_requires = set()
        self.python_optional = set()
        self.on_git = False
        self.on_wiki = True

    def set_status(self, status):
        self.update_status = status

    @staticmethod
    def get_best_icon_relative_path():
        return ""


class MockMacro:
    """Minimal Macro class"""

    def __init__(self, name="MockMacro"):
        self.name = name
        self.filename = self.name + ".FCMacro"
        self.icon = ""  # If set, should just be fake filename, doesn't have to exist
        self.xpm = ""
        self.code = ""
        self.raw_code_url = ""
        self.other_files = []  # If set, should be fake names, don't have to exist
        self.details_filled_from_file = False
        self.details_filled_from_code = False
        self.parsed_wiki_page = False
        self.on_git = False
        self.on_wiki = True

    def install(self, location: os.PathLike):
        """Installer function for the mock macro object: creates a file with the src_filename
        attribute, and optionally an icon, xpm, and other_files. The data contained in these files
        is not usable and serves only as a placeholder for the existence of the files.
        """

        with open(
            os.path.join(location, self.filename),
            "w",
            encoding="utf-8",
        ) as f:
            f.write("Test file for macro installation unit tests")
        if self.icon:
            with open(os.path.join(location, self.icon), "wb") as f:
                f.write(b"Fake icon data - nothing to see here\n")
        if self.xpm:
            with open(os.path.join(location, "MockMacro_icon.xpm"), "w", encoding="utf-8") as f:
                f.write(self.xpm)
        for name in self.other_files:
            if "/" in name:
                new_location = os.path.dirname(os.path.join(location, name))
                os.makedirs(new_location, exist_ok=True)
            with open(os.path.join(location, name), "w", encoding="utf-8") as f:
                f.write("# Fake macro data for unit testing\n")
        return True, []

    def fill_details_from_file(self, _):
        """Tracks that this function was called, but otherwise does nothing"""
        self.details_filled_from_file = True

    def fill_details_from_code(self, _):
        self.details_filled_from_code = True

    def parse_wiki_page(self, _):
        self.parsed_wiki_page = True


class SignalCatcher:
    """Object to track signals that it has caught.

    Usage:
    catcher = SignalCatcher()
    my_signal.connect(catcher.catch_signal)
    do_things_that_emit_the_signal()
    self.assertTrue(catcher.caught)
    """

    def __init__(self):
        self.caught = False
        self.killed = False
        self.args = None

    def catch_signal(self, *args):
        self.caught = True
        self.args = args

    def die(self):
        self.killed = True


class AddonSignalCatcher:
    """Signal catcher specifically designed for catching emitted addons."""

    def __init__(self):
        self.addons = []

    def catch_signal(self, addon):
        self.addons.append(addon)


class CallCatcher:
    """Generic call monitor -- use to override functions that are not themselves under
    test so that you can detect when the function has been called, and how many times.
    """

    def __init__(self):
        self.called = False
        self.call_count = 0
        self.args = None

    def catch_call(self, *args):
        self.called = True
        self.call_count += 1
        self.args = args


class MockGitManager:
    """A mock git manager: does NOT require a git installation. Takes no actions, only records
    which functions are called for instrumentation purposes. Can be forced to appear to fail as
    needed. Various member variables can be set to emulate necessary return responses.
    """

    def __init__(self):
        self.called_methods = []
        self.update_available_response = False
        self.current_tag_response = "main"
        self.current_branch_response = "main"
        self.get_remote_response = "No remote set"
        self.get_branches_response = ["main"]
        self.get_last_committers_response = {"John Doe": {"email": "jdoe@freecad.org", "count": 1}}
        self.get_last_authors_response = {"Jane Doe": {"email": "jdoe@freecad.org", "count": 1}}
        self.should_fail = False
        self.fail_once = False  # Switch back to success after the simulated failure

    def _check_for_failure(self):
        if self.should_fail:
            if self.fail_once:
                self.should_fail = False
            raise GitFailed("Unit test forced failure")

    def clone(self, _remote, _local_path, _args: List[str] = None):
        self.called_methods.append("clone")
        self._check_for_failure()

    def async_clone(self, _remote, _local_path, _progress_monitor, _args: List[str] = None):
        self.called_methods.append("async_clone")
        self._check_for_failure()

    def checkout(self, _local_path, _spec, _args: List[str] = None):
        self.called_methods.append("checkout")
        self._check_for_failure()

    def update(self, _local_path):
        self.called_methods.append("update")
        self._check_for_failure()

    def status(self, _local_path) -> str:
        self.called_methods.append("status")
        self._check_for_failure()
        return "Up-to-date"

    def reset(self, _local_path, _args: List[str] = None):
        self.called_methods.append("reset")
        self._check_for_failure()

    def async_fetch_and_update(self, _local_path, _progress_monitor, _args=None):
        self.called_methods.append("async_fetch_and_update")
        self._check_for_failure()

    def update_available(self, _local_path) -> bool:
        self.called_methods.append("update_available")
        self._check_for_failure()
        return self.update_available_response

    def current_tag(self, _local_path) -> str:
        self.called_methods.append("current_tag")
        self._check_for_failure()
        return self.current_tag_response

    def current_branch(self, _local_path) -> str:
        self.called_methods.append("current_branch")
        self._check_for_failure()
        return self.current_branch_response

    def repair(self, _remote, _local_path):
        self.called_methods.append("repair")
        self._check_for_failure()

    def get_remote(self, _local_path) -> str:
        self.called_methods.append("get_remote")
        self._check_for_failure()
        return self.get_remote_response

    def get_branches(self, _local_path) -> List[str]:
        self.called_methods.append("get_branches")
        self._check_for_failure()
        return self.get_branches_response

    def get_last_committers(self, _local_path, _n=10):
        self.called_methods.append("get_last_committers")
        self._check_for_failure()
        return self.get_last_committers_response

    def get_last_authors(self, _local_path, _n=10):
        self.called_methods.append("get_last_authors")
        self._check_for_failure()
        return self.get_last_authors_response


class MockSignal:
    """A purely synchronous signal, instrumented and intended only for use in unit testing.
    emit() is semi-functional, but does not use queued slots so cannot be used across
    threads."""

    def __init__(self, *args):
        self.expected_types = args
        self.connections = []
        self.disconnections = []
        self.emitted = False

    def connect(self, func):
        self.connections.append(func)

    def disconnect(self, func):
        if func in self.connections:
            self.connections.remove(func)
        self.disconnections.append(func)

    def emit(self, *args):
        self.emitted = True
        for connection in self.connections:
            connection(args)


class MockNetworkManager:
    """Instrumented mock for the NetworkManager. Does no network access, is not asynchronous, and
    does not require a running event loop. No submitted requests ever complete."""

    def __init__(self):
        self.urls = []
        self.aborted = []
        self.data = MockByteArray()
        self.called_methods = []

        self.completed = MockSignal(int, int, MockByteArray)
        self.progress_made = MockSignal(int, int, int)
        self.progress_complete = MockSignal(int, int, os.PathLike)

    def submit_unmonitored_get(self, url: str) -> int:
        self.urls.append(url)
        self.called_methods.append("submit_unmonitored_get")
        return len(self.urls) - 1

    def submit_monitored_get(self, url: str) -> int:
        self.urls.append(url)
        self.called_methods.append("submit_monitored_get")
        return len(self.urls) - 1

    def blocking_get(self, url: str):
        self.urls.append(url)
        self.called_methods.append("blocking_get")
        return self.data

    def abort_all(self):
        self.called_methods.append("abort_all")
        for url in self.urls:
            self.aborted.append(url)

    def abort(self, index: int):
        self.called_methods.append("abort")
        self.aborted.append(self.urls[index])


class MockByteArray:
    """Mock for QByteArray. Only provides the data() access member."""

    def __init__(self, data_to_wrap="data".encode("utf-8")):
        self.wrapped = data_to_wrap

    def data(self) -> bytes:
        return self.wrapped


class MockThread:
    """Mock for QThread for use when threading is not being used, but interruption
    needs to be tested. Set interrupt_after_n_calls to the call number to stop at."""

    def __init__(self):
        self.interrupt_after_n_calls = 0
        self.interrupt_check_counter = 0

    def isInterruptionRequested(self):
        self.interrupt_check_counter += 1
        if (
            self.interrupt_after_n_calls
            and self.interrupt_check_counter >= self.interrupt_after_n_calls
        ):
            return True
        return False


class MockPref:
    def __init__(self):
        self.prefs = {}
        self.pref_set_counter = {}
        self.pref_get_counter = {}

    def set_prefs(self, pref_dict: dict) -> None:
        self.prefs = pref_dict

    def GetInt(self, key: str, default: int) -> int:
        return self.Get(key, default)

    def GetString(self, key: str, default: str) -> str:
        return self.Get(key, default)

    def GetBool(self, key: str, default: bool) -> bool:
        return self.Get(key, default)

    def Get(self, key: str, default):
        if key not in self.pref_set_counter:
            self.pref_get_counter[key] = 1
        else:
            self.pref_get_counter[key] += 1
        if key in self.prefs:
            return self.prefs[key]
        raise ValueError(f"Expected key not in mock preferences: {key}")

    def SetInt(self, key: str, value: int) -> None:
        return self.Set(key, value)

    def SetString(self, key: str, value: str) -> None:
        return self.Set(key, value)

    def SetBool(self, key: str, value: bool) -> None:
        return self.Set(key, value)

    def Set(self, key: str, value):
        if key not in self.pref_set_counter:
            self.pref_set_counter[key] = 1
        else:
            self.pref_set_counter[key] += 1
        self.prefs[key] = value


class MockExists:
    def __init__(self, files: List[str] = None):
        """Returns True for all files in files, and False for all others"""
        self.files = files
        self.files_checked = []

    def exists(self, check_file: str):
        self.files_checked.append(check_file)
        if not self.files:
            return False
        for file in self.files:
            if check_file.endswith(file):
                return True
        return False
