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
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLanguage as PathLanguage
import PathScripts.PathLog as PathLog
import PathTests.PathTestUtils as PathTestUtils
import math


# PathLog.setLevel(PathLog.Level.DEBUG)
PathLog.setLevel(PathLog.Level.NOTICE)

PI = math.pi
DebugMode = PathLog.getLevel(PathLog.thisModule()) == PathLog.Level.DEBUG

class Kink (object):
    '''A Kink represents the angle at which two moves connect.
A positive kink angle represents a move to the left, and a negative angle represents a move to the right.'''

    def __init__(self, m0, m1):
        if m1 is None:
            m1 = m0[1]
            m0 = m0[0]
        self.m0 = m0
        self.m1 = m1
        self.t0 = m0.anglesOfTangents()[1]
        self.t1 = m1.anglesOfTangents()[0]

    def deflection(self):
        '''deflection() ... returns the tangential difference of the two edges at their intersection'''
        return PathGeom.normalizeAngle(self.t1 - self.t0)

    def normAngle(self):
        '''normAngle() ... returns the angle opposite between the two tangents'''

        # The normal angle is perpendicular to the "average tangent" of the kink. The question
        # is into which direction to turn. One lies in the center between the two edges and the
        # other is opposite to that. As it turns out, the magnitude of the tangents tell it all.
        if self.t0 > self.t1:
            return PathGeom.normalizeAngle((self.t0 + self.t1 + PI) / 2)
        return PathGeom.normalizeAngle((self.t0 + self.t1 - PI) / 2)

    def position(self):
        '''position() ... position of the edge's intersection'''
        return self.m0.positionEnd()

    def x(self):
        return self.position().x

    def y(self):
        return self.position().y

    def __repr__(self):
        return f"({self.x():.4f}, {self.y():.4f})[t0={180*self.t0/PI:.2f}, t1={180*self.t1/PI:.2f}, deflection={180*self.deflection()/PI:.2f}, normAngle={180*self.normAngle()/PI:.2f}]"

def createKinks(maneuver):
    k = []
    moves = maneuver.getMoves()
    if moves:
        move0 = moves[0]
        prev = move0
        for m in moves[1:]:
            k.append(Kink(prev, m))
            prev = m
        if PathGeom.pointsCoincide(move0.positionBegin(), prev.positionEnd()):
            k.append(Kink(prev, move0))
    return k


def findDogboneKinks(maneuver, threshold):
    '''findDogboneKinks(maneuver, threshold) ... return all kinks fitting the criteria.
A positive threshold angle returns all kinks on the right side, and a negative all kinks on the left side'''
    if threshold > 0:
        return [k for k in createKinks(maneuver) if k.deflection() > threshold]
    if threshold < 0:
        return [k for k in createKinks(maneuver) if k.deflection() < threshold]
    # you asked for it ...
    return createKinks(maneuver)


class Bone (object):
    '''A Bone holds all the information of a bone and the kink it is attached to'''

    def __init__(self, kink, angle, instr=None):
        self.kink = kink
        self.angle = angle
        self.instr = [] if instr is None else instr

    def addInstruction(self, instr):
        self.instr.append(instr)

def instruction_to_command(instr):
    return Path.Command(instr.cmd, instr.param)

def kink_to_path(kink, g0=False):
    return Path.Path([instruction_to_command(instr) for instr in [kink.m0, kink.m1]])

def bone_to_path(bone, g0=False):
    kink = bone.kink
    cmds = []
    if g0 and not PathGeom.pointsCoincide(kink.m0.positionBegin(), FreeCAD.Vector(0, 0, 0)):
        pos = kink.m0.positionBegin()
        param = {}
        if not PathGeom.isRoughly(pos.x, 0):
            param['X'] = pos.x
        if not PathGeom.isRoughly(pos.y, 0):
            param['Y'] = pos.y
        cmds.append(Path.Command('G0', param))
    for instr in [kink.m0, bone.instr[0], bone.instr[1], kink.m1]:
        cmds.append(instruction_to_command(instr))
    return Path.Path(cmds)

