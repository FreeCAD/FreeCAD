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
Path.Log.setLevel(Path.Log.Level.NOTICE)

PI = math.pi
DebugMode = Path.Log.getLevel(Path.Log.thisModule()) == Path.Log.Level.DEBUG


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
    """Unit tests for the Dogbone dressup."""

    def assertKinks(self, maneuver, s):
        kinks = [f"{k.deflection():4.2f}" for k in PathDressupDogboneII.createKinks(maneuver)]
        self.assertEqual(f"[{', '.join(kinks)}]", s)

    def assertBones(self, maneuver, threshold, s):
        bones = [f"({int(b.x())},{int(b.y())})" for b in PathDressupDogboneII.findDogboneKinks(maneuver, threshold)]
        self.assertEqual(f"[{', '.join(bones)}]", s)

    def assertBone(self, bone, s, digits=0):
        if DebugMode and FreeCAD.GuiUp:
            Path.show(PathDressupDogboneII.kink_to_path(bone.kink))
            FreeCAD.ActiveDocument.Objects[-1].Visibility = False
            Path.show(PathDressupDogboneII.bone_to_path(bone))
            FreeCAD.ActiveDocument.Objects[-1].Visibility = False
        Path.Log.debug(f"{bone.kink} : {bone.angle / PI:.2f}")

        b = [i.str(digits) for i in bone.instr]
        self.assertEqual(f"[{', '.join(b)}]", s)

    def test20(self):
        """Verify kinks of maneuvers"""
        self.assertKinks(MNVR('G1X1/G1Y1'), '[1.57]')
        self.assertKinks(MNVR('G1X1/G1Y-1'), '[-1.57]')
        self.assertKinks(MNVR('G1X1/G1Y1/G1X0'), '[1.57, 1.57]')
        self.assertKinks(MNVR('G1X1/G1Y1/G1X0/G1Y0'), '[1.57, 1.57, 1.57, 1.57]')

        self.assertKinks(MNVR('G1Y1/G1X1'), '[-1.57]')
        self.assertKinks(MNVR('G1Y1/G1X1/G1Y0'), '[-1.57, -1.57]')
        self.assertKinks(MNVR('G1Y1/G1X1/G1Y0/G1X0'), '[-1.57, -1.57, -1.57, -1.57]')

        # tangential arc moves
        self.assertKinks(MNVR('G1X1/G3Y2J1'), '[0.00]')
        self.assertKinks(MNVR('G1X1/G3Y2J1G1X0'), '[0.00, 0.00]')

        # folding back arc moves
        self.assertKinks(MNVR('G1X1/G2Y2J1'), '[-3.14]')
        self.assertKinks(MNVR('G1X1/G2Y2J1G1X0'), '[-3.14, 3.14]')

    def test30(self):
        """Verify dogbone detection"""
        self.assertBones(MNVR('G1X1/G1Y1/G1X0/G1Y0'),  PI/4, '[(1,0), (1,1), (0,1), (0,0)]')
        self.assertBones(MNVR('G1X1/G1Y1/G1X0/G1Y0'), -PI/4, '[]')

        # no bones on flat angle
        self.assertBones(MNVR('G1X1/G1X3Y1/G1X0/G1Y0'),  PI/4, '[(3,1), (0,1), (0,0)]')
        self.assertBones(MNVR('G1X1/G1X3Y1/G1X0/G1Y0'), -PI/4, '[]')

        # no bones on tangential arc
        self.assertBones(MNVR('G1X1/G3Y2J1/G1X0/G1Y0'),  PI/4, '[(0,2), (0,0)]')
        self.assertBones(MNVR('G1X1/G3Y2J1/G1X0/G1Y0'), -PI/4, '[]')

        # a bone on perpendicular arc
        self.assertBones(MNVR('G1X1/G3X3I1/G1Y1/G1X0/G1Y0'),  PI/4, '[(3,1), (0,1), (0,0)]')
        self.assertBones(MNVR('G1X1/G3X3I1/G1Y1/G1X0/G1Y0'), -PI/4, '[(1,0)]')

    def test40(self):
        """Verify horizontal t-bone creation"""
        # Uses test data from test30, if that broke, this can't succeed

        # single move right
        maneuver = MNVR('G1X1/G1Y1')
        kinks = PathDressupDogboneII.findDogboneKinks(maneuver, PI/4)
        self.assertEqual(len(kinks), 1)
        k = kinks[0]
        p = k.position()
        self.assertEqual(f"({int(p.x)}, {int(p.y)})", "(1, 0)")
        bone = dogboneII.generate_tbone_horizontal(k, 1)
        self.assertBone(bone, "[G1{X: 2}, G1{X: 1}]")

        # full loop CCW
        kinks = PathDressupDogboneII.findDogboneKinks(MNVR('G1X1/G1Y1/G1X0/G1Y0'), PI/4)
        bones = [dogboneII.generate_tbone_horizontal(k, 1) for k in kinks]
        self.assertEqual(len(bones), 4)
        self.assertBone(bones[0], "[G1{X: 2}, G1{X: 1}]")
        self.assertBone(bones[1], "[G1{X: 2}, G1{X: 1}]")
        self.assertBone(bones[2], "[G1{X: -1}, G1{X: 0}]")
        self.assertBone(bones[3], "[G1{X: -1}, G1{X: 0}]")

        # single move left
        maneuver = MNVR('G1X1/G1Y-1')
        kinks = PathDressupDogboneII.findDogboneKinks(maneuver, -PI/4)
        self.assertEqual(len(kinks), 1)
        k = kinks[0]
        p = k.position()
        self.assertEqual(f"({int(p.x)}, {int(p.y)})", "(1, 0)")
        bone = dogboneII.generate_tbone_horizontal(k, 1)
        self.assertBone(bone, "[G1{X: 2}, G1{X: 1}]")

        # full loop CW
        kinks = PathDressupDogboneII.findDogboneKinks(MNVR('G1X1/G1Y-1/G1X0/G1Y0'), -PI/4)
        bones = [dogboneII.generate_tbone_horizontal(k, 1) for k in kinks]
        self.assertEqual(len(bones), 4)
        self.assertBone(bones[0], "[G1{X: 2}, G1{X: 1}]")
        self.assertBone(bones[1], "[G1{X: 2}, G1{X: 1}]")
        self.assertBone(bones[2], "[G1{X: -1}, G1{X: 0}]")
        self.assertBone(bones[3], "[G1{X: -1}, G1{X: 0}]")

        # bones on arcs
        kinks = PathDressupDogboneII.findDogboneKinks(MNVR('G1X1/G3X3I1/G1Y1/G1X0/G1Y0'),  PI/4);
        bones = [dogboneII.generate_tbone_horizontal(k, 1) for k in kinks]
        self.assertEqual(len(bones), 3)
        self.assertBone(bones[0], "[G1{X: 4}, G1{X: 3}]")
        self.assertBone(bones[1], "[G1{X: -1}, G1{X: 0}]")
        self.assertBone(bones[2], "[G1{X: -1}, G1{X: 0}]")

        # bones on arcs
        kinks = PathDressupDogboneII.findDogboneKinks(MNVR('G1X1/G3X3I1/G1Y1/G1X0/G1Y0'),  -PI/4);
        bones = [dogboneII.generate_tbone_horizontal(k, 1) for k in kinks]
        self.assertEqual(len(bones), 1)
        self.assertBone(bones[0], "[G1{X: 2}, G1{X: 1}]")

    def test50(self):
        """Verify vertical t-bone creation"""
        # Uses test data from test30, if that broke, this can't succeed

        # single move right
        maneuver = MNVR('G1X1/G1Y1')
        kinks = PathDressupDogboneII.findDogboneKinks(maneuver, PI/4)
        self.assertEqual(len(kinks), 1)
        k = kinks[0]
        p = k.position()
        self.assertEqual(f"({int(p.x)}, {int(p.y)})", "(1, 0)")
        bone = dogboneII.generate_tbone_vertical(k, 1)
        self.assertBone(bone, "[G1{Y: -1}, G1{Y: 0}]")

        # full loop CCW
        kinks = PathDressupDogboneII.findDogboneKinks(MNVR('G1X1/G1Y1/G1X0/G1Y0'), PI/4)
        bones = [dogboneII.generate_tbone_vertical(k, 1) for k in kinks]
        self.assertEqual(len(bones), 4)
        self.assertBone(bones[0], "[G1{Y: -1}, G1{Y: 0}]")
        self.assertBone(bones[1], "[G1{Y: 2}, G1{Y: 1}]")
        self.assertBone(bones[2], "[G1{Y: 2}, G1{Y: 1}]")
        self.assertBone(bones[3], "[G1{Y: -1}, G1{Y: 0}]")

        # single move left
        maneuver = MNVR('G1X1/G1Y-1')
        kinks = PathDressupDogboneII.findDogboneKinks(maneuver, -PI/4)
        self.assertEqual(len(kinks), 1)
        k = kinks[0]
        p = k.position()
        self.assertEqual(f"({int(p.x)}, {int(p.y)})", "(1, 0)")
        bone = dogboneII.generate_tbone_vertical(k, 1)
        self.assertBone(bone, "[G1{Y: 1}, G1{Y: 0}]")

        # full loop CW
        kinks = PathDressupDogboneII.findDogboneKinks(MNVR('G1X1/G1Y-1/G1X0/G1Y0'), -PI/4)
        bones = [dogboneII.generate_tbone_vertical(k, 1) for k in kinks]
        self.assertEqual(len(bones), 4)
        self.assertBone(bones[0], "[G1{Y: 1}, G1{Y: 0}]")
        self.assertBone(bones[1], "[G1{Y: -2}, G1{Y: -1}]")
        self.assertBone(bones[2], "[G1{Y: -2}, G1{Y: -1}]")
        self.assertBone(bones[3], "[G1{Y: 1}, G1{Y: 0}]")

        # bones on arcs
        kinks = PathDressupDogboneII.findDogboneKinks(MNVR('G1X1/G3X3I1/G1Y1/G1X0/G1Y0'),  PI/4);
        bones = [dogboneII.generate_tbone_vertical(k, 1) for k in kinks]
        self.assertEqual(len(bones), 3)
        self.assertBone(bones[0], "[G1{Y: 2}, G1{Y: 1}]")
        self.assertBone(bones[1], "[G1{Y: 2}, G1{Y: 1}]")
        self.assertBone(bones[2], "[G1{Y: -1}, G1{Y: 0}]")

        # bones on arcs
        kinks = PathDressupDogboneII.findDogboneKinks(MNVR('G1X1/G3X3I1/G1Y1/G1X0/G1Y0'),  -PI/4);
        bones = [dogboneII.generate_tbone_vertical(k, 1) for k in kinks]
        self.assertEqual(len(bones), 1)
        self.assertBone(bones[0], "[G1{Y: 1}, G1{Y: 0}]")

    def test60(self):
        """Verify t-bones on edges"""

        # horizontal short edge
        bone = dogboneII.generate_tbone_on_short(KINK('G1X1/G1Y2'), 1)
        self.assertBone(bone, "[G1{Y: -1}, G1{Y: 0}]")

        bone = dogboneII.generate_tbone_on_short(KINK('G1X-1/G1Y2'), 1)
        self.assertBone(bone, "[G1{Y: -1}, G1{Y: 0}]")

        # vertical short edge
        bone = dogboneII.generate_tbone_on_short(KINK('G1Y1/G1X2'), 1)
        self.assertBone(bone, "[G1{X: -1}, G1{X: 0}]")

        bone = dogboneII.generate_tbone_on_short(KINK('G1Y1/G1X-2'), 1)
        self.assertBone(bone, "[G1{X: 1}, G1{X: 0}]")

        # some other angle
        bone = dogboneII.generate_tbone_on_short(KINK('G1X1Y1/G1Y-1'), 5)
        self.assertBone(bone, "[G1{X: -2.5, Y: 4.5}, G1{X: 1.0, Y: 1.0}]", 2)

        bone = dogboneII.generate_tbone_on_short(KINK('G1X-1Y-1/G1Y1'), 5)
        self.assertBone(bone, "[G1{X: 2.5, Y: -4.5}, G1{X: -1.0, Y: -1.0}]", 2)

        # some other angle
        bone = dogboneII.generate_tbone_on_short(KINK('G1X2Y1/G1Y-3'), 5)
        self.assertBone(bone, "[G1{X: -0.24, Y: 5.5}, G1{X: 2.0, Y: 1.0}]", 2)

        bone = dogboneII.generate_tbone_on_short(KINK('G1X-2Y-1/G1Y3'), 5)
        self.assertBone(bone, "[G1{X: 0.24, Y: -5.5}, G1{X: -2.0, Y: -1.0}]", 2)

        # short edge - the 2nd
        bone = dogboneII.generate_tbone_on_short(KINK('G1Y2/G1X1'), 1)
        self.assertBone(bone, "[G1{Y: 3}, G1{Y: 2}]")
        bone = dogboneII.generate_tbone_on_short(KINK('G1Y2/G1X-1'), 1)
        self.assertBone(bone, "[G1{Y: 3}, G1{Y: 2}]")

        bone = dogboneII.generate_tbone_on_short(KINK('G1Y-3/G1X2Y-2'), 5)
        self.assertBone(bone, "[G1{X: 2.2, Y: -7.5}, G1{X: 0.0, Y: -3.0}]", 2)

        bone = dogboneII.generate_tbone_on_short(KINK('G1Y3/G1X-2Y2'), 5)
        self.assertBone(bone, "[G1{X: -2.2, Y: 7.5}, G1{X: 0.0, Y: 3.0}]", 2)

        # long edge
        bone = dogboneII.generate_tbone_on_long(KINK('G1X2/G1Y1'), 1)
        self.assertBone(bone, "[G1{Y: -1}, G1{Y: 0}]")
        bone = dogboneII.generate_tbone_on_long(KINK('G1X-2/G1Y1'), 1)
        self.assertBone(bone, "[G1{Y: -1}, G1{Y: 0}]")

        bone = dogboneII.generate_tbone_on_long(KINK('G1Y-1/G1X2Y0'), 5)
        self.assertBone(bone, "[G1{X: 2.2, Y: -5.5}, G1{X: 0.0, Y: -1.0}]", 2)

        bone = dogboneII.generate_tbone_on_long(KINK('G1Y1/G1X-2Y0'), 5)
        self.assertBone(bone, "[G1{X: -2.2, Y: 5.5}, G1{X: 0.0, Y: 1.0}]", 2)

    def test70(self):
        """Verify dogbone angles"""
        self.assertRoughly(180 * KINK('G1X1/G1Y+1').normAngle() / PI, -45)
        self.assertRoughly(180 * KINK('G1X1/G1Y-1').normAngle() / PI, 45)

        self.assertRoughly(180 * KINK('G1X1/G1X2Y1').normAngle() / PI, -67.5)
        self.assertRoughly(180 * KINK('G1X1/G1X2Y-1').normAngle() / PI, 67.5)

        self.assertRoughly(180 * KINK('G1Y1/G1X+1').normAngle() / PI, 135)
        self.assertRoughly(180 * KINK('G1Y1/G1X-1').normAngle() / PI, 45)

        self.assertRoughly(180 * KINK('G1X-1/G1Y+1').normAngle() / PI, -135)
        self.assertRoughly(180 * KINK('G1X-1/G1Y-1').normAngle() / PI, 135)

        self.assertRoughly(180 * KINK('G1Y-1/G1X-1').normAngle() / PI, -45)
        self.assertRoughly(180 * KINK('G1Y-1/G1X+1').normAngle() / PI, -135)

    def test71(self):
        """Verify dogbones"""

        bone = dogboneII.generate_dogbone(KINK('G1X1/G1Y1'), 1)
        self.assertBone(bone, "[G1{X: 1.7, Y: -0.71}, G1{X: 1.0, Y: 0.0}]", 2)

        bone = dogboneII.generate_dogbone(KINK('G1X1/G1X3Y-1'), 1)
        self.assertBone(bone, "[G1{X: 1.2, Y: 0.97}, G1{X: 1.0, Y: 0.0}]", 2)

        bone = dogboneII.generate_dogbone(KINK('G1X1Y1/G1X2'), 1)
        self.assertBone(bone, "[G1{X: 0.62, Y: 1.9}, G1{X: 1.0, Y: 1.0}]", 2)

    def test80(self):
        """Verify adaptive length"""

        if True:
            # horizontal bones
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1/G1X2'), 0, 1, DebugMode), 0)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1/G1Y1'), 0, 1, DebugMode), 1)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1/G1X2Y1'), 0, 1, DebugMode), 0.414214)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1/G1X0Y1'), 0, 1, DebugMode), 2.414211)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1/G1X0'), 0, 1, DebugMode), 1)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1/G1X0Y-1'), 0, 1, DebugMode), 2.414211)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1/G1X1Y-1'), 0, 1, DebugMode), 1)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1/G1X2Y-1'), 0, 1, DebugMode), 0.414214)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1Y1/G1X0Y2'), 0, 1, DebugMode), 0.414214)

        if True:
            # more horizontal and some vertical bones
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y1/G1Y2'), 0, 1, DebugMode), 0)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y1/G1Y1X1'), PI, 1, DebugMode), 1)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y1/G1Y2X1'), PI, 1, DebugMode), 0.089820)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y1/G1Y2X1'), PI/2, 1, DebugMode), 0.414214)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y1/G1Y0X1'), PI/2, 1, DebugMode), 2.414211)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y1/G1Y0'), 0, 1, DebugMode), 1)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y1/G1Y0X-1'), PI/2, 1, DebugMode), 2.414211)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y1/G1Y1X-1'), 0, 1, DebugMode), 1)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y1/G1Y2X-1'), 0, 1, DebugMode), 0.089820)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y1/G1Y2X-1'), PI/2, 1, DebugMode), 0.414214)

        if True:
            # dogbones
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1/G1Y1'), -PI/4, 1, DebugMode), 0.414214)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1/G1X0Y1'), -PI/8, 1, DebugMode), 1.613126)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1/G1Y-1'), PI/4, 1, DebugMode), 0.414214)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1/G1X0Y-1'), PI/8, 1, DebugMode), 1.613126)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y1/G1X-1'), PI/4, 1, DebugMode), 0.414214)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y1/G1X1'), 3*PI/4, 1, DebugMode), 0.414214)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y-1/G1X1'), -3*PI/4, 1, DebugMode), 0.414214)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1Y-1/G1X-1'), -PI/4, 1, DebugMode), 0.414214)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1Y1/G1X0Y2'), 0, 1, DebugMode), 0.414214)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X-1Y1/G1X0Y2'), PI, 1, DebugMode), 0.414214)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1Y1/G1X2Y0'), PI/2, 2, DebugMode), 0.828428)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X-1Y-1/G1X-2Y0'), -PI/2, 2, DebugMode), 0.828428)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X-1Y1/G1X-2Y0'), PI/2, 2, DebugMode), 0.828428)
            self.assertRoughly(PathDressupDogboneII.calc_adaptive_length(KINK('G1X1Y-1/G1X2Y0'), -PI/2, 2, DebugMode), 0.828428)

