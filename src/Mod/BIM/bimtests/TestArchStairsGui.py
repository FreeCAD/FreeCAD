# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2026 FreeCAD contributors                               *
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

import Arch

from bimtests.TestArchBaseGui import TestArchBaseGui


class TestArchStairsGui(TestArchBaseGui):

    def _assert_visibility(self, stairs, expected):
        self.assertEqual(stairs.ViewObject.Visibility, expected)
        for railing in (stairs.RailingLeft, stairs.RailingRight):
            self.assertIsNotNone(railing)
            self.assertEqual(railing.ViewObject.Visibility, expected)

    def test_stairs_railings_follow_parent_visibility(self):
        stairs = Arch.makeStairs(length=3500, width=800, height=2500, steps=14)
        self.document.recompute()

        self.assertIsNotNone(stairs.RailingLeft)
        self.assertIsNotNone(stairs.RailingRight)
        self._assert_visibility(stairs, True)

        stairs.ViewObject.Visibility = False
        self.pump_gui_events()
        self.document.recompute()
        self._assert_visibility(stairs, False)

        stairs.ViewObject.Visibility = True
        self.pump_gui_events()
        self.document.recompute()
        self._assert_visibility(stairs, True)

        level = Arch.makeBuildingPart()
        level.Group = [stairs]
        self.document.recompute()

        level.ViewObject.Visibility = False
        self.pump_gui_events()
        self.document.recompute()
        self._assert_visibility(stairs, False)

        level.ViewObject.Visibility = True
        self.pump_gui_events()
        self.document.recompute()
        self._assert_visibility(stairs, True)
