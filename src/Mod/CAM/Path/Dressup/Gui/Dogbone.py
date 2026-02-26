# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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


from PySide import QtCore
from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import Path.Dressup.Utils as PathDressup
import PathScripts.PathUtils as PathUtils
import math
from pivy import coin

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

DraftGeomUtils = LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")
Part = LazyLoader("Part", globals(), "Part")


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.ERROR, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


movecommands = Path.Geom.CmdMoveStraight + Path.Geom.CmdMoveRapid + Path.Geom.CmdMoveArc


def debugMarker(vector, label, color=None, radius=0.5):
    if Path.Log.getLevel(Path.Log.thisModule()) == Path.Log.Level.DEBUG:
        obj = FreeCAD.ActiveDocument.addObject("Part::Sphere", label)
        obj.Label = label
        obj.Radius = radius
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0))
        if color:
            obj.ViewObject.ShapeColor = color


def debugCircle(vector, r, label, color=None):
    if Path.Log.getLevel(Path.Log.thisModule()) == Path.Log.Level.DEBUG:
        obj = FreeCAD.ActiveDocument.addObject("Part::Cylinder", label)
        obj.Label = label
        obj.Radius = r
        obj.Height = 1
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0))
        obj.ViewObject.Transparency = 90
        if color:
            obj.ViewObject.ShapeColor = color


def addAngle(a1, a2):
    a = a1 + a2
    while a <= -math.pi:
        a += 2 * math.pi
    while a > math.pi:
        a -= 2 * math.pi
    return a


def anglesAreParallel(a1, a2):
    an1 = addAngle(a1, 0)
    an2 = addAngle(a2, 0)
    if an1 == an2:
        return True
    if an1 == addAngle(an2, math.pi):
        return True
    return False


def getAngle(v):
    a = v.getAngle(FreeCAD.Vector(1, 0, 0))
    if v.y < 0:
        return -a
    return a


def pointFromCommand(cmd, pt, X="X", Y="Y", Z="Z"):
    x = cmd.Parameters.get(X, pt.x)
    y = cmd.Parameters.get(Y, pt.y)
    z = cmd.Parameters.get(Z, pt.z)
    return FreeCAD.Vector(x, y, z)


def edgesForCommands(cmds, startPt):
    edges = []
    lastPt = startPt
    for cmd in cmds:
        if cmd.Name in movecommands:
            pt = pointFromCommand(cmd, lastPt)
            if cmd.Name in Path.Geom.CmdMoveStraight:
                edges.append(Part.Edge(Part.LineSegment(lastPt, pt)))
            elif cmd.Name in Path.Geom.CmdMoveArc:
                center = lastPt + pointFromCommand(cmd, FreeCAD.Vector(0, 0, 0), "I", "J", "K")
                A = lastPt - center
                B = pt - center
                d = -B.x * A.y + B.y * A.x

                if d == 0:
                    # we're dealing with half a circle here
                    angle = getAngle(A) + math.pi / 2
                    if cmd.Name in Path.Geom.CmdMoveCW:
                        angle -= math.pi
                else:
                    C = A + B
                    angle = getAngle(C)

                R = (lastPt - center).Length
                ptm = center + FreeCAD.Vector(math.cos(angle), math.sin(angle), 0) * R

                edges.append(Part.Edge(Part.Arc(lastPt, ptm, pt)))
            lastPt = pt
    return edges


class Style(object):

    Dogbone = "Dogbone"
    Tbone_H = "T-bone horizontal"
    Tbone_V = "T-bone vertical"
    Tbone_L = "T-bone long edge"
    Tbone_S = "T-bone short edge"
    All = [Dogbone, Tbone_H, Tbone_V, Tbone_L, Tbone_S]


class Side(object):

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

    Fixed = "fixed"
    Adaptive = "adaptive"
    Custom = "custom"
    All = [Adaptive, Fixed, Custom]


class Smooth(object):

    Neither = 0
    In = 1
    Out = 2
    InAndOut = In | Out


