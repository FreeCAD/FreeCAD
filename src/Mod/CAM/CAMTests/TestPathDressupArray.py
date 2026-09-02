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
from Path.Dressup.Array import DressupArray, PathArray
import Path.Main.Job as PathJob
import Path.Op.Profile as PathProfile

from CAMTests.PathTestUtils import PathTestBase


class _TestEngrave:
    def __init__(self, path):
        self.Path = Path.Path([Path.Command(x) for x in path.split("\n")])
        self.ToolController = None  # default tool 5mm
        self.CoolantMode = "None"
        self.Name = "Engrave"
        self.Active = True

    def isDerivedFrom(self, type):
        if type == "Path::Feature":
            return True
        return False


class _TestFeature:
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

        expected_gcode = (
            "G0 X0.000000 Y0.000000 Z0.000000\n" "G1 X10.000000 Y10.000000 Z0.000000\n\n"
        )

        base = _TestEngrave(source_gcode)
        obj = _TestFeature()
        da = DressupArray(obj, base, None)
        da.execute(obj)
        self.assertEqual(obj.Path.toGCode(), expected_gcode, "Incorrect g-code generated")

    def test01(self):
        """Verify linear x/y/z 1D array with 1 copy."""

        source_gcode = "G0 X0 Y0 Z0\n" "G1 X10 Y10 Z0\n"

        expected_gcode = (
            "G0 X0.000000 Y0.000000 Z0.000000\n"
            "G1 X10.000000 Y10.000000 Z0.000000\n\n"
            "G0 X12.000000 Y12.000000 Z5.000000\n"
            "G1 X22.000000 Y22.000000 Z5.000000\n\n"
        )

        base = _TestEngrave(source_gcode)
        obj = _TestFeature()
        da = DressupArray(obj, base, None)
        obj.Copies = 1
        obj.Offset = FreeCAD.Vector(12, 12, 5)

        da.execute(obj)
        gcode = obj.Path.toGCode()
        self.assertEqual(
            gcode, expected_gcode, f"Incorrect g-code generated---\n{expected_gcode}---\n{gcode}"
        )

    def test02(self):
        """Verify linear x/y/z 2D array."""

        source_gcode = "G0 X0 Y0 Z0\n" "G1 X10 Y10 Z0\n"

        expected_gcode = (
            "G0 X0.000000 Y0.000000 Z0.000000\n"
            "G1 X10.000000 Y10.000000 Z0.000000\n\n"
            "G0 X0.000000 Y6.000000 Z0.000000\n"
            "G1 X10.000000 Y16.000000 Z0.000000\n\n"
            "G0 X12.000000 Y6.000000 Z0.000000\n"
            "G1 X22.000000 Y16.000000 Z0.000000\n\n"
            "G0 X12.000000 Y0.000000 Z0.000000\n"
            "G1 X22.000000 Y10.000000 Z0.000000\n\n"
            "G0 X24.000000 Y0.000000 Z0.000000\n"
            "G1 X34.000000 Y10.000000 Z0.000000\n\n"
            "G0 X24.000000 Y6.000000 Z0.000000\n"
            "G1 X34.000000 Y16.000000 Z0.000000\n\n"
        )

        base = _TestEngrave(source_gcode)
        obj = _TestFeature()
        da = DressupArray(obj, base, None)
        obj.Type = "Linear2D"
        obj.Copies = 0
        obj.CopiesX = 2
        obj.CopiesY = 1

        obj.Offset = FreeCAD.Vector(12, 6, 0)

        da.execute(obj)
        gcode = obj.Path.toGCode()
        self.assertEqual(
            gcode, expected_gcode, f"Incorrect g-code generated---\n{expected_gcode}---\n{gcode}"
        )

    def test03(self):
        """Tolerate annotations in getPath()
        Some operations make gcode with annotations, e.g. Drill
        """
        # This gcode has several characteristics that cause problems if given to Path.Path():
        # 'M' in RetractMode will be seen as the start of an M-code,
        # G98 in the annotation value will be seen as separate G-code.
        # e.g. Path.Path(source_gcode) will throw Base::BadFormatError("Badly formatted GCode command")
        # But, if things are left as Path.Command, things should be fine.
        source_gcode = "G81 F6 R0 X7 Y9 Z-3 ; RetractMode:'G98'"

        base = _TestEngrave(source_gcode)
        obj = _TestFeature()
        obj.Copies = 1
        obj.CopiesX = 2
        obj.CopiesY = 1
        obj.Offset = FreeCAD.Vector(12, 12, 5)
        pa = PathArray(
            base,
            "Linear1D",
            obj.Copies,
            obj.Offset,
            obj.CopiesX,
            obj.CopiesY,
            angle=None,
            centre=None,
            swapDirection=False,
            jitterMagnitude=0,
            jitterPercent=0,
            seed=obj.Name,
        )

        path = pa.getPath()
        gcode = "\n".join(x.toGCode() for x in path.Commands)

        # And we kept the annotation
        self.assertIn("; RetractMode:'G98'", gcode)
