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

class Instruction (object):
    '''An Instruction is pure python replacement of Path.Command which also tracks its start position.'''

    def __init__(self, start, cmd, param=None):
        self.start = start
        if type(cmd) == Path.Command:
            self.cmd = Path.Name
            self.param = Path.Parameters
        else:
            self.cmd = cmd
            if param is None:
                self.param = {}
            else:
                self.param = param

    def positionStart(self):
        '''positionStart() ... returns a Vector of the start position'''
        return self.start

    def positionEnd(self):
        '''positionEnd() ... returns a Vector of the end position'''
        return FreeCAD.Vector(self.x(start.x), self.y(start.y), self.z(start.z))

    def x(self, default=0):
        return self.param.get('X', default)

    def y(self, default=0):
        return self.param.get('Y', default)

    def z(self, default=0):
        return self.param.get('Z', default)

class MoveStraight (Instruction):

    def anglesOfTangents(self):
        '''angleOfTangents() ... return a tuple with the tangent angle at start and end position'''
        start = FreeCAD.Vector(self.start.x, self.start.y, 0)
        end  = FreeCAD.Vector(self.x(self.start.x), self.y(self.start.y), 0)
        if end == start:
            return (0, 0)
        a = PathGeom.getAngle(end - start)
        return (a, a)

def INSTR(s, c, x, y, z=None, i=None, j=None, k=None):
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
    return Instruction(s, c, param)

def POS(x, y, z=None):
    return FreeCAD.Vector(x, y, 0 if z is None else z)

def TAN(p, c, x, y):
    if len(p) == 1:
        p = FreeCAD.Vector(p[0], 0, 0)
    elif len(p) == 2:
        p = FreeCAD.Vector(p[0], p[1], 0)
    else:
        p = FreeCAD.Vector(p[0], p[1], p[2])
    return INSTR(p, c, x, y).anglesOfTangents()

class TestDressupDogboneII(PathTestBase):
    """Unit tests for the Dogbone dressup."""

    def assertTangents(self, t0, t1):
        """Assert that the two tangent angles are identical"""
        self.assertRoughly(t0[0], t1[0])
        self.assertRoughly(t0[1], t1[1])

    def test00(self):
        """Get tangents of moves."""

        self.assertTangents(TAN((0, 0), 'G0',  0,  0), (0, 0)) # by declaration
        self.assertTangents(TAN((0, 0), 'G0',  1,  0), (0, 0))
        self.assertTangents(TAN((0, 0), 'G0', -1,  0), (PI, PI))
        self.assertTangents(TAN((0, 0), 'G0',  0,  1), (PI/2,  PI/2))
        self.assertTangents(TAN((0, 0), 'G0',  0, -1), (-PI/2,  -PI/2))
        self.assertTangents(TAN((0, 0), 'G0',  1,  1), (PI/4,  PI/4))
        self.assertTangents(TAN((0, 0), 'G0', -1,  1), (3*PI/4,  3*PI/4))
        self.assertTangents(TAN((0, 0), 'G0', -1, -1), (-3*PI/4,  -3*PI/4))
        self.assertTangents(TAN((0, 0), 'G0',  1, -1), (-PI/4,  -PI/4))