# Chord
# A class to represent the start and end point of a path command. If the underlying
# Command is a rotate command the receiver does represent a chord in the geometric
# sense of the word. If the underlying command is a straight move then the receiver
# represents the actual move.
# This implementation really only deals with paths in the XY plane. Z is assumed to
# be constant in all calculated results.
# Instances of Chord are generally considered immutable and all movement member
# functions return new instances.
class Chord(object):
    def __init__(self, start=None, end=None):
        if not start:
            start = FreeCAD.Vector()
        if not end:
            end = FreeCAD.Vector()
        self.Start = start
        self.End = end

    def __str__(self):
        return "Chord([%g, %g, %g] -> [%g, %g, %g])" % (
            self.Start.x,
            self.Start.y,
            self.Start.z,
            self.End.x,
            self.End.y,
            self.End.z,
        )

    def moveTo(self, newEnd):
        return Chord(self.End, newEnd)

    def moveToParameters(self, params):
        x = params.get("X", self.End.x)
        y = params.get("Y", self.End.y)
        z = params.get("Z", self.End.z)
        return self.moveTo(FreeCAD.Vector(x, y, z))

    def moveBy(self, x, y, z):
        return self.moveTo(self.End + FreeCAD.Vector(x, y, z))

    def move(self, distance, angle):
        dx = distance * math.cos(angle)
        dy = distance * math.sin(angle)
        return self.moveBy(dx, dy, 0)

    def asVector(self):
        return self.End - self.Start

    def asDirection(self):
        return self.asVector().normalize()

    def asLine(self):
        return Part.LineSegment(self.Start, self.End)

    def asEdge(self):
        return Part.Edge(self.asLine())

    def getLength(self):
        return self.asVector().Length

    def getDirectionOfVector(self, B):
        A = self.asDirection()
        # if the 2 vectors are identical, they head in the same direction
        Path.Log.debug("   {}.getDirectionOfVector({})".format(A, B))
        if Path.Geom.pointsCoincide(A, B):
            return "Straight"
        d = -A.x * B.y + A.y * B.x
        if d < 0:
            return Side.Left
        if d > 0:
            return Side.Right
        # at this point the only direction left is backwards
        return "Back"

    def getDirectionOf(self, chordOrVector):
        if type(chordOrVector) is Chord:
            return self.getDirectionOfVector(chordOrVector.asDirection())
        return self.getDirectionOfVector(chordOrVector.normalize())

    def getAngleOfVector(self, ref):
        angle = self.asVector().getAngle(ref)
        # unfortunately they never figure out the sign :(
        # positive angles go up, so when the reference vector is left
        # then the receiver must go down
        if self.getDirectionOfVector(ref) == Side.Left:
            return -angle
        return angle

    def getAngle(self, refChordOrVector):
        if type(refChordOrVector) is Chord:
            return self.getAngleOfVector(refChordOrVector.asDirection())
        return self.getAngleOfVector(refChordOrVector.normalize())

    def getAngleXY(self):
        return self.getAngle(FreeCAD.Vector(1, 0, 0))

    def commandParams(self, f):
        params = {"X": self.End.x, "Y": self.End.y, "Z": self.End.z}
        if f:
            params["F"] = f
        return params

    def g1Command(self, f):
        return Path.Command("G1", self.commandParams(f))

    def arcCommand(self, cmd, center, f):
        params = self.commandParams(f)
        d = center - self.Start
        params["I"] = d.x
        params["J"] = d.y
        params["K"] = 0
        return Path.Command(cmd, params)

    def g2Command(self, center, f):
        return self.arcCommand("G2", center, f)

    def g3Command(self, center, f):
        return self.arcCommand("G3", center, f)

    def isAPlungeMove(self):
        return not Path.Geom.isRoughly(self.End.z, self.Start.z)

    def isANoopMove(self):
        Path.Log.debug(
            "{}.isANoopMove(): {}".format(self, Path.Geom.pointsCoincide(self.Start, self.End))
        )
        return Path.Geom.pointsCoincide(self.Start, self.End)

    def foldsBackOrTurns(self, chord, side):
        direction = chord.getDirectionOf(self)
        Path.Log.info("  - direction = %s/%s" % (direction, side))
        return direction == "Back" or direction == side

    def connectsTo(self, chord):
        return Path.Geom.pointsCoincide(self.End, chord.Start)


class Bone(object):
    def __init__(self, boneId, obj, lastCommand, inChord, outChord, smooth, F):
        self.obj = obj
        self.boneId = boneId
        self.lastCommand = lastCommand
        self.inChord = inChord
        self.outChord = outChord
        self.smooth = smooth
        self.smooth = Smooth.Neither
        self.F = F

        # initialized later
        self.cDist = None
        self.cAngle = None
        self.tAngle = None
        self.cPt = None

    def angle(self):
        if self.cAngle is None:
            baseAngle = self.inChord.getAngleXY()
            turnAngle = self.outChord.getAngle(self.inChord)
            theta = addAngle(baseAngle, (turnAngle - math.pi) / 2)
            if self.obj.Side == Side.Left:
                theta = addAngle(theta, math.pi)
            self.tAngle = turnAngle
            self.cAngle = theta
        return self.cAngle

    def distance(self, toolRadius):
        if self.cDist is None:
            self.angle()  # make sure the angles are initialized
            self.cDist = toolRadius / math.cos(self.tAngle / 2)
        return self.cDist

    def corner(self, toolRadius):
        if self.cPt is None:
            self.cPt = self.inChord.move(self.distance(toolRadius), self.angle()).End
        return self.cPt

    def location(self):
        return (self.inChord.End.x, self.inChord.End.y)

    def locationZ(self):
        return (self.inChord.End.x, self.inChord.End.y, self.inChord.End.z)

    def adaptiveLength(self, boneAngle, toolRadius):
        theta = self.angle()
        distance = self.distance(toolRadius)
        # there is something weird happening if the boneAngle came from a horizontal/vertical t-bone
        # for some reason pi/2 is not equal to pi/2
        if math.fabs(theta - boneAngle) < 0.00001:
            # moving directly towards the corner
            Path.Log.debug("adaptive - on target: %.2f - %.2f" % (distance, toolRadius))
            return distance - toolRadius
        Path.Log.debug(
            "adaptive - angles: corner=%.2f  bone=%.2f diff=%.12f"
            % (theta / math.pi, boneAngle / math.pi, theta - boneAngle)
        )

        # The bones root and end point form a triangle with the intersection of the tool path
        # with the toolRadius circle around the bone end point.
        # In case the math looks questionable, look for "triangle ssa"
        # c = distance
        # b = self.toolRadius
        # beta = fabs(boneAngle - theta)
        beta = math.fabs(addAngle(boneAngle, -theta))
        D = (distance / toolRadius) * math.sin(beta)
        if D > 1:  # no intersection
            Path.Log.debug("adaptive - no intersection - no bone")
            return 0
        gamma = math.asin(D)
        alpha = math.pi - beta - gamma
        if Path.Geom.isRoughly(0.0, math.sin(beta)):
            # it is not a good idea to divide by 0
            length = 0.0
        else:
            length = toolRadius * math.sin(alpha) / math.sin(beta)
            if D < 1 and toolRadius < distance:  # there exists a second solution
                beta2 = beta
                gamma2 = math.pi - gamma
                alpha2 = math.pi - beta2 - gamma2
                length2 = toolRadius * math.sin(alpha2) / math.sin(beta2)
                length = min(length, length2)

        Path.Log.debug(
            "adaptive corner=%.2f * %.2f˚ -> bone=%.2f * %.2f˚"
            % (distance, theta, length, boneAngle)
        )
        return length


