# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 phaseloop <phaseloop@protonmail.com>               *
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
from Path.Dressup.Array import DressupArray
import Path.Main.Job as PathJob
import Path.Op.Profile as PathProfile

from CAMTests.PathTestUtils import PathTestBase


class TestEngrave:
    def __init__(self, path):
        self.Path = Path.Path(path)
        self.ToolController = None  # default tool 5mm
        self.CoolantMode = "None"
        self.Name = "Engrave"

    def isDerivedFrom(self, type):
        if type == "Path::Feature":
            return True
        return False


class TestFeature:
    def __init__(self):
        self.Path = Path.Path()
        self.Name = ""

    def addProperty(self, typ, name, category, tip):
        setattr(self, name, None)

    def setEditorMode(self, prop, mode):
        pass


class TestDressupArray(PathTestBase):
    """Unit tests for the Array dressup."""

    def test00(self):
        """Verify array with zero copies provides original path."""

        source_gcode = "G0 X0 Y0 Z0\n" "G1 X10 Y10 Z0\n"

        expected_gcode = "G0 X0.000000 Y0.000000 Z0.000000\n" "G1 X10.000000 Y10.000000 Z0.000000\n"

        base = TestEngrave(source_gcode)
        obj = TestFeature()
        da = DressupArray(obj, base, None)
        da.execute(obj)
        self.assertTrue(obj.Path.toGCode() == expected_gcode, "Incorrect g-code generated")

    def test01(self):
        """Verify linear x/y/z 1D array with 1 copy."""

        source_gcode = "G0 X0 Y0 Z0\n" "G1 X10 Y10 Z0\n"

        expected_gcode = (
            "G0 X0.000000 Y0.000000 Z0.000000\n"
            "G1 X10.000000 Y10.000000 Z0.000000\n"
            "G0 X12.000000 Y12.000000 Z5.000000\n"
            "G1 X22.000000 Y22.000000 Z5.000000\n"
        )

        base = TestEngrave(source_gcode)
        obj = TestFeature()
        da = DressupArray(obj, base, None)
        obj.Copies = 1
        obj.Offset = FreeCAD.Vector(12, 12, 5)

        da.execute(obj)
        self.assertTrue(obj.Path.toGCode() == expected_gcode, "Incorrect g-code generated")

    def test02(self):
        """Verify linear x/y/z 2D array."""

        source_gcode = "G0 X0 Y0 Z0\n" "G1 X10 Y10 Z0\n"

        expected_gcode = (
            "G0 X0.000000 Y0.000000 Z0.000000\n"
            "G1 X10.000000 Y10.000000 Z0.000000\n"
            "G0 X0.000000 Y6.000000 Z0.000000\n"
            "G1 X10.000000 Y16.000000 Z0.000000\n"
            "G0 X12.000000 Y6.000000 Z0.000000\n"
            "G1 X22.000000 Y16.000000 Z0.000000\n"
            "G0 X12.000000 Y0.000000 Z0.000000\n"
            "G1 X22.000000 Y10.000000 Z0.000000\n"
            "G0 X24.000000 Y0.000000 Z0.000000\n"
            "G1 X34.000000 Y10.000000 Z0.000000\n"
            "G0 X24.000000 Y6.000000 Z0.000000\n"
            "G1 X34.000000 Y16.000000 Z0.000000\n"
        )

        base = TestEngrave(source_gcode)
        obj = TestFeature()
        da = DressupArray(obj, base, None)
        obj.Type = "Linear2D"
        obj.Copies = 0
        obj.CopiesX = 2
        obj.CopiesY = 1

        obj.Offset = FreeCAD.Vector(12, 6, 0)

        da.execute(obj)
        self.assertTrue(obj.Path.toGCode() == expected_gcode, "Incorrect g-code generated")
