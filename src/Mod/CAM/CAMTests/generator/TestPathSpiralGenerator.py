# SPDX-License-Identifier: LGPL-2.1-or-later
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

import Constants
import FreeCAD
import math
import Part
import Path
import Path.Base.Generator.spiral as generator
import CAMTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


def _resetArgs():
    return {
        "center": FreeCAD.Vector(10, 10, 0),
        "outer_radius": 7.5,
        "step": 2.5,
        "inner_radius": 2.5,
        "direction": "CW",
        "startAt": "Inside",
        "dir_angle_rad": 0,
    }


class TestPathSpiralGenerator(PathTestUtils.PathTestBase):
    def test00(self):
        """Test Basic Spiral Generator Return"""
        args = _resetArgs()
        cmds = generator.generate(**args)
        self.assertTrue(cmds)
        self.assertTrue(isinstance(cmds, list))
        self.assertTrue(all(isinstance(cmd, Path.Command) for cmd in cmds))

        # first command is a straight move to start point
        self.assertIn(cmds[0].Name, Constants.GCODE_MOVE_STRAIGHT, "Init move should be G1")

        # check clockwise direction
        args["direction"] = "CW"
        cmds = generator.generate(**args)[1:]
        self.assertTrue(all(cmd.Name in Constants.GCODE_MOVE_CW for cmd in cmds))

        # check counterclockwise direction
        args["direction"] = "CCW"
        cmds = generator.generate(**args)[1:]
        self.assertTrue(all(cmd.Name in Constants.GCODE_MOVE_CCW for cmd in cmds))

    def test01(self):
        """Test Value and Type checking"""
        args = _resetArgs()
        args["center"] = ""
        self.assertRaises(TypeError, generator.generate, **args)
        args["center"] = (0, 0, 0)
        self.assertRaises(TypeError, generator.generate, **args)
        args["center"] = [0, 0, 0]
        self.assertRaises(TypeError, generator.generate, **args)

        # outer_radius is a length and can not be 0 or negative
        args = _resetArgs()
        args["outer_radius"] = 0
        self.assertRaises(ValueError, generator.generate, **args)
        args["outer_radius"] = -7.5
        self.assertRaises(ValueError, generator.generate, **args)
        args["outer_radius"] = "7.5"
        self.assertRaises(TypeError, generator.generate, **args)

        # step is a length and can not be 0 or negative
        args = _resetArgs()
        args["step"] = 0
        self.assertRaises(ValueError, generator.generate, **args)
        args["step"] = -3
        self.assertRaises(ValueError, generator.generate, **args)
        args["step"] = "5"
        self.assertRaises(TypeError, generator.generate, **args)

        # inner_radius is a length and can not be negative
        args = _resetArgs()
        args["inner_radius"] = -7.5
        self.assertRaises(ValueError, generator.generate, **args)
        args["inner_radius"] = "7.5"
        self.assertRaises(TypeError, generator.generate, **args)

        # outer_radius can not be less than inner_radius
        args = _resetArgs()
        args["inner_radius"] = 10
        args["outer_radius"] = 9
        self.assertRaises(ValueError, generator.generate, **args)

        # direction should be a string "CW" or "CCW"
        args = _resetArgs()
        args["direction"] = "clock"
        self.assertRaises(ValueError, generator.generate, **args)
        args["direction"] = "cw"
        self.assertRaises(ValueError, generator.generate, **args)

        # startAt should be a string "Inside" or "Outside"
        args = _resetArgs()
        args["startAt"] = "Other"
        self.assertRaises(ValueError, generator.generate, **args)
        args["startAt"] = "inside"
        self.assertRaises(ValueError, generator.generate, **args)

        # dir_angle_rad is a angle (radians) and can be any numerical value
        args = _resetArgs()
        args["dir_angle_rad"] = "0.0"
        self.assertRaises(TypeError, generator.generate, **args)

    def test02(self):
        """Test deviation spiral path from spiral shape"""
        inner_radius = 5.1
        step = 5
        turns = 3
        center = FreeCAD.Vector(10, 5, 0)
        dir_angle = 30

        # create Part spiral
        doc = FreeCAD.newDocument("TestPathSpiralGenerator")
        spiral = doc.addObject("Part::Spiral", "Spiral")
        spiral.Placement.Base = center
        spiral.Placement.Rotation.Angle = math.radians(dir_angle)
        spiral.Growth = step
        spiral.Radius = inner_radius
        spiral.Rotations = turns
        partSpiral = spiral.Shape

        # create Path spiral
        args = {}
        args["center"] = center
        args["step"] = step
        args["inner_radius"] = inner_radius
        args["outer_radius"] = inner_radius + step * turns
        args["direction"] = "CCW"
        args["startAt"] = "Inside"
        args["dir_angle_rad"] = math.radians(dir_angle)
        cmds = generator.generate(**args)[3:-2]

        # convert Path to Part.Wire
        edges = []
        pl = FreeCAD.Placement()
        pl.rotate(center, FreeCAD.Vector(0, 0, 1), dir_angle)
        startPoint = pl.multVec(FreeCAD.Vector(center.x + inner_radius, center.y, 0))
        for cmd in cmds:
            edges.append(Path.Geom.edgeForCmd(cmd, startPoint))
            startPoint = FreeCAD.Vector(cmd.x, cmd.y, cmd.z)
        pathSpiral = Part.Wire(Part.__sortEdges__(edges))

        # wires should have similar length
        lenghtDiff = abs(pathSpiral.Length - partSpiral.Length)
        self.assertLess(lenghtDiff, 0.1)

        # amount points after discretization should be identical
        pointsPathSpiral = pathSpiral.discretize(Distance=1)
        pointsPartSpiral = partSpiral.discretize(Distance=1)
        self.assertEqual(len(pointsPathSpiral), len(pointsPartSpiral))

        # for spiral with inner radius greater than 5 mm, deviation should be less than 0.1 mm
        for i in range(len(pointsPathSpiral)):
            dist = pointsPathSpiral[i].distanceToPoint(pointsPartSpiral[i])
            self.assertLess(dist, 0.1)

        # not checked here, but for spiral with inner radius less than or equel 5 mm,
        # deviation should be less than 0.01 mm

        FreeCAD.closeDocument(doc.Name)