class ObjectDressup(object):
    def __init__(self, obj, base):
        # Tool Properties
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "The base path to modify"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "Side",
            "Dressup",
            QT_TRANSLATE_NOOP("App::Property", "The side of path to insert bones"),
        )
        obj.Side = [Side.Left, Side.Right]
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
            "App::PropertyIntegerList",
            "BoneBlacklist",
            "Dressup",
            QT_TRANSLATE_NOOP("App::Property", "Bones that are not dressed up"),
        )
        obj.BoneBlacklist = []
        obj.setEditorMode("BoneBlacklist", 2)  # hide this one
        obj.addProperty(
            "App::PropertyEnumeration",
            "Incision",
            "Dressup",
            QT_TRANSLATE_NOOP("App::Property", "The algorithm to determine the bone length"),
        )
        obj.Incision = Incision.All
        obj.Incision = Incision.Adaptive
        obj.addProperty(
            "App::PropertyFloat",
            "Custom",
            "Dressup",
            QT_TRANSLATE_NOOP("App::Property", "Dressup length if incision is set to 'custom'"),
        )
        obj.Custom = 0.0
        obj.Proxy = self
        obj.Base = base

        # initialized later
        self.boneShapes = None
        self.toolRadius = 0
        self.dbg = None
        self.locationBlacklist = None
        self.shapes = None
        self.boneId = None
        self.bones = None

    def onDocumentRestored(self, obj):
        obj.setEditorMode("BoneBlacklist", 2)  # hide this one

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def theOtherSideOf(self, side):
        if side == Side.Left:
            return Side.Right
        return Side.Left

    # Answer true if a dogbone could be on either end of the chord, given its command
    def canAttachDogbone(self, cmd, chord):
        return (
            cmd.Name in Path.Geom.CmdMoveStraight
            and not chord.isAPlungeMove()
            and not chord.isANoopMove()
        )

    def shouldInsertDogbone(self, obj, inChord, outChord):
        return outChord.foldsBackOrTurns(inChord, self.theOtherSideOf(obj.Side))

    def findPivotIntersection(self, pivot, pivotEdge, edge, refPt, d, color):
        Path.Log.track(
            "(%.2f, %.2f)^%.2f  - [(%.2f, %.2f), (%.2f, %.2f)]"
            % (
                pivotEdge.Curve.Center.x,
                pivotEdge.Curve.Center.y,
                pivotEdge.Curve.Radius,
                edge.Vertexes[0].Point.x,
                edge.Vertexes[0].Point.y,
                edge.Vertexes[1].Point.x,
                edge.Vertexes[1].Point.y,
            )
        )
        ppt = None
        pptDistance = 0
        for pt in DraftGeomUtils.findIntersection(edge, pivotEdge, dts=False):
            # debugMarker(pt, "pti.%d-%s.in" % (self.boneId, d), color, 0.2)
            distance = (pt - refPt).Length
            Path.Log.debug("        -->  (%.2f, %.2f): %.2f" % (pt.x, pt.y, distance))
            if not ppt or pptDistance < distance:
                ppt = pt
                pptDistance = distance
        if not ppt:
            tangent = DraftGeomUtils.findDistance(pivot, edge)
            if tangent:
                Path.Log.debug("Taking tangent as intersect %s" % tangent)
                ppt = pivot + tangent
            else:
                Path.Log.debug("Taking chord start as intersect %s" % edge.Vertexes[0].Point)
                ppt = edge.Vertexes[0].Point
            # debugMarker(ppt, "ptt.%d-%s.in" % (self.boneId, d), color, 0.2)
            Path.Log.debug("        -->  (%.2f, %.2f)" % (ppt.x, ppt.y))
        return ppt

    def pointIsOnEdge(self, point, edge):
        param = edge.Curve.parameter(point)
        return edge.FirstParameter <= param <= edge.LastParameter

    def smoothChordCommands(self, bone, inChord, outChord, edge, wire, corner, smooth, color=None):
        if smooth == 0:
            Path.Log.info(" No smoothing requested")
            return [bone.lastCommand, outChord.g1Command(bone.F)]

        d = "in"
        refPoint = inChord.Start
        if smooth == Smooth.Out:
            d = "out"
            refPoint = outChord.End

        if DraftGeomUtils.areColinear(inChord.asEdge(), outChord.asEdge()):
            Path.Log.info(" straight edge %s" % d)
            return [outChord.g1Command(bone.F)]

        pivot = None
        pivotDistance = 0

        Path.Log.info(
            "smooth:  (%.2f, %.2f)-(%.2f, %.2f)"
            % (
                edge.Vertexes[0].Point.x,
                edge.Vertexes[0].Point.y,
                edge.Vertexes[1].Point.x,
                edge.Vertexes[1].Point.y,
            )
        )
        for e in wire.Edges:
            self.dbg.append(e)
            if type(e.Curve) == Part.LineSegment or type(e.Curve) == Part.Line:
                Path.Log.debug(
                    "         (%.2f, %.2f)-(%.2f, %.2f)"
                    % (
                        e.Vertexes[0].Point.x,
                        e.Vertexes[0].Point.y,
                        e.Vertexes[1].Point.x,
                        e.Vertexes[1].Point.y,
                    )
                )
            else:
                Path.Log.debug(
                    "         (%.2f, %.2f)^%.2f"
                    % (e.Curve.Center.x, e.Curve.Center.y, e.Curve.Radius)
                )
            for pt in DraftGeomUtils.findIntersection(edge, e, True, findAll=True):
                if not Path.Geom.pointsCoincide(pt, corner) and self.pointIsOnEdge(pt, e):
                    # debugMarker(pt, "candidate-%d-%s" % (self.boneId, d), color, 0.05)
                    Path.Log.debug("         -> candidate")
                    distance = (pt - refPoint).Length
                    if not pivot or pivotDistance > distance:
                        pivot = pt
                        pivotDistance = distance
                else:
                    Path.Log.debug("         -> corner intersect")

        if pivot:
            # debugCircle(pivot, self.toolRadius, "pivot.%d-%s" % (self.boneId, d), color)

            pivotEdge = Part.Edge(Part.Circle(pivot, FreeCAD.Vector(0, 0, 1), self.toolRadius))
            t1 = self.findPivotIntersection(
                pivot, pivotEdge, inChord.asEdge(), inChord.End, d, color
            )
            t2 = self.findPivotIntersection(
                pivot, pivotEdge, outChord.asEdge(), inChord.End, d, color
            )

            commands = []
            if not Path.Geom.pointsCoincide(t1, inChord.Start):
                Path.Log.debug("  add lead in")
                commands.append(Chord(inChord.Start, t1).g1Command(bone.F))
            if bone.obj.Side == Side.Left:
                Path.Log.debug("  add g3 command")
                commands.append(Chord(t1, t2).g3Command(pivot, bone.F))
            else:
                Path.Log.debug(
                    "  add g2 command center=(%.2f, %.2f) -> from (%2f, %.2f) to (%.2f, %.2f"
                    % (pivot.x, pivot.y, t1.x, t1.y, t2.x, t2.y)
                )
                commands.append(Chord(t1, t2).g2Command(pivot, bone.F))
            if not Path.Geom.pointsCoincide(t2, outChord.End):
                Path.Log.debug("  add lead out")
                commands.append(Chord(t2, outChord.End).g1Command(bone.F))

            # debugMarker(pivot, "pivot.%d-%s"     % (self.boneId, d), color, 0.2)
            # debugMarker(t1,    "pivot.%d-%s.in"  % (self.boneId, d), color, 0.1)
            # debugMarker(t2,    "pivot.%d-%s.out" % (self.boneId, d), color, 0.1)

            return commands

        Path.Log.info(" no pivot found - straight command")
        return [inChord.g1Command(bone.F), outChord.g1Command(bone.F)]

    def inOutBoneCommands(self, bone, boneAngle, fixedLength):
        corner = bone.corner(self.toolRadius)

        bone.tip = bone.inChord.End  # in case there is no bone

        Path.Log.debug("corner = (%.2f, %.2f)" % (corner.x, corner.y))
        # debugMarker(corner, 'corner', (1., 0., 1.), self.toolRadius)

        length = fixedLength
        if bone.obj.Incision == Incision.Custom:
            length = bone.obj.Custom
        if bone.obj.Incision == Incision.Adaptive:
            length = bone.adaptiveLength(boneAngle, self.toolRadius)

        if length == 0:
            Path.Log.info("no bone after all ..")
            return [bone.lastCommand, bone.outChord.g1Command(bone.F)]

        # track length for marker visuals
        self.length = max(self.length, length)

        boneInChord = bone.inChord.move(length, boneAngle)
        boneOutChord = boneInChord.moveTo(bone.outChord.Start)

        # debugCircle(boneInChord.Start, self.toolRadius, 'boneStart')
        # debugCircle(boneInChord.End, self.toolRadius, 'boneEnd')

        bone.tip = boneInChord.End

        if bone.smooth == 0:
            return [
                bone.lastCommand,
                boneInChord.g1Command(bone.F),
                boneOutChord.g1Command(bone.F),
                bone.outChord.g1Command(bone.F),
            ]

        # reconstruct the corner and convert to an edge
        offset = corner - bone.inChord.End
        iChord = Chord(bone.inChord.Start + offset, bone.inChord.End + offset)
        oChord = Chord(bone.outChord.Start + offset, bone.outChord.End + offset)
        iLine = iChord.asLine()
        oLine = oChord.asLine()
        cornerShape = Part.Shape([iLine, oLine])

        # construct a shape representing the cut made by the bone
        vt0 = FreeCAD.Vector(0, self.toolRadius, 0)
        vt1 = FreeCAD.Vector(length, self.toolRadius, 0)
        vb0 = FreeCAD.Vector(0, -self.toolRadius, 0)
        vb1 = FreeCAD.Vector(length, -self.toolRadius, 0)
        vm2 = FreeCAD.Vector(length + self.toolRadius, 0, 0)

        boneBot = Part.LineSegment(vb1, vb0)
        boneLid = Part.LineSegment(vb0, vt0)
        boneTop = Part.LineSegment(vt0, vt1)

        # what we actually want is an Arc - but findIntersect only returns the coincident if one exists
        # which really sucks because that's the one we're probably not interested in ....
        boneArc = Part.Arc(vt1, vm2, vb1)
        # boneArc = Part.Circle(FreeCAD.Vector(length, 0, 0), FreeCAD.Vector(0,0,1), self.toolRadius)
        boneWire = Part.Shape([boneTop, boneArc, boneBot, boneLid])
        boneWire.rotate(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1), boneAngle * 180 / math.pi)
        boneWire.translate(bone.inChord.End)
        self.boneShapes = [cornerShape, boneWire]

        bone.inCommands = self.smoothChordCommands(
            bone,
            bone.inChord,
            boneInChord,
            Part.Edge(iLine),
            boneWire,
            corner,
            bone.smooth & Smooth.In,
            (1.0, 0.0, 0.0),
        )
        bone.outCommands = self.smoothChordCommands(
            bone,
            boneOutChord,
            bone.outChord,
            Part.Edge(oLine),
            boneWire,
            corner,
            bone.smooth & Smooth.Out,
            (0.0, 1.0, 0.0),
        )
        return bone.inCommands + bone.outCommands

    def dogbone(self, bone):
        boneAngle = bone.angle()
        length = self.toolRadius * 0.41422  # 0.41422 = 2/sqrt(2) - 1 + (a tiny bit)
        return self.inOutBoneCommands(bone, boneAngle, length)

    def tboneHorizontal(self, bone):
        angle = bone.angle()
        boneAngle = 0
        if math.fabs(angle) > math.pi / 2:
            boneAngle = math.pi
        return self.inOutBoneCommands(bone, boneAngle, self.toolRadius)

    def tboneVertical(self, bone):
        angle = bone.angle()
        boneAngle = math.pi / 2
        if Path.Geom.isRoughly(angle, math.pi) or angle < 0:
            boneAngle = -boneAngle
        return self.inOutBoneCommands(bone, boneAngle, self.toolRadius)

    def tboneEdgeCommands(self, bone, onIn):
        if onIn:
            boneAngle = bone.inChord.getAngleXY()
        else:
            boneAngle = bone.outChord.getAngleXY()

        if Side.Right == bone.outChord.getDirectionOf(bone.inChord):
            boneAngle = boneAngle - math.pi / 2
        else:
            boneAngle = boneAngle + math.pi / 2

        onInString = "out"
        if onIn:
            onInString = "in"
        Path.Log.debug(
            "tboneEdge boneAngle[%s]=%.2f   (in=%.2f, out=%.2f)"
            % (
                onInString,
                boneAngle / math.pi,
                bone.inChord.getAngleXY() / math.pi,
                bone.outChord.getAngleXY() / math.pi,
            )
        )
        return self.inOutBoneCommands(bone, boneAngle, self.toolRadius)

    def tboneLongEdge(self, bone):
        inChordIsLonger = bone.inChord.getLength() > bone.outChord.getLength()
        return self.tboneEdgeCommands(bone, inChordIsLonger)

    def tboneShortEdge(self, bone):
        inChordIsShorter = bone.inChord.getLength() < bone.outChord.getLength()
        return self.tboneEdgeCommands(bone, inChordIsShorter)

    def boneIsBlacklisted(self, bone):
        blacklisted = False
        parentConsumed = False
        if bone.boneId in bone.obj.BoneBlacklist:
            blacklisted = True
        elif bone.location() in self.locationBlacklist:
            bone.obj.BoneBlacklist.append(bone.boneId)
            blacklisted = True
        elif hasattr(bone.obj.Base, "BoneBlacklist"):
            parentConsumed = bone.boneId not in bone.obj.Base.BoneBlacklist
            blacklisted = parentConsumed
        if blacklisted:
            self.locationBlacklist.add(bone.location())
        return (blacklisted, parentConsumed)

    # Generate commands necessary to execute the dogbone
    def boneCommands(self, bone, enabled):
        if enabled:
            if bone.obj.Style == Style.Dogbone:
                return self.dogbone(bone)
            if bone.obj.Style == Style.Tbone_H:
                return self.tboneHorizontal(bone)
            if bone.obj.Style == Style.Tbone_V:
                return self.tboneVertical(bone)
            if bone.obj.Style == Style.Tbone_L:
                return self.tboneLongEdge(bone)
            if bone.obj.Style == Style.Tbone_S:
                return self.tboneShortEdge(bone)
        else:
            return [bone.lastCommand, bone.outChord.g1Command(bone.F)]

    def insertBone(self, bone):
        Path.Log.debug(
            ">----------------------------------- %d --------------------------------------"
            % bone.boneId
        )
        self.boneShapes = []
        blacklisted, inaccessible = self.boneIsBlacklisted(bone)
        enabled = not blacklisted
        self.bones.append((bone.boneId, bone.locationZ(), enabled, inaccessible))

        self.boneId = bone.boneId
        # Specific debugging `if` statement
        # if Path.Log.getLevel(LOG_MODULE) == Path.Log.Level.DEBUG and bone.boneId > 2:
        #    commands = self.boneCommands(bone, False)
        # else:
        #    commands = self.boneCommands(bone, enabled)
        commands = self.boneCommands(bone, enabled)
        bone.commands = commands

        self.shapes[bone.boneId] = self.boneShapes
        Path.Log.debug(
            "<----------------------------------- %d --------------------------------------"
            % bone.boneId
        )
        return commands

    def removePathCrossing(self, commands, bone1, bone2):
        commands.append(bone2.lastCommand)
        bones = bone2.commands
        if True and hasattr(bone1, "outCommands") and hasattr(bone2, "inCommands"):
            inEdges = edgesForCommands(bone1.outCommands, bone1.tip)
            outEdges = edgesForCommands(bone2.inCommands, bone2.inChord.Start)
            for i in range(len(inEdges)):
                e1 = inEdges[i]
                for j in range(len(outEdges) - 1, -1, -1):
                    e2 = outEdges[j]
                    cutoff = DraftGeomUtils.findIntersection(e1, e2)
                    for pt in cutoff:
                        # debugCircle(e1.Curve.Center, e1.Curve.Radius, "bone.%d-1" % (self.boneId), (1.,0.,0.))
                        # debugCircle(e2.Curve.Center, e2.Curve.Radius, "bone.%d-2" % (self.boneId), (0.,1.,0.))
                        if Path.Geom.pointsCoincide(
                            pt, e1.valueAt(e1.LastParameter)
                        ) or Path.Geom.pointsCoincide(pt, e2.valueAt(e2.FirstParameter)):
                            continue
                        # debugMarker(pt, "it", (0.0, 1.0, 1.0))
                        # 1. remove all redundant commands
                        commands = commands[: -(len(inEdges) - i)]
                        # 2., correct where c1 ends
                        c1 = bone1.outCommands[i]
                        c1Params = c1.Parameters
                        c1Params.update({"X": pt.x, "Y": pt.y, "Z": pt.z})
                        c1 = Path.Command(c1.Name, c1Params)
                        commands.append(c1)
                        # 3. change where c2 starts, this depends on the command itself
                        c2 = bone2.inCommands[j]
                        if c2.Name in Path.Geom.CmdMoveArc:
                            center = e2.Curve.Center
                            offset = center - pt
                            c2Params = c2.Parameters
                            c2Params.update({"I": offset.x, "J": offset.y, "K": offset.z})
                            c2 = Path.Command(c2.Name, c2Params)
                            bones = [c2]
                            bones.extend(bone2.commands[j + 1 :])
                        else:
                            bones = bone2.commands[j:]
                        # there can only be the one ...
                        return commands, bones

        return commands, bones

    def execute(self, obj, forReal=True):
        if not obj.Base:
            return
        if forReal and not obj.Base.isDerivedFrom("Path::Feature"):
            return
        if not obj.Base.Path:
            return
        if not obj.Base.Path.Commands:
            return

        self.setup(obj, False)

        commands = []  # the dressed commands
        lastChord = Chord()  # the last chord
        lastCommand = None  # the command that generated the last chord
        lastBone = None  # track last bone for optimizations
        oddsAndEnds = []  # track chords that are connected to plunges - in case they form a loop

        boneId = 1
        self.bones = []
        self.locationBlacklist = set()
        self.length = 0
        # boneIserted = False

        for i, thisCommand in enumerate(PathUtils.getPathWithPlacement(obj.Base).Commands):
            # if i > 14:
            #    if lastCommand:
            #        commands.append(lastCommand)
            #        lastCommand = None
            #    commands.append(thisCommand)
            #    continue
            Path.Log.info("%3d: %s" % (i, thisCommand))
            if thisCommand.Name in movecommands:
                thisChord = lastChord.moveToParameters(thisCommand.Parameters)
                thisIsACandidate = self.canAttachDogbone(thisCommand, thisChord)

                if (
                    thisIsACandidate
                    and lastCommand
                    and self.shouldInsertDogbone(obj, lastChord, thisChord)
                ):
                    Path.Log.info("  Found bone corner: {}".format(lastChord.End))
                    bone = Bone(
                        boneId,
                        obj,
                        lastCommand,
                        lastChord,
                        thisChord,
                        Smooth.InAndOut,
                        thisCommand.Parameters.get("F"),
                    )
                    bones = self.insertBone(bone)
                    boneId += 1
                    if lastBone:
                        Path.Log.info("  removing potential path crossing")
                        # debugMarker(thisChord.Start, "it", (1.0, 0.0, 1.0))
                        commands, bones = self.removePathCrossing(commands, lastBone, bone)
                    commands.extend(bones[:-1])
                    lastCommand = bones[-1]
                    lastBone = bone
                elif lastCommand and thisChord.isAPlungeMove():
                    Path.Log.info("  Looking for connection in odds and ends")
                    haveNewLastCommand = False
                    for chord in (chord for chord in oddsAndEnds if lastChord.connectsTo(chord)):
                        if self.shouldInsertDogbone(obj, lastChord, chord):
                            Path.Log.info("    and there is one")
                            Path.Log.debug("    odd/end={} last={}".format(chord, lastChord))
                            bone = Bone(
                                boneId,
                                obj,
                                lastCommand,
                                lastChord,
                                chord,
                                Smooth.In,
                                lastCommand.Parameters.get("F"),
                            )
                            bones = self.insertBone(bone)
                            boneId += 1
                            if lastBone:
                                Path.Log.info("    removing potential path crossing")
                                # debugMarker(chord.Start, "it", (0.0, 1.0, 1.0))
                                commands, bones = self.removePathCrossing(commands, lastBone, bone)
                            commands.extend(bones[:-1])
                            lastCommand = bones[-1]
                            haveNewLastCommand = True
                    if not haveNewLastCommand:
                        commands.append(lastCommand)
                    lastCommand = None
                    commands.append(thisCommand)
                    lastBone = None
                elif thisIsACandidate:
                    Path.Log.info("  is a candidate, keeping for later")
                    if lastCommand:
                        commands.append(lastCommand)
                    lastCommand = thisCommand
                    lastBone = None
                elif thisChord.isANoopMove():
                    Path.Log.info("  ignoring and dropping noop move")
                    continue
                else:
                    Path.Log.info("  nope")
                    if lastCommand:
                        commands.append(lastCommand)
                        lastCommand = None
                    commands.append(thisCommand)
                    lastBone = None

                if lastChord.isAPlungeMove() and thisIsACandidate:
                    Path.Log.info("  adding to odds and ends")
                    oddsAndEnds.append(thisChord)

                lastChord = thisChord
            else:
                if thisCommand.Name[0] != "(":
                    Path.Log.info("  Clean slate")
                    if lastCommand:
                        commands.append(lastCommand)
                        lastCommand = None
                    lastBone = None
                commands.append(thisCommand)
        # for cmd in commands:
        #    Path.Log.debug("cmd = '%s'" % cmd)
        path = Path.Path(commands)
        obj.Path = path

    def setup(self, obj, initial):
        Path.Log.info("Here we go ... ")
        if initial:
            if hasattr(obj.Base, "BoneBlacklist"):
                # dressing up a bone dressup
                obj.Side = obj.Base.Side
            else:
                Path.Log.info("Default side = right")
                # otherwise dogbones are opposite of the base path's side
                side = Side.Right
                if hasattr(obj.Base, "Side") and obj.Base.Side == "Inside":
                    Path.Log.info("inside -> side = left")
                    side = Side.Left
                else:
                    Path.Log.info("not inside -> side stays right")
                if hasattr(obj.Base, "Direction") and obj.Base.Direction == "CCW":
                    Path.Log.info("CCW -> switch sides")
                    side = Side.oppositeOf(side)
                else:
                    Path.Log.info("CW -> stay on side")
                obj.Side = side

        self.toolRadius = 5
        tc = PathDressup.toolController(obj.Base)
        if tc is None or tc.ToolNumber == 0:
            self.toolRadius = 5
        else:
            tool = tc.Proxy.getTool(tc)  # PathUtils.getTool(obj, tc.ToolNumber)
            if not tool or float(tool.Diameter) == 0:
                self.toolRadius = 5
            else:
                self.toolRadius = float(tool.Diameter) / 2

        self.shapes = {}
        self.dbg = []

    def boneStateList(self, obj):
        state = {}
        # If the receiver was loaded from file, then it never generated the bone list.
        if not hasattr(self, "bones"):
            self.execute(obj)
        for nr, loc, enabled, inaccessible in self.bones:
            item = state.get((loc[0], loc[1]))
            if item:
                item[2].append(nr)
                item[3].append(loc[2])
            else:
                state[(loc[0], loc[1])] = (enabled, inaccessible, [nr], [loc[2]])
        return state