def generate_tbone(kink, length, dim):
    def getAxisX(v):
        return v.x
    def getAxisY(v):
        return v.y
    axis = getAxisY if dim == 'Y' else getAxisX
    angle = 0 if dim == 'X' else PI/2

    d0 = axis(kink.position())
    dd = axis(kink.m0.positionEnd()) - axis(kink.m0.positionBegin())
    if dd < 0 or (PathGeom.isRoughly(dd, 0) and (axis(kink.m1.positionEnd()) > axis(kink.m1.positionBegin()))):
        length = -length
        angle = angle - PI

    d1 = d0 + length

    moveIn = PathLanguage.MoveStraight(kink.position(), 'G1', {dim: d1})
    moveOut = PathLanguage.MoveStraight(moveIn.positionEnd(), 'G1', {dim: d0})
    return Bone(kink, angle, [moveIn, moveOut])

def generate_tbone_horizontal(kink, length):
    return generate_tbone(kink, length, 'X')

def generate_tbone_vertical(kink, length):
    return generate_tbone(kink, length, 'Y')

def generate_bone(kink, length, angle):
    # These two special cases could be removed, they are more efficient though because they
    # don't require trigonometric function calls. They are also a gentle introduction into
    # the dog/t/bone world, so we'll leave them here for now
    if PathGeom.isRoughly(0, angle) or PathGeom.isRoughly(abs(angle), PI):
        return generate_tbone_horizontal(kink, length)
    if PathGeom.isRoughly(abs(angle), PI/2):
        return generate_tbone_vertical(kink, length)

    #if kink.deflection() > 0:
    #    length = -length

    dx = length * math.cos(angle)
    dy = length * math.sin(angle)
    p0 = kink.position()

    moveIn = PathLanguage.MoveStraight(kink.position(), 'G1', {'X': p0.x + dx, 'Y': p0.y + dy})
    moveOut = PathLanguage.MoveStraight(moveIn.positionEnd(), 'G1', {'X': p0.x, 'Y': p0.y})

    return Bone(kink, angle, [moveIn, moveOut])

def generate_tbone_on_short(kink, length):
    if kink.m0.pathLength() < kink.m1.pathLength():
        a = PathGeom.normalizeAngle(kink.t0 + PI/2)
    else:
        a = PathGeom.normalizeAngle(kink.t1 - PI/2)
    return generate_bone(kink, length, a)

def generate_tbone_on_long(kink, length):
    if kink.m0.pathLength() > kink.m1.pathLength():
        a = PathGeom.normalizeAngle(kink.t0 + PI/2)
    else:
        a = PathGeom.normalizeAngle(kink.t1 - PI/2)
    return generate_bone(kink, length, a)

def generate_dogbone(kink, length):
    #return generate_bone(kink, length, PathGeom.normalizeAngle((kink.deflection() + PI) / 2))
    return generate_bone(kink, length, kink.normAngle())

