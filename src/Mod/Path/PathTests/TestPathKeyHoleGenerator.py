# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2021 wayofwood <code@wayofwood.com>                     *
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

import Path
import FreeCAD
import Generators.keyhole_generator as generator
import PathScripts.PathLog as PathLog
import PathTests.PathTestUtils as PathTestUtils

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
PathLog.trackModule(PathLog.thisModule())

class TestPathKeyHoleGenerator(PathTestUtils.PathTestBase):
    def test00(self):
        """Test KeyHole in X-direction."""
        # Circle in the center of the coordinate system
        sp = FreeCAD.Base.Vector(0, 0, 0)
        ep = FreeCAD.Base.Vector(10, 0, 0)

        result = generator.generate(sp, ep)

        self.assertTrue(type(result) is list)
        self.assertTrue(type(result[0]) is Path.Command)

        command = result[0]
        self.assertTrue(command.Name == "G1")
        self.assertTrue(command.Parameters["X"] < 1e-06)
        self.assertTrue(command.Parameters["Y"] < 1e-06)
        self.assertTrue(command.Parameters["Z"] == 0)

        command = result[1]
        self.assertTrue(command.Name == "G1")
        print(command.Parameters["X"])
        self.assertTrue(command.Parameters["X"] - 10 < 1e-06)
        self.assertTrue(command.Parameters["X"] - 10 > -1e-06)
        self.assertTrue(command.Parameters["Y"] < 1e-06)
        self.assertTrue(command.Parameters["Z"] == 0)

        command = result[2]
        self.assertTrue(command.Name == "G1")
        self.assertTrue(command.Parameters["X"] < 1e-06)
        self.assertTrue(command.Parameters["Y"] < 1e-06)
        self.assertTrue(command.Parameters["Z"] == 0)

    def test10(self):
        """Test KeyHole in Y-direction."""
        sp = FreeCAD.Base.Vector(0, 0, 0)
        ep = FreeCAD.Base.Vector(0, 10, 0)

        result = generator.generate(sp, ep)

        self.assertTrue(type(result) is list)
        self.assertTrue(type(result[0]) is Path.Command)

        command = result[0]
        self.assertTrue(command.Name == "G1")
        self.assertTrue(command.Parameters["X"] < 1e-06)
        self.assertTrue(command.Parameters["Y"] < 1e-06)
        self.assertTrue(command.Parameters["Z"] == 0)

        command = result[1]
        self.assertTrue(command.Name == "G1")
        self.assertTrue(command.Parameters["X"] < 1e-06)
        self.assertTrue(command.Parameters["Y"] - 10 < 1e-06)
        self.assertTrue(command.Parameters["Y"] - 10 > -1e-06)
        self.assertTrue(command.Parameters["Z"] == 0)

        command = result[2]
        self.assertTrue(command.Name == "G1")
        self.assertTrue(command.Parameters["X"] < 1e-06)
        self.assertTrue(command.Parameters["Y"] < 1e-06)
        self.assertTrue(command.Parameters["Z"] == 0)
