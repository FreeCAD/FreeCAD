# -*- coding: utf-8 -*-
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
import Path.Base.Generator.drill as generator
import Tests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestPathDrillGenerator(PathTestUtils.PathTestBase):
    def test00(self):
        """Test Basic Drill Generator Return"""
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)

        e = Part.makeLine(v1, v2)

        result = generator.generate(e)

        self.assertTrue(type(result) is list)
        self.assertTrue(type(result[0]) is Path.Command)

        command = result[0]

        self.assertTrue(command.Name == "G81")
        self.assertTrue(command.Parameters["R"] == 10)
        self.assertTrue(command.Parameters["X"] == 0)
        self.assertTrue(command.Parameters["Y"] == 0)
        self.assertTrue(command.Parameters["Z"] == 0)

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

    def test20(self):
        """Test Basic Peck Drill Generator Return"""
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)

        e = Part.makeLine(v1, v2)

        result = generator.generate(e, peckdepth=1.2)

        self.assertTrue(type(result) is list)
        self.assertTrue(type(result[0]) is Path.Command)

        command = result[0]

        self.assertTrue(command.Name == "G83")
        self.assertTrue(command.Parameters["Q"] == 1.2)

        # peckdepth must be a float
        args = {"edge": e, "peckdepth": 1}
        self.assertRaises(ValueError, generator.generate, **args)

    def test30(self):
        """Test Basic Dwell Drill Generator Return"""
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)

        e = Part.makeLine(v1, v2)

        result = generator.generate(e, dwelltime=0.5)

        self.assertTrue(type(result) is list)
        self.assertTrue(type(result[0]) is Path.Command)

        command = result[0]

        self.assertTrue(command.Name == "G82")
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

    def test42(self):
        """Non-float retract height should raise ValueError"""
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)

        e = Part.makeLine(v1, v2)

        args = {"edge": e, "retractheight": 1}
        self.assertRaises(ValueError, generator.generate, **args)
        args = {"edge": e, "retractheight": "1"}
        self.assertRaises(ValueError, generator.generate, **args)

    def test50(self):
        """Test Error if dwell and peck"""
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)

        e = Part.makeLine(v1, v2)

        # dwelltime should be a float
        args = {"edge": e, "dwelltime": 1.0, "peckdepth": 1.0}
        self.assertRaises(ValueError, generator.generate, **args)

    def test60(self):
        """Test chipBreak"""
        v1 = FreeCAD.Vector(0, 0, 10)
        v2 = FreeCAD.Vector(0, 0, 0)

        e = Part.makeLine(v1, v2)

        args = {"edge": e, "peckdepth": 1.0, "chipBreak": True}
        result = generator.generate(**args)
        command = result[0]

        self.assertTrue(command.Name == "G73")