def calc_adaptive_length(kink, angle, nominal_length):
    if PathGeom.isRoughly(abs(kink.deflection()), 0):
        return 0

    # If the kink poses a 180deg turn the adaptive length is undefined. Mathematically
    # it's infinite but that is not practical.
    # We define the adaptive length to be the nominal length for this case.
    if PathGeom.isRoughly(abs(kink.deflection()), PI):
        return nominal_length

    # The distance of the (estimated) corner from the kink position depends only on the
    # deflection of the kink.
    # Some sample values to build up intuition:
    #           deflection :   dog bone  : norm distance : calc
    #      ----------------:-------------:---------------:--------------
    #               0      :    -PI/2    :   1
    #              PI/6    :  -5*PI/12   :   1.03528     : 1/cos(  (pi/6) / 2)
    #              PI/4    :  -3*PI/8    :   1.08239     : 1/cos(  (pi/4) / 2)
    #              PI/3    :    -PI/3    :   1.1547      : 1/cos(  (pi/3) / 2)
    #              PI/2    :    -PI/4    :   1.41421     : 1/cos(  (pi/2) / 2)
    #            2*PI/3    :    -PI/6    :   2           : 1/cos((2*pi/3) / 2)
    #            3*PI/4    :    -PI/8    :   2.61313     : 1/cos((3*pi/4) / 2)
    #            5*PI/6    :    -PI/12   :   3.8637      : 1/cos((5*pi/6) / 2)
    #              PI      :      0      :   nan  <-- see above
    # The last column can be geometrically derived or found by experimentation.
    dist = nominal_length / math.cos(kink.deflection() / 2)

    # The depth of the bone depends on the direction of the bone in relation to the
    # direction of the corner. If the direction is identical then the depth is the same
    # as the distance of the corner minus the nominal_length (which corresponds to the
    # radius of the tool).
    # If the corner's direction is PI/4 off the bone angle the intersecion of the tool
    # with the corner is the projection of the corner onto the bone.
    # If the corner's direction is perpendicular to the bone's angle there is, strictly
    # speaking no intersection and the bone is ineffective. However, giving it our 
    # best shot we should probably move the entire depth.

    da = PathGeom.normalizeAngle(kink.normAngle() - angle)
    depth = dist * math.cos(da)
    if depth < 0:
        PathLog.debug(f"depth={depth:4f}: kink={kink}, angle={180*angle/PI}, dist={dist:.4f}, da={180*da/PI} -> depth=0.0")
        depth = 0
    else:
        height = dist * abs(math.sin(da))
        if height < nominal_length:
            depth = depth - math.sqrt(nominal_length * nominal_length - height * height)
        PathLog.debug(f"{kink}: angle={180*angle/PI}, dist={dist:.4f}, da={180*da/PI}, depth={depth:.4f}")

    if DebugMode and FreeCAD.GuiUp:
        import Part
        FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup","Group")
        group = FreeCAD.ActiveDocument.ActiveObject
        bone = generate_dogbone(kink, dist)
        Path.show(bone_to_path(bone, True), 'adaptive')
        group.addObject(FreeCAD.ActiveDocument.ActiveObject)
        instr = bone.instr[0]
        Part.show(Part.Edge(Part.makeCircle(.025, instr.positionEnd())), 'adaptive')
        group.addObject(FreeCAD.ActiveDocument.ActiveObject)
        if depth != 0:
            x = kink.position().x + depth * math.cos(angle)
            y = kink.position().y + depth * math.sin(angle)
            pos = FreeCAD.Vector(x, y, 0)
            Part.show(Part.Edge(Part.makeLine(kink.position(), pos)), 'adaptive')
            group.addObject(FreeCAD.ActiveDocument.ActiveObject)
            Part.show(Part.Edge(Part.makeCircle(nominal_length, pos)), 'adaptive')
            group.addObject(FreeCAD.ActiveDocument.ActiveObject)
        group.Visibility = False

    return depth

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
    return Kink(maneuver.instr[0], maneuver.instr[1])

