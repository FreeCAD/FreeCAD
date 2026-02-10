# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

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
import Path.Dressup.Utils as PathDressup
import PathScripts.PathUtils as PathUtils
import math

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())

PI = math.pi


def calc_length_adaptive(kink, angle, nominal_length, custom_length):
    Path.Log.track(kink, angle, nominal_length, custom_length)

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
        Path.Log.debug(
            f"depth={depth:4f}: kink={kink}, angle={180*angle/PI}, dist={dist:.4f}, da={180*da/PI} -> depth=0.0"
        )
        depth = 0
    else:
        height = dist * abs(math.sin(da))
        if height < nominal_length:
            depth = depth - math.sqrt(nominal_length * nominal_length - height * height)
        Path.Log.debug(
            f"{kink}: angle={180*angle/PI}, dist={dist:.4f}, da={180*da/PI}, depth={depth:.4f}"
        )

    return depth


def calc_length_nominal(kink, angle, nominal_length, custom_length):
    return nominal_length


def calc_length_custom(kink, angle, nominal_length, custom_length):
    return custom_length


class Style(object):
    """Style - enumeration class for the supported bone styles"""

    Dogbone = "Dogbone"
    Tbone_H = "T-bone horizontal"
    Tbone_V = "T-bone vertical"
    Tbone_L = "T-bone long edge"
    Tbone_S = "T-bone short edge"
    All = [Dogbone, Tbone_H, Tbone_V, Tbone_L, Tbone_S]

    Generator = {
        Dogbone: dogboneII.GeneratorDogbone,
        Tbone_H: dogboneII.GeneratorTBoneHorizontal,
        Tbone_V: dogboneII.GeneratorTBoneVertical,
        Tbone_S: dogboneII.GeneratorTBoneOnShort,
        Tbone_L: dogboneII.GeneratorTBoneOnLong,
    }


class Side(object):
    """Side - enumeration class for the side of the path to attach bones"""

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
    """Incision - enumeration class for the different depths of bone incision"""

    Fixed = "fixed"
    Adaptive = "adaptive"
    Custom = "custom"
    All = [Adaptive, Fixed, Custom]

    Calc = {
        Fixed: calc_length_nominal,
        Adaptive: calc_length_adaptive,
        Custom: calc_length_custom,
    }


def insertBone(obj, kink):
    """insertBone(kink, side) - return True if a bone should be inserted into the kink"""
    if not kink.isKink():
        Path.Log.debug("not a kink")
        return False

    if obj.Side == Side.Right and kink.goesRight():
        return False
    if obj.Side == Side.Left and kink.goesLeft():
        return False
    return True


class BoneState(object):
    def __init__(self, bone, nr, enabled=True):
        self.bone = bone
        self.bones = {nr: bone}
        self.enabled = enabled
        pos = bone.position()
        self.pos = FreeCAD.Vector(pos.x, pos.y, 0)

    def isEnabled(self):
        return self.enabled

    def addBone(self, bone, nr):
        self.bones[nr] = bone

    def position(self):
        return self.pos

    def boneTip(self):
        return self.bone.tip()

    def boneIDs(self):
        return sorted(self.bones)

    def zLevels(self):
        return sorted([bone.position().z for bone in self.bones.values()])

    def length(self):
        return self.bone.length


