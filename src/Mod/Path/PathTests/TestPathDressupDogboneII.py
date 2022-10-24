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
import Path.Dressup.DogboneII
import PathTests.PathTestUtils as PathTestUtils
import math

PI = math.pi

class MockTB(object):

    def __init__(self, dia):
        self.Name = 'ToolBit'
        self.Label = 'ToolBit'
        self.Diameter = FreeCAD.Units.Quantity(dia, FreeCAD.Units.Length)

class MockTC(object):

    def __init__(self, dia=2):
        self.Name = 'TC'
        self.Label = 'TC'
        self.Tool = MockTB(dia)

class MockOp(object):

    def __init__(self, path, dia=2):
        self.Path = Path.Path(path)
        self.ToolController = MockTC(dia)

class MockFeaturePython(object):

    def __init__(self, name):
        self.prop = {}
        self.addProperty('App::PropertyString', 'Name', val=name)
        self.addProperty('App::PropertyString', 'Label', val=name)
        self.addProperty('App::PropertyLink', 'Proxy')
        self.addProperty('Path::Path', 'Path', val=Path.Path())

    def addProperty(self, typ, name, grp=None, desc=None, val=None):
        self.prop[name] = (typ, val)

    def setEditorMode(self, name, mode):
        pass

    def __setattr__(self, name, val):
        if name == 'prop':
            return super().__setattr__(name, val)
        self.prop[name] = (self.prop[name][0], val)

    def __getattr__(self, name):
        if name == 'prop':
            return super().__getattr__(name)
        typ, val = self.prop[name]
        if typ == 'App::PropertyLength':
            if type(val) == float or type(val) == int:
                return FreeCAD.Units.Quantity(val, FreeCAD.Units.Length)
            return FreeCAD.Units.Quantity(val)
        return val

def CreateDressup(path):
    op = MockOp(path)
    obj = MockFeaturePython("DogboneII")
    db = Path.Dressup.DogboneII.Proxy(obj, op)
    obj.Proxy = db
    return obj

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

    def assertEqualPath(self, path, s):
        def cmd2str(cmd):
            param = [f"{k}{v:g}" for k, v in cmd.Parameters.items()]
            return f"{cmd.Name}{''.join(param)}"

        p = '/'.join([cmd2str(cmd) for cmd in path.Commands])
        self.assertEqual(p, s)

    def test00(self):
        """Verify adaptive length"""

        def adaptive(k, a, n): 
            return Path.Dressup.DogboneII.calc_length_adaptive(k, a, n, n)

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
            return Path.Dressup.DogboneII.calc_length_nominal(k, a, n, 0)

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
            return Path.Dressup.DogboneII.calc_length_custom(k, a, 0, c)

        # neither angle nor kink matter
        self.assertRoughly(custom(KINK('G1X1/G1X2'), 0, 7), 7)
        self.assertRoughly(custom(KINK('G1X1/G1X2'), PI/2, 7), 7)
        self.assertRoughly(custom(KINK('G1X1/G1X2'), PI, 7), 7)
        self.assertRoughly(custom(KINK('G1X1/G1X2'), -PI/2, 7), 7)
        self.assertRoughly(custom(KINK('G1X8/G1X12'), 0, 7), 7)
        self.assertRoughly(custom(KINK('G1X9/G1X0'), 0, 7), 7)
        self.assertRoughly(custom(KINK('G1X7/G1X9'), 0, 7), 7)
        self.assertRoughly(custom(KINK('G1X5/G1X1'), 0, 7), 7)


    def test10(self):
        """Verify basic op dressup"""

        obj = CreateDressup("G1X10/G1Y20")
        obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
        obj.Style = Path.Dressup.DogboneII.Style.Tbone_H

        # bones on right side
        obj.Side = Path.Dressup.DogboneII.Side.Right
        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, 'G1X10/G1X11/G1X10/G1Y20')

        # no bones on left side
        obj.Side = Path.Dressup.DogboneII.Side.Left
        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, 'G1X10/G1Y20')

    def test11(self):
        """Verify retaining non-move instructions"""

        obj = CreateDressup("G1X10/(some comment)/G1Y20")
        obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
        obj.Style = Path.Dressup.DogboneII.Style.Tbone_H

        # bone on right side
        obj.Side = Path.Dressup.DogboneII.Side.Right
        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, 'G1X10/(some comment)/G1X11/G1X10/G1Y20')

        # no bone on left side
        obj.Side = Path.Dressup.DogboneII.Side.Left
        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, 'G1X10/(some comment)/G1Y20')

    def test20(self):
        """Verify bone on plunge moves"""

        obj = CreateDressup("G0Z10/G1Z0/G1X10/G1Y10/G1X0/G1Y0/G0Z10")
        obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
        obj.Style = Path.Dressup.DogboneII.Style.Tbone_H
        obj.Side = Path.Dressup.DogboneII.Side.Right

        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, 'G0Z10/G1Z0/G1X10/G1X11/G1X10/G1Y10/G1X11/G1X10/G1X0/G1X-1/G1X0/G1Y0/G1X-1/G1X0/G0Z10')

    def test21(self):
        """Verify ignoring plunge moves that don't connect"""

        obj = CreateDressup("G0Z10/G1Z0/G1X10/G1Y10/G1X0/G1Y5/G0Z10")
        obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
        obj.Style = Path.Dressup.DogboneII.Style.Tbone_H
        obj.Side = Path.Dressup.DogboneII.Side.Right

        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, 'G0Z10/G1Z0/G1X10/G1X11/G1X10/G1Y10/G1X11/G1X10/G1X0/G1X-1/G1X0/G1Y5/G0Z10')

