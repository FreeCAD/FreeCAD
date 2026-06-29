# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2026 LubuSeb                                            *
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

"""GUI tests for the Arch Curtain Wall command."""

from unittest.mock import patch

import FreeCAD
import FreeCADGui

from bimcommands.BimCurtainwall import Arch_CurtainWall
from bimtests import TestArchBaseGui


class TestArchCurtainWallGui(TestArchBaseGui.TestArchBaseGui):

    def test_second_interactive_point_uses_line_mode(self):
        """The second point needs line mode so length/angle input is segment-relative."""

        calls = []

        def fake_get_point(**kwargs):
            calls.append(kwargs)

        command = Arch_CurtainWall()

        try:
            FreeCADGui.Selection.clearSelection()
            with patch("FreeCADGui.Snapper.getPoint", side_effect=fake_get_point):
                command.Activated()
                command.getPoint(FreeCAD.Vector(0, 0, 0))
                command.getPoint(None)
        finally:
            FreeCADGui.Selection.clearSelection()
            if getattr(FreeCAD, "activeDraftCommand", None) is command:
                FreeCAD.activeDraftCommand = None

        self.assertEqual(len(calls), 2)
        self.assertNotIn("mode", calls[0])
        self.assertEqual(calls[1]["mode"], "line")
        self.assertEqual(calls[1]["last"], FreeCAD.Vector(0, 0, 0))
