# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2026 CCNUdhj                                            *
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
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""GUI tests for the BIM dimension commands."""

import Part

import FreeCAD as App
import FreeCADGui as Gui
from bimcommands import BimDimensions
from bimtests.TestArchBaseGui import TestArchBaseGui


class TestBimDimensionGui(TestArchBaseGui):
    """Test BIM dimension command behavior."""

    def test_preselected_edge_respects_horizontal_and_vertical_direction(self):
        """A linked dimension should retain the direction of the command."""
        # Regression test for:
        # https://github.com/FreeCAD/FreeCAD/issues/29766
        edge_object = self.document.addObject("Part::Feature", "DiagonalEdge")
        edge_object.Shape = Part.makeLine(App.Vector(), App.Vector(10, 10, 0))
        self.document.recompute()

        cases = (
            (BimDimensions.BIM_DimensionHorizontal, App.Vector(1, 0, 0)),
            (BimDimensions.BIM_DimensionVertical, App.Vector(0, 1, 0)),
        )
        for command_class, expected_direction in cases:
            with self.subTest(command=command_class.__name__):
                Gui.Selection.clearSelection()
                Gui.Selection.addSelection(edge_object, "Edge1")
                command = command_class()
                command.Activated()
                try:
                    command.node.append(App.Vector(5, 15, 0))
                    before = set(self.document.Objects)
                    command.createObject()
                    command.finish(cont=None)
                    command = None
                    self.pump_gui_events()
                    created = set(self.document.Objects) - before
                    self.assertEqual(len(created), 1)
                    dimension = created.pop()
                    self.assertTrue(dimension.Direction.isEqual(expected_direction, 1e-7))
                finally:
                    if command is not None:
                        command.finish(cont=None)
