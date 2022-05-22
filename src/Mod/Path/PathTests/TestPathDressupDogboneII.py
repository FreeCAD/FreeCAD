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
import math

from PathTests.PathTestUtils import PathTestBase

CmdMoveStraight = PathGeom.CmdMoveStraight + PathGeom.CmdMoveRapid

PI = math.pi

def normalizeAngle(a):
    while a > PI:
        a = a - 2 * PI
    while a < -PI:
        a = a + 2 * PI
    return a

class Instruction (object):
    '''An Instruction is pure python replacement of Path.Command which also tracks its begin position.'''

    def __init__(self, begin, cmd, param=None):
        self.begin = begin
        if type(cmd) == Path.Command:
            self.cmd = Path.Name
            self.param = Path.Parameters
        else:
            self.cmd = cmd
            if param is None:
                self.param = {}
            else:
                self.param = param

    def anglesOfTangents(self):
        return (0, 0)

    def setPositionBegin(self, begin):
        self.begin = begin

    def positionBegin(self):
        '''positionBegin() ... returns a Vector of the begin position'''
        return self.begin

    def positionEnd(self):
        '''positionEnd() ... returns a Vector of the end position'''
        return FreeCAD.Vector(self.x(self.begin.x), self.y(self.begin.y), self.z(self.begin.z))

    def pathLength(self):
        '''pathLength() ... returns the lenght in mm'''
        return 0

    def isMove(self):
        return False

    def x(self, default=0):
        return self.param.get('X', default)

    def y(self, default=0):
        return self.param.get('Y', default)

    def z(self, default=0):
        return self.param.get('Z', default)

    def i(self, default=0):
        return self.param.get('I', default)

    def j(self, default=0):
        return self.param.get('J', default)

    def k(self, default=0):
        return self.param.get('K', default)

    def xyBegin(self):
        '''xyBegin() ... internal convenience function'''
        return FreeCAD.Vector(self.begin.x, self.begin.y, 0)
    def xyEnd(self):
        '''xyEnd() ... internal convenience function'''
        return FreeCAD.Vector(self.x(self.begin.x), self.y(self.begin.y), 0)

    def __repr__(self):
        return f"{self.cmd}{self.param}"

    def str(self, digits=2):
        if digits == 0:
            s = [f"{k}: {int(v)}" for k, v in self.param.items()]
        else:
            fmt = f"{{}}: {{:.{digits}}}"
            s = [fmt.format(k, v) for k, v in self.param.items()]
        return f"{self.cmd}{{{', '.join(s)}}}"

class MoveStraight (Instruction):

    def anglesOfTangents(self):
        '''anglesOfTangents() ... return a tuple with the tangent angles at begin and end position'''
        begin = self.xyBegin()
        end = self.xyEnd()
        if end == begin:
            return (0, 0)
        a = PathGeom.getAngle(end - begin)
        return (a, a)

    def isMove(self):
        return True

    def pathLength(self):
        return (self.positionEnd() - self.positionBegin()).Length

class MoveArc (Instruction):

    def anglesOfTangents(self):
        '''anglesOfTangents() ... return a tuple with the tangent angles at begin and end position'''
        begin = self.xyBegin()
        end = self.xyEnd()
        center = self.xyCenter()
        # calculate angle of the hypotenuse at begin and end
        s0 = PathGeom.getAngle(begin - center)
        s1 = PathGeom.getAngle(end - center)
        # the tangents are perpendicular to the hypotenuse with the sign determined by the
        # direction of the arc
        return (normalizeAngle(s0 + self.arcDirection()), normalizeAngle(s1 + self.arcDirection()))

    def isMove(self):
        return True

    def isArc(self):
        return True

    def isCW(self):
        return self.arcDirection() < 0

    def isCCW(self):
        return self.arcDirection() > 0

    def arcAngle(self):
        '''arcAngle() ... return the angle of the arc opening'''
        begin = self.xyBegin()
        end = self.xyEnd()
        center = self.xyCenter()
        s0 = PathGeom.getAngle(begin - center)
        s1 = PathGeom.getAngle(end - center)

        if self.isCW():
            while s0 < s1:
                s0 = s0 + 2 * PI
            return s0 - s1

        # CCW
        while s1 < s0:
            s1 = s1 + 2 * PI
        return s1 - s0

    def arcRadius(self):
        '''arcRadius() ... return the radius'''
        return (self.xyBegin() - self.xyCenter()).Length

    def pathLength(self):
        return self.arcAngle() * self.arcRadius()

    def xyCenter(self):
        return FreeCAD.Vector(self.begin.x + self.i(), self.begin.y + self.j(), 0)

