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

import sys
import unittest

sys.path.append("../..")

from Widgets.addonmanager_widget_progress_bar import Progress


class TestProgress(unittest.TestCase):

    def test_default_construction(self):
        """Given no parameters, a single-task Progress object is initialized with zero progress"""
        progress = Progress()
        self.assertEqual(progress.status_text, "")
        self.assertEqual(progress.number_of_tasks, 1)
        self.assertEqual(progress.current_task, 0)
        self.assertEqual(progress.current_task_progress, 0.0)

    def test_good_parameters(self):
        """Given good parameters, no exception is raised"""
        _ = Progress(
            status_text="Some text", number_of_tasks=1, current_task=0, current_task_progress=0.0
        )

    def test_zero_task_count(self):
        with self.assertRaises(ValueError):
            _ = Progress(number_of_tasks=0)

    def test_negative_task_count(self):
        with self.assertRaises(ValueError):
            _ = Progress(number_of_tasks=-1)

    def test_setting_status_post_creation(self):
        progress = Progress()
        self.assertEqual(progress.status_text, "")
        progress.status_text = "Some status"
        self.assertEqual(progress.status_text, "Some status")

    def test_setting_task_count(self):
        progress = Progress()
        progress.number_of_tasks = 10
        self.assertEqual(progress.number_of_tasks, 10)

    def test_setting_negative_task_count(self):
        progress = Progress()
        with self.assertRaises(ValueError):
            progress.number_of_tasks = -1

    def test_setting_invalid_task_count(self):
        progress = Progress()
        with self.assertRaises(TypeError):
            progress.number_of_tasks = 3.14159

    def test_setting_current_task(self):
        progress = Progress(number_of_tasks=10)
        progress.number_of_tasks = 5
        self.assertEqual(progress.number_of_tasks, 5)

    def test_setting_current_task_greater_than_task_count(self):
        progress = Progress()
        progress.number_of_tasks = 10
        with self.assertRaises(ValueError):
            progress.current_task = 11

    def test_setting_current_task_equal_to_task_count(self):
        """current_task is zero-indexed, so this is too high"""
        progress = Progress()
        progress.number_of_tasks = 10
        with self.assertRaises(ValueError):
            progress.current_task = 10

    def test_setting_current_task_negative(self):
        progress = Progress()
        with self.assertRaises(ValueError):
            progress.current_task = -1

    def test_setting_current_task_invalid(self):
        progress = Progress()
        with self.assertRaises(TypeError):
            progress.current_task = 2.718281

    def test_setting_current_task_progress(self):
        progress = Progress()
        progress.current_task_progress = 50.0
        self.assertEqual(progress.current_task_progress, 50.0)

    def test_setting_current_task_progress_too_low(self):
        progress = Progress()
        progress.current_task_progress = -0.01
        self.assertEqual(progress.current_task_progress, 0.0)

    def test_setting_current_task_progress_too_high(self):
        progress = Progress()
        progress.current_task_progress = 100.001
        self.assertEqual(progress.current_task_progress, 100.0)

    def test_incrementing_task(self):
        progress = Progress(number_of_tasks=10, current_task_progress=100.0)
        progress.next_task()
        self.assertEqual(progress.current_task, 1)
        self.assertEqual(progress.current_task_progress, 0.0)

    def test_incrementing_task_too_high(self):
        progress = Progress(number_of_tasks=10, current_task=9, current_task_progress=100.0)
        with self.assertRaises(ValueError):
            progress.next_task()

    def test_overall_progress_simple(self):
        progress = Progress()
        self.assertEqual(progress.overall_progress(), 0.0)

    def test_overall_progress_with_ranges(self):
        progress = Progress(number_of_tasks=2, current_task=1, current_task_progress=0.0)
        self.assertAlmostEqual(progress.overall_progress(), 0.5)

    def test_overall_progress_with_ranges_and_progress(self):
        progress = Progress(number_of_tasks=10, current_task=5, current_task_progress=50.0)
        self.assertAlmostEqual(progress.overall_progress(), 0.55)


if __name__ == "__main__":
    unittest.main()
