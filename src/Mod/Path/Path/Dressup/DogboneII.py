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

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import Path.Base.Generator.dogboneII as dogboneII
import Path.Base.Language as PathLanguage
import math

PI = math.pi

def calc_length_adaptive(kink, angle, nominal_length, custom_length):
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

    return depth

def calc_length_nominal(kink, angle, nominal_length, custom_length):
    return nominal_length

def calc_length_custom(kink, angle, nominal_length, custom_length):
    return custom_length


class Style(object):
    '''Style - enumeration class for the supported bone styles'''

    Dogbone = "Dogbone"
    Tbone_H = "T-bone horizontal"
    Tbone_V = "T-bone vertical"
    Tbone_L = "T-bone long edge"
    Tbone_S = "T-bone short edge"
    All = [Dogbone, Tbone_H, Tbone_V, Tbone_L, Tbone_S]

    #Generator = {
    #    Dogbone : dogboneII.generate_dogbone,
    #    Tbone_H : dogboneII.generate_tbone_horizontal,
    #    Tbone_V : dogboneII.generate_tbone_vertical,
    #    Tbone_S : dogboneII.generate_tbone_on_short,
    #    Tbone_L : dogboneII.generate_tbone_on_long,
    #    }

class Side(object):
    '''Side - enumeration class for the side of the path to attach bones'''

    Left = "Left"
    Right = "Right"
    All = [Left, Right]

    @classmethod
    def oppositeOf(cls, side):
        if side == cls.Left:
            return cls.Right
        if side == cls.Right:
            return cls.Left
        return None


class Incision(object):
    '''Incision - enumeration class for the different depths of bone incision'''

    Fixed = "fixed"
    Adaptive = "adaptive"
    Custom = "custom"
    All = [Adaptive, Fixed, Custom]

    Calc = {
        Fixed : calc_length_nominal,
        Adaptive : calc_length_adaptive,
        Custom : calc_length_custom,
        }


def insertBone(kink, side):
    '''insertBone(kink, side) - return True if a bone should be inserted into the kink'''
    if not kink.isKink():
        return False

    if obj.Side == Side.Right and kink.goesRight():
        return False
    if obj.Side == Side.Left and kink.goesLeft():
        return False
    return True

class Proxy(object):
    def __init__(self, obj, base):
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "The base path to dress up"),
        )

        obj.addProperty(
            "App::PropertyEnumeration",
            "Side",
            "Dressup",
            QT_TRANSLATE_NOOP("App::Property", "The side of path to insert bones"),
        )
        obj.Side = Side.All
        obj.Side = Side.Right

        obj.addProperty(
            "App::PropertyEnumeration",
            "Style",
            "Dressup",
            QT_TRANSLATE_NOOP("App::Property", "The style of bones"),
        )
        obj.Style = Style.All
        obj.Style = Style.Dogbone

        obj.addProperty(
            "App::PropertyEnumeration",
            "Incision",
            "Dressup",
            QT_TRANSLATE_NOOP(
                "App::Property", "The algorithm to determine the bone length"
            ),
        )
        obj.Incision = Incision.All
        obj.Incision = Incision.Adaptive

        obj.addProperty(
            "App::PropertyFloat",
            "Custom",
            "Dressup",
            QT_TRANSLATE_NOOP("App::Property", "Dressup length if Incision == Incision.Custom"),
        )
        obj.Custom = 0.0

        obj.addProperty(
            "App::PropertyIntegerList",
            "BoneBlacklist",
            "Dressup",
            QT_TRANSLATE_NOOP("App::Property", "Bones that aren't dressed up"),
        )
        obj.BoneBlacklist = []

        self.onDocumentRestored(obj)

    def onDocumentRestored(self, obj):
        self.obj = obj
        obj.setEditorMode("BoneBlacklist", 2)  # hide

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def boneMoves(self, obj, move0, move1): 
        moves = []
        kink = dogboneII.Kink(lastMove, thisMove)
        if insertBone(kink, obj.Side):
            pass

        moves.append(move1)
        return moves

    def execute(self, obj):
        maneuver = PathLanguage.Maneuver()
        lastMove = None
        moveAfterPlunge = None
        if obj.Base and obj.Base.Path and obj.Base.Path.Commands:
            for i, instr in enumerate(PathLanguage.Maneuver.FromPath(obj.Base.Path)):
                if instr.isMove():
                    thisMove = instr
                    if thisMove.isPlunge():
                        if lastMove and moveAfterPlunge:
                            pass
                        lastMove = None
                        moveAfterPlunge = None
                    else:
                        if moveAfterPlunge is None:
                            moveAfterPlunge = thisMove
                        if lastMove:
                            pass
                        lastMove = thisMove
                    maneuver.addInstruction(thisMove)
                else:
                    # none move instructions get added verbatim
                    maneuver.addInstruction(instr)

        obj.Path = maneuver.toPath()

