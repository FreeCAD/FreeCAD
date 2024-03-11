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
import Tests.PathTestUtils as PathTestUtils
import math

PI = math.pi


class MockTB(object):
    def __init__(self, dia):
        self.Name = "ToolBit"
        self.Label = "ToolBit"
        self.Diameter = FreeCAD.Units.Quantity(dia, FreeCAD.Units.Length)


class MockTC(object):
    def __init__(self, dia=2):
        self.Name = "TC"
        self.Label = "TC"
        self.Tool = MockTB(dia)


class MockOp(object):
    def __init__(self, path, dia=2):
        self.Name = "OP"
        self.Label = "OP"
        self.Path = Path.Path(path)
        self.ToolController = MockTC(dia)


class MockFeaturePython(object):
    def __init__(self, name):
        self.prop = {}
        self.addProperty("App::PropertyString", "Name", val=name)
        self.addProperty("App::PropertyString", "Label", val=name)
        self.addProperty("App::PropertyLink", "Proxy")
        self.addProperty("Path::Path", "Path", val=Path.Path())

    def addProperty(self, typ, name, grp=None, desc=None, val=None):
        self.prop[name] = (typ, val)

    def setEditorMode(self, name, mode):
        pass

    def __setattr__(self, name, val):
        if name == "prop":
            return super().__setattr__(name, val)
        self.prop[name] = (self.prop[name][0], val)

    def __getattr__(self, name):
        if name == "prop":
            return super().__getattr__(name)
        typ, val = self.prop.get(name, (None, None))
        if typ is None and val is None:
            raise AttributeError
        if typ == "App::PropertyLength":
            if type(val) == float or type(val) == int:
                return FreeCAD.Units.Quantity(val, FreeCAD.Units.Length)
            return FreeCAD.Units.Quantity(val)
        return val


def CreateDressup(path):
    op = MockOp(path)
    obj = MockFeaturePython("DressupDogbone")
    db = Path.Dressup.DogboneII.Proxy(obj, op)
    obj.Proxy = db
    return obj