class MoveArcCW (MoveArc):
    def arcDirection(self):
        return -PI/2

class MoveArcCCW (MoveArc):
    def arcDirection(self):
        return PI/2

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
        return normalizeAngle(self.t1 - self.t0)

    def normAngle(self):
        return normalizeAngle((self.t1 + self.t0 + PI) / 2)

    def position(self):
        return self.m0.positionEnd()

    def x(self):
        return self.position().x

    def y(self):
        return self.position().y

class Maneuver (object):
    '''A series of instructions and moves'''

    def __init__(self, begin=None, instr=None):
        self.instr = instr if instr else []
        self.setPositionBegin(begin if begin else FreeCAD.Vector(0, 0, 0))

    def setPositionBegin(self, begin):
        self.begin = begin
        for i in self.instr:
            i.setPositionBegin(begin)
            begin = i.positionEnd()

    def positionBegin(self):
        return self.begin

    def getMoves(self):
        return [instr for instr in self.instr if instr.isMove()]

    def __repr__(self):
        if self.instr:
            return '\n'.join([str(i) for i in self.instr])
        return ''

    @classmethod
    def InstructionFromCommand(cls, cmd, begin=None):
        if not begin:
            begin = FreeCAD.Vector(0, 0, 0)

        if cmd.Name in CmdMoveStraight:
            return MoveStraight(begin, cmd.Name, cmd.Parameters)
        if cmd.Name in PathGeom.CmdMoveCW:
            return MoveArcCW(begin, cmd.Name, cmd.Parameters)
        if cmd.Name in PathGeom.CmdMoveCCW:
            return MoveArcCCW(begin, cmd.Name, cmd.Parameters)
        return Instruction(begin, cmd.Name, cmd.Parameters)

    @classmethod
    def FromPath(cls, path, begin=None):
        maneuver = Maneuver(begin)
        instr = []
        begin = maneuver.positionBegin()
        for cmd in path.Commands:
            i = cls.InstructionFromCommand(cmd, begin)
            instr.append(i)
            begin = i.positionEnd()
        maneuver.instr = instr
        return maneuver

    @classmethod
    def FromGCode(cls, gcode, begin=None):
        return cls.FromPath(Path.Path(gcode), begin)

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

    def __init__(self, kink, instr=None):
        self.kink = kink
        self.instr = [] if instr is None else instr

    def addInstruction(self, instr):
        self.instr.append(instr)

def generate_tbone(kink, length, dim):
    def getAxisX(v):
        return v.x
    def getAxisY(v):
        return v.y
    axis = getAxisY if dim == 'Y' else getAxisX
    d0 = axis(kink.position())
    dd = axis(kink.m0.positionEnd()) - axis(kink.m0.positionBegin())
    if dd < 0 or (PathGeom.isRoughly(dd, 0) and (axis(kink.m1.positionEnd()) > axis(kink.m1.positionBegin()))):
        length = -length

    d1 = d0 + length

    moveIn = MoveStraight(kink.position(), 'G1', {dim: d1})
    moveOut = MoveStraight(moveIn.positionEnd(), 'G1', {dim: d0})
    return Bone(kink, [moveIn, moveOut])

def generate_tbone_horizontal(kink, length):
    return generate_tbone(kink, length, 'X')

