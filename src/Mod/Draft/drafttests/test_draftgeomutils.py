# ***************************************************************************
# *   Copyright (c) 2020 Antoine Lafr                                       *
# *   Copyright (c) 2025 FreeCAD Project Association                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Unit tests for the Draft Workbench, DraftGeomUtils module tests."""

import Part
import DraftGeomUtils
from FreeCAD import Vector
from drafttests import test_base
from draftutils.messages import _msg


class TestDraftGeomUtils(test_base.DraftTestCaseNoDoc):
    """Testing the functions in the file DraftGeomUtils.py"""

    def check_wire(self, wire):
        offset_values = (2000.0, 0.0, -1000, -2000, -3000, -5500)
        for offset_start in offset_values:
            for offset_end in offset_values:
                if offset_start + offset_end > -wire.Length:
                    # print ("'start={0}, end={1}' failed".format(offset_start, offset_end))
                    try:
                        extended = DraftGeomUtils.get_extended_wire(wire, offset_start, offset_end)
                        # Test that the extended wire's length is correctly changed
                        self.assertAlmostEqual(extended.Length, wire.Length + offset_start + offset_end,
                                               DraftGeomUtils.precision(), "'start={0}, end={1}' failed".format(offset_start, offset_end))
                        if offset_start == 0.0:
                            # If offset_start is 0.0, check that the wire's start point is unchanged
                            self.assertAlmostEqual(extended.OrderedVertexes[0].Point.distanceToPoint(wire.OrderedVertexes[0].Point), 0.0,
                                                   DraftGeomUtils.precision(), "'start={0}, end={1}' failed".format(offset_start, offset_end))
                        if offset_end == 0.0:
                            # If offset_end is 0.0, check that the wire's end point is unchanged
                            self.assertAlmostEqual(extended.OrderedVertexes[-1].Point.distanceToPoint(wire.OrderedVertexes[-1].Point), 0.0,
                                                   DraftGeomUtils.precision(), "'start={0}, end={1}' failed".format(offset_start, offset_end))
                    except Exception as exc:
                        print ("get_extended_wire failed for 'start={0}, end={1}'".format(offset_start, offset_end))
                        raise exc

    def test_get_extended_wire1(self):
        """Test the DraftGeomUtils.get_extended_wire function."""
        operation = "DraftGeomUtils.get_extended_wire1"
        _msg("  Test '{}'".format(operation))

        # Build wires made with straight edges and various combination of Orientation: the wires 1-4 are all equivalent
        points = [Vector(0.0, 0.0, 0.0),
                  Vector(1500.0, 2000.0, 0.0),
                  Vector(4500.0, 2000.0, 0.0),
                  Vector(4500.0, 2000.0, 2500.0)]

        edges = []
        for start, end in zip(points[:-1], points[1:]):
            edge = Part.makeLine(start, end)
            edges.append(edge)
        wire = Part.Wire(edges)
        self.check_wire(wire)

    def test_get_extended_wire2(self):
        """Test the DraftGeomUtils.get_extended_wire function."""
        operation = "DraftGeomUtils.get_extended_wire2"
        _msg("  Test '{}'".format(operation))

        # Build wires made with straight edges and various combination of Orientation: the wires 1-4 are all equivalent
        points = [Vector(0.0, 0.0, 0.0),
                  Vector(1500.0, 2000.0, 0.0),
                  Vector(4500.0, 2000.0, 0.0),
                  Vector(4500.0, 2000.0, 2500.0)]

        edges = []
        for start, end in zip(points[:-1], points[1:]):
            edge = Part.makeLine(end, start)
            edge.Orientation = "Reversed"
            edges.append(edge)
        wire = Part.Wire(edges)
        self.check_wire(wire)

    def test_get_extended_wire3(self):
        """Test the DraftGeomUtils.get_extended_wire function."""
        operation = "DraftGeomUtils.get_extended_wire3"
        _msg("  Test '{}'".format(operation))

        # Build wires made with straight edges and various combination of Orientation: the wires 1-4 are all equivalent
        points = [Vector(0.0, 0.0, 0.0),
                  Vector(1500.0, 2000.0, 0.0),
                  Vector(4500.0, 2000.0, 0.0),
                  Vector(4500.0, 2000.0, 2500.0)]

        edges = []
        for start, end in zip(points[:-1], points[1:]):
            edge = Part.makeLine(start, end)
            edge.Orientation = "Reversed"
            edges.insert(0, edge)
        wire = Part.Wire(edges)
        wire.Orientation = "Reversed"
        self.check_wire(wire)

    def test_get_extended_wire4(self):
        """Test the DraftGeomUtils.get_extended_wire function."""
        operation = "DraftGeomUtils.get_extended_wire4"
        _msg("  Test '{}'".format(operation))

        # Build wires made with straight edges and various combination of Orientation: the wires 1-4 are all equivalent
        points = [Vector(0.0, 0.0, 0.0),
                  Vector(1500.0, 2000.0, 0.0),
                  Vector(4500.0, 2000.0, 0.0),
                  Vector(4500.0, 2000.0, 2500.0)]

        edges = []
        for start, end in zip(points[:-1], points[1:]):
            edge = Part.makeLine(end, start)
            edges.insert(0, edge)
        wire = Part.Wire(edges)
        wire.Orientation = "Reversed"
        self.check_wire(wire)

    def test_get_extended_wire5(self):
        """Test the DraftGeomUtils.get_extended_wire function."""
        operation = "DraftGeomUtils.get_extended_wire5"
        _msg("  Test '{}'".format(operation))

        # Build wires made with arcs and various combination of Orientation: the wires 5-8 are all equivalent
        points = [Vector(0.0, 0.0, 0.0),
                  Vector(1000.0, 1000.0, 0.0),
                  Vector(2000.0, 0.0, 0.0),
                  Vector(3000.0, 0.0, 1000.0),
                  Vector(4000.0, 0.0, 0.0)]

        edges = []
        for start, mid, end in zip(points[:-2], points[1:-1], points[2:]):
            edge = Part.Arc(start, mid, end).toShape()
            edges.append(edge)
        wire = Part.Wire(edges)
        self.check_wire(wire)

    def test_get_extended_wire6(self):
        """Test the DraftGeomUtils.get_extended_wire function."""
        operation = "DraftGeomUtils.get_extended_wire6"
        _msg("  Test '{}'".format(operation))

        # Build wires made with arcs and various combination of Orientation: the wires 5-8 are all equivalent
        points = [Vector(0.0, 0.0, 0.0),
                  Vector(1000.0, 1000.0, 0.0),
                  Vector(2000.0, 0.0, 0.0),
                  Vector(3000.0, 0.0, 1000.0),
                  Vector(4000.0, 0.0, 0.0)]

        edges = []
        for start, mid, end in zip(points[:-2], points[1:-1], points[2:]):
            edge = Part.Arc(end, mid, start).toShape()
            edge.Orientation = "Reversed"
            edges.append(edge)
        wire = Part.Wire(edges)
        self.check_wire(wire)

    def test_get_extended_wire7(self):
        """Test the DraftGeomUtils.get_extended_wire function."""
        operation = "DraftGeomUtils.get_extended_wire7"
        _msg("  Test '{}'".format(operation))

        # Build wires made with arcs and various combination of Orientation: the wires 5-8 are all equivalent
        points = [Vector(0.0, 0.0, 0.0),
                  Vector(1000.0, 1000.0, 0.0),
                  Vector(2000.0, 0.0, 0.0),
                  Vector(3000.0, 0.0, 1000.0),
                  Vector(4000.0, 0.0, 0.0)]

        edges = []
        for start, mid, end in zip(points[:-2], points[1:-1], points[2:]):
            edge = Part.Arc(start, mid, end).toShape()
            edge.Orientation = "Reversed"
            edges.insert(0, edge)
        wire = Part.Wire(edges)
        wire.Orientation = "Reversed"
        self.check_wire(wire)

    def test_get_extended_wire8(self):
        """Test the DraftGeomUtils.get_extended_wire function."""
        operation = "DraftGeomUtils.get_extended_wire8"
        _msg("  Test '{}'".format(operation))

        # Build wires made with arcs and various combination of Orientation: the wires 5-8 are all equivalent
        points = [Vector(0.0, 0.0, 0.0),
                  Vector(1000.0, 1000.0, 0.0),
                  Vector(2000.0, 0.0, 0.0),
                  Vector(3000.0, 0.0, 1000.0),
                  Vector(4000.0, 0.0, 0.0)]

        edges = []
        for start, mid, end in zip(points[:-2], points[1:-1], points[2:]):
            edge = Part.Arc(end, mid, start).toShape()
            edges.insert(0, edge)
        wire = Part.Wire(edges)
        wire.Orientation = "Reversed"
        self.check_wire(wire)

# suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestDraftGeomUtils)
# unittest.TextTestRunner().run(suite)
