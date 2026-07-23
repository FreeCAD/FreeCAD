# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2021 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENSE text file.                                 *
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
import Path.Base.Generator.tapping as generator
import CAMTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestPathTapGenerator(PathTestUtils.PathTestBase):
    def test00(self):
        """Test Basic Tap Generator Return"""
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)

        e = Part.makeLine(v1, v2)

        result = generator.generate(e)

        self.assertTrue(type(result) is list)
        self.assertTrue(type(result[0]) is Path.Command)

        command = result[0]

        self.assertTrue(command.Name == "G84")
        self.assertEqual(command.Parameters["R"], 10)
        self.assertEqual(command.Parameters["X"], 0)
        self.assertEqual(command.Parameters["Y"], 0)
        self.assertEqual(command.Parameters["Z"], 0)
        self.assertEqual(command.Annotations["rigid"], "False")

        # repeat must be > 0
        args = {"edge": e, "repeat": 0}
        self.assertRaises(ValueError, generator.generate, **args)

        # repeat must be integer
        args = {"edge": e, "repeat": 1.5}
        self.assertRaises(ValueError, generator.generate, **args)

    def test10(self):
        """Test edge alignment check"""
        v1 = FreeCAD.Vector(0, 10, 10)
        v2 = FreeCAD.Vector(0, 0, 0)
        e = Part.makeLine(v1, v2)
        self.assertRaises(ValueError, generator.generate, e)

        v1 = FreeCAD.Vector(0, 0, 0)
        v2 = FreeCAD.Vector(0, 0, 10)
        e = Part.makeLine(v1, v2)

        self.assertRaises(ValueError, generator.generate, e)

    def test30(self):
        """Test Basic Dwell Tap Generator Return"""
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)

        e = Part.makeLine(v1, v2)

        result = generator.generate(e, dwelltime=0.5)

        self.assertTrue(type(result) is list)
        self.assertTrue(type(result[0]) is Path.Command)

        command = result[0]

        self.assertTrue(command.Name == "G84")
        self.assertTrue(command.Parameters["P"] == 0.5)

        # dwelltime should be a float
        args = {"edge": e, "dwelltime": 1}
        self.assertRaises(ValueError, generator.generate, **args)

    def test40(self):
        """Specifying retract height should set R parameter to specified value"""
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)

        e = Part.makeLine(v1, v2)

        result = generator.generate(e, retractheight=20.0)

        command = result[0]

        self.assertTrue(command.Parameters["R"] == 20.0)

    def test41(self):
        """Not specifying retract height should set R parameter to Z position of start point"""
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)

        e = Part.makeLine(v1, v2)

        result = generator.generate(e)

        command = result[0]

        self.assertTrue(command.Parameters["R"] == 10.0)

    def test44(self):
        """Non-float retract height should raise ValueError"""
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)

        e = Part.makeLine(v1, v2)

        args = {"edge": e, "retractheight": 1}
        self.assertRaises(ValueError, generator.generate, **args)
        args = {"edge": e, "retractheight": "1"}
        self.assertRaises(ValueError, generator.generate, **args)

    def test50_rigid_tap_feed_locked_to_pitch_and_speed(self):
        """G84 feed rate must be pitch * spindle_speed (mm/s), not pitch alone.

        Regression test: F was previously set to the raw pitch value with
        spindle_speed unused, silently ignoring the RPM entirely. Rigid tap
        feed must stay mechanically locked to spindle speed: mm/min =
        pitch(mm) * RPM. Values below match a real 10-32 tap (0.79248mm
        pitch) at 128 RPM.
        """
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)
        e = Part.makeLine(v1, v2)

        pitch = 0.79248
        spindle_speed = 128.0

        result = generator.generate(e, pitch=pitch, spindle_speed=spindle_speed)
        command = result[0]

        expected_f = pitch * spindle_speed / 60.0
        self.assertAlmostEqual(command.Parameters["F"], expected_f, places=6)
        # F must not equal the raw pitch alone (the previous, buggy behavior)
        self.assertNotAlmostEqual(command.Parameters["F"], pitch, places=3)
        self.assertEqual(command.Parameters["S"], spindle_speed)

    def test51_rigid_tap_feed_sanity_default_without_pitch_or_speed(self):
        """Without pitch/spindle_speed, F should fall back to the sanity default."""
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)
        e = Part.makeLine(v1, v2)

        result = generator.generate(e)
        self.assertEqual(result[0].Parameters["F"], 100.0)

        result = generator.generate(e, pitch=0.79248)
        self.assertEqual(result[0].Parameters["F"], 100.0)
