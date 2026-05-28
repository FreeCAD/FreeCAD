# SPDX-License-Identifier: LGPL-2.1-or-later
"""Lightweight tests for the local Copilot planner."""

import unittest

from provider import LocalIntentProvider, _validate_plan


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

    def test_validate_openrouter_style_plan(self):
        plan = _validate_plan([{"action": "create_sphere", "radius": 12}])
        self.assertEqual(plan[0]["action"], "create_sphere")

    def test_validate_function_style_action(self):
        plan = _validate_plan([{"action": "create_box(40,30,20,'Box')"}])
        self.assertEqual(plan[0]["action"], "create_box")
        self.assertEqual(plan[0]["length"], 40)
        self.assertEqual(plan[0]["width"], 30)
        self.assertEqual(plan[0]["height"], 20)
        self.assertEqual(plan[0]["name"], "Box")

    def test_validate_function_style_step_string(self):
        plan = _validate_plan(["move_selected(10, 0, 5)"])
        self.assertEqual(plan[0]["action"], "move_selected")
        self.assertEqual(plan[0]["x"], 10)
        self.assertEqual(plan[0]["y"], 0)
        self.assertEqual(plan[0]["z"], 5)

    def test_validate_keyword_function_style_action(self):
        plan = _validate_plan([{"action": "create_cylinder(radius=15,height=20,name='Cylinder')"}])
        self.assertEqual(plan[0]["action"], "create_cylinder")
        self.assertEqual(plan[0]["radius"], 15)
        self.assertEqual(plan[0]["height"], 20)
        self.assertEqual(plan[0]["name"], "Cylinder")

    def test_validate_placeholder_function_style_action(self):
        plan = _validate_plan([{"action": "create_box(length,width,height,name)"}])
        self.assertEqual(plan[0]["action"], "create_box")
        self.assertNotIn("length", plan[0])

    def test_validate_complex_actions(self):
        plan = _validate_plan(
            [
                "create_torus(20, 4, 'Wheel', 0, 0, 5)",
                {"action": "set_color('blue','Wheel')"},
                {"action": "boolean_fuse", "objects": ["Wheel", "Hub"], "name": "WheelAssembly"},
            ]
        )
        self.assertEqual(plan[0]["action"], "create_torus")
        self.assertEqual(plan[0]["x"], 0)
        self.assertEqual(plan[1]["color"], "blue")
        self.assertEqual(plan[2]["action"], "boolean_fuse")


if __name__ == "__main__":
    unittest.main()
