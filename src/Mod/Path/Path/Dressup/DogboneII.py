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
import math

def createKinks(maneuver):
    k = []
    moves = maneuver.getMoves()
    if moves:
        move0 = moves[0]
        prev = move0
        for m in moves[1:]:
            k.append(dogboneII.Kink(prev, m))
            prev = m
        if Path.Geom.pointsCoincide(move0.positionBegin(), prev.positionEnd()):
            k.append(dogboneII.Kink(prev, move0))
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


def kink_to_path(kink, g0=False):
    return Path.Path([PathLanguage.instruction_to_command(instr) for instr in [kink.m0, kink.m1]])

def bone_to_path(bone, g0=False):
    kink = bone.kink
    cmds = []
    if g0 and not Path.Geom.pointsCoincide(kink.m0.positionBegin(), FreeCAD.Vector(0, 0, 0)):
        pos = kink.m0.positionBegin()
        param = {}
        if not Path.Geom.isRoughly(pos.x, 0):
            param['X'] = pos.x
        if not Path.Geom.isRoughly(pos.y, 0):
            param['Y'] = pos.y
        cmds.append(Path.Command('G0', param))
    for instr in [kink.m0, bone.instr[0], bone.instr[1], kink.m1]:
        cmds.append(PathLanguage.instruction_to_command(instr))
    return Path.Path(cmds)

