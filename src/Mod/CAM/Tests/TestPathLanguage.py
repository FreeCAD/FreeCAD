# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2022 sliptonic <shopinthewoods@gmail.com>               *
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

import Path.Base.Language as PathLanguage
import Tests.PathTestUtils as PathTestUtils
import math

PI = math.pi


def MNVR(gcode, begin=None):
    # 'turns out the replace() isn't really necessary
    # leave it here anyway for clarity
    return PathLanguage.Maneuver.FromGCode(gcode.replace("/", "\n"), begin)


def INSTR(gcode, begin=None):
    return MNVR(gcode, begin).instr[0]


class TestPathLanguage(PathTestUtils.PathTestBase):
    """Unit tests for the Language classes."""

    def assertTangents(self, instr, t1):
        """Assert that the two tangent angles are identical"""
        t0 = instr.anglesOfTangents()
        self.assertRoughly(t0[0], t1[0])
        self.assertRoughly(t0[1], t1[1])

    def test00(self):
        """Verify G0 instruction construction"""
        self.assertEqual(str(MNVR("")), "")
        self.assertEqual(len(MNVR("").instr), 0)

        self.assertEqual(str(MNVR("G0")), "G0{}")
        self.assertEqual(str(MNVR("G0X3")), "G0{'X': 3.0}")
        self.assertEqual(str(MNVR("G0X3Y7")), "G0{'X': 3.0, 'Y': 7.0}")
        self.assertEqual(
            str(MNVR("G0X3Y7/G0Z0")), "G0{'X': 3.0, 'Y': 7.0}\nG0{'Z': 0.0}"
        )
        self.assertEqual(len(MNVR("G0X3Y7").instr), 1)
        self.assertEqual(len(MNVR("G0X3Y7/G0Z0").instr), 2)
        self.assertEqual(type(MNVR("G0X3Y7").instr[0]), PathLanguage.MoveStraight)

    def test10(self):
        """Verify G1 instruction construction"""
        self.assertEqual(str(MNVR("G1")), "G1{}")
        self.assertEqual(str(MNVR("G1X3")), "G1{'X': 3.0}")
        self.assertEqual(str(MNVR("G1X3Y7")), "G1{'X': 3.0, 'Y': 7.0}")
        self.assertEqual(
            str(MNVR("G1X3Y7/G1Z0")), "G1{'X': 3.0, 'Y': 7.0}\nG1{'Z': 0.0}"
        )
        self.assertEqual(len(MNVR("G1X3Y7").instr), 1)
        self.assertEqual(len(MNVR("G1X3Y7/G1Z0").instr), 2)
        self.assertEqual(type(MNVR("G1X3Y7").instr[0]), PathLanguage.MoveStraight)

    def test20(self):
        """Verify G2 instruction construction"""
        self.assertEqual(str(MNVR("G2X2Y2I1")), "G2{'I': 1.0, 'X': 2.0, 'Y': 2.0}")
        self.assertEqual(len(MNVR("G2X2Y2I1").instr), 1)
        self.assertEqual(type(MNVR("G2X2Y2I1").instr[0]), PathLanguage.MoveArcCW)

    def test30(self):
        """Verify G3 instruction construction"""
        self.assertEqual(str(MNVR("G3X2Y2I1")), "G3{'I': 1.0, 'X': 2.0, 'Y': 2.0}")
        self.assertEqual(len(MNVR("G3X2Y2I1").instr), 1)
        self.assertEqual(type(MNVR("G3X2Y2I1").instr[0]), PathLanguage.MoveArcCCW)

    def test40(self):
        """Verify pathLength correctness"""
        self.assertRoughly(MNVR("G1X3").instr[0].pathLength(), 3)
        self.assertRoughly(MNVR("G1X-7").instr[0].pathLength(), 7)
        self.assertRoughly(MNVR("G1X3").instr[0].pathLength(), 3)

        self.assertRoughly(MNVR("G1X3Y4").instr[0].pathLength(), 5)
        self.assertRoughly(MNVR("G1X3Y-4").instr[0].pathLength(), 5)
        self.assertRoughly(MNVR("G1X-3Y-4").instr[0].pathLength(), 5)
        self.assertRoughly(MNVR("G1X-3Y4").instr[0].pathLength(), 5)

        self.assertRoughly(MNVR("G2X2I1").instr[0].pathLength(), PI)
        self.assertRoughly(MNVR("G2X1Y1I1").instr[0].pathLength(), PI / 2)

        self.assertRoughly(MNVR("G3X2I1").instr[0].pathLength(), PI)
        self.assertRoughly(MNVR("G3X1Y1I1").instr[0].pathLength(), 3 * PI / 2)

    def test50(self):
        """Verify tangents of moves."""

        self.assertTangents(INSTR("G1 X0  Y0"), (0, 0))  # by declaration
        self.assertTangents(INSTR("G1 X1  Y0"), (0, 0))
        self.assertTangents(INSTR("G1 X-1 Y0"), (PI, PI))
        self.assertTangents(INSTR("G1 X0  Y1"), (PI / 2, PI / 2))
        self.assertTangents(INSTR("G1 X0  Y-1"), (-PI / 2, -PI / 2))
        self.assertTangents(INSTR("G1 X1  Y1"), (PI / 4, PI / 4))
        self.assertTangents(INSTR("G1 X-1 Y1"), (3 * PI / 4, 3 * PI / 4))
        self.assertTangents(INSTR("G1 X-1 Y -1"), (-3 * PI / 4, -3 * PI / 4))
        self.assertTangents(INSTR("G1 X1  Y-1"), (-PI / 4, -PI / 4))

        self.assertTangents(INSTR("G2 X2  Y0  I1 J0"), (PI / 2, -PI / 2))
        self.assertTangents(INSTR("G2 X2  Y2  I1 J1"), (3 * PI / 4, -PI / 4))
        self.assertTangents(INSTR("G2 X0  Y-2 I0 J-1"), (0, -PI))

        self.assertTangents(INSTR("G3 X2  Y0  I1 J0"), (-PI / 2, PI / 2))
        self.assertTangents(INSTR("G3 X2  Y2  I1 J1"), (-PI / 4, 3 * PI / 4))
        self.assertTangents(INSTR("G3 X0  Y-2 I0 J-1"), (PI, 0))
