# SPDX-License-Identifier: LGPL-2.1-or-later
"""Lightweight tests for the local Copilot planner."""

import unittest

from provider import LocalIntentProvider


class CopilotProviderTest(unittest.TestCase):
    def setUp(self):
        self.provider = LocalIntentProvider()

    def test_create_box(self):
        plan = self.provider.plan("create a box length 40 width 20 height 10")
        self.assertEqual(plan[0]["action"], "create_box")
        self.assertEqual(plan[0]["length"], 40)
        self.assertEqual(plan[0]["width"], 20)
        self.assertEqual(plan[0]["height"], 10)

    def test_move_selected(self):
        plan = self.provider.plan("move selected x 5 y -2 z 1")
        self.assertEqual(plan[0]["action"], "move_selected")
        self.assertEqual(plan[0]["x"], 5)
        self.assertEqual(plan[0]["y"], -2)
        self.assertEqual(plan[0]["z"], 1)

    def test_color_selected(self):
        plan = self.provider.plan("make selected blue")
        self.assertEqual(plan[0]["action"], "set_color")
        self.assertEqual(plan[0]["color"], [0.0, 0.2, 1.0])


if __name__ == "__main__":
    unittest.main()