class Marker(object):
    def __init__(self, pt, r, h):
        if Path.Geom.isRoughly(h, 0):
            h = 0.1
        self.pt = pt
        self.r = r
        self.h = h
        self.sep = coin.SoSeparator()
        self.pos = coin.SoTranslation()
        self.pos.translation = (pt.x, pt.y, pt.z + h / 2)
        self.rot = coin.SoRotationXYZ()
        self.rot.axis = self.rot.X
        self.rot.angle = math.pi / 2
        self.cyl = coin.SoCylinder()
        self.cyl.radius = r
        self.cyl.height = h
        # self.cyl.removePart(self.cyl.TOP)
        # self.cyl.removePart(self.cyl.BOTTOM)
        self.material = coin.SoMaterial()
        self.sep.addChild(self.pos)
        self.sep.addChild(self.rot)
        self.sep.addChild(self.material)
        self.sep.addChild(self.cyl)
        self.lowlight()

    def setSelected(self, selected):
        if selected:
            self.highlight()
        else:
            self.lowlight()

    def highlight(self):
        self.material.diffuseColor = self.color(1)
        self.material.transparency = 0.45

    def lowlight(self):
        self.material.diffuseColor = self.color(0)
        self.material.transparency = 0.75

    def color(self, id):
        if id == 1:
            return coin.SbColor(0.9, 0.9, 0.5)
        return coin.SbColor(0.9, 0.5, 0.9)


