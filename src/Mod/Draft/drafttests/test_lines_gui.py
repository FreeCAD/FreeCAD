# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   FreeCAD is free software; you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License (LGPL)       *
# *   as published by the Free Software Foundation; either version 2.1 of   *
# *   the License, or (at your option) any later version.                   *
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

"""Unit tests for Draft line GUI input handling."""

from types import SimpleNamespace
import unittest
from unittest import mock

import FreeCAD as App
from draftguitools import gui_lines


class DraftGuiLines(unittest.TestCase):
    """Tests for the public line and wire GUI input paths."""

    def test_numeric_input_rejects_duplicate_line_point(self):
        """Duplicate numeric input should be rejected for straight lines."""
        command = gui_lines.Line()
        first = App.Vector(0, 0, 0)
        command.node = [first]
        command.ui = mock.Mock()

        with (
            mock.patch.object(gui_lines, "_wrn") as warning,
            mock.patch.object(gui_lines, "_toolmsg") as toolmsg,
            mock.patch.object(command, "update_hints") as update_hints,
            mock.patch.object(command, "drawUpdate") as draw_update,
            mock.patch.object(command, "finish") as finish,
        ):
            command.numericInput(0, 0, 0)

        self.assertEqual(command.node, [first])
        warning.assert_called_once_with(
            gui_lines.translate("draft", "Point identical to previous point")
        )
        toolmsg.assert_not_called()
        update_hints.assert_not_called()
        draw_update.assert_not_called()
        finish.assert_not_called()
        command.ui.setNextFocus.assert_called_once_with()

    def test_mouse_action_rejects_duplicate_line_point(self):
        """Duplicate mouse input should be rejected before preview drawing."""
        command = gui_lines.Line()
        first = App.Vector(0, 0, 0)
        command.node = [first]
        command.point = first
        command.pos = None
        command.support = True
        command.ui = mock.Mock()
        command.obj = SimpleNamespace(ViewObject=SimpleNamespace(Selectable=True))

        with (
            mock.patch.object(gui_lines, "_wrn") as warning,
            mock.patch.object(gui_lines, "_toolmsg") as toolmsg,
            mock.patch.object(command, "update_hints") as update_hints,
            mock.patch.object(command, "drawUpdate") as draw_update,
        ):
            command.action(
                {
                    "Type": "SoMouseButtonEvent",
                    "State": "DOWN",
                    "Button": "BUTTON1",
                    "Position": (10, 10),
                }
            )

        self.assertEqual(command.node, [first])
        self.assertFalse(command.obj.ViewObject.Selectable)
        command.ui.redraw.assert_called_once_with()
        warning.assert_called_once_with(
            gui_lines.translate("draft", "Point identical to previous point")
        )
        toolmsg.assert_not_called()
        update_hints.assert_not_called()
        draw_update.assert_not_called()

    def test_numeric_input_allows_wire_closure_point(self):
        """Closing a wire by reusing the first point must remain allowed."""
        command = gui_lines.Wire()
        first = App.Vector(0, 0, 0)
        second = App.Vector(2, 0, 0)
        third = App.Vector(2, 2, 0)
        command.node = [first, second, third]
        command.ui = mock.Mock()

        with (
            mock.patch.object(gui_lines, "_wrn") as warning,
            mock.patch.object(gui_lines, "_toolmsg") as toolmsg,
            mock.patch.object(command, "update_hints") as update_hints,
            mock.patch.object(command, "drawUpdate") as draw_update,
        ):
            command.numericInput(0, 0, 0)

        self.assertEqual(command.node, [first, second, third, first])
        warning.assert_not_called()
        toolmsg.assert_not_called()
        update_hints.assert_not_called()
        draw_update.assert_called_once_with(first)
        command.ui.setNextFocus.assert_called_once_with()

    def test_mouse_action_closes_wire_on_first_point(self):
        """Snapping back to the first point must still close the wire."""
        command = gui_lines.Wire()
        first = App.Vector(0, 0, 0)
        second = App.Vector(2, 0, 0)
        third = App.Vector(2, 2, 0)
        command.node = [first, second, third]
        command.point = first
        command.pos = None
        command.support = True
        command.ui = mock.Mock()
        command.obj = SimpleNamespace(ViewObject=SimpleNamespace(Selectable=True))

        with (
            mock.patch.object(gui_lines, "_wrn") as warning,
            mock.patch.object(gui_lines, "_toolmsg") as toolmsg,
            mock.patch.object(command, "drawUpdate") as draw_update,
            mock.patch.object(
                command, "undolast", side_effect=lambda: command.node.pop()
            ) as undolast,
            mock.patch.object(command, "finish") as finish,
        ):
            command.action(
                {
                    "Type": "SoMouseButtonEvent",
                    "State": "DOWN",
                    "Button": "BUTTON1",
                    "Position": (10, 10),
                }
            )

        self.assertEqual(command.node, [first, second, third])
        self.assertFalse(command.obj.ViewObject.Selectable)
        command.ui.redraw.assert_called_once_with()
        warning.assert_not_called()
        toolmsg.assert_not_called()
        draw_update.assert_called_once_with(first)
        undolast.assert_called_once_with()
        finish.assert_called_once_with(cont=None, closed=True)
