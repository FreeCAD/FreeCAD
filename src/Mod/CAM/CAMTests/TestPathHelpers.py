# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Part
import Path
import Path.Base.FeedRate as PathFeedRate
import Path.Base.MachineState as PathMachineState
from Path.Tool.toolbit import ToolBit
import Path.Tool.Controller as PathToolController
import PathScripts.PathUtils as PathUtils

from CAMTests.PathTestUtils import PathTestBase


def createTool(name="t1", diameter=1.75):
    attrs = {
        "name": name or "t1",
        "shape": "endmill.fcstd",
        "parameter": {"Diameter": diameter},
        "attribute": {},
    }
    toolbit = ToolBit.from_dict(attrs)
    return toolbit.attach_to_doc(doc=FreeCAD.ActiveDocument)


class TestPathHelpers(PathTestBase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("TestPathUtils")

        c1 = Path.Command("G0 Z10 F1000")
        c2 = Path.Command("G0 X20 Y10 F1001")
        c3 = Path.Command("G1 X20 Y10 Z5 F110")
        c4 = Path.Command("G1 X20 Y20 F111")

        self.commandlist = [c1, c2, c3, c4]

    def tearDown(self):
        FreeCAD.closeDocument("TestPathUtils")

    def test00(self):
        """Test that FeedRate Helper populates horiz and vert feed rate based on TC"""
        t = createTool("test", 5.0)
        tc = PathToolController.Create("TC0", t)
        tc.VertRapid = 5
        tc.HorizRapid = 10
        tc.VertFeed = 15
        tc.HorizFeed = 20

        resultlist = PathFeedRate.setFeedRate(self.commandlist, tc)
        print(resultlist)

        self.assertTrue(resultlist[0].Parameters["F"] == 5)
        self.assertTrue(resultlist[1].Parameters["F"] == 10)
        self.assertTrue(resultlist[2].Parameters["F"] == 15)
        self.assertTrue(resultlist[3].Parameters["F"] == 20)

    def test01(self):
        """Test that Machine State initializes and updates position correctly"""

        machine = PathMachineState.MachineState()
        self.assertTrue(machine.X == 0)
        self.assertTrue(machine.Y == 0)
        self.assertTrue(machine.Z == 0)
        self.assertTrue(machine.F == 0)
        self.assertTrue(machine.G0F == 0)
        self.assertTrue(machine.ReturnMode == "Z")
        self.assertTrue(machine.WCS == "G54")

        for c in self.commandlist:
            result = machine.addCommand(c)

        self.assertTrue(machine.X == 20)
        self.assertTrue(machine.Y == 20)
        self.assertTrue(machine.Z == 5)
        # G0F separate from F
        # The commandlist does G0...G1, so G1 is after G0:
        # (Does not prove (yet) that G0 does-not affect F, see below)
        self.assertTrue(machine.F == 111) # G1's
        self.assertTrue(machine.G0F == 1001)

        machine.addCommand(Path.Command("M3 S200"))
        self.assertTrue(machine.S == 200)
        self.assertTrue(machine.Spindle == "CW")

        machine.addCommand(Path.Command("M4 S200"))
        self.assertTrue(machine.Spindle == "CCW")

        machine.addCommand(Path.Command("M2"))
        self.assertTrue(machine.Spindle == "off")
        self.assertTrue(machine.S == 0)

        machine.addCommand(Path.Command("G57"))
        self.assertTrue(machine.WCS == "G57")

        machine.addCommand(Path.Command("M6 T5"))
        self.assertTrue(machine.T == 5)

        # Test that non-change commands return false
        result = machine.addCommand(Path.Command("G0 X20"))
        self.assertFalse(result)

        result = machine.addCommand(Path.Command("G0 X30"))
        self.assertTrue(result)

        # Test that Drilling moves are handled correctly
        result = machine.addCommands([
            Path.Command("G98"),
            Path.Command("G81 X50 Y50 Z0"),
        ])
        self.assertTrue(machine.X == 50)
        self.assertTrue(machine.Y == 50)
        self.assertTrue(machine.Z == 5)
        self.assertTrue(machine.ReturnMode == "Z")

        result = machine.addCommands([
            Path.Command("G99"),
            Path.Command("G81 X50 Y50 Z0 R10"),
        ])
        self.assertTrue(machine.Z == 10)
        self.assertTrue(machine.ReturnMode == "R")

        # Prove G0 F does not interfere with G1 F
        # Would only interfere if a G0 was after a G1, so, final G0:
        machine.addCommand( Path.Command("G0 X510 Y500 Z0 F99"))
        self.assertTrue(machine.F == 111)
        self.assertEqual(machine.G0F, 99)

        # Test process list of commands
        machine = PathMachineState.MachineState()
        machine.addCommands(self.commandlist[0])
        machine.addCommands(self.commandlist[1:])
        self.assertEqual(machine.X, 20)
        self.assertEqual(machine.Y, 20)
        self.assertEqual(machine.Z, 5)

        # Test Tracked and .setState
        # we are relying on MachineState to not validate the tracked parameters
        expected = { k:"x"+k for k in PathMachineState.MachineState.Tracked }
        self.assertNotEqual( expected, {}, "Tracked has some keys in it" ) # sanity
        machine.setState( expected )
        self.assertEqual( machine.getState(), expected, "Set all w/Tracked" )

        machine.setState( None )
        expected = { k:None for k in PathMachineState.MachineState.Tracked }
        self.assertEqual( machine.getState(), expected, "All None" )

    def test02(self):
        """Test PathUtils filterarcs"""

        # filter a full circle
        c = Part.Circle()
        c.Radius = 5
        edge = c.toShape()

        results = PathUtils.filterArcs(edge)
        self.assertTrue(len(results) == 2)
        e1 = results[0]
        self.assertTrue(isinstance(e1.Curve, Part.Circle))
        self.assertTrue(Path.Geom.pointsCoincide(edge.Curve.Location, e1.Curve.Location))
        self.assertTrue(edge.Curve.Radius == e1.Curve.Radius)

        # filter a 180 degree arc
        results = PathUtils.filterArcs(e1)
        self.assertTrue(len(results) == 1)

        # Handle a straight segment
        v1 = FreeCAD.Vector(0, 0, 0)
        v2 = FreeCAD.Vector(10, 0, 0)
        l = Part.makeLine(v1, v2)
        results = PathUtils.filterArcs(l)
        self.assertTrue(len(results) == 0)
