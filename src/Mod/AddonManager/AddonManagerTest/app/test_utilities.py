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

import unittest
import os
import FreeCAD

from Addon import Addon

from addonmanager_utilities import (
    recognized_git_location,
    get_readme_url,
    get_assigned_string_literal,
    get_macro_version_from_file,
)


class TestUtilities(unittest.TestCase):

    MODULE = "test_utilities"  # file name without extension

    def setUp(self):
        self.test_dir = os.path.join(
            FreeCAD.getHomePath(), "Mod", "AddonManager", "AddonManagerTest", "data"
        )

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

    def test_get_macro_version_from_file(self):
        good_file = os.path.join(self.test_dir, "good_macro_metadata.FCStd")
        version = get_macro_version_from_file(good_file)
        self.assertEqual(version, "1.2.3")

        bad_file = os.path.join(self.test_dir, "bad_macro_metadata.FCStd")
        version = get_macro_version_from_file(bad_file)
        self.assertEqual(version, "", "Bad version did not yield empty string")

        empty_file = os.path.join(self.test_dir, "missing_macro_metadata.FCStd")
        version = get_macro_version_from_file(empty_file)
        self.assertEqual(version, "", "Missing version did not yield empty string")