def generate_tbone_vertical(kink, length):
    return generate_tbone(kink, length, 'Y')

def generate_bone(kink, length, a):
    # These two special cases could be removed, they are more efficient though because they
    # don't require trigonometric function calls. They are also a gentle introduction into
    # the dog/t/bone world, so we'll leave them here for now
    if PathGeom.isRoughly(0, a) or PathGeom.isRoughly(abs(a), PI):
        return generate_tbone_horizontal(kink, length)
    if PathGeom.isRoughly(abs(a), PI/2):
        return generate_tbone_vertical(kink, length)

    if kink.deflection() > 0:
        length = -length

    dx = length * math.cos(a)
    dy = length * math.sin(a)
    p0 = kink.position()

    moveIn = MoveStraight(kink.position(), 'G1', {'X': p0.x + dx, 'Y': p0.y + dy})
    moveOut = MoveStraight(moveIn.positionEnd(), 'G1', {'X': p0.x, 'Y': p0.y})

    return Bone(kink, [moveIn, moveOut])

def generate_tbone_on_short(kink, length):
    if kink.m0.pathLength() < kink.m1.pathLength():
        a = normalizeAngle(kink.m0.anglesOfTangents()[1] + PI/2)
    else:
        a = normalizeAngle(kink.m1.anglesOfTangents()[0] + PI/2)
    return generate_bone(kink, length, a)

def generate_tbone_on_long(kink, length):
    if kink.m0.pathLength() > kink.m1.pathLength():
        a = normalizeAngle(kink.m0.anglesOfTangents()[1] + PI/2)
    else:
        a = normalizeAngle(kink.m1.anglesOfTangents()[0] + PI/2)
    return generate_bone(kink, length, a)

def generate_dogbone(kink, length):
    #return generate_bone(kink, length, normalizeAngle((kink.deflection() + PI) / 2))
    return generate_bone(kink, length, kink.normAngle())

def calc_adaptive_length(kink, angle, nominal_length):
    # If the kink poses a 180deg turn the adaptive length is undefined. Mathematically
    # it's infinite but that is not practical.
    # We define the adaptive length to be the nominal length for this case.
    if PathGeom.isRoughly(abs(kink.deflection()), PI):
        return nominal_length

    # In order to determine the actual (estimated) corner we construct a right-angled
    # triangle with one corner being the kink position, and the nominal_length which
    # is the length of the leg perpendicular to the incoming or outgoing tangent.
    # The corner is in the direction of the kink's normAngle and we can calculate
    # its length.
    a1 = kink.t1 + PI / 2 # global angle of perpendicular triangle leg
    ab = kink.normAngle() # global angle of actual corner from kink position
    a = normalizeAngle(a1 - ab)
    d = nominal_length / math.cos(a) # distance of actual corner from kink position

    # Now that we know where the actual corner is we need to determine the projection
    # of that on the actual bone
    al = normalizeAngle(ab - angle)
    l = abs(d * math.cos(al))
    print(f"a1={a1/(PI/4)}, ab={ab/(PI/4)}, a={a/(PI/4)}, d={d}  ->  al={al/(PI/4)},  l={l}")

    return l

def MNVR(gcode, begin=None):
    # 'turns out the replace() isn't really necessary
    # leave it here anyway for clarity
    return Maneuver.FromGCode(gcode.replace('/', '\n'), begin)

def INSTR(gcode, begin=None):
    return MNVR(gcode, begin).instr[0]

def KINK(gcode, begin=None):
    maneuver = MNVR(gcode, begin)
    if len(maneuver.instr) != 2:
        return None
    return Kink(maneuver.instr[0], maneuver.instr[1])

