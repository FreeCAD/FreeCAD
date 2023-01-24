# ***************************************************************************
# *   Copyright (c) 2020 Antoine Lafr                                       *
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
"""Unit test for the DraftGeomUtils module."""

import unittest
import FreeCAD
import Part
import DraftGeomUtils
import drafttests.auxiliary as aux
from draftutils.messages import _msg

class TestDraftGeomUtils(unittest.TestCase):
    """Testing the functions in the file DraftGeomUtils.py"""

    def setUp(self):
        """Prepare the test. Nothing to do here, DraftGeomUtils doesn't need a document."""
        aux.draw_header()

    def test_get_extended_wire(self):
        """Test the DraftGeomUtils.get_extended_wire function."""
        operation = "DraftGeomUtils.get_extended_wire"
        _msg("  Test '{}'".format(operation))

        # Build wires made with straight edges and various combination of Orientation: the wires 1-4 are all equivalent
        points = [FreeCAD.Vector(0.0, 0.0, 0.0),
                  FreeCAD.Vector(1500.0, 2000.0, 0.0),
                  FreeCAD.Vector(4500.0, 2000.0, 0.0),
                  FreeCAD.Vector(4500.0, 2000.0, 2500.0)]

        edges = []
        for start, end in zip(points[:-1], points[1:]):
            edge = Part.makeLine(start, end)
            edges.append(edge)
        wire1 = Part.Wire(edges)

        edges = []
        for start, end in zip(points[:-1], points[1:]):
            edge = Part.makeLine(end, start)
            edge.Orientation = "Reversed"
            edges.append(edge)
        wire2 = Part.Wire(edges)

        edges = []
        for start, end in zip(points[:-1], points[1:]):
            edge = Part.makeLine(start, end)
            edge.Orientation = "Reversed"
            edges.insert(0, edge)
        wire3 = Part.Wire(edges)
        wire3.Orientation = "Reversed"

        edges = []
        for start, end in zip(points[:-1], points[1:]):
            edge = Part.makeLine(end, start)
            edges.insert(0, edge)
        wire4 = Part.Wire(edges)
        wire4.Orientation = "Reversed"

        # Build wires made with arcs and various combination of Orientation: the wires 5-8 are all equivalent
        points = [FreeCAD.Vector(0.0, 0.0, 0.0),
                  FreeCAD.Vector(1000.0, 1000.0, 0.0),
                  FreeCAD.Vector(2000.0, 0.0, 0.0),
                  FreeCAD.Vector(3000.0, 0.0, 1000.0),
                  FreeCAD.Vector(4000.0, 0.0, 0.0)]

        edges = []
        for start, mid, end in zip(points[:-2], points[1:-1], points[2:]):
            edge = Part.Arc(start, mid, end).toShape()
            edges.append(edge)
        wire5 = Part.Wire(edges)

        edges = []
        for start, mid, end in zip(points[:-2], points[1:-1], points[2:]):
            edge = Part.Arc(end, mid, start).toShape()
            edge.Orientation = "Reversed"
            edges.append(edge)
        wire6 = Part.Wire(edges)

        edges = []
        for start, mid, end in zip(points[:-2], points[1:-1], points[2:]):
            edge = Part.Arc(start, mid, end).toShape()
            edge.Orientation = "Reversed"
            edges.insert(0, edge)
        wire7 = Part.Wire(edges)
        wire7.Orientation = "Reversed"

        edges = []
        for start, mid, end in zip(points[:-2], points[1:-1], points[2:]):
            edge = Part.Arc(end, mid, start).toShape()
            edges.insert(0, edge)
        wire8 = Part.Wire(edges)
        wire8.Orientation = "Reversed"

        # Run "get_extended_wire" for all the wires with various offset_start, offset_end combinations
        num_subtests = 0
        offset_values = (2000.0, 0.0, -1000, -2000, -3000, -5500)
        for i, wire in enumerate((wire1, wire2, wire3, wire4, wire5, wire6, wire7, wire8)):
            _msg("  Running tests with wire{}".format(i + 1))
            for offset_start in offset_values:
                for offset_end in offset_values:
                    if offset_start + offset_end > -wire.Length:
                        subtest = "get_extended_wire(wire{0}, {1}, {2})".format(i + 1, offset_start, offset_end)
                        num_subtests += 1 # TODO: it should be "with self.subtest(subtest):" but then it doesn't report failures.
                        extended = DraftGeomUtils.get_extended_wire(wire, offset_start, offset_end)
                        # Test that the extended wire's length is correctly changed
                        self.assertAlmostEqual(extended.Length, wire.Length + offset_start + offset_end,
                                               DraftGeomUtils.precision(), "'{0}.{1}' failed".format(operation, subtest))
                        if offset_start == 0.0:
                            # If offset_start is 0.0, check that the wire's start point is unchanged
                            self.assertAlmostEqual(extended.OrderedVertexes[0].Point.distanceToPoint(wire.OrderedVertexes[0].Point), 0.0,
                                                   DraftGeomUtils.precision(), "'{0}.{1}' failed".format(operation, subtest))
                        if offset_end == 0.0:
                            # If offset_end is 0.0, check that the wire's end point is unchanged
                            self.assertAlmostEqual(extended.OrderedVertexes[-1].Point.distanceToPoint(wire.OrderedVertexes[-1].Point), 0.0,
                                                   DraftGeomUtils.precision(), "'{0}.{1}' failed".format(operation, subtest))
        _msg("  Test completed, {} subtests run".format(num_subtests))

    def tearDown(self):
        """Finish the test. Nothing to do here, DraftGeomUtils doesn't need a document."""
        pass

# suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestDraftGeomUtils)
# unittest.TextTestRunner().run(suite)
