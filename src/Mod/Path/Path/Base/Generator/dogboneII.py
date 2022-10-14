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

import Path
import Path.Base.Language as PathLanguage

import math

PI = math.pi

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
        return Path.Geom.normalizeAngle(self.t1 - self.t0)

    def normAngle(self):
        '''normAngle() ... returns the angle opposite between the two tangents'''

        # The normal angle is perpendicular to the "average tangent" of the kink. The question
        # is into which direction to turn. One lies in the center between the two edges and the
        # other is opposite to that. As it turns out, the magnitude of the tangents tell it all.
        if self.t0 > self.t1:
            return Path.Geom.normalizeAngle((self.t0 + self.t1 + math.pi) / 2)
        return Path.Geom.normalizeAngle((self.t0 + self.t1 - math.pi) / 2)

    def position(self):
        '''position() ... position of the edge's intersection'''
        return self.m0.positionEnd()

    def x(self):
        return self.position().x

    def y(self):
        return self.position().y

    def __repr__(self):
        return f"({self.x():.4f}, {self.y():.4f})[t0={180*self.t0/math.pi:.2f}, t1={180*self.t1/math.pi:.2f}, deflection={180*self.deflection()/math.pi:.2f}, normAngle={180*self.normAngle()/math.pi:.2f}]"

class Bone (object):
    '''A Bone holds all the information of a bone and the kink it is attached to'''

    def __init__(self, kink, angle, instr=None):
        self.kink = kink
        self.angle = angle
        self.instr = [] if instr is None else instr

    def addInstruction(self, instr):
        self.instr.append(instr)

def generate_tbone(kink, length, dim):
    def getAxisX(v):
        return v.x
    def getAxisY(v):
        return v.y
    axis = getAxisY if dim == 'Y' else getAxisX
    angle = 0 if dim == 'X' else PI/2

    d0 = axis(kink.position())
    dd = axis(kink.m0.positionEnd()) - axis(kink.m0.positionBegin())
    if dd < 0 or (Path.Geom.isRoughly(dd, 0) and (axis(kink.m1.positionEnd()) > axis(kink.m1.positionBegin()))):
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
    if Path.Geom.isRoughly(0, angle) or Path.Geom.isRoughly(abs(angle), PI):
        return generate_tbone_horizontal(kink, length)
    if Path.Geom.isRoughly(abs(angle), PI/2):
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
        a = Path.Geom.normalizeAngle(kink.t0 + PI/2)
    else:
        a = Path.Geom.normalizeAngle(kink.t1 - PI/2)
    return generate_bone(kink, length, a)

def generate_tbone_on_long(kink, length):
    if kink.m0.pathLength() > kink.m1.pathLength():
        a = Path.Geom.normalizeAngle(kink.t0 + PI/2)
    else:
        a = Path.Geom.normalizeAngle(kink.t1 - PI/2)
    return generate_bone(kink, length, a)

def generate_dogbone(kink, length):
    #return generate_bone(kink, length, Path.Geom.normalizeAngle((kink.deflection() + PI) / 2))
    return generate_bone(kink, length, kink.normAngle())
