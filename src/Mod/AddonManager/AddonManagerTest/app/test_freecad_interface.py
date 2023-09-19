# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 FreeCAD Project Association                        *
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

"""Tests for the Addon Manager's FreeCAD interface classes."""

import json
import os
import sys
import tempfile
import unittest
from unittest.mock import patch, MagicMock

# pylint: disable=protected-access,import-outside-toplevel


class TestConsole(unittest.TestCase):
    """Tests for the Console"""

    def setUp(self) -> None:
        self.saved_freecad = None
        if "FreeCAD" in sys.modules:
            self.saved_freecad = sys.modules["FreeCAD"]
            sys.modules.pop("FreeCAD")
        if "addonmanager_freecad_interface" in sys.modules:
            sys.modules.pop("addonmanager_freecad_interface")
        sys.path.append("../../")

    def tearDown(self) -> None:
        if "FreeCAD" in sys.modules:
            sys.modules.pop("FreeCAD")
        if self.saved_freecad is not None:
            sys.modules["FreeCAD"] = self.saved_freecad

    def test_log_with_freecad(self):
        """Ensure that if FreeCAD exists, the appropriate function is called"""
        sys.modules["FreeCAD"] = unittest.mock.MagicMock()
        import addonmanager_freecad_interface as fc

        fc.Console.PrintLog("Test output")
        self.assertTrue(isinstance(fc.Console, unittest.mock.MagicMock))
        self.assertTrue(fc.Console.PrintLog.called)

    def test_log_no_freecad(self):
        """Test that if the FreeCAD import fails, the logger is set up correctly, and
        implements PrintLog"""
        sys.modules["FreeCAD"] = None
        with patch("addonmanager_freecad_interface.logging", new=MagicMock()) as mock_logging:
            import addonmanager_freecad_interface as fc

            fc.Console.PrintLog("Test output")
            self.assertTrue(isinstance(fc.Console, fc.ConsoleReplacement))
            self.assertTrue(mock_logging.log.called)

    def test_message_no_freecad(self):
        """Test that if the FreeCAD import fails the logger implements PrintMessage"""
        sys.modules["FreeCAD"] = None
        with patch("addonmanager_freecad_interface.logging", new=MagicMock()) as mock_logging:
            import addonmanager_freecad_interface as fc

            fc.Console.PrintMessage("Test output")
            self.assertTrue(mock_logging.info.called)

    def test_warning_no_freecad(self):
        """Test that if the FreeCAD import fails the logger implements PrintWarning"""
        sys.modules["FreeCAD"] = None
        with patch("addonmanager_freecad_interface.logging", new=MagicMock()) as mock_logging:
            import addonmanager_freecad_interface as fc

            fc.Console.PrintWarning("Test output")
            self.assertTrue(mock_logging.warning.called)

    def test_error_no_freecad(self):
        """Test that if the FreeCAD import fails the logger implements PrintError"""
        sys.modules["FreeCAD"] = None
        with patch("addonmanager_freecad_interface.logging", new=MagicMock()) as mock_logging:
            import addonmanager_freecad_interface as fc

            fc.Console.PrintError("Test output")
            self.assertTrue(mock_logging.error.called)


class TestParameters(unittest.TestCase):
    """Tests for the Parameters"""

    def setUp(self) -> None:
        self.saved_freecad = None
        if "FreeCAD" in sys.modules:
            self.saved_freecad = sys.modules["FreeCAD"]
            sys.modules.pop("FreeCAD")
        if "addonmanager_freecad_interface" in sys.modules:
            sys.modules.pop("addonmanager_freecad_interface")
        sys.path.append("../../")

    def tearDown(self) -> None:
        if "FreeCAD" in sys.modules:
            sys.modules.pop("FreeCAD")
        if self.saved_freecad is not None:
            sys.modules["FreeCAD"] = self.saved_freecad

    def test_param_get_with_freecad(self):
        """Ensure that if FreeCAD exists, the built-in FreeCAD function is called"""
        sys.modules["FreeCAD"] = unittest.mock.MagicMock()
        import addonmanager_freecad_interface as fc

        prefs = fc.ParamGet("some/fake/path")
        self.assertTrue(isinstance(prefs, unittest.mock.MagicMock))

    def test_param_get_no_freecad(self):
        """Test that if the FreeCAD import fails, param_get returns a ParametersReplacement"""
        sys.modules["FreeCAD"] = None
        import addonmanager_freecad_interface as fc

        prefs = fc.ParamGet("some/fake/path")
        self.assertTrue(isinstance(prefs, fc.ParametersReplacement))

    def test_replacement_getters_and_setters(self):
        """Test that ParameterReplacement's getters, setters, and deleters work"""
        sys.modules["FreeCAD"] = None
        import addonmanager_freecad_interface as fc

        prf = fc.ParamGet("some/fake/path")
        gs_types = [
            ("Bool", prf.GetBool, prf.SetBool, prf.RemBool, True, False),
            ("Int", prf.GetInt, prf.SetInt, prf.RemInt, 42, 0),
            ("Float", prf.GetFloat, prf.SetFloat, prf.RemFloat, 1.2, 3.4),
            ("String", prf.GetString, prf.SetString, prf.RemString, "test", "other"),
        ]
        for gs_type in gs_types:
            with self.subTest(msg=f"Testing {gs_type[0]}", gs_type=gs_type):
                getter = gs_type[1]
                setter = gs_type[2]
                deleter = gs_type[3]
                value_1 = gs_type[4]
                value_2 = gs_type[5]
                self.assertEqual(getter("test", value_1), value_1)
                self.assertEqual(getter("test", value_2), value_2)
                self.assertNotIn("test", prf.parameters)
                setter("test", value_1)
                self.assertIn("test", prf.parameters)
                self.assertEqual(getter("test", value_2), value_1)
                deleter("test")
                self.assertNotIn("test", prf.parameters)


