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
import PathScripts.Path.Log as Path.Log
import math

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
            return PathGeom.normalizeAngle((self.t0 + self.t1 + math.pi) / 2)
        return PathGeom.normalizeAngle((self.t0 + self.t1 - math.pi) / 2)

    def position(self):
        '''position() ... position of the edge's intersection'''
        return self.m0.positionEnd()

    def x(self):
        return self.position().x

    def y(self):
        return self.position().y

    def __repr__(self):
        return f"({self.x():.4f}, {self.y():.4f})[t0={180*self.t0/math.pi:.2f}, t1={180*self.t1/math.pi:.2f}, deflection={180*self.deflection()/math.pi:.2f}, normAngle={180*self.normAngle()/math.pi:.2f}]"

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

def kink_to_path(kink, g0=False):
    return Path.Path([PathLanguage.instruction_to_command(instr) for instr in [kink.m0, kink.m1]])

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
        cmds.append(PathLanguage.instruction_to_command(instr))
    return Path.Path(cmds)

