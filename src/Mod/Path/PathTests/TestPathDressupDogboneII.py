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

    def positionBegin(self):
        '''positionBegin() ... returns a Vector of the begin position'''
        return self.begin

    def positionEnd(self):
        '''positionEnd() ... returns a Vector of the end position'''
        return FreeCAD.Vector(self.x(begin.x), self.y(begin.y), self.z(begin.z))

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

class MoveStraight (Instruction):

    def anglesOfTangents(self):
        '''angleOfTangents() ... return a tuple with the tangent angles at begin and end position'''
        begin = self.xyBegin()
        end = self.xyEnd()
        if end == begin:
            return (0, 0)
        a = PathGeom.getAngle(end - begin)
        return (a, a)

class MoveArc (Instruction):

    def anglesOfTangents(self):
        '''angleOfTangents() ... return a tuple with the tangent angles at begin and end position'''
        begin = self.xyBegin()
        end = self.xyEnd()
        center = FreeCAD.Vector(self.begin.x + self.i(), self.begin.y + self.j(), 0)
        # calculate angle of the hypotenuse at begin and end
        s0 = PathGeom.getAngle(begin - center)
        s1 = PathGeom.getAngle(end - center)
        # the tangents are perpendicular to the hypotenuse with the sign determined by the
        # direction of the arc
        return (normalizeAngle(s0 + self.arcDirection()), normalizeAngle(s1 + self.arcDirection()))

class MoveArcCW (MoveArc):
    def arcDirection(self):
        return -PI/2

class MoveArcCCW (MoveArc):
    def arcDirection(self):
        return PI/2

def INSTR(s, c, x, y, z=None, i=None, j=None, k=None):
    if len(s) == 1:
        s = FreeCAD.Vector(s[0], 0, 0)
    elif len(s) == 2:
        s = FreeCAD.Vector(s[0], s[1], 0)
    else:
        s = FreeCAD.Vector(s[0], s[1], s[2])

    def upd(d, l, v):
        if not v is None:
            d[l] = v

    param = {'X': x, 'Y': y}
    upd(param, 'Z', z)
    upd(param, 'I', i)
    upd(param, 'J', j)
    upd(param, 'K', k)
    if c in CmdMoveStraight:
        return MoveStraight(s, c, param)
    if c in PathGeom.CmdMoveCW:
        return MoveArcCW(s, c, param)
    if c in PathGeom.CmdMoveCCW:
        return MoveArcCCW(s, c, param)
    return Instruction(s, c, param)

def G1(s, x, y):
    return INSTR(s, 'G1', x, y)

def G2(s, x, y, i, j):
    return INSTR(s, 'G2', x, y, None, i, j)
def G3(s, x, y, i, j):
    return INSTR(s, 'G3', x, y, None, i, j)

def POS(x, y, z=None):
    return FreeCAD.Vector(x, y, 0 if z is None else z)

def TAN(i):
    return i.anglesOfTangents()

class TestDressupDogboneII(PathTestBase):
    """Unit tests for the Dogbone dressup."""

    def assertTangents(self, t0, t1):
        """Assert that the two tangent angles are identical"""
        self.assertRoughly(t0[0], t1[0])
        self.assertRoughly(t0[1], t1[1])

    def test00(self):
        """Get tangents of moves."""

        self.assertTangents(TAN(G1((0, 0),  0,  0)), (0, 0)) # by declaration
        self.assertTangents(TAN(G1((0, 0),  1,  0)), (0, 0))
        self.assertTangents(TAN(G1((0, 0), -1,  0)), (PI, PI))
        self.assertTangents(TAN(G1((0, 0),  0,  1)), (PI/2,  PI/2))
        self.assertTangents(TAN(G1((0, 0),  0, -1)), (-PI/2,  -PI/2))
        self.assertTangents(TAN(G1((0, 0),  1,  1)), (PI/4,  PI/4))
        self.assertTangents(TAN(G1((0, 0), -1,  1)), (3*PI/4,  3*PI/4))
        self.assertTangents(TAN(G1((0, 0), -1, -1)), (-3*PI/4,  -3*PI/4))
        self.assertTangents(TAN(G1((0, 0),  1, -1)), (-PI/4,  -PI/4))

        self.assertTangents(TAN(G2((0, 0),  2,  0,  1,  0)), (PI/2, -PI/2))
        self.assertTangents(TAN(G2((0, 0),  2,  2,  1,  1)), (3*PI/4, -PI/4))
        self.assertTangents(TAN(G2((0, 0),  0, -2,  0, -1)), (0, -PI))

        self.assertTangents(TAN(G3((0, 0),  2,  0,  1,  0)), (-PI/2, PI/2))
        self.assertTangents(TAN(G3((0, 0),  2,  2,  1,  1)), (-PI/4, 3*PI/4))
        self.assertTangents(TAN(G3((0, 0),  0, -2,  0, -1)), (PI, 0))