class TaskPanel(object):
    DataIds = QtCore.Qt.ItemDataRole.UserRole
    DataKey = QtCore.Qt.ItemDataRole.UserRole + 1
    DataLoc = QtCore.Qt.ItemDataRole.UserRole + 2

    def __init__(self, viewProvider, obj):
        self.viewProvider = viewProvider
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/DogboneEdit.ui")
        self.s = None
        FreeCAD.ActiveDocument.openTransaction("Edit Dogbone Dress-up")
        self.height = 10
        self.markers = []

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)
        self.cleanup()

    def accept(self):
        self.getFields()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)
        FreeCAD.ActiveDocument.recompute()
        self.cleanup()

    def cleanup(self):
        self.viewProvider.showMarkers(False)
        for m in self.markers:
            self.viewProvider.switch.removeChild(m.sep)
        self.markers = []

    def getFields(self):
        self.obj.Style = str(self.form.styleCombo.currentText())
        self.obj.Side = str(self.form.sideCombo.currentText())
        self.obj.Incision = str(self.form.incisionCombo.currentText())
        self.obj.Custom = self.form.custom.value()
        blacklist = []
        for i in range(0, self.form.bones.count()):
            item = self.form.bones.item(i)
            if item.checkState() == QtCore.Qt.CheckState.Unchecked:
                blacklist.extend(item.data(self.DataIds))
        self.obj.BoneBlacklist = sorted(blacklist)
        self.obj.Proxy.execute(self.obj)

    def updateBoneList(self):
        itemList = []
        for loc, (enabled, inaccessible, ids, zs) in self.obj.Proxy.boneStateList(self.obj).items():
            lbl = "(%.2f, %.2f): %s" % (loc[0], loc[1], ",".join(str(id) for id in ids))
            item = QtGui.QListWidgetItem(lbl)
            if enabled:
                item.setCheckState(QtCore.Qt.CheckState.Checked)
            else:
                item.setCheckState(QtCore.Qt.CheckState.Unchecked)
            flags = QtCore.Qt.ItemFlag.ItemIsSelectable
            if not inaccessible:
                flags |= QtCore.Qt.ItemFlag.ItemIsEnabled | QtCore.Qt.ItemFlag.ItemIsUserCheckable
            item.setFlags(flags)
            item.setData(self.DataIds, ids)
            item.setData(self.DataKey, ids[0])
            item.setData(self.DataLoc, loc)
            itemList.append(item)
        self.form.bones.clear()
        markers = []
        for item in sorted(itemList, key=lambda item: item.data(self.DataKey)):
            self.form.bones.addItem(item)
            loc = item.data(self.DataLoc)
            r = max(self.obj.Proxy.length, 1)
            markers.append(
                Marker(
                    FreeCAD.Vector(loc[0], loc[1], min(zs)),
                    r,
                    max(1, max(zs) - min(zs)),
                )
            )
        for m in self.markers:
            self.viewProvider.switch.removeChild(m.sep)
        for m in markers:
            self.viewProvider.switch.addChild(m.sep)
        self.markers = markers

    def updateUI(self):
        customSelected = self.obj.Incision == Incision.Custom
        self.form.custom.setEnabled(customSelected)
        self.form.customLabel.setEnabled(customSelected)
        self.updateBoneList()

        if Path.Log.getLevel(Path.Log.thisModule()) == Path.Log.Level.DEBUG:
            for obj in FreeCAD.ActiveDocument.Objects:
                if obj.Name.startswith("Shape"):
                    FreeCAD.ActiveDocument.removeObject(obj.Name)
            Path.Log.info("object name %s" % self.obj.Name)
            if hasattr(self.obj.Proxy, "shapes"):
                Path.Log.info("showing shapes attribute")
                for shapes in self.obj.Proxy.shapes.values():
                    for shape in shapes:
                        Part.show(shape)
            else:
                Path.Log.info("no shapes attribute found")

    def updateModel(self):
        self.getFields()
        self.updateUI()
        FreeCAD.ActiveDocument.recompute()

    def setupCombo(self, combo, text, items):
        if items and len(items) > 0:
            for i in range(combo.count(), -1, -1):
                combo.removeItem(i)
            combo.addItems(items)
        index = combo.findText(text, QtCore.Qt.MatchFixedString)
        if index >= 0:
            combo.setCurrentIndex(index)

    def setFields(self):
        self.setupCombo(self.form.styleCombo, self.obj.Style, Style.All)
        self.setupCombo(self.form.sideCombo, self.obj.Side, Side.All)
        self.setupCombo(self.form.incisionCombo, self.obj.Incision, Incision.All)
        self.form.custom.setMinimum(0.0)
        self.form.custom.setDecimals(3)
        self.form.custom.setValue(self.obj.Custom)
        self.updateUI()

    def open(self):
        self.s = SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)

    def setupUi(self):
        self.setFields()
        # now that the form is filled, setup the signal handlers
        self.form.styleCombo.currentIndexChanged.connect(self.updateModel)
        self.form.sideCombo.currentIndexChanged.connect(self.updateModel)
        self.form.incisionCombo.currentIndexChanged.connect(self.updateModel)
        self.form.custom.valueChanged.connect(self.updateModel)
        self.form.bones.itemChanged.connect(self.updateModel)
        self.form.bones.itemSelectionChanged.connect(self.updateMarkers)

        self.viewProvider.showMarkers(True)

    def updateMarkers(self):
        index = self.form.bones.currentRow()
        for i, m in enumerate(self.markers):
            m.setSelected(i == index)


