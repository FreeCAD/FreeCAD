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

"""Tests for the MacroParser class"""

import io
import os
import sys
import unittest

sys.path.append("../../")  # So the IDE can find the classes to run with

from addonmanager_macro_parser import MacroParser
from AddonManagerTest.app.mocks import MockConsole, CallCatcher, MockThread


# pylint: disable=protected-access, too-many-public-methods


class TestMacroParser(unittest.TestCase):
    """Test the MacroParser class"""

    def setUp(self) -> None:
        self.test_object = MacroParser("UnitTestMacro")
        self.test_object.console = MockConsole()
        self.test_object.current_thread = MockThread()

    def tearDown(self) -> None:
        pass

    def test_fill_details_from_code_normal(self):
        """Test to make sure _process_line gets called as expected"""
        catcher = CallCatcher()
        self.test_object._process_line = catcher.catch_call
        fake_macro_data = self.given_some_lines(20, 10)
        self.test_object.fill_details_from_code(fake_macro_data)
        self.assertEqual(catcher.call_count, 10)

    def test_fill_details_from_code_too_many_lines(self):
        """Test to make sure _process_line gets limited as expected"""
        catcher = CallCatcher()
        self.test_object._process_line = catcher.catch_call
        self.test_object.MAX_LINES_TO_SEARCH = 5
        fake_macro_data = self.given_some_lines(20, 10)
        self.test_object.fill_details_from_code(fake_macro_data)
        self.assertEqual(catcher.call_count, 5)

    def test_fill_details_from_code_thread_interrupted(self):
        """Test to make sure _process_line gets stopped as expected"""
        catcher = CallCatcher()
        self.test_object._process_line = catcher.catch_call
        self.test_object.current_thread.interrupt_after_n_calls = 6  # Stop on the 6th
        fake_macro_data = self.given_some_lines(20, 10)
        self.test_object.fill_details_from_code(fake_macro_data)
        self.assertEqual(catcher.call_count, 5)

    @staticmethod
    def given_some_lines(num_lines, num_dunder_lines) -> str:
        """Generate fake macro header data with the given number of lines and number of
        lines beginning with a double-underscore."""
        result = ""
        for i in range(num_lines):
            if i < num_dunder_lines:
                result += f"__something_{i}__ = 'Test{i}'  # A line to be scanned\n"
            else:
                result += f"# Nothing to see on line {i}\n"
        return result

    def test_process_line_known_lines(self):
        """Lines starting with keys are processed"""
        test_lines = ["__known_key__ = 'Test'", "__another_known_key__ = 'Test'"]
        for line in test_lines:
            with self.subTest(line=line):
                self.test_object.remaining_item_map = {
                    "__known_key__": "known_key",
                    "__another_known_key__": "another_known_key",
                }
                content_lines = io.StringIO(line)
                read_in_line = content_lines.readline()
                catcher = CallCatcher()
                self.test_object._process_key = catcher.catch_call
                self.test_object._process_line(read_in_line, content_lines)
                self.assertTrue(catcher.called, "_process_key was not called for a known key")

    def test_process_line_unknown_lines(self):
        """Lines starting with non-keys are not processed"""
        test_lines = [
            "# Just a line with a comment",
            "\n",
            "__dont_know_this_one__ = 'Who cares?'",
            "# __known_key__ = 'Aha, but it is commented out!'",
        ]
        for line in test_lines:
            with self.subTest(line=line):
                self.test_object.remaining_item_map = {
                    "__known_key__": "known_key",
                    "__another_known_key__": "another_known_key",
                }
                content_lines = io.StringIO(line)
                read_in_line = content_lines.readline()
                catcher = CallCatcher()
                self.test_object._process_key = catcher.catch_call
                self.test_object._process_line(read_in_line, content_lines)
                self.assertFalse(catcher.called, "_process_key was called for an unknown key")

    def test_process_key_standard(self):
        """Normal expected data is processed"""
        self.test_object._reset_map()
        in_memory_data = '__comment__ = "Test"'
        content_lines = io.StringIO(in_memory_data)
        line = content_lines.readline()
        self.test_object._process_key("__comment__", line, content_lines)
        self.assertTrue(self.test_object.parse_results["comment"], "Test")

    def test_process_key_special(self):
        """Special handling for version = date is processed"""
        self.test_object._reset_map()
        self.test_object.parse_results["date"] = "2001-01-01"
        in_memory_data = "__version__ = __date__"
        content_lines = io.StringIO(in_memory_data)
        line = content_lines.readline()
        self.test_object._process_key("__version__", line, content_lines)
        self.assertTrue(self.test_object.parse_results["version"], "2001-01-01")

    def test_handle_backslash_continuation_no_backslashes(self):
        """The backslash handling code doesn't change a line with no backslashes"""
        in_memory_data = '"Not a backslash in sight"'
        content_lines = io.StringIO(in_memory_data)
        line = content_lines.readline()
        result = self.test_object._handle_backslash_continuation(line, content_lines)
        self.assertEqual(result, in_memory_data)

    def test_handle_backslash_continuation(self):
        """Lines ending in a backslash get stripped and concatenated"""
        in_memory_data = '"Line1\\\nLine2\\\nLine3\\\nLine4"'
        content_lines = io.StringIO(in_memory_data)
        line = content_lines.readline()
        result = self.test_object._handle_backslash_continuation(line, content_lines)
        self.assertEqual(result, '"Line1Line2Line3Line4"')

    def test_handle_triple_quoted_string_no_triple_quotes(self):
        """The triple-quote handler leaves alone lines without triple-quotes"""
        in_memory_data = '"Line1"'
        content_lines = io.StringIO(in_memory_data)
        line = content_lines.readline()
        result, was_triple_quoted = self.test_object._handle_triple_quoted_string(
            line, content_lines
        )
        self.assertEqual(result, in_memory_data)
        self.assertFalse(was_triple_quoted)

    def test_handle_triple_quoted_string(self):
        """Data is extracted across multiple lines for a triple-quoted string"""
        in_memory_data = '"""Line1\nLine2\nLine3\nLine4"""\nLine5\n'
        content_lines = io.StringIO(in_memory_data)
        line = content_lines.readline()
        result, was_triple_quoted = self.test_object._handle_triple_quoted_string(
            line, content_lines
        )
        self.assertEqual(result, '"""Line1\nLine2\nLine3\nLine4"""')
        self.assertTrue(was_triple_quoted)

    def test_strip_quotes_single(self):
        """Single quotes are stripped from the final string"""
        expected = "test"
        quoted = f"'{expected}'"
        actual = self.test_object._strip_quotes(quoted)
        self.assertEqual(actual, expected)

    def test_strip_quotes_double(self):
        """Double quotes are stripped from the final string"""
        expected = "test"
        quoted = f'"{expected}"'
        actual = self.test_object._strip_quotes(quoted)
        self.assertEqual(actual, expected)

    def test_strip_quotes_triple(self):
        """Triple quotes are stripped from the final string"""
        expected = "test"
        quoted = f'"""{expected}"""'
        actual = self.test_object._strip_quotes(quoted)
        self.assertEqual(actual, expected)

    def test_strip_quotes_unquoted(self):
        """Unquoted data results in None"""
        unquoted = "This has no quotation marks of any kind"
        actual = self.test_object._strip_quotes(unquoted)
        self.assertIsNone(actual)

    def test_standard_extraction_string(self):
        """String variables are extracted and stored"""
        string_keys = [
            "comment",
            "url",
            "wiki",
            "version",
            "author",
            "date",
            "icon",
            "xpm",
        ]
        for key in string_keys:
            with self.subTest(key=key):
                self.test_object._standard_extraction(key, "test")
                self.assertEqual(self.test_object.parse_results[key], "test")

    def test_standard_extraction_list(self):
        """List variable is extracted and stored"""
        key = "other_files"
        self.test_object._standard_extraction(key, "test1, test2, test3")
        self.assertIn("test1", self.test_object.parse_results[key])
        self.assertIn("test2", self.test_object.parse_results[key])
        self.assertIn("test3", self.test_object.parse_results[key])

    def test_apply_special_handling_version(self):
        """If the tag is __version__, apply our special handling"""
        self.test_object._reset_map()
        self.test_object._apply_special_handling("__version__", 42)
        self.assertNotIn("__version__", self.test_object.remaining_item_map)
        self.assertEqual(self.test_object.parse_results["version"], "42")

    def test_apply_special_handling_not_version(self):
        """If the tag is not __version__, raise an error"""
        self.test_object._reset_map()
        with self.assertRaises(SyntaxError):
            self.test_object._apply_special_handling("__not_version__", 42)
        self.assertIn("__version__", self.test_object.remaining_item_map)

    def test_process_noncompliant_version_date(self):
        """Detect and allow __date__ for the __version__"""
        self.test_object.parse_results["date"] = "1/2/3"
        self.test_object._process_noncompliant_version("__date__")
        self.assertEqual(
            self.test_object.parse_results["version"],
            self.test_object.parse_results["date"],
        )

    def test_process_noncompliant_version_float(self):
        """Detect and allow floats for the __version__"""
        self.test_object._process_noncompliant_version(1.2)
        self.assertEqual(self.test_object.parse_results["version"], "1.2")

    def test_process_noncompliant_version_int(self):
        """Detect and allow integers for the __version__"""
        self.test_object._process_noncompliant_version(42)
        self.assertEqual(self.test_object.parse_results["version"], "42")

    def test_detect_illegal_content_prefixed_string(self):
        """Detect and raise an error for various kinds of prefixed strings"""
        illegal_strings = [
            "f'Some fancy {thing}'",
            'f"Some fancy {thing}"',
            "r'Some fancy {thing}'",
            'r"Some fancy {thing}"',
            "u'Some fancy {thing}'",
            'u"Some fancy {thing}"',
            "fr'Some fancy {thing}'",
            'fr"Some fancy {thing}"',
            "rf'Some fancy {thing}'",
            'rf"Some fancy {thing}"',
        ]
        for test_string in illegal_strings:
            with self.subTest(test_string=test_string):
                with self.assertRaises(SyntaxError):
                    MacroParser._detect_illegal_content(test_string)

    def test_detect_illegal_content_not_a_string(self):
        """Detect and raise an error for (some) non-strings"""
        illegal_strings = [
            "no quotes",
            "do_stuff()",
            'print("A function call sporting quotes!")',
            "__name__",
            "__version__",
            "1.2.3",
        ]
        for test_string in illegal_strings:
            with self.subTest(test_string=test_string):
                with self.assertRaises(SyntaxError):
                    MacroParser._detect_illegal_content(test_string)

    def test_detect_illegal_content_no_failure(self):
        """Recognize strings of various kinds, plus ints, and floats"""
        legal_strings = [
            '"Some legal value in double quotes"',
            "'Some legal value in single quotes'",
            '"""Some legal value in triple quotes"""',
            "__date__",
            "42",
            "4.2",
        ]
        for test_string in legal_strings:
            with self.subTest(test_string=test_string):
                MacroParser._detect_illegal_content(test_string)

    #####################
    # INTEGRATION TESTS #
    #####################

    def test_macro_parser(self):
        """INTEGRATION TEST: Given "real" data, ensure the parsing yields the expected results."""
        data_dir = os.path.join(os.path.dirname(__file__), "../data")
        macro_file = os.path.join(data_dir, "DoNothing.FCMacro")
        with open(macro_file, "r", encoding="utf-8") as f:
            code = f.read()
        self.test_object.fill_details_from_code(code)
        self.assertEqual(len(self.test_object.console.errors), 0)
        self.assertEqual(len(self.test_object.console.warnings), 0)
        self.assertEqual(self.test_object.parse_results["author"], "Chris Hennes")
        self.assertEqual(self.test_object.parse_results["version"], "1.0")
        self.assertEqual(self.test_object.parse_results["date"], "2022-02-28")
        self.assertEqual(
            self.test_object.parse_results["comment"],
            "Do absolutely nothing. For Addon Manager integration tests.",
        )
        self.assertEqual(
            self.test_object.parse_results["url"], "https://github.com/FreeCAD/FreeCAD"
        )
        self.assertEqual(self.test_object.parse_results["icon"], "not_real.png")
        self.assertListEqual(
            self.test_object.parse_results["other_files"],
            ["file1.py", "file2.py", "file3.py"],
        )
        self.assertNotEqual(self.test_object.parse_results["xpm"], "")
