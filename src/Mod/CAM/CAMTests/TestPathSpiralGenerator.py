# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

import FreeCAD
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

    expectedSpiralGCode = "G1 X12.500000 Y10.000000 Z0.000000\
G2 I-2.500000 J0.000000 X7.500000 Y10.000000 Z0.000000\
G2 I2.500000 J0.000000 X12.500000 Y10.000000 Z0.000000\
G2 I-2.654608 J-0.383000 X11.458333 Y7.474093 Z0.000000\
G2 I-1.869182 J2.475521 X8.333333 Y7.113249 Z0.000000\
G2 I1.421870 J3.221280 X6.250000 Y10.000000 Z0.000000\
G2 I3.921685 J0.377771 X7.916667 Y13.608439 Z0.000000\
G2 I2.496990 J-3.571950 X12.291667 Y13.969283 Z0.000000\
G2 I-2.055668 J-4.311251 X15.000000 Y10.000000 Z0.000000\
G2 I-5.180609 J-0.374388 X12.708333 Y5.309029 Z0.000000\
G2 I-3.123169 J4.662427 X7.083333 Y4.948185 Z0.000000\
G2 I2.686086 J5.397979 X3.750000 Y10.000000 Z0.000000\
G2 I6.436062 J0.372095 X6.666667 Y15.773503 Z0.000000\
G2 I3.748771 J-5.750081 X13.541667 Y16.134347 Z0.000000\
G2 I-3.314743 J-6.483194 X17.500000 Y10.000000 Z0.000000\
G2 I-7.500000 J0.000000 X2.500000 Y10.000000 Z0.000000\
G2 I7.500000 J0.000000 X17.500000 Y10.000000 Z0.000000"

    def test00(self):
        """Test Basic Spiral Generator Return"""
        args = _resetArgs()
        result = generator.generate(**args)
        self.assertTrue(isinstance(result, list))
        self.assertTrue(isinstance(result[0], Path.Command))

        gcode = "".join([r.toGCode() for r in result])
        print()
        print("\n".join([str(r.toGCode()) + "\\" for r in result]))
        self.assertTrue(gcode == self.expectedSpiralGCode, "Incorrect spiral g-code generated")

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
