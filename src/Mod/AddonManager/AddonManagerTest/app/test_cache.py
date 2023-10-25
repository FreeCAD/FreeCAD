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
import datetime
import sys
import unittest
from datetime import date
from unittest import TestCase
from unittest.mock import MagicMock, patch

sys.path.append("../..")

import addonmanager_cache as cache
from AddonManagerTest.app.mocks import MockPref, MockExists


class TestCache(TestCase):
    @patch("addonmanager_freecad_interface.getUserCachePath")
    @patch("addonmanager_freecad_interface.ParamGet")
    @patch("os.remove", MagicMock())
    @patch("os.makedirs", MagicMock())
    def test_local_cache_needs_update(self, param_mock: MagicMock, cache_mock: MagicMock):
        cache_mock.return_value = ""
        param_mock.return_value = MockPref()
        default_prefs = {
            "UpdateFrequencyComboEntry": 0,
            "LastCacheUpdate": "2000-01-01",
            "CustomRepoHash": "",
            "CustomRepositories": "",
        }
        today = date.today().isoformat()
        yesterday = (date.today() - datetime.timedelta(1)).isoformat()

        # Organize these as subtests because of all the patching that has to be done: once we are in this function,
        # the patch is complete, and we can just modify the return values of the fakes one by one
        tests = (
            {
                "case": "No existing cache",
                "files_that_exist": [],
                "prefs_to_set": {},
                "expect": True,
            },
            {
                "case": "Last cache update was interrupted",
                "files_that_exist": ["CACHE_UPDATE_INTERRUPTED"],
                "prefs_to_set": {},
                "expect": True,
            },
            {
                "case": "Cache exists and updating is blocked",
                "files_that_exist": ["AddonManager"],
                "prefs_to_set": {},
                "expect": False,
            },
            {
                "case": "Daily updates set and last update was long ago",
                "files_that_exist": ["AddonManager"],
                "prefs_to_set": {"UpdateFrequencyComboEntry": 1},
                "expect": True,
            },
            {
                "case": "Daily updates set and last update was today",
                "files_that_exist": ["AddonManager"],
                "prefs_to_set": {"UpdateFrequencyComboEntry": 1, "LastCacheUpdate": today},
                "expect": False,
            },
            {
                "case": "Daily updates set and last update was yesterday",
                "files_that_exist": ["AddonManager"],
                "prefs_to_set": {"UpdateFrequencyComboEntry": 1, "LastCacheUpdate": yesterday},
                "expect": True,
            },
            {
                "case": "Weekly updates set and last update was long ago",
                "files_that_exist": ["AddonManager"],
                "prefs_to_set": {"UpdateFrequencyComboEntry": 1},
                "expect": True,
            },
            {
                "case": "Weekly updates set and last update was yesterday",
                "files_that_exist": ["AddonManager"],
                "prefs_to_set": {"UpdateFrequencyComboEntry": 2, "LastCacheUpdate": yesterday},
                "expect": False,
            },
            {
                "case": "Custom repo list changed",
                "files_that_exist": ["AddonManager"],
                "prefs_to_set": {"CustomRepositories": "NewRepo"},
                "expect": True,
            },
        )
        for test_case in tests:
            with self.subTest(test_case["case"]):
                case_prefs = default_prefs
                for pref, setting in test_case["prefs_to_set"].items():
                    case_prefs[pref] = setting
                param_mock.return_value.set_prefs(case_prefs)
                exists_mock = MockExists(test_case["files_that_exist"])
                with patch("os.path.exists", exists_mock.exists):
                    if test_case["expect"]:
                        self.assertTrue(cache.local_cache_needs_update())
                    else:
                        self.assertFalse(cache.local_cache_needs_update())


if __name__ == "__main__":
    unittest.main()