def MNVR(gcode, begin=None):
    # 'turns out the replace() isn't really necessary
    # leave it here anyway for clarity
    return PathLanguage.Maneuver.FromGCode(gcode.replace("/", "\n"), begin)


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
            param = [
                f"{k}{v:g}" if Path.Geom.isRoughly(0, v - int(v)) else f"{k}{v:.2f}"
                for k, v in cmd.Parameters.items()
            ]
            return f"{cmd.Name}{''.join(param)}"

        p = "/".join([cmd2str(cmd) for cmd in path.Commands])
        self.assertEqual(p, s)

    def test00(self):
        """Verify adaptive length"""

        def adaptive(k, a, n):
            return Path.Dressup.DogboneII.calc_length_adaptive(k, a, n, n)

        if True:
            # horizontal bones
            self.assertRoughly(adaptive(KINK("G1X1/G1X2"), 0, 1), 0)
            self.assertRoughly(adaptive(KINK("G1X1/G1Y1"), 0, 1), 1)
            self.assertRoughly(adaptive(KINK("G1X1/G1X2Y1"), 0, 1), 0.414214)
            self.assertRoughly(adaptive(KINK("G1X1/G1X0Y1"), 0, 1), 2.414211)
            self.assertRoughly(adaptive(KINK("G1X1/G1X0"), 0, 1), 1)
            self.assertRoughly(adaptive(KINK("G1X1/G1X0Y-1"), 0, 1), 2.414211)
            self.assertRoughly(adaptive(KINK("G1X1/G1X1Y-1"), 0, 1), 1)
            self.assertRoughly(adaptive(KINK("G1X1/G1X2Y-1"), 0, 1), 0.414214)
            self.assertRoughly(adaptive(KINK("G1X1Y1/G1X0Y2"), 0, 1), 0.414214)

        if True:
            # more horizontal and some vertical bones
            self.assertRoughly(adaptive(KINK("G1Y1/G1Y2"), 0, 1), 0)
            self.assertRoughly(adaptive(KINK("G1Y1/G1Y1X1"), PI, 1), 1)
            self.assertRoughly(adaptive(KINK("G1Y1/G1Y2X1"), PI, 1), 0.089820)
            self.assertRoughly(adaptive(KINK("G1Y1/G1Y2X1"), PI / 2, 1), 0.414214)
            self.assertRoughly(adaptive(KINK("G1Y1/G1Y0X1"), PI / 2, 1), 2.414211)
            self.assertRoughly(adaptive(KINK("G1Y1/G1Y0"), 0, 1), 1)
            self.assertRoughly(adaptive(KINK("G1Y1/G1Y0X-1"), PI / 2, 1), 2.414211)
            self.assertRoughly(adaptive(KINK("G1Y1/G1Y1X-1"), 0, 1), 1)
            self.assertRoughly(adaptive(KINK("G1Y1/G1Y2X-1"), 0, 1), 0.089820)
            self.assertRoughly(adaptive(KINK("G1Y1/G1Y2X-1"), PI / 2, 1), 0.414214)

        if True:
            # dogbones
            self.assertRoughly(adaptive(KINK("G1X1/G1Y1"), -PI / 4, 1), 0.414214)
            self.assertRoughly(adaptive(KINK("G1X1/G1X0Y1"), -PI / 8, 1), 1.613126)
            self.assertRoughly(adaptive(KINK("G1X1/G1Y-1"), PI / 4, 1), 0.414214)
            self.assertRoughly(adaptive(KINK("G1X1/G1X0Y-1"), PI / 8, 1), 1.613126)
            self.assertRoughly(adaptive(KINK("G1Y1/G1X-1"), PI / 4, 1), 0.414214)
            self.assertRoughly(adaptive(KINK("G1Y1/G1X1"), 3 * PI / 4, 1), 0.414214)
            self.assertRoughly(adaptive(KINK("G1Y-1/G1X1"), -3 * PI / 4, 1), 0.414214)
            self.assertRoughly(adaptive(KINK("G1Y-1/G1X-1"), -PI / 4, 1), 0.414214)
            self.assertRoughly(adaptive(KINK("G1X1Y1/G1X0Y2"), 0, 1), 0.414214)
            self.assertRoughly(adaptive(KINK("G1X-1Y1/G1X0Y2"), PI, 1), 0.414214)
            self.assertRoughly(adaptive(KINK("G1X1Y1/G1X2Y0"), PI / 2, 2), 0.828428)
            self.assertRoughly(adaptive(KINK("G1X-1Y-1/G1X-2Y0"), -PI / 2, 2), 0.828428)
            self.assertRoughly(adaptive(KINK("G1X-1Y1/G1X-2Y0"), PI / 2, 2), 0.828428)
            self.assertRoughly(adaptive(KINK("G1X1Y-1/G1X2Y0"), -PI / 2, 2), 0.828428)

    def test01(self):
        """Verify nominal length"""

        def nominal(k, a, n):
            return Path.Dressup.DogboneII.calc_length_nominal(k, a, n, 0)

        # neither angle nor kink matter
        self.assertRoughly(nominal(KINK("G1X1/G1X2"), 0, 13), 13)
        self.assertRoughly(nominal(KINK("G1X1/G1X2"), PI / 2, 13), 13)
        self.assertRoughly(nominal(KINK("G1X1/G1X2"), PI, 13), 13)
        self.assertRoughly(nominal(KINK("G1X1/G1X2"), -PI / 2, 13), 13)
        self.assertRoughly(nominal(KINK("G1X8/G1X12"), 0, 13), 13)
        self.assertRoughly(nominal(KINK("G1X9/G1X0"), 0, 13), 13)
        self.assertRoughly(nominal(KINK("G1X7/G1X9"), 0, 13), 13)
        self.assertRoughly(nominal(KINK("G1X5/G1X1"), 0, 13), 13)

    def test02(self):
        """Verify custom length"""

        def custom(k, a, c):
            return Path.Dressup.DogboneII.calc_length_custom(k, a, 0, c)

        # neither angle nor kink matter
        self.assertRoughly(custom(KINK("G1X1/G1X2"), 0, 7), 7)
        self.assertRoughly(custom(KINK("G1X1/G1X2"), PI / 2, 7), 7)
        self.assertRoughly(custom(KINK("G1X1/G1X2"), PI, 7), 7)
        self.assertRoughly(custom(KINK("G1X1/G1X2"), -PI / 2, 7), 7)
        self.assertRoughly(custom(KINK("G1X8/G1X12"), 0, 7), 7)
        self.assertRoughly(custom(KINK("G1X9/G1X0"), 0, 7), 7)
        self.assertRoughly(custom(KINK("G1X7/G1X9"), 0, 7), 7)
        self.assertRoughly(custom(KINK("G1X5/G1X1"), 0, 7), 7)

    def test10(self):
        """Verify basic op dressup"""

        obj = CreateDressup("G1X10/G1Y20")
        obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
        obj.Style = Path.Dressup.DogboneII.Style.Tbone_H

        # bones on right side
        obj.Side = Path.Dressup.DogboneII.Side.Right
        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, "G1X10/G1X11/G1X10/G1Y20")

        # no bones on left side
        obj.Side = Path.Dressup.DogboneII.Side.Left
        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, "G1X10/G1Y20")

    def test11(self):
        """Verify retaining non-move instructions"""

        obj = CreateDressup("G1X10/(some comment)/G1Y20")
        obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
        obj.Style = Path.Dressup.DogboneII.Style.Tbone_H

        # bone on right side
        obj.Side = Path.Dressup.DogboneII.Side.Right
        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, "G1X10/(some comment)/G1X11/G1X10/G1Y20")

        # no bone on left side
        obj.Side = Path.Dressup.DogboneII.Side.Left
        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, "G1X10/(some comment)/G1Y20")

    def test20(self):
        """Verify bone on plunge moves"""

        obj = CreateDressup("G0Z10/G1Z0/G1X10/G1Y10/G1X0/G1Y0/G0Z10")
        obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
        obj.Style = Path.Dressup.DogboneII.Style.Tbone_H
        obj.Side = Path.Dressup.DogboneII.Side.Right

        obj.Proxy.execute(obj)
        self.assertEqualPath(
            obj.Path,
            "G0Z10/G1Z0/G1X10/G1X11/G1X10/G1Y10/G1X11/G1X10/G1X0/G1X-1/G1X0/G1Y0/G1X-1/G1X0/G0Z10",
        )

    def test21(self):
        """Verify ignoring plunge moves that don't connect"""

        obj = CreateDressup("G0Z10/G1Z0/G1X10/G1Y10/G1X0/G1Y5/G0Z10")
        obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
        obj.Style = Path.Dressup.DogboneII.Style.Tbone_H
        obj.Side = Path.Dressup.DogboneII.Side.Right

        obj.Proxy.execute(obj)
        self.assertEqualPath(
            obj.Path,
            "G0Z10/G1Z0/G1X10/G1X11/G1X10/G1Y10/G1X11/G1X10/G1X0/G1X-1/G1X0/G1Y5/G0Z10",
        )

    def test30(self):
        """Verify TBone_V style"""

        def check_tbone(d, i, path, out, right):
            obj = CreateDressup(f"({d}.{i:02})/{path}")
            obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
            if right:
                obj.Side = Path.Dressup.DogboneII.Side.Right
            else:
                obj.Side = Path.Dressup.DogboneII.Side.Left
            obj.Style = Path.Dressup.DogboneII.Style.Tbone_V
            obj.Proxy.execute(obj)
            self.assertEqualPath(obj.Path, f"({d}.{i:02})/{out}")

        # test data with a horizontal lead in
        test_data_h = [
            # top right quadrant
            ("G1X10Y0/G1X10Y10", "G1X10Y0/G1Y-1/G1Y0/G1X10Y10", True),
            ("G1X10Y0/G1X20Y10", "G1X10Y0/G1Y-1/G1Y0/G1X20Y10", True),
            ("G1X10Y0/G1X90Y10", "G1X10Y0/G1Y-1/G1Y0/G1X90Y10", True),
            ("G1X10Y0/G1X0Y10", "G1X10Y0/G1Y-1/G1Y0/G1X0Y10", True),
            # bottom right quadrant
            ("G1X10Y0/G1X90Y-10", "G1X10Y0/G1Y1/G1Y0/G1X90Y-10", False),
            ("G1X10Y0/G1X20Y-10", "G1X10Y0/G1Y1/G1Y0/G1X20Y-10", False),
            ("G1X10Y0/G1X10Y-10", "G1X10Y0/G1Y1/G1Y0/G1X10Y-10", False),
            ("G1X10Y0/G1X0Y-10", "G1X10Y0/G1Y1/G1Y0/G1X0Y-10", False),
            # top left quadrant
            ("G1X-10Y0/G1X-10Y10", "G1X-10Y0/G1Y-1/G1Y0/G1X-10Y10", False),
            ("G1X-10Y0/G1X-20Y10", "G1X-10Y0/G1Y-1/G1Y0/G1X-20Y10", False),
            ("G1X-10Y0/G1X-90Y10", "G1X-10Y0/G1Y-1/G1Y0/G1X-90Y10", False),
            ("G1X-10Y0/G1X-0Y10", "G1X-10Y0/G1Y-1/G1Y0/G1X-0Y10", False),
            # bottom left quadrant
            ("G1X-10Y0/G1X-90Y-10", "G1X-10Y0/G1Y1/G1Y0/G1X-90Y-10", True),
            ("G1X-10Y0/G1X-20Y-10", "G1X-10Y0/G1Y1/G1Y0/G1X-20Y-10", True),
            ("G1X-10Y0/G1X-10Y-10", "G1X-10Y0/G1Y1/G1Y0/G1X-10Y-10", True),
            ("G1X-10Y0/G1X-0Y-10", "G1X-10Y0/G1Y1/G1Y0/G1X-0Y-10", True),
        ]

        for i, (path, out, right) in enumerate(test_data_h):
            check_tbone("h", i, path, out, right)

        # test data with a vertical lead in
        test_data_v = [
            # top right quadrant
            ("G1X0Y10/G1X10Y10", "G1X0Y10/G1Y11/G1Y10/G1X10Y10", False),
            ("G1X0Y10/G1X10Y20", "G1X0Y10/G1Y11/G1Y10/G1X10Y20", False),
            ("G1X0Y10/G1X10Y90", "G1X0Y10/G1Y11/G1Y10/G1X10Y90", False),
            ("G1X0Y10/G1X10Y0", "G1X0Y10/G1Y11/G1Y10/G1X10Y0", False),
            # bottom right quadrant
            ("G1X0Y-10/G1X10Y-90", "G1X0Y-10/G1Y-11/G1Y-10/G1X10Y-90", True),
            ("G1X0Y-10/G1X10Y-20", "G1X0Y-10/G1Y-11/G1Y-10/G1X10Y-20", True),
            ("G1X0Y-10/G1X10Y-10", "G1X0Y-10/G1Y-11/G1Y-10/G1X10Y-10", True),
            ("G1X0Y-10/G1X10Y-0", "G1X0Y-10/G1Y-11/G1Y-10/G1X10Y-0", True),
            # top left quadrant
            ("G1X0Y10/G1X-10Y10", "G1X0Y10/G1Y11/G1Y10/G1X-10Y10", True),
            ("G1X0Y10/G1X-10Y20", "G1X0Y10/G1Y11/G1Y10/G1X-10Y20", True),
            ("G1X0Y10/G1X-10Y90", "G1X0Y10/G1Y11/G1Y10/G1X-10Y90", True),
            ("G1X0Y10/G1X-10Y0", "G1X0Y10/G1Y11/G1Y10/G1X-10Y0", True),
            # bottom left quadrant
            ("G1X0Y-10/G1X-10Y-90", "G1X0Y-10/G1Y-11/G1Y-10/G1X-10Y-90", False),
            ("G1X0Y-10/G1X-10Y-20", "G1X0Y-10/G1Y-11/G1Y-10/G1X-10Y-20", False),
            ("G1X0Y-10/G1X-10Y-10", "G1X0Y-10/G1Y-11/G1Y-10/G1X-10Y-10", False),
            ("G1X0Y-10/G1X-10Y-0", "G1X0Y-10/G1Y-11/G1Y-10/G1X-10Y-0", False),
        ]

        for i, (path, out, right) in enumerate(test_data_v):
            check_tbone("v", i, path, out, right)

    def test40(self):
        """Verify TBone_S style"""

        def check_tbone_s(d, i, path, out, right):
            obj = CreateDressup(f"(m{d}.{i:02})/{path}")
            obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
            if right:
                obj.Side = Path.Dressup.DogboneII.Side.Right
            else:
                obj.Side = Path.Dressup.DogboneII.Side.Left
            obj.Style = Path.Dressup.DogboneII.Style.Tbone_S
            obj.Proxy.execute(obj)
            self.assertEqualPath(obj.Path, f"(m{d}.{i:02})/{out}")

        # short edge m0
        test_data_0 = [
            # CCW
            ("G1X10/G1Y20", "G1X10/G1Y-1/G1Y0/G1Y20", True),
            ("G1X10Y10/G1X-10Y30", "G1X10Y10/G1X10.71Y9.29/G1X10Y10/G1X-10Y30", True),
            ("G1Y10/G1X-20", "G1Y10/G1X1/G1X0/G1X-20", True),
            (
                "G1X-10Y10/G1X-30Y-10",
                "G1X-10Y10/G1X-9.29Y10.71/G1X-10Y10/G1X-30Y-10",
                True,
            ),
            ("G1X-10/G1Y-20", "G1X-10/G1Y1/G1Y0/G1Y-20", True),
            (
                "G1X-10Y-10/G1X10Y-30",
                "G1X-10Y-10/G1X-10.71Y-9.29/G1X-10Y-10/G1X10Y-30",
                True,
            ),
            ("G1Y-10/G1X20", "G1Y-10/G1X-1/G1X0/G1X20", True),
            ("G1X10Y-10/G1X30Y10", "G1X10Y-10/G1X9.29Y-10.71/G1X10Y-10/G1X30Y10", True),
            # CW
            ("G1X10/G1Y-20", "G1X10/G1Y1/G1Y0/G1Y-20", False),
            ("G1X10Y10/G1X30Y-10", "G1X10Y10/G1X9.29Y10.71/G1X10Y10/G1X30Y-10", False),
            ("G1Y10/G1X20", "G1Y10/G1X-1/G1X0/G1X20", False),
            (
                "G1X-10Y10/G1X10Y30",
                "G1X-10Y10/G1X-10.71Y9.29/G1X-10Y10/G1X10Y30",
                False,
            ),
            ("G1X-10/G1Y20", "G1X-10/G1Y-1/G1Y0/G1Y20", False),
            (
                "G1X-10Y-10/G1X-30Y10",
                "G1X-10Y-10/G1X-9.29Y-10.71/G1X-10Y-10/G1X-30Y10",
                False,
            ),
            ("G1Y-10/G1X-20", "G1Y-10/G1X1/G1X0/G1X-20", False),
            (
                "G1X10Y-10/G1X-10Y-30",
                "G1X10Y-10/G1X10.71Y-9.29/G1X10Y-10/G1X-10Y-30",
                False,
            ),
        ]

        for i, (path, out, right) in enumerate(test_data_0):
            check_tbone_s("0", i, path, out, right)

        # short edge m1
        test_data_1 = [
            # CCW
            ("G1X20/G1Y10", "G1X20/G1X21/G1X20/G1Y10", True),
            ("G1X20Y20/G1X10Y30", "G1X20Y20/G1X20.71Y20.71/G1X20Y20/G1X10Y30", True),
            ("G1Y20/G1X-10", "G1Y20/G1Y21/G1Y20/G1X-10", True),
            (
                "G1X-20Y20/G1X-30Y10",
                "G1X-20Y20/G1X-20.71Y20.71/G1X-20Y20/G1X-30Y10",
                True,
            ),
            ("G1X-20/G1Y-10", "G1X-20/G1X-21/G1X-20/G1Y-10", True),
            (
                "G1X-20Y-20/G1X-10Y-30",
                "G1X-20Y-20/G1X-20.71Y-20.71/G1X-20Y-20/G1X-10Y-30",
                True,
            ),
            ("G1Y-20/G1X10", "G1Y-20/G1Y-21/G1Y-20/G1X10", True),
            (
                "G1X20Y-20/G1X30Y-10",
                "G1X20Y-20/G1X20.71Y-20.71/G1X20Y-20/G1X30Y-10",
                True,
            ),
            # CW
            ("G1X20/G1Y-10", "G1X20/G1X21/G1X20/G1Y-10", False),
            ("G1X20Y20/G1X30Y10", "G1X20Y20/G1X20.71Y20.71/G1X20Y20/G1X30Y10", False),
            ("G1Y20/G1X10", "G1Y20/G1Y21/G1Y20/G1X10", False),
            (
                "G1X-20Y20/G1X-10Y30",
                "G1X-20Y20/G1X-20.71Y20.71/G1X-20Y20/G1X-10Y30",
                False,
            ),
            ("G1X-20/G1Y10", "G1X-20/G1X-21/G1X-20/G1Y10", False),
            (
                "G1X-20Y-20/G1X-30Y-10",
                "G1X-20Y-20/G1X-20.71Y-20.71/G1X-20Y-20/G1X-30Y-10",
                False,
            ),
            ("G1Y-20/G1X-10", "G1Y-20/G1Y-21/G1Y-20/G1X-10", False),
            (
                "G1X20Y-20/G1X10Y-30",
                "G1X20Y-20/G1X20.71Y-20.71/G1X20Y-20/G1X10Y-30",
                False,
            ),
        ]

        for i, (path, out, right) in enumerate(test_data_1):
            check_tbone_s("1", i, path, out, right)

    def test50(self):
        """Verify TBone_L style"""

        def check_tbone_l(d, i, path, out, right):
            obj = CreateDressup(f"(m{d}.{i:02})/{path}")
            obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
            if right:
                obj.Side = Path.Dressup.DogboneII.Side.Right
            else:
                obj.Side = Path.Dressup.DogboneII.Side.Left
            obj.Style = Path.Dressup.DogboneII.Style.Tbone_L
            obj.Proxy.execute(obj)
            self.assertEqualPath(obj.Path, f"(m{d}.{i:02})/{out}")

        # long edge m1
        test_data_1 = [
            # CCW
            ("G1X10/G1Y20", "G1X10/G1X11/G1X10/G1Y20", True),
            ("G1X10Y10/G1X-10Y30", "G1X10Y10/G1X10.71Y10.71/G1X10Y10/G1X-10Y30", True),
            ("G1Y10/G1X-20", "G1Y10/G1Y11/G1Y10/G1X-20", True),
            (
                "G1X-10Y10/G1X-30Y-10",
                "G1X-10Y10/G1X-10.71Y10.71/G1X-10Y10/G1X-30Y-10",
                True,
            ),
            ("G1X-10/G1Y-20", "G1X-10/G1X-11/G1X-10/G1Y-20", True),
            (
                "G1X-10Y-10/G1X10Y-30",
                "G1X-10Y-10/G1X-10.71Y-10.71/G1X-10Y-10/G1X10Y-30",
                True,
            ),
            ("G1Y-10/G1X20", "G1Y-10/G1Y-11/G1Y-10/G1X20", True),
            (
                "G1X10Y-10/G1X30Y10",
                "G1X10Y-10/G1X10.71Y-10.71/G1X10Y-10/G1X30Y10",
                True,
            ),
            # CW
            ("G1X10/G1Y-20", "G1X10/G1X11/G1X10/G1Y-20", False),
            ("G1X10Y10/G1X30Y-10", "G1X10Y10/G1X10.71Y10.71/G1X10Y10/G1X30Y-10", False),
            ("G1Y10/G1X20", "G1Y10/G1Y11/G1Y10/G1X20", False),
            (
                "G1X-10Y10/G1X10Y30",
                "G1X-10Y10/G1X-10.71Y10.71/G1X-10Y10/G1X10Y30",
                False,
            ),
            ("G1X-10/G1Y20", "G1X-10/G1X-11/G1X-10/G1Y20", False),
            (
                "G1X-10Y-10/G1X-30Y10",
                "G1X-10Y-10/G1X-10.71Y-10.71/G1X-10Y-10/G1X-30Y10",
                False,
            ),
            ("G1Y-10/G1X-20", "G1Y-10/G1Y-11/G1Y-10/G1X-20", False),
            (
                "G1X10Y-10/G1X-10Y-30",
                "G1X10Y-10/G1X10.71Y-10.71/G1X10Y-10/G1X-10Y-30",
                False,
            ),
        ]

        for i, (path, out, right) in enumerate(test_data_1):
            check_tbone_l("1", i, path, out, right)

        # long edge m0
        test_data_0 = [
            # CCW
            ("G1X20/G1Y10", "G1X20/G1Y-1/G1Y0/G1Y10", True),
            ("G1X20Y20/G1X10Y30", "G1X20Y20/G1X20.71Y19.29/G1X20Y20/G1X10Y30", True),
            ("G1Y20/G1X-10", "G1Y20/G1X1/G1X0/G1X-10", True),
            (
                "G1X-20Y20/G1X-30Y10",
                "G1X-20Y20/G1X-19.29Y20.71/G1X-20Y20/G1X-30Y10",
                True,
            ),
            ("G1X-20/G1Y-10", "G1X-20/G1Y1/G1Y0/G1Y-10", True),
            (
                "G1X-20Y-20/G1X-10Y-30",
                "G1X-20Y-20/G1X-20.71Y-19.29/G1X-20Y-20/G1X-10Y-30",
                True,
            ),
            ("G1Y-20/G1X10", "G1Y-20/G1X-1/G1X0/G1X10", True),
            (
                "G1X20Y-20/G1X30Y-10",
                "G1X20Y-20/G1X19.29Y-20.71/G1X20Y-20/G1X30Y-10",
                True,
            ),
            # CW
            ("G1X20/G1Y-10", "G1X20/G1Y1/G1Y0/G1Y-10", False),
            ("G1X20Y20/G1X30Y10", "G1X20Y20/G1X19.29Y20.71/G1X20Y20/G1X30Y10", False),
            ("G1Y20/G1X10", "G1Y20/G1X-1/G1X0/G1X10", False),
            (
                "G1X-20Y20/G1X-10Y30",
                "G1X-20Y20/G1X-20.71Y19.29/G1X-20Y20/G1X-10Y30",
                False,
            ),
            ("G1X-20/G1Y10", "G1X-20/G1Y-1/G1Y0/G1Y10", False),
            (
                "G1X-20Y-20/G1X-30Y-10",
                "G1X-20Y-20/G1X-19.29Y-20.71/G1X-20Y-20/G1X-30Y-10",
                False,
            ),
            ("G1Y-20/G1X-10", "G1Y-20/G1X1/G1X0/G1X-10", False),
            (
                "G1X20Y-20/G1X10Y-30",
                "G1X20Y-20/G1X20.71Y-19.29/G1X20Y-20/G1X10Y-30",
                False,
            ),
        ]

        for i, (path, out, right) in enumerate(test_data_0):
            check_tbone_l("0", i, path, out, right)

    def test60(self):
        """Verify Dogbone style"""

        obj = CreateDressup("G1X10/G1Y20")
        obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
        obj.Side = Path.Dressup.DogboneII.Side.Right

        obj.Style = Path.Dressup.DogboneII.Style.Dogbone
        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, "G1X10/G1X10.71Y-0.71/G1X10Y0/G1Y20")

    def test70(self):
        """Verify custom length."""

        obj = CreateDressup("G0Z10/G1Z0/G1X10/G1Y10/G1X0/G1Y0/G0Z10")
        obj.Style = Path.Dressup.DogboneII.Style.Tbone_H
        obj.Side = Path.Dressup.DogboneII.Side.Right

        obj.Incision = Path.Dressup.DogboneII.Incision.Custom
        obj.Custom = 3
        obj.Proxy.execute(obj)
        self.assertEqualPath(
            obj.Path,
            "G0Z10/G1Z0/G1X10/G1X13/G1X10/G1Y10/G1X13/G1X10/G1X0/G1X-3/G1X0/G1Y0/G1X-3/G1X0/G0Z10",
        )

        obj.Custom = 2
        obj.Proxy.execute(obj)
        self.assertEqualPath(
            obj.Path,
            "G0Z10/G1Z0/G1X10/G1X12/G1X10/G1Y10/G1X12/G1X10/G1X0/G1X-2/G1X0/G1Y0/G1X-2/G1X0/G0Z10",
        )

    def test80(self):
        """Verify adaptive length."""

        obj = CreateDressup("G1X10/G1Y20")
        obj.Incision = Path.Dressup.DogboneII.Incision.Adaptive
        obj.Side = Path.Dressup.DogboneII.Side.Right

        obj.Style = Path.Dressup.DogboneII.Style.Dogbone
        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, "G1X10/G1X10.29Y-0.29/G1X10Y0/G1Y20")

    def test81(self):
        """Verify adaptive length II."""

        obj = CreateDressup("G1X10/G1X20Y20")
        obj.Incision = Path.Dressup.DogboneII.Incision.Adaptive
        obj.Side = Path.Dressup.DogboneII.Side.Right

        obj.Style = Path.Dressup.DogboneII.Style.Dogbone
        obj.Proxy.execute(obj)
        self.assertEqualPath(obj.Path, "G1X10/G1X10.09Y-0.15/G1X10Y0/G1X20Y20")

    def test90(self):
        """Verify dogbone blacklist"""

        obj = CreateDressup("G0Z10/G1Z0/G1X10/G1Y10/G1X0/G1Y0/G0Z10")
        obj.Incision = Path.Dressup.DogboneII.Incision.Fixed
        obj.Style = Path.Dressup.DogboneII.Style.Tbone_H
        obj.Side = Path.Dressup.DogboneII.Side.Right
        obj.BoneBlacklist = [0, 2]
        obj.Proxy.execute(obj)
        self.assertEqualPath(
            obj.Path, "G0Z10/G1Z0/G1X10/G1Y10/G1X11/G1X10/G1X0/G1Y0/G1X-1/G1X0/G0Z10"
        )
        return obj

    def test91(self):
        """Verify dogbone on dogbone"""

        obj = self.test90()

        obj2 = MockFeaturePython("DressupDogbone001")
        db2 = Path.Dressup.DogboneII.Proxy(obj2, obj)
        obj2.Proxy = db2
        obj2.Incision = Path.Dressup.DogboneII.Incision.Fixed
        obj2.Style = Path.Dressup.DogboneII.Style.Tbone_H
        obj2.Side = Path.Dressup.DogboneII.Side.Right
        obj2.BoneBlacklist = [1]
        obj2.Proxy.execute(obj2)
        self.assertEqualPath(
            obj2.Path,
            "G0Z10/G1Z0/G1X10/G1X11/G1X10/G1Y10/G1X11/G1X10/G1X0/G1X-1/G1X0/G1Y0/G1X-1/G1X0/G0Z10",
        )