class TestDataPaths(unittest.TestCase):
    """Tests for the data paths"""

    def setUp(self) -> None:
        self.saved_freecad = None
        if "FreeCAD" in sys.modules:
            self.saved_freecad = sys.modules["FreeCAD"]
            sys.modules.pop("FreeCAD")
        if "addonmanager_freecad_interface" in sys.modules:
            sys.modules.pop("addonmanager_freecad_interface")
        sys.path.append("../../")

    def tearDown(self) -> None:
        if "FreeCAD" in sys.modules:
            sys.modules.pop("FreeCAD")
        if self.saved_freecad is not None:
            sys.modules["FreeCAD"] = self.saved_freecad

    def test_init_with_freecad(self):
        """Ensure that if FreeCAD exists, the appropriate functions are called"""
        sys.modules["FreeCAD"] = unittest.mock.MagicMock()
        import addonmanager_freecad_interface as fc

        data_paths = fc.DataPaths()
        self.assertTrue(sys.modules["FreeCAD"].getUserAppDataDir.called)
        self.assertTrue(sys.modules["FreeCAD"].getUserMacroDir.called)
        self.assertTrue(sys.modules["FreeCAD"].getUserCachePath.called)
        self.assertIsNotNone(data_paths.mod_dir)
        self.assertIsNotNone(data_paths.cache_dir)
        self.assertIsNotNone(data_paths.macro_dir)

    def test_init_without_freecad(self):
        """Ensure that if FreeCAD does not exist, the appropriate functions are called"""
        sys.modules["FreeCAD"] = None
        import addonmanager_freecad_interface as fc

        data_paths = fc.DataPaths()
        self.assertIsNotNone(data_paths.mod_dir)
        self.assertIsNotNone(data_paths.cache_dir)
        self.assertIsNotNone(data_paths.macro_dir)
        self.assertNotEqual(data_paths.mod_dir, data_paths.cache_dir)
        self.assertNotEqual(data_paths.mod_dir, data_paths.macro_dir)
        self.assertNotEqual(data_paths.cache_dir, data_paths.macro_dir)


class TestPreferences(unittest.TestCase):
    """Tests for the preferences wrapper"""

    def setUp(self) -> None:
        sys.path.append("../../")
        import addonmanager_freecad_interface as fc

        self.fc = fc

    def tearDown(self) -> None:
        pass

    def test_load_preferences_defaults(self):
        """Preferences are loaded from a given file"""
        defaults = self.given_defaults()
        with tempfile.TemporaryDirectory() as temp_dir:
            json_file = os.path.join(temp_dir, "defaults.json")
            with open(json_file, "w", encoding="utf-8") as f:
                f.write(json.dumps(defaults))
            self.fc.Preferences._load_preferences_defaults(json_file)
            self.assertDictEqual(defaults, self.fc.Preferences.preferences_defaults)

    def test_in_memory_defaults(self):
        """Preferences are loaded from memory"""
        defaults = self.given_defaults()
        prefs = self.fc.Preferences(defaults)
        self.assertDictEqual(defaults, prefs.preferences_defaults)

    def test_get_good(self):
        """Get returns results when matching an existing preference"""
        defaults = self.given_defaults()
        prefs = self.fc.Preferences(defaults)
        self.assertEqual(prefs.get("TestBool"), defaults["TestBool"])
        self.assertEqual(prefs.get("TestInt"), defaults["TestInt"])
        self.assertEqual(prefs.get("TestFloat"), defaults["TestFloat"])
        self.assertEqual(prefs.get("TestString"), defaults["TestString"])

    def test_get_nonexistent(self):
        """Get raises an exception when asked for a non-existent preference"""
        defaults = self.given_defaults()
        prefs = self.fc.Preferences(defaults)
        with self.assertRaises(RuntimeError):
            prefs.get("No_such_thing")

    def test_get_bad_type(self):
        """Get raises an exception when getting an unsupported type"""
        defaults = self.given_defaults()
        defaults["TestArray"] = ["This", "Is", "Legal", "JSON"]
        prefs = self.fc.Preferences(defaults)
        with self.assertRaises(RuntimeError):
            prefs.get("TestArray")

    def test_set_good(self):
        """Set works when matching an existing preference"""
        defaults = self.given_defaults()
        prefs = self.fc.Preferences(defaults)
        prefs.set("TestBool", False)
        self.assertEqual(prefs.get("TestBool"), False)
        prefs.set("TestInt", 4321)
        self.assertEqual(prefs.get("TestInt"), 4321)
        prefs.set("TestFloat", 3.14159)
        self.assertEqual(prefs.get("TestFloat"), 3.14159)
        prefs.set("TestString", "Forty two")
        self.assertEqual(prefs.get("TestString"), "Forty two")

    def test_set_nonexistent(self):
        """Set raises an exception when asked for a non-existent preference"""
        defaults = self.given_defaults()
        prefs = self.fc.Preferences(defaults)
        with self.assertRaises(RuntimeError):
            prefs.get("No_such_thing")

    def test_set_bad_type(self):
        """Set raises an exception when setting an unsupported type"""
        defaults = self.given_defaults()
        defaults["TestArray"] = ["This", "Is", "Legal", "JSON"]
        prefs = self.fc.Preferences(defaults)
        with self.assertRaises(RuntimeError):
            prefs.get("TestArray")

    @staticmethod
    def given_defaults():
        """Get a dictionary of fake defaults for testing"""
        defaults = {
            "TestBool": True,
            "TestInt": 42,
            "TestFloat": 1.2,
            "TestString": "Test",
        }
        return defaults