class SelObserver(object):
    def __init__(self):
        import Path.Op.Gui.Selection as PST

        PST.eselect()

    def __del__(self):
        import Path.Op.Gui.Selection as PST

        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):
        FreeCADGui.doCommand("Gui.Selection.addSelection(FreeCAD.ActiveDocument." + obj + ")")
        FreeCADGui.updateGui()


class ViewProviderDressup(object):
    def __init__(self, vobj):
        self.vobj = vobj
        self.obj = None

    def attach(self, vobj):
        self.obj = vobj.Object
        if self.obj and self.obj.Base:
            for i in self.obj.Base.InList:
                if hasattr(i, "Group"):
                    group = i.Group
                    for g in group:
                        if g.Name == self.obj.Base.Name:
                            group.remove(g)
                    i.Group = group
            # FreeCADGui.ActiveDocument.getObject(obj.Base.Name).Visibility = False
        self.switch = coin.SoSwitch()
        vobj.RootNode.addChild(self.switch)

    def showMarkers(self, on):
        sw = coin.SO_SWITCH_ALL if on else coin.SO_SWITCH_NONE
        self.switch.whichChild = sw

    def claimChildren(self):
        return [self.obj.Base]

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        panel = TaskPanel(self, vobj.Object)
        FreeCADGui.Control.showDialog(panel)
        panel.setupUi()
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onDelete(self, arg1=None, arg2=None):
        """this makes sure that the base operation is added back to the project and visible"""
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(arg1.Object)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True