class TestDressupDogboneII(PathTestBase):
    """Unit tests for the Dogbone dressup."""

    def assertTangents(self, instr, t1):
        """Assert that the two tangent angles are identical"""
        t0 = instr.anglesOfTangents()
        self.assertRoughly(t0[0], t1[0])
        self.assertRoughly(t0[1], t1[1])

    def assertKinks(self, maneuver, s):
        kinks = [f"{k.deflection():4.2f}" for k in createKinks(maneuver)]
        self.assertEqual(f"[{', '.join(kinks)}]", s)

    def assertBones(self, maneuver, threshold, s):
        bones = [f"({int(b.x())},{int(b.y())})" for b in findDogboneKinks(maneuver, threshold)]
        self.assertEqual(f"[{', '.join(bones)}]", s)

    def assertBone(self, bone, s, digits=0):
        b = [i.str(digits) for i in bone.instr]
        self.assertEqual(f"[{', '.join(b)}]", s)

    def test00(self):
        """Verify G0 instruction construction"""
        self.assertEqual(str(Maneuver.FromGCode('')), '')
        self.assertEqual(len(Maneuver.FromGCode('').instr), 0)

        self.assertEqual(str(Maneuver.FromGCode('G0')), 'G0{}')
        self.assertEqual(str(Maneuver.FromGCode('G0X3')), "G0{'X': 3.0}")
        self.assertEqual(str(Maneuver.FromGCode('G0X3Y7')), "G0{'X': 3.0, 'Y': 7.0}")
        self.assertEqual(str(Maneuver.FromGCode('G0X3Y7/G0Z0')), "G0{'X': 3.0, 'Y': 7.0}\nG0{'Z': 0.0}")
        self.assertEqual(len(Maneuver.FromGCode('G0X3Y7').instr), 1)
        self.assertEqual(len(Maneuver.FromGCode('G0X3Y7/G0Z0').instr), 2)
        self.assertEqual(type(Maneuver.FromGCode('G0X3Y7').instr[0]), MoveStraight)

    def test01(self):
        """Verify G1 instruction construction"""
        self.assertEqual(str(Maneuver.FromGCode('G1')), 'G1{}')
        self.assertEqual(str(Maneuver.FromGCode('G1X3')), "G1{'X': 3.0}")
        self.assertEqual(str(Maneuver.FromGCode('G1X3Y7')), "G1{'X': 3.0, 'Y': 7.0}")
        self.assertEqual(str(Maneuver.FromGCode('G1X3Y7/G1Z0')), "G1{'X': 3.0, 'Y': 7.0}\nG1{'Z': 0.0}")
        self.assertEqual(len(Maneuver.FromGCode('G1X3Y7').instr), 1)
        self.assertEqual(len(Maneuver.FromGCode('G1X3Y7/G1Z0').instr), 2)
        self.assertEqual(type(Maneuver.FromGCode('G1X3Y7').instr[0]), MoveStraight)

    def test02(self):
        """Verify G2 instruction construction"""
        self.assertEqual(str(Maneuver.FromGCode('G2X2Y2I1')), "G2{'I': 1.0, 'X': 2.0, 'Y': 2.0}")
        self.assertEqual(len(Maneuver.FromGCode('G2X2Y2I1').instr), 1)
        self.assertEqual(type(Maneuver.FromGCode('G2X2Y2I1').instr[0]), MoveArcCW)

    def test03(self):
        """Verify G3 instruction construction"""
        self.assertEqual(str(Maneuver.FromGCode('G3X2Y2I1')), "G3{'I': 1.0, 'X': 2.0, 'Y': 2.0}")
        self.assertEqual(len(Maneuver.FromGCode('G3X2Y2I1').instr), 1)
        self.assertEqual(type(Maneuver.FromGCode('G3X2Y2I1').instr[0]), MoveArcCCW)

    def test04(self):
        """Verify pathLength correctness"""
        self.assertRoughly(Maneuver.FromGCode('G1X3').instr[0].pathLength(), 3)
        self.assertRoughly(Maneuver.FromGCode('G1X-7').instr[0].pathLength(), 7)
        self.assertRoughly(Maneuver.FromGCode('G1X3').instr[0].pathLength(), 3)

        self.assertRoughly(Maneuver.FromGCode('G1X3Y4').instr[0].pathLength(), 5)
        self.assertRoughly(Maneuver.FromGCode('G1X3Y-4').instr[0].pathLength(), 5)
        self.assertRoughly(Maneuver.FromGCode('G1X-3Y-4').instr[0].pathLength(), 5)
        self.assertRoughly(Maneuver.FromGCode('G1X-3Y4').instr[0].pathLength(), 5)

        self.assertRoughly(Maneuver.FromGCode('G2X2I1').instr[0].pathLength(), PI)
        self.assertRoughly(Maneuver.FromGCode('G2X1Y1I1').instr[0].pathLength(), PI/2)

        self.assertRoughly(Maneuver.FromGCode('G3X2I1').instr[0].pathLength(), PI)
        self.assertRoughly(Maneuver.FromGCode('G3X1Y1I1').instr[0].pathLength(), 3*PI/2)


    def test10(self):
        """Verify tangents of moves."""

        self.assertTangents(INSTR('G1 X0  Y0'), (0, 0)) # by declaration
        self.assertTangents(INSTR('G1 X1  Y0'), (0, 0))
        self.assertTangents(INSTR('G1 X-1 Y0'), (PI, PI))
        self.assertTangents(INSTR('G1 X0  Y1'), (PI/2,  PI/2))
        self.assertTangents(INSTR('G1 X0  Y-1'), (-PI/2,  -PI/2))
        self.assertTangents(INSTR('G1 X1  Y1'), (PI/4,  PI/4))
        self.assertTangents(INSTR('G1 X-1 Y1'), (3*PI/4,  3*PI/4))
        self.assertTangents(INSTR('G1 X-1 Y -1'), (-3*PI/4,  -3*PI/4))
        self.assertTangents(INSTR('G1 X1  Y-1'), (-PI/4,  -PI/4))

        self.assertTangents(INSTR('G2 X2  Y0  I1 J0'), (PI/2, -PI/2))
        self.assertTangents(INSTR('G2 X2  Y2  I1 J1'), (3*PI/4, -PI/4))
        self.assertTangents(INSTR('G2 X0  Y-2 I0 J-1'), (0, -PI))

        self.assertTangents(INSTR('G3 X2  Y0  I1 J0'), (-PI/2, PI/2))
        self.assertTangents(INSTR('G3 X2  Y2  I1 J1'), (-PI/4, 3*PI/4))
        self.assertTangents(INSTR('G3 X0  Y-2 I0 J-1'), (PI, 0))


    def test20(self):
        """Verify kinks of maneuvers"""
        self.assertKinks(MNVR('G1X1/G1Y1'), '[1.57]')
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
        """Verify dogbones"""

        bone = generate_dogbone(KINK('G1X1/G1Y1'), 1)
        self.assertBone(bone, "[G1{X: 1.7, Y: -0.71}, G1{X: 1.0, Y: 0.0}]", 2)

        bone = generate_dogbone(KINK('G1X1/G1X3Y-1'), 1)
        self.assertBone(bone, "[G1{X: 1.2, Y: 0.97}, G1{X: 1.0, Y: 0.0}]", 2)

        bone = generate_dogbone(KINK('G1X1Y1/G1X2'), 1)
        self.assertBone(bone, "[G1{X: 0.62, Y: 1.9}, G1{X: 1.0, Y: 1.0}]", 2)

    def test80(self):
        """Verify adaptive length for horizontal bone"""

        print()
        self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X2'), 0, 1), 0)
        self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X1Y1'), 0, 1), 1)
        self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X2Y1'), 0, 1), 0.414214)
        self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X0Y1'), 0, 1), 2.414211)
        self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X0'), 0, 1), 1)
        self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X0Y-1'), 0, 1), 2.414211)
        self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X1Y-1'), 0, 1), 1)
        self.assertRoughly(calc_adaptive_length(KINK('G1X1/G1X2Y-1'), 0, 1), 0.414214)

        #self.assertRoughly(calc_adaptive_length(KINK('G1Y1/G1X1'), PI, 1), 1)