class Proxy(object):
    def __init__(self, obj, base):
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "The base path to dress up"),
        )
        obj.Base = base

        obj.addProperty(
            "App::PropertyEnumeration",
            "Side",
            "Dressup",
            QT_TRANSLATE_NOOP("App::Property", "The side of path to insert bones"),
        )
        obj.Side = Side.All
        if hasattr(base, "BoneBlacklist"):
            obj.Side = base.Side
        else:
            side = Side.Right
            if hasattr(obj.Base, "Side") and obj.Base.Side == "Inside":
                side = Side.Left
            if hasattr(obj.Base, "Direction") and obj.Base.Direction == "CCW":
                side = Side.oppositeOf(side)
            obj.Side = side

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
            QT_TRANSLATE_NOOP("App::Property", "The algorithm to determine the bone length"),
        )
        obj.Incision = Incision.All
        obj.Incision = Incision.Adaptive

        obj.addProperty(
            "App::PropertyLength",
            "Custom",
            "Dressup",
            QT_TRANSLATE_NOOP("App::Property", "Dressup length if incision is set to 'custom'"),
        )
        obj.Custom = 0.0

        obj.addProperty(
            "App::PropertyIntegerList",
            "BoneBlacklist",
            "Dressup",
            QT_TRANSLATE_NOOP("App::Property", "Bones that aren't dressed up"),
        )
        obj.BoneBlacklist = []

        obj.addProperty(
            "App::PropertyBool",
            "OnlyClosedProfiles",
            "Dressup",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Create bones only for outer closed profiles\nCan be useful for multi profile operations, e.g. Pocket with ZigZagOffset pattern",
            ),
        )
        self.onDocumentRestored(obj)

    def onDocumentRestored(self, obj):
        self.obj = obj
        obj.setEditorMode("BoneBlacklist", 2)  # hide

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, obj, prop):
        if prop == "Path" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def toolRadius(self, obj):
        return PathDressup.toolController(obj.Base).Tool.Diameter.Value / 2

    def createBone(self, obj, move0, move1):
        if move0.isRapid() and move1.isRapid():
            return None
        kink = dogboneII.Kink(move0, move1)
        Path.Log.debug(f"{obj.Label}.createBone({kink})")
        if insertBone(obj, kink):
            generator = Style.Generator[obj.Style]
            calc_length = Incision.Calc[obj.Incision]
            nominal = self.toolRadius(obj)
            custom = obj.Custom.Value
            return dogboneII.generate(kink, generator, calc_length, nominal, custom)
        return None

    # Get start index of closed loop in Area
    def findStartIndexClosedProfile(self, source, startAreaIndex, endAreaIndex):
        points = []
        points.append(source[endAreaIndex].positionEnd())
        points.append(source[endAreaIndex - 1].positionEnd())
        for i in range(endAreaIndex - 2, startAreaIndex - 1, -1):
            point = source[i].positionBegin()
            for j, p in enumerate(points):
                # compare this point with all points before
                if Path.Geom.pointsCoincide(point, p):
                    return i, endAreaIndex - j
            points.append(point)
        return None, None

    # Compare two boundboxes
    def isEquelBoundboxes(self, bb1, bb2):
        if not Path.Geom.isRoughly(bb1.XMin, bb2.XMin):
            return False
        if not Path.Geom.isRoughly(bb1.XMax, bb2.XMax):
            return False
        if not Path.Geom.isRoughly(bb1.YMin, bb2.YMin):
            return False
        if not Path.Geom.isRoughly(bb1.YMax, bb2.YMax):
            return False
        if not Path.Geom.isRoughly(bb1.ZMin, bb2.ZMin):
            return False
        if not Path.Geom.isRoughly(bb1.ZMax, bb2.ZMax):
            return False
        return True

    # Search inner profiles which should be excluded
    def getIndexInnerProfiles(self, source, indexList):
        boundboxList = []
        for i, areaIndexList in enumerate(indexList):
            minX, minY, maxX, maxY = None, None, None, None
            for index in areaIndexList:
                point = source[index].positionEnd()
                minX = point.x if minX is None or point.x < minX else minX
                maxX = point.x if maxX is None or point.x > maxX else maxX
                minY = point.y if minY is None or point.y < minY else minY
                maxY = point.y if maxY is None or point.y > maxY else maxY
            boundbox = FreeCAD.BoundBox(minX, minY, 0, maxX, maxY, 0)
            boundboxList.append(boundbox)

        excludeList = []
        for i, boundbox in enumerate(boundboxList):
            for bb in boundboxList:
                if not self.isEquelBoundboxes(boundbox, bb) and bb.isInside(boundbox):
                    excludeList.append(i)
                    break
        return excludeList

    # Check command
    def isCuttingMove(self, instr):
        result = instr.isMove() and not instr.isRapid() and not instr.isPlunge()
        return result

    def getIndexOuterClosedProfiles(self, source):
        closedProfilesIndex = []
        startArea = None
        endArea = None
        for i, instr in enumerate(source):
            if (
                startArea is None
                and endArea is None
                and self.isCuttingMove(source[i])
                and (i == 0 or not self.isCuttingMove(source[i - 1]))
            ):
                # start mill index of the area
                startArea = i

            if (
                startArea is not None
                and endArea is None
                and (i == len(source) - 1 or not self.isCuttingMove(source[i + 1]))
            ):
                # end mill index of the area
                endArea = i

            if startArea and endArea:
                p1 = source[startArea].positionBegin()
                p2 = source[endArea].positionEnd()
                if Path.Geom.pointsCoincide(p1, p2):
                    # simple case
                    # one closed profile in the area
                    closedProfilesIndex.append(list(range(startArea, endArea + 1)))
                else:
                    # points is not coincide
                    # try to find last closed profile in the area
                    startIndex, endIndex = self.findStartIndexClosedProfile(
                        source, startArea, endArea
                    )
                    if startIndex is not None and endIndex is not None:
                        closedProfilesIndex.append(list(range(startIndex, endIndex + 1)))

                startArea = None
                endArea = None

        excludeList = self.getIndexInnerProfiles(source, closedProfilesIndex)

        outerClosedProfilesIndex = []
        for i, area in enumerate(closedProfilesIndex):
            if i not in excludeList:
                for j in area:
                    outerClosedProfilesIndex.append(j)

        return outerClosedProfilesIndex

    def execute(self, obj):
        Path.Log.track(obj.Label)
        maneuver = PathLanguage.Maneuver()
        bones = []
        lastMove = None
        moveAfterPlunge = None
        dressingUpDogbone = hasattr(obj.Base, "BoneBlacklist")

        if obj.Base and obj.Base.Path and obj.Base.Path.Commands:
            source = PathLanguage.Maneuver.FromPath(PathUtils.getPathWithPlacement(obj.Base)).instr

            # get indexes of outer closed profile in each multi work area
            if hasattr(obj, "OnlyClosedProfiles") and obj.OnlyClosedProfiles:
                closedProfilesIndex = self.getIndexOuterClosedProfiles(source)
            else:
                closedProfilesIndex = None

            for index, instr in enumerate(source):
                # Path.Log.debug(f"instr: {instr}")
                if instr.isMove():
                    thisMove = instr
                    bone = None
                    if thisMove.isPlunge() or (
                        closedProfilesIndex is not None and index not in closedProfilesIndex
                    ):
                        if lastMove and moveAfterPlunge and lastMove.leadsInto(moveAfterPlunge):
                            bone = self.createBone(obj, lastMove, moveAfterPlunge)
                        lastMove = None
                        moveAfterPlunge = None
                    else:
                        if moveAfterPlunge is None:
                            moveAfterPlunge = thisMove
                        if lastMove:
                            bone = self.createBone(obj, lastMove, thisMove)
                        lastMove = thisMove
                    if bone:
                        enabled = len(bones) not in obj.BoneBlacklist
                        if enabled and not (
                            dressingUpDogbone and obj.Base.Proxy.includesBoneAt(bone.position())
                        ):
                            maneuver.addInstructions(bone.instr)
                        else:
                            Path.Log.debug(f"{bone.kink} disabled {enabled}")
                        bones.append(bone)
                    maneuver.addInstruction(thisMove)
                else:
                    # non-move instructions get added verbatim
                    maneuver.addInstruction(instr)

        else:
            Path.Log.info(f"No Path found to dress up in op {obj.Base}")
        self.maneuver = maneuver
        self.bones = bones
        self.boneTips = None
        obj.Path = maneuver.toPath()

    def boneStates(self, obj):
        state = {}
        if hasattr(self, "bones"):
            for nr, bone in enumerate(self.bones):
                pos = bone.position()
                loc = f"({pos.x:.4f}, {pos.y:.4f})"
                if state.get(loc, None):
                    state[loc].addBone(bone, nr)
                else:
                    state[loc] = BoneState(bone, nr)
                if nr in obj.BoneBlacklist:
                    state[loc].enabled = False
        return state.values()

    def includesBoneAt(self, pos):
        if hasattr(self, "bones"):
            for nr, bone in enumerate(self.bones):
                if Path.Geom.pointsCoincide(bone.position(), pos):
                    return nr not in self.obj.BoneBlacklist
        return False


def Create(base, name="DressupDogbone"):
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    pxy = Proxy(obj, base)

    obj.Proxy = pxy

    return obj
