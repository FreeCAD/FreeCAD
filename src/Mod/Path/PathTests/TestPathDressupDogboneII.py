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

import FreeCAD
import Path
import Path.Base.Generator.dogboneII as dogboneII
import Path.Base.Language as PathLanguage
import Path.Dressup.DogboneII as PathDressupDogboneII
import PathTests.PathTestUtils as PathTestUtils
import math


# Path.Log.setLevel(Path.Log.Level.DEBUG)

PI = math.pi

def MNVR(gcode, begin=None):
    # 'turns out the replace() isn't really necessary
    # leave it here anyway for clarity
    return PathLanguage.Maneuver.FromGCode(gcode.replace('/', '\n'), begin)

def INSTR(gcode, begin=None):
    return MNVR(gcode, begin).instr[0]

def KINK(gcode, begin=None):
    maneuver = MNVR(gcode, begin)
    if len(maneuver.instr) != 2:
        return None
    return dogboneII.Kink(maneuver.instr[0], maneuver.instr[1])

class TestDressupDogboneII(PathTestUtils.PathTestBase):
    """Unit tests for the DogboneII dressup."""

    def test00(self):
        """Verify adaptive length"""

        def adaptive(k, a, n): 
            return PathDressupDogboneII.calc_length_adaptive(k, a, n, n)

        if True:
            # horizontal bones
            self.assertRoughly(adaptive(KINK('G1X1/G1X2'), 0, 1), 0)
            self.assertRoughly(adaptive(KINK('G1X1/G1Y1'), 0, 1), 1)
            self.assertRoughly(adaptive(KINK('G1X1/G1X2Y1'), 0, 1), 0.414214)
            self.assertRoughly(adaptive(KINK('G1X1/G1X0Y1'), 0, 1), 2.414211)
            self.assertRoughly(adaptive(KINK('G1X1/G1X0'), 0, 1), 1)
            self.assertRoughly(adaptive(KINK('G1X1/G1X0Y-1'), 0, 1), 2.414211)
            self.assertRoughly(adaptive(KINK('G1X1/G1X1Y-1'), 0, 1), 1)
            self.assertRoughly(adaptive(KINK('G1X1/G1X2Y-1'), 0, 1), 0.414214)
            self.assertRoughly(adaptive(KINK('G1X1Y1/G1X0Y2'), 0, 1), 0.414214)

        if True:
            # more horizontal and some vertical bones
            self.assertRoughly(adaptive(KINK('G1Y1/G1Y2'), 0, 1), 0)
            self.assertRoughly(adaptive(KINK('G1Y1/G1Y1X1'), PI, 1), 1)
            self.assertRoughly(adaptive(KINK('G1Y1/G1Y2X1'), PI, 1), 0.089820)
            self.assertRoughly(adaptive(KINK('G1Y1/G1Y2X1'), PI/2, 1), 0.414214)
            self.assertRoughly(adaptive(KINK('G1Y1/G1Y0X1'), PI/2, 1), 2.414211)
            self.assertRoughly(adaptive(KINK('G1Y1/G1Y0'), 0, 1), 1)
            self.assertRoughly(adaptive(KINK('G1Y1/G1Y0X-1'), PI/2, 1), 2.414211)
            self.assertRoughly(adaptive(KINK('G1Y1/G1Y1X-1'), 0, 1), 1)
            self.assertRoughly(adaptive(KINK('G1Y1/G1Y2X-1'), 0, 1), 0.089820)
            self.assertRoughly(adaptive(KINK('G1Y1/G1Y2X-1'), PI/2, 1), 0.414214)

        if True:
            # dogbones
            self.assertRoughly(adaptive(KINK('G1X1/G1Y1'), -PI/4, 1), 0.414214)
            self.assertRoughly(adaptive(KINK('G1X1/G1X0Y1'), -PI/8, 1), 1.613126)
            self.assertRoughly(adaptive(KINK('G1X1/G1Y-1'), PI/4, 1), 0.414214)
            self.assertRoughly(adaptive(KINK('G1X1/G1X0Y-1'), PI/8, 1), 1.613126)
            self.assertRoughly(adaptive(KINK('G1Y1/G1X-1'), PI/4, 1), 0.414214)
            self.assertRoughly(adaptive(KINK('G1Y1/G1X1'), 3*PI/4, 1), 0.414214)
            self.assertRoughly(adaptive(KINK('G1Y-1/G1X1'), -3*PI/4, 1), 0.414214)
            self.assertRoughly(adaptive(KINK('G1Y-1/G1X-1'), -PI/4, 1), 0.414214)
            self.assertRoughly(adaptive(KINK('G1X1Y1/G1X0Y2'), 0, 1), 0.414214)
            self.assertRoughly(adaptive(KINK('G1X-1Y1/G1X0Y2'), PI, 1), 0.414214)
            self.assertRoughly(adaptive(KINK('G1X1Y1/G1X2Y0'), PI/2, 2), 0.828428)
            self.assertRoughly(adaptive(KINK('G1X-1Y-1/G1X-2Y0'), -PI/2, 2), 0.828428)
            self.assertRoughly(adaptive(KINK('G1X-1Y1/G1X-2Y0'), PI/2, 2), 0.828428)
            self.assertRoughly(adaptive(KINK('G1X1Y-1/G1X2Y0'), -PI/2, 2), 0.828428)

    def test01(self):
        """Verify nominal length"""

        def nominal(k, a, n):
            return PathDressupDogboneII.calc_length_nominal(k, a, n, 0)

        # neither angle nor kink matter
        self.assertRoughly(nominal(KINK('G1X1/G1X2'), 0, 13), 13)
        self.assertRoughly(nominal(KINK('G1X1/G1X2'), PI/2, 13), 13)
        self.assertRoughly(nominal(KINK('G1X1/G1X2'), PI, 13), 13)
        self.assertRoughly(nominal(KINK('G1X1/G1X2'), -PI/2, 13), 13)
        self.assertRoughly(nominal(KINK('G1X8/G1X12'), 0, 13), 13)
        self.assertRoughly(nominal(KINK('G1X9/G1X0'), 0, 13), 13)
        self.assertRoughly(nominal(KINK('G1X7/G1X9'), 0, 13), 13)
        self.assertRoughly(nominal(KINK('G1X5/G1X1'), 0, 13), 13)

    def test02(self):
        """Verify custom length"""

        def custom(k, a, c):
            return PathDressupDogboneII.calc_length_custom(k, a, 0, c)

        # neither angle nor kink matter
        self.assertRoughly(custom(KINK('G1X1/G1X2'), 0, 7), 7)
        self.assertRoughly(custom(KINK('G1X1/G1X2'), PI/2, 7), 7)
        self.assertRoughly(custom(KINK('G1X1/G1X2'), PI, 7), 7)
        self.assertRoughly(custom(KINK('G1X1/G1X2'), -PI/2, 7), 7)
        self.assertRoughly(custom(KINK('G1X8/G1X12'), 0, 7), 7)
        self.assertRoughly(custom(KINK('G1X9/G1X0'), 0, 7), 7)
        self.assertRoughly(custom(KINK('G1X7/G1X9'), 0, 7), 7)
        self.assertRoughly(custom(KINK('G1X5/G1X1'), 0, 7), 7)
