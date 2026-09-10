# SPDX-License-Identifier: LGPL-2.1-or-later

import FreeCAD

from bimcommands import BimNudge
from bimtests.TestArchBaseGui import TestArchBaseGui


class TestBimNudgeGui(TestArchBaseGui):

    def test_presets_follow_active_schema(self):
        self.document.UnitSystem = 0
        metric = BimNudge.get_nudge_presets()

        self.document.UnitSystem = 5
        imperial = BimNudge.get_nudge_presets()

        self.assertEqual(
            [label for label, _quantity in metric],
            [
                "1 mm",
                "5 mm",
                "1 cm",
                "5 cm",
                "10 cm",
                "50 cm",
            ],
        )
        self.assertEqual(
            [quantity.Value for _label, quantity in metric],
            [
                1.0,
                5.0,
                10.0,
                50.0,
                100.0,
                500.0,
            ],
        )
        self.assertEqual(
            [label for label, _quantity in imperial],
            [
                '1/16"',
                '1/8"',
                '1/4"',
                '1"',
                '6"',
                "1'",
            ],
        )
        for actual, expected in zip(
            [quantity.Value for _label, quantity in imperial],
            [1.5875, 3.175, 6.35, 25.4, 152.4, 304.8],
        ):
            self.assertAlmostEqual(actual, expected)
