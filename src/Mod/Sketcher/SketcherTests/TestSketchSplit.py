# SPDX-License-Identifier: LGPL-2.1-or-later

# **************************************************************************
#   Copyright (c) 2026 The FreeCAD Project Association                     *
#                                                                         *
#   This file is part of the FreeCAD CAx development system.              *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU Lesser General Public License (LGPL)    *
#   as published by the Free Software Foundation; either version 2 of     *
#   the License, or (at your option) any later version.                   *
#   for detail see the LICENCE text file.                                 *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful,            *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU Library General Public License for more details.                  *
#                                                                         *
#   You should have received a copy of the GNU Library General Public     *
#   License along with FreeCAD; if not, write to the Free Software        *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#   USA                                                                   *
# **************************************************************************

import unittest

import FreeCAD
import Part
import Sketcher


App = FreeCAD


class TestSketchSplit(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("TestSketchSplit")

    def tearDown(self):
        FreeCAD.closeDocument("TestSketchSplit")

    def assertHasLineLineDistance(self, sketch, split_line_ids, other_line):
        for constraint in sketch.Constraints:
            if constraint.Type != "Distance":
                continue
            if constraint.FirstPos != 0 or constraint.SecondPos != 0:
                continue
            first_matches = constraint.First in split_line_ids and constraint.Second == other_line
            second_matches = constraint.Second in split_line_ids and constraint.First == other_line
            if first_matches or second_matches:
                return

        self.fail("Line-to-line distance constraint was not preserved on a split piece")

    def assertHasPointOnObject(self, sketch, point_geo, point_pos, curve_geo):
        for constraint in sketch.Constraints:
            if constraint.Type != "PointOnObject":
                continue
            first_is_point = (
                constraint.First == point_geo
                and constraint.FirstPos == point_pos
                and constraint.Second == curve_geo
                and constraint.SecondPos == 0
            )
            second_is_point = (
                constraint.Second == point_geo
                and constraint.SecondPos == point_pos
                and constraint.First == curve_geo
                and constraint.FirstPos == 0
            )
            if first_is_point or second_is_point:
                return

        self.fail("Point-on-object constraint was not preserved on the expected split piece")

    def makeParallelLinesSketch(self, reversed_distance=False):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "ParallelLines")
        first_line = sketch.addGeometry(
            Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0))
        )
        second_line = sketch.addGeometry(
            Part.LineSegment(App.Vector(0, 5, 0), App.Vector(10, 5, 0))
        )
        sketch.addConstraint(Sketcher.Constraint("Parallel", first_line, second_line))
        if reversed_distance:
            sketch.addConstraint(Sketcher.Constraint("Distance", second_line, first_line, 5.0))
        else:
            sketch.addConstraint(Sketcher.Constraint("Distance", first_line, second_line, 5.0))
        self.Doc.recompute()
        return sketch, first_line, second_line

    def testSplitPreservesLineLineDistance(self):
        sketch, split_line, other_line = self.makeParallelLinesSketch()

        sketch.split(split_line, App.Vector(4, 0, 0))
        self.Doc.recompute()

        self.assertHasLineLineDistance(sketch, {0, 2}, other_line)

    def testSplitPreservesReversedLineLineDistance(self):
        sketch, split_line, other_line = self.makeParallelLinesSketch(reversed_distance=True)

        sketch.split(split_line, App.Vector(4, 0, 0))
        self.Doc.recompute()

        self.assertHasLineLineDistance(sketch, {0, 2}, other_line)

    def testSplitPreservesPointOnObjectOnSecondPiece(self):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "PointOnObjectSplit")
        split_line = sketch.addGeometry(
            Part.LineSegment(App.Vector(0, 0, 0), App.Vector(10, 0, 0))
        )
        crossing_line = sketch.addGeometry(
            Part.LineSegment(App.Vector(7, 0, 0), App.Vector(7, 4, 0))
        )
        sketch.addConstraint(Sketcher.Constraint("PointOnObject", crossing_line, 1, split_line))
        self.Doc.recompute()

        sketch.split(split_line, App.Vector(4, 0, 0))
        self.Doc.recompute()

        self.assertHasPointOnObject(sketch, crossing_line, 1, 2)
