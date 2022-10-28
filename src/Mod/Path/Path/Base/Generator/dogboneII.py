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
import Path.Base.Language as PathLanguage
import math

# Path.Log.trackModule(Path.Log.thisModule())

PI = math.pi


class Kink(object):
    """A Kink represents the angle at which two moves connect.
    A positive kink angle represents a move to the left, and a negative angle represents a move to the right."""

    def __init__(self, m0, m1):
        if m1 is None:
            m1 = m0[1]
            m0 = m0[0]
        self.m0 = m0
        self.m1 = m1
        self.t0 = m0.anglesOfTangents()[1]
        self.t1 = m1.anglesOfTangents()[0]
        if Path.Geom.isRoughly(self.t0, self.t1):
            self.defl = 0
        else:
            self.defl = Path.Geom.normalizeAngle(self.t1 - self.t0)

    def isKink(self):
        return self.defl != 0

    def goesLeft(self):
        return self.defl > 0

    def goesRight(self):
        return self.defl < 0

    def deflection(self):
        """deflection() ... returns the tangential difference of the two edges at their intersection"""
        return self.defl

    def normAngle(self):
        """normAngle() ... returns the angle opposite between the two tangents"""

        # The normal angle is perpendicular to the "average tangent" of the kink. The question
        # is into which direction to turn. One lies in the center between the two edges and the
        # other is opposite to that. As it turns out, the magnitude of the tangents tell it all.
        if self.t0 > self.t1:
            return Path.Geom.normalizeAngle((self.t0 + self.t1 + math.pi) / 2)
        return Path.Geom.normalizeAngle((self.t0 + self.t1 - math.pi) / 2)

    def position(self):
        """position() ... position of the edge's intersection"""
        return self.m0.positionEnd()

    def x(self):
        return self.position().x

    def y(self):
        return self.position().y

    def __repr__(self):
        return f"({self.x():.4f}, {self.y():.4f})[t0={180*self.t0/math.pi:.2f}, t1={180*self.t1/math.pi:.2f}, deflection={180*self.defl/math.pi:.2f}, normAngle={180*self.normAngle()/math.pi:.2f}]"


class Bone(object):
    """A Bone holds all the information of a bone and the kink it is attached to"""

    def __init__(self, kink, angle, length, instr=None):
        self.kink = kink
        self.angle = angle
        self.length = length
        self.instr = [] if instr is None else instr

    def addInstruction(self, instr):
        self.instr.append(instr)

    def position(self):
        """pos() ... return the position of the bone"""
        return self.kink.position()

    def tip(self):
        """tip() ... return the tip of the bone."""
        dx = abs(self.length) * math.cos(self.angle)
        dy = abs(self.length) * math.sin(self.angle)
        return self.position() + FreeCAD.Vector(dx, dy, 0)


def kink_to_path(kink, g0=False):
    return Path.Path(
        [PathLanguage.instruction_to_command(instr) for instr in [kink.m0, kink.m1]]
    )


def bone_to_path(bone, g0=False):
    kink = bone.kink
    cmds = []
    if g0 and not Path.Geom.pointsCoincide(
        kink.m0.positionBegin(), FreeCAD.Vector(0, 0, 0)
    ):
        pos = kink.m0.positionBegin()
        param = {}
        if not Path.Geom.isRoughly(pos.x, 0):
            param["X"] = pos.x
        if not Path.Geom.isRoughly(pos.y, 0):
            param["Y"] = pos.y
        cmds.append(Path.Command("G0", param))
    for instr in [kink.m0, bone.instr[0], bone.instr[1], kink.m1]:
        cmds.append(PathLanguage.instruction_to_command(instr))
    return Path.Path(cmds)


def generate_bone(kink, length, angle):
    dx = length * math.cos(angle)
    dy = length * math.sin(angle)
    p0 = kink.position()

    if Path.Geom.isRoughly(0, dx):
        # vertical bone
        moveIn = PathLanguage.MoveStraight(kink.position(), "G1", {"Y": p0.y + dy})
        moveOut = PathLanguage.MoveStraight(moveIn.positionEnd(), "G1", {"Y": p0.y})
    elif Path.Geom.isRoughly(0, dy):
        # horizontal bone
        moveIn = PathLanguage.MoveStraight(kink.position(), "G1", {"X": p0.x + dx})
        moveOut = PathLanguage.MoveStraight(moveIn.positionEnd(), "G1", {"X": p0.x})
    else:
        moveIn = PathLanguage.MoveStraight(
            kink.position(), "G1", {"X": p0.x + dx, "Y": p0.y + dy}
        )
        moveOut = PathLanguage.MoveStraight(
            moveIn.positionEnd(), "G1", {"X": p0.x, "Y": p0.y}
        )

    return Bone(kink, angle, length, [moveIn, moveOut])


class Generator(object):
    def __init__(self, calc_length, nominal_length, custom_length):
        self.calc_length = calc_length
        self.nominal_length = nominal_length
        self.custom_length = custom_length

    def length(self, kink, angle):
        return self.calc_length(kink, angle, self.nominal_length, self.custom_length)

    def generate_func(self):
        return generate_bone

    def generate(self, kink):
        angle = self.angle(kink)
        return self.generate_func()(kink, self.length(kink, angle), angle)


class GeneratorTBoneHorizontal(Generator):
    def angle(self, kink):
        if abs(kink.normAngle()) > (PI / 2):
            return -PI
        else:
            return 0


class GeneratorTBoneVertical(Generator):
    def angle(self, kink):
        if kink.normAngle() > 0:
            return PI / 2
        else:
            return -PI / 2


class GeneratorTBoneOnShort(Generator):
    def angle(self, kink):
        rot = PI / 2 if kink.goesRight() else -PI / 2

        if kink.m0.pathLength() < kink.m1.pathLength():
            return Path.Geom.normalizeAngle(kink.t0 + rot)
        else:
            return Path.Geom.normalizeAngle(kink.t1 + rot)


class GeneratorTBoneOnLong(Generator):
    def angle(self, kink):
        rot = PI / 2 if kink.goesRight() else -PI / 2

        if kink.m0.pathLength() > kink.m1.pathLength():
            return Path.Geom.normalizeAngle(kink.t0 + rot)
        else:
            return Path.Geom.normalizeAngle(kink.t1 + rot)


class GeneratorDogbone(Generator):
    def angle(self, kink):
        return kink.normAngle()


def generate(kink, generator, calc_length, nominal_length, custom_length=None):
    if custom_length is None:
        custom_length = nominal_length
    gen = generator(calc_length, nominal_length, custom_length)
    return gen.generate(kink)
