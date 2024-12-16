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

from datetime import datetime
import unittest
from unittest.mock import patch, mock_open
import os

from Addon import Addon

from addonmanager_utilities import (
    get_assigned_string_literal,
    get_macro_version_from_file,
    get_readme_url,
    process_date_string_to_python_datetime,
    recognized_git_location,
)


class TestUtilities(unittest.TestCase):

    MODULE = "test_utilities"  # file name without extension

    @classmethod
    def tearDownClass(cls):
        try:
            os.remove("AM_INSTALLATION_DIGEST.txt")
        except FileNotFoundError:
            pass

    def test_recognized_git_location(self):
        recognized_urls = [
            "https://github.com/FreeCAD/FreeCAD",
            "https://gitlab.com/freecad/FreeCAD",
            "https://framagit.org/freecad/FreeCAD",
            "https://salsa.debian.org/science-team/freecad",
        ]
        for url in recognized_urls:
            repo = Addon("Test Repo", url, Addon.Status.NOT_INSTALLED, "branch")
            self.assertTrue(recognized_git_location(repo), f"{url} was unexpectedly not recognized")

        unrecognized_urls = [
            "https://google.com",
            "https://freecad.org",
            "https://not.quite.github.com/FreeCAD/FreeCAD",
            "https://github.com.malware.com/",
        ]
        for url in unrecognized_urls:
            repo = Addon("Test Repo", url, Addon.Status.NOT_INSTALLED, "branch")
            self.assertFalse(recognized_git_location(repo), f"{url} was unexpectedly recognized")

    def test_get_readme_url(self):
        github_urls = [
            "https://github.com/FreeCAD/FreeCAD",
        ]
        gitlab_urls = [
            "https://gitlab.com/freecad/FreeCAD",
            "https://framagit.org/freecad/FreeCAD",
            "https://salsa.debian.org/science-team/freecad",
            "https://unknown.location/and/path",
        ]

        # GitHub and Gitlab have two different schemes for file URLs: unrecognized URLs are
        # presumed to be local instances of a GitLab server. Note that in neither case does this
        # take into account the redirects that are used to actually fetch the data.

        for url in github_urls:
            branch = "branchname"
            expected_result = f"{url}/raw/{branch}/README.md"
            repo = Addon("Test Repo", url, Addon.Status.NOT_INSTALLED, branch)
            actual_result = get_readme_url(repo)
            self.assertEqual(actual_result, expected_result)

        for url in gitlab_urls:
            branch = "branchname"
            expected_result = f"{url}/-/raw/{branch}/README.md"
            repo = Addon("Test Repo", url, Addon.Status.NOT_INSTALLED, branch)
            actual_result = get_readme_url(repo)
            self.assertEqual(actual_result, expected_result)

    def test_get_assigned_string_literal(self):
        good_lines = [
            ["my_var = 'Single-quoted literal'", "Single-quoted literal"],
            ['my_var = "Double-quoted literal"', "Double-quoted literal"],
            ["my_var   =  \t 'Extra whitespace'", "Extra whitespace"],
            ["my_var   =  42", "42"],
            ["my_var   =  1.23", "1.23"],
        ]
        for line in good_lines:
            result = get_assigned_string_literal(line[0])
            self.assertEqual(result, line[1])

        bad_lines = [
            "my_var = __date__",
            "my_var 'No equals sign'",
            "my_var = 'Unmatched quotes\"",
            "my_var = No quotes at all",
            "my_var = 1.2.3",
        ]
        for line in bad_lines:
            result = get_assigned_string_literal(line)
            self.assertIsNone(result)

    def test_get_macro_version_from_file_good_metadata(self):
        good_metadata = """__Version__       = "1.2.3" """
        with patch("builtins.open", new_callable=mock_open, read_data=good_metadata):
            version = get_macro_version_from_file("mocked_file.FCStd")
            self.assertEqual(version, "1.2.3")

    def test_get_macro_version_from_file_missing_quotes(self):
        bad_metadata = """__Version__       = 1.2.3 """  # No quotes
        with patch("builtins.open", new_callable=mock_open, read_data=bad_metadata):
            version = get_macro_version_from_file("mocked_file.FCStd")
            self.assertEqual(version, "", "Bad version did not yield empty string")

    def test_get_macro_version_from_file_no_version(self):
        good_metadata = ""
        with patch("builtins.open", new_callable=mock_open, read_data=good_metadata):
            version = get_macro_version_from_file("mocked_file.FCStd")
            self.assertEqual(version, "", "Missing version did not yield empty string")

    def test_process_date_string_to_python_datetime_year_first(self):
        result = process_date_string_to_python_datetime("2024-01-31")
        expected_result = datetime(2024, 1, 31, 0, 0)
        self.assertEqual(result, expected_result)

    def test_process_date_string_to_python_datetime_day_first(self):
        result = process_date_string_to_python_datetime("31-01-2024")
        expected_result = datetime(2024, 1, 31, 0, 0)
        self.assertEqual(result, expected_result)

    def test_process_date_string_to_python_datetime_month_first(self):
        result = process_date_string_to_python_datetime("01-31-2024")
        expected_result = datetime(2024, 1, 31, 0, 0)
        self.assertEqual(result, expected_result)

    def test_process_date_string_to_python_datetime_ambiguous(self):
        """In the ambiguous case, the code should assume that the date is in the DD-MM-YYYY format."""
        result = process_date_string_to_python_datetime("01-12-2024")
        expected_result = datetime(2024, 12, 1, 0, 0)
        self.assertEqual(result, expected_result)

    def test_process_date_string_to_python_datetime_invalid_date(self):
        with self.assertRaises(ValueError):
            process_date_string_to_python_datetime("13-31-2024")

    def test_process_date_string_to_python_datetime_too_many_components(self):
        with self.assertRaises(ValueError):
            process_date_string_to_python_datetime("01-01-31-2024")

    def test_process_date_string_to_python_datetime_too_few_components(self):
        """Month-Year-only dates are not supported"""
        with self.assertRaises(ValueError):
            process_date_string_to_python_datetime("01-2024")

    def test_process_date_string_to_python_datetime_unrecognizable(self):
        """Two-digit years are not supported"""
        with self.assertRaises(ValueError):
            process_date_string_to_python_datetime("01-02-24")