def Create(base, name="DogboneDressup"):
    """
    Create(obj, name='DogboneDressup') ... dresses the given Path.Op.Profile/PathContour object with dogbones.
    """
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    dbo = ObjectDressup(obj, base)
    job = PathUtils.findParentJob(base)
    job.Proxy.addOperation(obj, base)

    if FreeCAD.GuiUp:
        obj.ViewObject.Proxy = ViewProviderDressup(obj.ViewObject)
        obj.Base.ViewObject.Visibility = False

    dbo.setup(obj, True)
    return obj


class CommandDressupDogbone(object):
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupDogbone", "Dogbone"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_DressupDogbone",
                "Creates a dogbone dress-up object from a selected toolpath",
            ),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(
                translate("CAM_DressupDogbone", "Select one toolpath object") + "\n"
            )
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(
                translate("CAM_DressupDogbone", "The selected object is not a toolpath") + "\n"
            )
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create Dogbone Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.Dogbone")
        FreeCADGui.doCommand(
            "Path.Dressup.Gui.Dogbone.Create(FreeCAD.ActiveDocument.%s)" % baseObject.Name
        )
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()` called via TaskPanel.accept()
        FreeCAD.ActiveDocument.recompute()


# obsolete, replaced by DogboneII
# if FreeCAD.GuiUp:
#    import FreeCADGui
#    from PySide import QtGui
#    from pivy import coin
#
#    FreeCADGui.addCommand("CAM_DressupDogbone", CommandDressupDogbone())

FreeCAD.Console.PrintLog("Loading DressupDogbone… done\n")
