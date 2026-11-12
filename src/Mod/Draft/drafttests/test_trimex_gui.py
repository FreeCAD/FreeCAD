# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This file is free software; you can redistribute it and/or modify it  *
# *   under the terms of the GNU Lesser General Public License (LGPL)       *
# *   as published by the Free Software Foundation; either version 2.1 of  *
# *   the License, or (at your option) any later version.                   *
# *                                                                         *
# ***************************************************************************

"""Unit tests for the Draft Trimex GUI command."""

from unittest import mock

import FreeCAD as App
import Part
import Draft

from draftguitools import gui_trimex
from drafttests.test_base import DraftTestCaseDoc


class DraftTrimexGui(DraftTestCaseDoc):
    """Tests for Trimex coordinate handling."""

    def test_global_shape_in_translated_part(self):
        """Trimex geometry should include the placement of an App Part."""
        part = self.doc.addObject("App::Part", "Part")
        part.Placement.Base = App.Vector(400, 400, 0)
        line = Draft.make_line(App.Vector(100, 100, 0), App.Vector(500, 100, 0))
        part.addObject(line)
        line.Placement.Base = App.Vector(-400, -400, 0)
        self.doc.recompute()

        self.assertEqual(
            [vertex.Point for vertex in line.Shape.Vertexes],
            [App.Vector(-300, -300, 0), App.Vector(100, -300, 0)],
        )
        shape = gui_trimex._get_global_shape(line)
        self.assertEqual(
            [vertex.Point for vertex in shape.Vertexes],
            [App.Vector(100, 100, 0), App.Vector(500, 100, 0)],
        )

    def test_global_shape_preserves_root_object_placement(self):
        """Root-level Trimex geometry should remain unchanged."""
        line = self.doc.addObject("Part::Feature", "Line")
        line.Shape = Part.makeLine(App.Vector(0, 0, 0), App.Vector(10, 0, 0))
        line.Placement.Base = App.Vector(20, 30, 0)
        self.doc.recompute()

        shape = gui_trimex._get_global_shape(line)
        self.assertEqual(
            [vertex.Point for vertex in shape.Vertexes],
            [App.Vector(20, 30, 0), App.Vector(30, 30, 0)],
        )

    def test_resolve_selection_through_link(self):
        """Trimex should use the selected link instance coordinates."""
        part = self.doc.addObject("App::Part", "Part")
        line = Draft.make_line(App.Vector(0, 0, 0), App.Vector(10, 0, 0))
        part.addObject(line)
        link = self.doc.addObject("App::Link", "Link")
        link.LinkedObject = part
        link.Placement.Base = App.Vector(20, 30, 0)
        self.doc.recompute()
        selection = mock.Mock(Object=link, SubElementNames=(f"{line.Name}.Edge1",))

        obj, placement, shape = gui_trimex._resolve_selection(selection)

        self.assertEqual(obj, line)
        self.assertEqual(placement.Base, App.Vector(20, 30, 0))
        self.assertEqual(
            [vertex.Point for vertex in shape.Vertexes],
            [App.Vector(20, 30, 0), App.Vector(30, 30, 0)],
        )

    def test_extend_object_through_link(self):
        """Trimex should write linked-instance geometry back locally."""
        part = self.doc.addObject("App::Part", "Part")
        horizontal = Draft.make_line(App.Vector(0, 0, 0), App.Vector(10, 0, 0))
        vertical = Draft.make_line(App.Vector(20, -5, 0), App.Vector(20, 5, 0))
        part.addObject(horizontal)
        part.addObject(vertical)
        link = self.doc.addObject("App::Link", "Link")
        link.LinkedObject = part
        link.Placement.Base = App.Vector(100, 100, 0)
        self.doc.recompute()
        selection = mock.Mock(Object=link, SubElementNames=(f"{horizontal.Name}.Edge1",))

        obj, placement, shape = gui_trimex._resolve_selection(selection)
        command = gui_trimex.Trimex()
        command.obj = obj
        command.placement = placement
        command.edges = shape.Edges
        command.ghost = [mock.Mock()]
        command.activePoint = 0
        command.force = None
        command.doc = self.doc
        command.extrudeMode = False
        command.point = App.Vector(120, 100, 0)
        command.snapped = {
            "ParentObject": link,
            "SubName": f"{vertical.Name}.Edge1",
        }
        command.shift = False
        command.alt = False

        command.trimObject()

        self.assertEqual(horizontal.Points, [App.Vector(20, 0, 0), App.Vector(0, 0, 0)])

    def test_extend_to_object_in_translated_part(self):
        """Trimex should extend between world-space shapes in an App Part."""
        part = self.doc.addObject("App::Part", "Part")
        part.Placement.Base = App.Vector(400, 400, 0)
        horizontal = Draft.make_line(App.Vector(100, 100, 0), App.Vector(500, 100, 0))
        vertical = Draft.make_line(App.Vector(700, 0, 0), App.Vector(700, 200, 0))
        part.addObject(horizontal)
        part.addObject(vertical)
        horizontal.Placement.Base = App.Vector(-400, -400, 0)
        vertical.Placement.Base = App.Vector(-400, -400, 0)
        self.doc.recompute()

        command = gui_trimex.Trimex()
        command.edges = gui_trimex._get_global_shape(horizontal).Edges
        command.ghost = [mock.Mock()]
        command.activePoint = 0
        command.force = None
        edges = command.redraw(
            App.Vector(700, 100, 0),
            {"ParentObject": vertical, "SubName": "Edge1"},
            real=True,
        )

        points = [vertex.Point for vertex in Part.Wire(edges).Vertexes]
        self.assertEqual(points, [App.Vector(700, 100, 0), App.Vector(100, 100, 0)])

        command.obj = horizontal
        command.doc = self.doc
        command.placement = horizontal.getGlobalPlacement()
        command.extrudeMode = False
        command.point = App.Vector(700, 100, 0)
        command.snapped = {"ParentObject": vertical, "SubName": "Edge1"}
        command.shift = False
        command.alt = False
        command.trimObject()
        self.assertEqual(
            horizontal.Points,
            [App.Vector(700, 100, 0), App.Vector(100, 100, 0)],
        )
