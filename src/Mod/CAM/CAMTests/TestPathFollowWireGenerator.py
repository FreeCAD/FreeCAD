# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import Part
import Path
import Path.Base.Generator.follow_wire as generator

import CAMTests.PathTestUtils as PathTestUtils


class TestPathFollowWireGenerator(PathTestUtils.PathTestBase):
    """Tests for the wire-follow generator.

    The generator is deliberately dumb: it takes a wire that already
    represents exactly one pass at the correct depth and turns it into
    commands.  All depth scaling and pass logic belongs to the caller.
    """

    def test00(self):
        """Test basic wire follow generator return"""
        wire = Part.Wire([Part.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, -5))])

        result = generator.generate(wire, retract_z=5, horiz_feed=100, vert_feed=50, arc_chord=0.1)

        self.assertIsInstance(result, list)
        self.assertIsInstance(result[0], Path.Command)

        # G0 to entry XY, G1 plunge, G1 cut, G0 retract
        self.assertEqual(len(result), 4)

        self.assertEqual(result[0].Name, "G0")
        self.assertRoughly(result[0].Parameters["X"], 0)
        self.assertRoughly(result[0].Parameters["Y"], 0)
        # the entry rapid must not command Z -- the caller positions above
        # the work, and adding Z here would plunge at rapid speed
        self.assertNotIn("Z", result[0].Parameters)

        self.assertEqual(result[1].Name, "G1")
        self.assertRoughly(result[1].Parameters["Z"], 0)
        self.assertEqual(result[1].Parameters["F"], 50)

        self.assertEqual(result[2].Name, "G1")
        self.assertRoughly(result[2].Parameters["X"], 10)
        self.assertRoughly(result[2].Parameters["Z"], -5)
        self.assertEqual(result[2].Parameters["F"], 100)

        self.assertEqual(result[3].Name, "G0")
        self.assertEqual(result[3].Parameters["Z"], 5)

    def test10(self):
        """Test that the first point of the wire is the entry point"""
        # same segment drawn the other way round
        wire = Part.Wire([Part.makeLine(FreeCAD.Vector(10, 0, -5), FreeCAD.Vector(0, 0, 0))])

        result = generator.generate(wire, retract_z=5, horiz_feed=100, vert_feed=50, arc_chord=0.1)

        # entry follows the wire's own direction, no reordering
        self.assertRoughly(result[0].Parameters["X"], 10)
        self.assertRoughly(result[1].Parameters["Z"], -5)
        self.assertRoughly(result[2].Parameters["X"], 0)
        self.assertRoughly(result[2].Parameters["Z"], 0)

    def test20(self):
        """Test multi-edge wire produces one cutting move per waypoint"""
        v0 = FreeCAD.Vector(0, 0, 0)
        v1 = FreeCAD.Vector(10, 0, -2)
        v2 = FreeCAD.Vector(10, 10, -4)
        wire = Part.Wire([Part.makeLine(v0, v1), Part.makeLine(v1, v2)])

        result = generator.generate(wire, retract_z=5, horiz_feed=100, vert_feed=50, arc_chord=0.1)

        # G0, plunge, 2 cutting moves, retract
        self.assertEqual(len(result), 5)
        self.assertRoughly(result[2].Parameters["X"], 10)
        self.assertRoughly(result[2].Parameters["Y"], 0)
        self.assertRoughly(result[3].Parameters["X"], 10)
        self.assertRoughly(result[3].Parameters["Y"], 10)
        self.assertRoughly(result[3].Parameters["Z"], -4)

    def test30(self):
        """Test arc_chord controls how finely curves are discretized"""
        arc = Part.Edge(Part.Circle(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1), 10), 0, 1.5)
        wire = Part.Wire([arc])

        coarse = generator.generate(wire, retract_z=5, horiz_feed=100, vert_feed=50, arc_chord=1.0)
        fine = generator.generate(wire, retract_z=5, horiz_feed=100, vert_feed=50, arc_chord=0.05)

        # a tighter chord tolerance means more G1 segments along the same arc
        self.assertGreater(len(fine), len(coarse))
