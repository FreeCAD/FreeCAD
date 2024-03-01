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
import Path
import Path.Base.Generator.rotation as generator
import Tests.PathTestUtils as PathTestUtils
import numpy as np

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestPathRotationGenerator(PathTestUtils.PathTestBase):
    def test00(self):
        """Test relAngle function"""
        v = FreeCAD.Vector(0.5, 0.5, 0.5)
        self.assertTrue(np.isclose(generator.relAngle(v, generator.refAxis.x), 45))
        self.assertTrue(np.isclose(generator.relAngle(v, generator.refAxis.y), 45))
        self.assertTrue(np.isclose(generator.relAngle(v, generator.refAxis.z), 45))

        v = FreeCAD.Vector(-0.5, 0.5, 0.5)
        self.assertTrue(np.isclose(generator.relAngle(v, generator.refAxis.x), 135))
        self.assertTrue(np.isclose(generator.relAngle(v, generator.refAxis.y), -45))
        self.assertTrue(np.isclose(generator.relAngle(v, generator.refAxis.z), 45))

        v = FreeCAD.Vector(-0.5, -0.5, -0.5)
        self.assertTrue(np.isclose(generator.relAngle(v, generator.refAxis.x), -135))
        self.assertTrue(np.isclose(generator.relAngle(v, generator.refAxis.y), -135))
        self.assertTrue(np.isclose(generator.relAngle(v, generator.refAxis.z), -135))

    def test10(self):
        """Test Basic Rotation Generator Return"""
        v1 = FreeCAD.Vector(0.0, 0.0, 1.0)
        args = {
            "normalVector": v1,
            "aMin": -360,
            "aMax": 360,
            "cMin": -360,
            "cMax": 360,
            "compound": True,
        }

        result = generator.generate(**args)

        self.assertTrue(type(result) is list)
        self.assertTrue(len(result) == 1)
        self.assertTrue(type(result[0]) is Path.Command)

        command = result[0]
        self.assertTrue(np.isclose(command.Parameters["A"], 0))
        self.assertTrue(np.isclose(command.Parameters["C"], 0))

        args["compound"] = False
        result = generator.generate(**args)
        self.assertTrue(len(result) == 2)

        Path.Log.debug(result)

    def test20(self):
        """Test non-zero rotation"""
        v1 = FreeCAD.Vector(0.5, 0.5, 0.5)
        args = {
            "normalVector": v1,
            "aMin": -360,
            "aMax": 360,
            "cMin": -360,
            "cMax": 360,
            "compound": True,
        }

        result = generator.generate(**args)

        command = result[0]
        Path.Log.debug(command.Parameters)
        self.assertTrue(np.isclose(command.Parameters["A"], 54.736))
        self.assertTrue(np.isclose(command.Parameters["C"], 45))

        Path.Log.track(result)

    def test30(self):
        """Test A limits"""
        v1 = FreeCAD.Vector(0.5, 0.5, 0.5)

        args = {"normalVector": v1, "cMin": -360, "cMax": 360, "compound": True}

        # Constrain a axis rotation negative
        args["aMin"] = -90
        args["aMax"] = 0

        result = generator.generate(**args)
        Path.Log.debug(result)

        command = result[0]
        self.assertTrue(np.isclose(command.Parameters["A"], -54.736))
        self.assertTrue(np.isclose(command.Parameters["C"], -135))

        # Constrain a axis rotation positive
        args["aMin"] = 0
        args["aMax"] = 90

        result = generator.generate(**args)
        Path.Log.debug(result)

        command = result[0]
        self.assertTrue(np.isclose(command.Parameters["A"], 54.736))
        self.assertTrue(np.isclose(command.Parameters["C"], 45))

    def test40(self):
        """Test C limits"""
        v1 = FreeCAD.Vector(0.5, 0.5, 0.5)

        args = {"normalVector": v1, "aMin": -360, "aMax": 360, "compound": True}

        # Constrain a axis rotation negative
        args["cMin"] = -180
        args["cMax"] = 0

        result = generator.generate(**args)
        Path.Log.debug(result)

        command = result[0]
        self.assertTrue(np.isclose(command.Parameters["A"], -54.736))
        self.assertTrue(np.isclose(command.Parameters["C"], -135))

        # Constrain a axis rotation positive
        args["cMin"] = 0
        args["cMax"] = 180

        result = generator.generate(**args)
        Path.Log.debug(result)

        command = result[0]
        self.assertTrue(np.isclose(command.Parameters["A"], 54.736))
        self.assertTrue(np.isclose(command.Parameters["C"], 45))

    def test50(self):
        """Test handling of no valid solution"""
        v1 = FreeCAD.Vector(0.5, 0.5, 0.5)
        args = {
            "normalVector": v1,
            "aMin": 0,
            "aMax": 10,
            "cMin": 0,
            "cMax": 10,
            "compound": True,
        }

        self.assertRaises(ValueError, generator.generate, **args)