class TestPathDressupDogboneII(PathTestUtils.PathTestBase):
    """Unit tests for the Dogbone dressup."""

    def assertKinks(self, maneuver, s):
        kinks = [f"{k.deflection():4.2f}" for k in createKinks(maneuver)]
        self.assertEqual(f"[{', '.join(kinks)}]", s)

    def assertBones(self, maneuver, threshold, s):
        bones = [f"({int(b.x())},{int(b.y())})" for b in findDogboneKinks(maneuver, threshold)]
        self.assertEqual(f"[{', '.join(bones)}]", s)

    def assertBone(self, bone, s, digits=0):
        if DebugMode and FreeCAD.GuiUp:
            Path.show(kink_to_path(bone.kink))
            FreeCAD.ActiveDocument.Objects[-1].Visibility = False
            Path.show(bone_to_path(bone))
            FreeCAD.ActiveDocument.Objects[-1].Visibility = False
        PathLog.debug(f"{bone.kink} : {bone.angle / PI:.2f}")

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
        kinks = findDogboneKinks(maneuver, PI/4)
        self.assertEqual(len(kinks), 1)
        k = kinks[0]
        p = k.position()
        self.assertEqual(f"({int(p.x)}, {int(p.y)})", "(1, 0)")
        bone = generate_tbone_horizontal(k, 1)
        self.assertBone(bone, "[G1{X: 2}, G1{X: 1}]")

        # full loop CCW
        kinks = findDogboneKinks(MNVR('G1X1/G1Y1/G1X0/G1Y0'), PI/4)
        bones = [generate_tbone_horizontal(k, 1) for k in kinks]
        self.assertEqual(len(bones), 4)
        self.assertBone(bones[0], "[G1{X: 2}, G1{X: 1}]")
        self.assertBone(bones[1], "[G1{X: 2}, G1{X: 1}]")
        self.assertBone(bones[2], "[G1{X: -1}, G1{X: 0}]")
        self.assertBone(bones[3], "[G1{X: -1}, G1{X: 0}]")

        # single move left
        maneuver = MNVR('G1X1/G1Y-1')
        kinks = findDogboneKinks(maneuver, -PI/4)
        self.assertEqual(len(kinks), 1)
        k = kinks[0]
        p = k.position()
        self.assertEqual(f"({int(p.x)}, {int(p.y)})", "(1, 0)")
        bone = generate_tbone_horizontal(k, 1)
        self.assertBone(bone, "[G1{X: 2}, G1{X: 1}]")

        # full loop CW
        kinks = findDogboneKinks(MNVR('G1X1/G1Y-1/G1X0/G1Y0'), -PI/4)
        bones = [generate_tbone_horizontal(k, 1) for k in kinks]
        self.assertEqual(len(bones), 4)
        self.assertBone(bones[0], "[G1{X: 2}, G1{X: 1}]")
        self.assertBone(bones[1], "[G1{X: 2}, G1{X: 1}]")
        self.assertBone(bones[2], "[G1{X: -1}, G1{X: 0}]")
        self.assertBone(bones[3], "[G1{X: -1}, G1{X: 0}]")

        # bones on arcs
        kinks = findDogboneKinks(MNVR('G1X1/G3X3I1/G1Y1/G1X0/G1Y0'),  PI/4);
        bones = [generate_tbone_horizontal(k, 1) for k in kinks]
        self.assertEqual(len(bones), 3)
        self.assertBone(bones[0], "[G1{X: 4}, G1{X: 3}]")
        self.assertBone(bones[1], "[G1{X: -1}, G1{X: 0}]")
        self.assertBone(bones[2], "[G1{X: -1}, G1{X: 0}]")

        # bones on arcs
        kinks = findDogboneKinks(MNVR('G1X1/G3X3I1/G1Y1/G1X0/G1Y0'),  -PI/4);
        bones = [generate_tbone_horizontal(k, 1) for k in kinks]
        self.assertEqual(len(bones), 1)
        self.assertBone(bones[0], "[G1{X: 2}, G1{X: 1}]")

    def test50(self):
        """Verify vertical t-bone creation"""
        # Uses test data from test30, if that broke, this can't succeed

        # single move right
        maneuver = MNVR('G1X1/G1Y1')
        kinks = findDogboneKinks(maneuver, PI/4)
        self.assertEqual(len(kinks), 1)
        k = kinks[0]
        p = k.position()
        self.assertEqual(f"({int(p.x)}, {int(p.y)})", "(1, 0)")
        bone = generate_tbone_vertical(k, 1)
        self.assertBone(bone, "[G1{Y: -1}, G1{Y: 0}]")

        # full loop CCW
        kinks = findDogboneKinks(MNVR('G1X1/G1Y1/G1X0/G1Y0'), PI/4)
        bones = [generate_tbone_vertical(k, 1) for k in kinks]
        self.assertEqual(len(bones), 4)
        self.assertBone(bones[0], "[G1{Y: -1}, G1{Y: 0}]")
        self.assertBone(bones[1], "[G1{Y: 2}, G1{Y: 1}]")
        self.assertBone(bones[2], "[G1{Y: 2}, G1{Y: 1}]")
        self.assertBone(bones[3], "[G1{Y: -1}, G1{Y: 0}]")

        # single move left
        maneuver = MNVR('G1X1/G1Y-1')
        kinks = findDogboneKinks(maneuver, -PI/4)
        self.assertEqual(len(kinks), 1)
        k = kinks[0]
        p = k.position()
        self.assertEqual(f"({int(p.x)}, {int(p.y)})", "(1, 0)")
        bone = generate_tbone_vertical(k, 1)
        self.assertBone(bone, "[G1{Y: 1}, G1{Y: 0}]")

        # full loop CW
        kinks = findDogboneKinks(MNVR('G1X1/G1Y-1/G1X0/G1Y0'), -PI/4)
        bones = [generate_tbone_vertical(k, 1) for k in kinks]
        self.assertEqual(len(bones), 4)
        self.assertBone(bones[0], "[G1{Y: 1}, G1{Y: 0}]")
        self.assertBone(bones[1], "[G1{Y: -2}, G1{Y: -1}]")
        self.assertBone(bones[2], "[G1{Y: -2}, G1{Y: -1}]")
        self.assertBone(bones[3], "[G1{Y: 1}, G1{Y: 0}]")

        # bones on arcs
        kinks = findDogboneKinks(MNVR('G1X1/G3X3I1/G1Y1/G1X0/G1Y0'),  PI/4);
        bones = [generate_tbone_vertical(k, 1) for k in kinks]
        self.assertEqual(len(bones), 3)
        self.assertBone(bones[0], "[G1{Y: 2}, G1{Y: 1}]")
        self.assertBone(bones[1], "[G1{Y: 2}, G1{Y: 1}]")
        self.assertBone(bones[2], "[G1{Y: -1}, G1{Y: 0}]")

        # bones on arcs
        kinks = findDogboneKinks(MNVR('G1X1/G3X3I1/G1Y1/G1X0/G1Y0'),  -PI/4);
        bones = [generate_tbone_vertical(k, 1) for k in kinks]
        self.assertEqual(len(bones), 1)
        self.assertBone(bones[0], "[G1{Y: 1}, G1{Y: 0}]")

    def test60(self):
        """Verify t-bones on edges"""

        # horizontal short edge
        bone = generate_tbone_on_short(KINK('G1X1/G1Y2'), 1)
        self.assertBone(bone, "[G1{Y: -1}, G1{Y: 0}]")

        bone = generate_tbone_on_short(KINK('G1X-1/G1Y2'), 1)
        self.assertBone(bone, "[G1{Y: -1}, G1{Y: 0}]")

        # vertical short edge
        bone = generate_tbone_on_short(KINK('G1Y1/G1X2'), 1)
        self.assertBone(bone, "[G1{X: -1}, G1{X: 0}]")

        bone = generate_tbone_on_short(KINK('G1Y1/G1X-2'), 1)
        self.assertBone(bone, "[G1{X: 1}, G1{X: 0}]")

        # some other angle
        bone = generate_tbone_on_short(KINK('G1X1Y1/G1Y-1'), 5)
        self.assertBone(bone, "[G1{X: -2.5, Y: 4.5}, G1{X: 1.0, Y: 1.0}]", 2)

        bone = generate_tbone_on_short(KINK('G1X-1Y-1/G1Y1'), 5)
        self.assertBone(bone, "[G1{X: 2.5, Y: -4.5}, G1{X: -1.0, Y: -1.0}]", 2)

        # some other angle
        bone = generate_tbone_on_short(KINK('G1X2Y1/G1Y-3'), 5)
        self.assertBone(bone, "[G1{X: -0.24, Y: 5.5}, G1{X: 2.0, Y: 1.0}]", 2)

        bone = generate_tbone_on_short(KINK('G1X-2Y-1/G1Y3'), 5)
        self.assertBone(bone, "[G1{X: 0.24, Y: -5.5}, G1{X: -2.0, Y: -1.0}]", 2)

        # short edge - the 2nd
        bone = generate_tbone_on_short(KINK('G1Y2/G1X1'), 1)
        self.assertBone(bone, "[G1{Y: 3}, G1{Y: 2}]")
        bone = generate_tbone_on_short(KINK('G1Y2/G1X-1'), 1)
        self.assertBone(bone, "[G1{Y: 3}, G1{Y: 2}]")

        bone = generate_tbone_on_short(KINK('G1Y-3/G1X2Y-2'), 5)
        self.assertBone(bone, "[G1{X: 2.2, Y: -7.5}, G1{X: 0.0, Y: -3.0}]", 2)

        bone = generate_tbone_on_short(KINK('G1Y3/G1X-2Y2'), 5)
        self.assertBone(bone, "[G1{X: -2.2, Y: 7.5}, G1{X: 0.0, Y: 3.0}]", 2)

        # long edge
        bone = generate_tbone_on_long(KINK('G1X2/G1Y1'), 1)
        self.assertBone(bone, "[G1{Y: -1}, G1{Y: 0}]")
        bone = generate_tbone_on_long(KINK('G1X-2/G1Y1'), 1)
        self.assertBone(bone, "[G1{Y: -1}, G1{Y: 0}]")

        bone = generate_tbone_on_long(KINK('G1Y-1/G1X2Y0'), 5)
        self.assertBone(bone, "[G1{X: 2.2, Y: -5.5}, G1{X: 0.0, Y: -1.0}]", 2)

        bone = generate_tbone_on_long(KINK('G1Y1/G1X-2Y0'), 5)
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

        bone = generate_dogbone(KINK('G1X1/G1Y1'), 1)
        self.assertBone(bone, "[G1{X: 1.7, Y: -0.71}, G1{X: 1.0, Y: 0.0}]", 2)

        bone = generate_dogbone(KINK('G1X1/G1X3Y-1'), 1)
        self.assertBone(bone, "[G1{X: 1.2, Y: 0.97}, G1{X: 1.0, Y: 0.0}]", 2)

        bone = generate_dogbone(KINK('G1X1Y1/G1X2'), 1)
        self.assertBone(bone, "[G1{X: 0.62, Y: 1.9}, G1{X: 1.0, Y: 1.0}]", 2)

    def test80(self):
        """Verify adaptive length"""

        if True:
            # horizontal bones
            self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X2'), 0, 1), 0)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1Y1'), 0, 1), 1)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X2Y1'), 0, 1), 0.414214)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X0Y1'), 0, 1), 2.414211)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X0'), 0, 1), 1)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X0Y-1'), 0, 1), 2.414211)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X1Y-1'), 0, 1), 1)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X2Y-1'), 0, 1), 0.414214)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1Y1/G1X0Y2'), 0, 1), 0.414214)

        if True:
            # more horizontal and some vertical bones
            self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1Y2'), 0, 1), 0)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1Y1X1'), PI, 1), 1)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1Y2X1'), PI, 1), 0.089820)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1Y2X1'), PI/2, 1), 0.414214)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1Y0X1'), PI/2, 1), 2.414211)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1Y0'), 0, 1), 1)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1Y0X-1'), PI/2, 1), 2.414211)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1Y1X-1'), 0, 1), 1)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1Y2X-1'), 0, 1), 0.089820)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1Y2X-1'), PI/2, 1), 0.414214)

        if True:
            # dogbones
            self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1Y1'), -PI/4, 1), 0.414214)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X0Y1'), -PI/8, 1), 1.613126)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1Y-1'), PI/4, 1), 0.414214)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X0Y-1'), PI/8, 1), 1.613126)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1X-1'), PI/4, 1), 0.414214)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1X1'), 3*PI/4, 1), 0.414214)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y-1/G1X1'), -3*PI/4, 1), 0.414214)
            self.assertRoughly(calc_adaptive_length(KINK('G1Y-1/G1X-1'), -PI/4, 1), 0.414214)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1Y1/G1X0Y2'), 0, 1), 0.414214)
            self.assertRoughly(calc_adaptive_length(KINK('G1X-1Y1/G1X0Y2'), PI, 1), 0.414214)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1Y1/G1X2Y0'), PI/2, 2), 0.828428)
            self.assertRoughly(calc_adaptive_length(KINK('G1X-1Y-1/G1X-2Y0'), -PI/2, 2), 0.828428)
            self.assertRoughly(calc_adaptive_length(KINK('G1X-1Y1/G1X-2Y0'), PI/2, 2), 0.828428)
            self.assertRoughly(calc_adaptive_length(KINK('G1X1Y-1/G1X2Y0'), -PI/2, 2), 0.828428)

