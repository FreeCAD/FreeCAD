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

PI = math.pi

def calc_adaptive_length(kink, angle, nominal_length, debugMode=False):
    if Path.Geom.isRoughly(abs(kink.deflection()), 0):
        return 0

    # If the kink poses a 180deg turn the adaptive length is undefined. Mathematically
    # it's infinite but that is not practical.
    # We define the adaptive length to be the nominal length for this case.
    if Path.Geom.isRoughly(abs(kink.deflection()), PI):
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

    da = Path.Geom.normalizeAngle(kink.normAngle() - angle)
    depth = dist * math.cos(da)
    if depth < 0:
        Path.Log.debug(f"depth={depth:4f}: kink={kink}, angle={180*angle/PI}, dist={dist:.4f}, da={180*da/PI} -> depth=0.0")
        depth = 0
    else:
        height = dist * abs(math.sin(da))
        if height < nominal_length:
            depth = depth - math.sqrt(nominal_length * nominal_length - height * height)
        Path.Log.debug(f"{kink}: angle={180*angle/PI}, dist={dist:.4f}, da={180*da/PI}, depth={depth:.4f}")

    if debugMode and FreeCAD.GuiUp:
        import Part
        FreeCAD.ActiveDocument.addObject("App::DocumentObjectGroup","Group")
        group = FreeCAD.ActiveDocument.ActiveObject
        bone = dogboneII.generate_dogbone(kink, dist)
        Path.show(PathDressupDogboneII.bone_to_path(bone, True), 'adaptive')
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

