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

import functools
import os
import subprocess
import tempfile
from time import sleep
import unittest

from addonmanager_dependency_installer import DependencyInstaller


class CompleteProcessMock(subprocess.CompletedProcess):
    def __init__(self):
        super().__init__(["fake_arg"], 0)
        self.stdout = "Mock subprocess call stdout result"


class SubprocessMock:
    def __init__(self):
        self.arg_log = []
        self.called = False
        self.call_count = 0
        self.delay = 0
        self.succeed = True

    def subprocess_interceptor(self, args):
        self.arg_log.append(args)
        self.called = True
        self.call_count += 1
        sleep(self.delay)
        if self.succeed:
            return CompleteProcessMock()
        raise subprocess.CalledProcessError(1, " ".join(args), "Unit test mock output")


class FakeFunction:
    def __init__(self):
        self.called = False
        self.call_count = 0
        self.return_value = None
        self.arg_log = []

    def func_call(self, *args):
        self.arg_log.append(args)
        self.called = True
        self.call_count += 1
        return self.return_value


class TestDependencyInstaller(unittest.TestCase):
    """Test the dependency installation class"""

    def setUp(self):
        self.subprocess_mock = SubprocessMock()
        self.test_object = DependencyInstaller([], ["required_py_package"], ["optional_py_package"])
        self.test_object._subprocess_wrapper = self.subprocess_mock.subprocess_interceptor
        self.signals_caught = []
        self.test_object.failure.connect(functools.partial(self.catch_signal, "failure"))
        self.test_object.finished.connect(functools.partial(self.catch_signal, "finished"))
        self.test_object.no_pip.connect(functools.partial(self.catch_signal, "no_pip"))
        self.test_object.no_python_exe.connect(
            functools.partial(self.catch_signal, "no_python_exe")
        )

    def tearDown(self):
        pass

    def catch_signal(self, signal_name, *_):
        self.signals_caught.append(signal_name)

    def test_run_no_pip(self):
        self.test_object._verify_pip = lambda: False
        self.test_object.run()
        self.assertIn("finished", self.signals_caught)

    def test_run_with_pip(self):
        ff = FakeFunction()
        self.test_object._verify_pip = lambda: True
        self.test_object._install_python_packages = ff.func_call
        self.test_object.run()
        self.assertIn("finished", self.signals_caught)
        self.assertTrue(ff.called)

    def test_run_with_no_packages(self):
        ff = FakeFunction()
        self.test_object._verify_pip = lambda: True
        self.test_object._install_python_packages = ff.func_call
        self.test_object.python_requires = []
        self.test_object.python_optional = []
        self.test_object.run()
        self.assertIn("finished", self.signals_caught)
        self.assertFalse(ff.called)

    def test_install_python_packages_new_location(self):
        ff_required = FakeFunction()
        ff_optional = FakeFunction()
        self.test_object._install_required = ff_required.func_call
        self.test_object._install_optional = ff_optional.func_call
        with tempfile.TemporaryDirectory() as td:
            self.test_object.location = os.path.join(td, "UnitTestLocation")
            self.test_object._install_python_packages()
            self.assertTrue(ff_required.called)
            self.assertTrue(ff_optional.called)
            self.assertTrue(os.path.exists(self.test_object.location))

    def test_install_python_packages_existing_location(self):
        ff_required = FakeFunction()
        ff_optional = FakeFunction()
        self.test_object._install_required = ff_required.func_call
        self.test_object._install_optional = ff_optional.func_call
        with tempfile.TemporaryDirectory() as td:
            self.test_object.location = td
            self.test_object._install_python_packages()
            self.assertTrue(ff_required.called)
            self.assertTrue(ff_optional.called)

    def test_verify_pip_no_python(self):
        self.test_object._get_python = lambda: None
        should_continue = self.test_object._verify_pip()
        self.assertFalse(should_continue)
        self.assertEqual(len(self.signals_caught), 0)

    def test_verify_pip_no_pip(self):
        sm = SubprocessMock()
        sm.succeed = False
        self.test_object._subprocess_wrapper = sm.subprocess_interceptor
        self.test_object._get_python = lambda: "fake_python"
        result = self.test_object._verify_pip()
        self.assertFalse(result)
        self.assertIn("no_pip", self.signals_caught)

    def test_verify_pip_with_pip(self):
        sm = SubprocessMock()
        sm.succeed = True
        self.test_object._subprocess_wrapper = sm.subprocess_interceptor
        self.test_object._get_python = lambda: "fake_python"
        result = self.test_object._verify_pip()
        self.assertTrue(result)
        self.assertNotIn("no_pip", self.signals_caught)

    def test_install_required_loops(self):
        sm = SubprocessMock()
        sm.succeed = True
        self.test_object._subprocess_wrapper = sm.subprocess_interceptor
        self.test_object._get_python = lambda: "fake_python"
        self.test_object.python_requires = ["test1", "test2", "test3"]
        self.test_object._install_required("vendor_path")
        self.assertEqual(sm.call_count, 3)

    def test_install_required_failure(self):
        sm = SubprocessMock()
        sm.succeed = False
        self.test_object._subprocess_wrapper = sm.subprocess_interceptor
        self.test_object._get_python = lambda: "fake_python"
        self.test_object.python_requires = ["test1", "test2", "test3"]
        self.test_object._install_required("vendor_path")
        self.assertEqual(sm.call_count, 1)
        self.assertIn("failure", self.signals_caught)

    def test_install_optional_loops(self):
        sm = SubprocessMock()
        sm.succeed = True
        self.test_object._subprocess_wrapper = sm.subprocess_interceptor
        self.test_object._get_python = lambda: "fake_python"
        self.test_object.python_optional = ["test1", "test2", "test3"]
        self.test_object._install_optional("vendor_path")
        self.assertEqual(sm.call_count, 3)

    def test_install_optional_failure(self):
        sm = SubprocessMock()
        sm.succeed = False
        self.test_object._subprocess_wrapper = sm.subprocess_interceptor
        self.test_object._get_python = lambda: "fake_python"
        self.test_object.python_optional = ["test1", "test2", "test3"]
        self.test_object._install_optional("vendor_path")
        self.assertEqual(sm.call_count, 3)

    def test_run_pip(self):
        pass
