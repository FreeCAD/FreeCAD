# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import FreeCAD
import FreeCADGui
import Path
from PathScripts import PathUtils
from PySide import QtCore, QtGui
import math
import Part
import DraftGeomUtils

"""Dogbone Dressup object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)

except AttributeError:

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

movecommands = ['G0', 'G00', 'G1', 'G01', 'G2', 'G02', 'G3', 'G03']
movestraight = ['G1', 'G01']

debugDogbone = False

def debugPrint(msg):
    if debugDogbone:
        print(msg)

def debugMarker(vector, label, color = None, radius = 0.5):
    if debugDogbone:
        obj = FreeCAD.ActiveDocument.addObject("Part::Sphere", label)
        obj.Label = label
        obj.Radius = radius
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        if color:
            obj.ViewObject.ShapeColor = color

def debugCircle(vector, r, label, color = None):
    if debugDogbone:
        obj = FreeCAD.ActiveDocument.addObject("Part::Cylinder", label)
        obj.Label = label
        obj.Radius = r
        obj.Height = 1
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        obj.ViewObject.Transparency = 90
        if color:
            obj.ViewObject.ShapeColor = color

def addAngle(a1, a2):
    a = a1 + a2
    while a <= -math.pi:
        a += 2*math.pi
    while a > math.pi:
        a -= 2*math.pi
    return a

def anglesAreParallel(a1, a2):
    an1 = addAngle(a1, 0)
    an2 = addAngle(a2, 0)
    if an1 == an2:
        return True
    if an1 == addAngle(an2, math.pi):
        return True
    return False

class Style:
    Dogbone = 'Dogbone'
    Tbone_H = 'T-bone horizontal'
    Tbone_V = 'T-bone vertical'
    Tbone_L = 'T-bone long edge'
    Tbone_S = 'T-bone short edge'
    All = [Dogbone, Tbone_H, Tbone_V, Tbone_L, Tbone_S]

class Side:
    Left = 'Left'
    Right = 'Right'
    All = [Left, Right]

    @classmethod
    def oppositeOf(cls, side):
        if side == cls.Left:
            return cls.Right
        if side == cls.Right:
            return cls.Left
        return None

class Incision:
    Fixed = 'fixed'
    Adaptive = 'adaptive'
    Custom = 'custom'
    All = [Adaptive, Fixed, Custom]

class Smooth:
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
class Chord (object):
    def __init__(self, start = None, end = None):
        if not start:
            start = FreeCAD.Vector()
        if not end:
            end = FreeCAD.Vector()
        self.Start = start
        self.End = end

    def __str__(self):
        return "Chord([%g, %g, %g] -> [%g, %g, %g])" % (self.Start.x, self.Start.y, self.Start.z, self.End.x, self.End.y, self.End.z)

    def moveTo(self, newEnd):
        #debugPrint("Chord(%s -> %s)" % (self.End, newEnd))
        return Chord(self.End, newEnd)

    def moveToParameters(self, params):
        x = params.get('X', self.End.x)
        y = params.get('Y', self.End.y)
        z = params.get('Z', self.End.z)
        return self.moveTo(FreeCAD.Vector(x, y, z))

    def moveBy(self, x, y, z):
        return self.moveTo(self.End + FreeCAD.Vector(x, y, z))

    def move(self, distance, angle):
        dx = distance * math.cos(angle)
        dy = distance * math.sin(angle)
        return self.moveBy(dx, dy, 0)

    def asVector(self):
        return self.End - self.Start

    def asLine(self):
        return Part.Line(self.Start, self.End)

    def asEdge(self):
        return Part.Edge(self.asLine())

    def getLength(self):
        return self.asVector().Length

    def getDirectionOfVector(self, B):
        A = self.asVector()
        # if the 2 vectors are identical, they head in the same direction
        if A == B:
            return 'Straight'
        d = -A.x*B.y + A.y*B.x
        if d < 0:
            return Side.Left
        if d > 0:
            return Side.Right
        # at this point the only direction left is backwards
        return 'Back'

    def getDirectionOf(self, chordOrVector):
        if type(chordOrVector) is Chord:
            return self.getDirectionOfVector(chordOrVector.asVector())
        return self.getDirectionOfVector(chordOrVector)

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
            return self.getAngleOfVector(refChordOrVector.asVector())
        return self.getAngleOfVector(refChordOrVector)

    def getAngleXY(self):
        return self.getAngle(FreeCAD.Vector(1,0,0))

    def withLength(self, length):
        return Chord(self.Start, self.Start).move(length, self.getAngleXY())

    def g1Command(self):
        return Path.Command("G1", {"X": self.End.x, "Y": self.End.y})

    def arcCommand(self, cmd, center):
        d = center - self.Start
        debugPrint("arc (%.2f, %.2f) (%.2f, %.2f) (%.2f, %.2f)" % (self.Start.x, self.Start.y, center.x, center.y, self.End.x, self.End.y))
        return Path.Command(cmd, {"X": self.End.x, "Y": self.End.y, "I": d.x, "J": d.y, "K": 0})

    def g2Command(self, center):
        return self.arcCommand("G2", center)
    def g3Command(self, center):
        return self.arcCommand("G3", center)

    def isAPlungeMove(self):
        return self.End.z != self.Start.z

    def foldsBackOrTurns(self, chord, side):
        dir = chord.getDirectionOf(self)
        return dir == 'Back' or dir == side

    def connectsTo(self, chord):
        return self.End == chord.Start

    def lineEquation(self):
        if self.End.x != self.Start.x:
            slope = (self.End.y - self.Start.y) / (self.End.x - self.Start.x)
            offset = self.Start.y - self.Start.x * slope
            return (offset, slope)
        return (None, None)

    def intersection(self, chord, emergencyOffset = 0):
        itsOffset, itsSlope = chord.lineEquation()
        myOffset, mySlope = self.lineEquation()
        x = 0
        y = 0
        if itsSlope is not None and mySlope is not None:
            if itsSlope == mySlope:
                # Now this is wierd, but it happens when the path folds onto itself
                angle = self.getAngleXY()
                x = self.End.x + emergencyOffset * math.cos(angle)
                y = self.End.y + emergencyOffset * math.sin(angle)
            else:
                x = (myOffset - itsOffset) / (itsSlope - mySlope)
                y = myOffset + mySlope * x
        elif itsSlope is not None:
            x = self.Start.x
            y = itsOffset + x * itsSlope
        elif mySlope is not None:
            x = chord.Start.x
            y = myOffset + x * mySlope
        else:
            print("Now this really sucks")

        return FreeCAD.Vector(x, y, self.End.z)

    def perpendicular(self):
        v = self.asVector()
        return FreeCAD.Vector(-v.y, v.x, 0)

    def footOfPerpendicularFrom(self, vector):
        return self.intersection(Chord(vector, vector + self.perpendicular()))

    def offsetBy(self, distance, direction):
        if direction == Side.Left:
            angle = addAngle(self.getAngleXY(),  math.pi/2)
        else:
            angle = addAngle(self.getAngleXY(), -math.pi/2)
        dx = distance * math.cos(angle)
        dy = distance * math.sin(angle)
        d = FreeCAD.Vector(dx, dy, 0)
        return Chord(self.Start + v, self.End.v)

class ObjectDressup:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLink", "Base","Base", QtCore.QT_TRANSLATE_NOOP("Dogbone_Dressup", "The base path to modify"))
        obj.addProperty("App::PropertyEnumeration", "Side", "Dressup", QtCore.QT_TRANSLATE_NOOP("Dogbone_Dressup", "The side of path to insert bones"))
        obj.Side = [Side.Left, Side.Right]
        obj.Side = Side.Right
        obj.addProperty("App::PropertyEnumeration", "Style", "Dressup", QtCore.QT_TRANSLATE_NOOP("Dogbone_Dressup", "The style of boness"))
        obj.Style = Style.All
        obj.Style = Style.Dogbone
        obj.addProperty("App::PropertyIntegerList", "BoneBlacklist", "Dressup", QtCore.QT_TRANSLATE_NOOP("Dogbone_Dressup", "Bones that aren't dressed up"))
        obj.BoneBlacklist = []
        obj.setEditorMode('BoneBlacklist', 2)  # hide this one
        obj.addProperty("App::PropertyEnumeration", "Incision", "Dressup", QtCore.QT_TRANSLATE_NOOP("Dogbone_Dressup", "The algorithm to determine the bone length"))
        obj.Incision = Incision.All
        obj.Incision = Incision.Adaptive
        obj.addProperty("App::PropertyFloat", "Custom", "Dressup", QtCore.QT_TRANSLATE_NOOP("Dogbone_Dressup", "Dressup length if Incision == custom"))
        obj.Custom = 0.0
        obj.Proxy = self
        self.shapes = {}

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def theOtherSideOf(self, side):
        if side == Side.Left:
            return Side.Right
        return Side.Left

    # Answer true if a dogbone could be on either end of the chord, given its command
    def canAttachDogbone(self, cmd, chord):
        return cmd.Name in movestraight and not chord.isAPlungeMove()

    def shouldInsertDogbone(self, obj, inChord, outChord):
        return outChord.foldsBackOrTurns(inChord, self.theOtherSideOf(obj.Side))

    def cornerDistanceAndAngle(self, obj, inChord, outChord):
        baseAngle = inChord.getAngleXY()
        turnAngle = outChord.getAngle(inChord)
        boneAngle = addAngle(baseAngle, (turnAngle - math.pi)/2)
        if obj.Side == Side.Left:
            boneAngle = addAngle(boneAngle, math.pi)
        #debugPrint("base=%+3.2f turn=%+3.2f bone=%+3.2f" % (baseAngle/math.pi, turnAngle/math.pi, boneAngle/math.pi))
        distance = self.toolRadius / math.cos(turnAngle/2)
        return (distance, boneAngle)

    def adaptiveBoneLength(self, obj, inChord, outChord, boneAngle):
        cornerDistance, cornerAngle = self.cornerDistanceAndAngle(obj, inChord, outChord)
        if cornerAngle == boneAngle:
            # moving directly towards the corner
            return cornerDistance - self.toolRadius

        # The bones root and end point form a triangle with the intersection of the tool path
        # with the toolRadius circle around the bone end point.
        # In case the math looks questionable, look for "triangle ssa"
        # c = cornerDistance
        # b = self.toolRadius
        # beta = fabs(boneAngle - cornerAngle)
        beta = math.fabs(addAngle(boneAngle, -cornerAngle))
        D = (cornerDistance / self.toolRadius) * math.sin(beta)
        if D > 1: # no intersection
            return 0
        gamma = math.asin(D)
        alpha = math.pi - beta - gamma
        boneDistance = self.toolRadius * math.sin(alpha) / math.sin(beta)
        if D < 1 and self.toolRadius < cornerDistance: # there exists a second solution
            beta2 = beta
            gamma2 = math.pi - gamma
            alpha2 = math.pi - beta2 - gamma2
            boneDistance2 = self.toolRadius * math.sin(alpha2) / math.sin(beta2)
            boneDistance = min(boneDistance, boneDistance2)

        debugPrint("corner=%.2f * %.2f -> bone=%.2f * %.2f" % (cornerDistance, cornerAngle, boneDistance, boneAngle))
        return boneDistance

    def smoothChordCommands(self, obj, inChord, outChord, edge, bone, corner, smooth, color):
        if smooth == 0:
            debugPrint(" No smoothing requested")
            return [ inChord.g1Command(), outChord.g1Command() ]

        d = 'in'
        if smooth == Smooth.Out:
            d = 'out'

        if DraftGeomUtils.areColinear(inChord.asEdge(), outChord.asEdge()):
            debugPrint(" straight edge %s" % d)
            return [ outChord.g1Command() ]

        pivot = None
        pivotDistance = 0

        for e in bone.Edges:
            for pt in DraftGeomUtils.findIntersection(edge, e, True):
                if pt != corner:
                    distance = (pt - inChord.End).Length
                    if not pivot or pivotDistance > distance:
                        pivot = pt
                        pivotDistance = distance
                else:
                    debugPrint(" corner intersect %s (%.2f, %.2f) (%.2f, %.2f)" % (d, corner.x, corner.y, pt.x, pt.y))

        if pivot:
            debugCircle(pivot, self.toolRadius, "pivot.%d-%s" % (self.boneId, d), color)
            inTangent = DraftGeomUtils.findDistance(pivot, inChord.asEdge(), True)
            if not inTangent:
                inTangent = inChord.Start - pivot
            outTangent = DraftGeomUtils.findDistance(pivot, outChord.asEdge(), True)
            if not outTangent:
                outTangent = outChord.End - pivot
            t1 = pivot + inTangent
            t2 = pivot + outTangent

            commands = []
            if t1 != inChord.Start:
                commands.append(Chord(inChord.Start, t1).g1Command())
            if obj.Side == Side.Left:
                commands.append(Chord(t1, t2).g3Command(pivot))
            else:
                commands.append(Chord(t1, t2).g2Command(pivot))
            if t2 != outChord.End:
                commands.append(Chord(t2, outChord.End).g1Command())

            debugMarker(t1, "pivot.%d-%s.in" % (self.boneId, d), color, 0.2)
            debugMarker(t2, "pivot.%d-%s.out" % (self.boneId, d), color, 0.2)

            return commands

        return [ inChord.g1Command(), outChord.g1Command() ]

    def inOutBoneCommands(self, obj, inChord, outChord, boneAngle, fixedLength, smooth):
        cornerDistance, cornerAngle = self.cornerDistanceAndAngle(obj, inChord, outChord)
        corner = inChord.move(cornerDistance, cornerAngle).End

        debugPrint("corner = (%.2f, %.2f)" % (corner.x, corner.y))
        #debugMarker(corner, 'corner', (1., 0., 1.))

        length = fixedLength
        if obj.Incision == Incision.Custom:
            length = obj.Custom
        if obj.Incision == Incision.Adaptive:
            length = self.adaptiveBoneLength(obj, inChord, outChord, boneAngle)

        if length == 0:
            # no bone after all ..
            return [ inChord.g1Command(), outChord.g1Command() ]

        boneInChord = inChord.move(length, boneAngle)
        boneOutChord = boneInChord.moveTo(outChord.Start)

        bones = []

        #debugCircle(boneInChord.Start, self.toolRadius, 'boneStart')
        #debugCircle(boneInChord.End, self.toolRadius, 'boneEnd')

        if smooth == 0:
            return [ inChord.g1Command(), boneInChord.g1Command(), boneOutChord.g1Command(), outChord.g1Command()]

        # reconstruct the corner and convert to an edge
        offset = corner - inChord.End
        iChord = Chord(inChord.Start + offset, inChord.End + offset)
        oChord = Chord(outChord.Start + offset, outChord.End + offset)
        iLine = iChord.asLine()
        oLine = oChord.asLine()
        cornerEdge = Part.Shape([iLine, oLine])

        # construct a shape representing the cut made by the bone
        vt0 = FreeCAD.Vector(     0,  self.toolRadius, 0)
        vt1 = FreeCAD.Vector(length,  self.toolRadius, 0)
        vb0 = FreeCAD.Vector(     0, -self.toolRadius, 0)
        vb1 = FreeCAD.Vector(length, -self.toolRadius, 0)
        vm2 = FreeCAD.Vector(length + self.toolRadius, 0, 0)

        boneBot = Part.Line(vb1, vb0)
        boneTop = Part.Line(vt0, vt1)
        boneFlat = Part.Line(vb0, vt0)
        # what we actually want is an Arc - but findIntersect only returns the coincident if one exists
        # which really sucks because that's the one we're probably not interested in ....
        #boneArc = Part.Arc(vt1, vm2, vb1)
        boneArc = Part.Circle(FreeCAD.Vector(length, 0, 0), FreeCAD.Vector(0,0,1), self.toolRadius)
        boneWire = Part.Shape([boneTop,boneArc,boneBot])
        boneWire.rotate(FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,0,1), boneAngle * 180 / math.pi)
        boneWire.translate(inChord.End)
        self.boneShapes = [cornerEdge, boneWire]

        bones.extend(self.smoothChordCommands(obj, inChord,   boneInChord, Part.Edge(iLine), boneWire, corner, smooth & Smooth.In, (1., 0., 0.)))
        bones.extend(self.smoothChordCommands(obj, boneOutChord, outChord, Part.Edge(oLine), boneWire, corner, smooth & Smooth.Out, (0., 1., 0.)))
        return bones

    def dogboneAngle(self, obj, inChord, outChord):
        distance, angle = self.cornerDistanceAndAngle(obj, inChord, outChord)
        return angle

    def dogbone(self, obj, inChord, outChord, smooth):
        boneAngle = self.dogboneAngle(obj, inChord, outChord)
        length = self.toolRadius * 0.41422 # 0.41422 = 2/sqrt(2) - 1 + (a tiny bit)
        return self.inOutBoneCommands(obj, inChord, outChord, boneAngle, length, smooth)

    def tboneHorizontal(self, obj, inChord, outChord, smooth):
        angle = self.dogboneAngle(obj, inChord, outChord)
        boneAngle = 0
        if angle == math.pi or math.fabs(angle) > math.pi/2:
            boneAngle = -math.pi
        return self.inOutBoneCommands(obj, inChord, outChord, boneAngle, self.toolRadius, smooth)

    def tboneVertical(self, obj, inChord, outChord, smooth):
        angle = self.dogboneAngle(obj, inChord, outChord)
        boneAngle = math.pi/2
        if angle == math.pi or angle < 0:
            boneAngle = -boneAngle
        return self.inOutBoneCommands(obj, inChord, outChord, boneAngle, self.toolRadius, smooth)

    def tboneEdgeCommands(self, obj, inChord, outChord, onIn, smooth):
        boneAngle = outChord.getAngleXY()
        if onIn:
            boneAngle = inChord.getAngleXY()
        boneAngle = boneAngle + math.pi/2
        if Side.Right == outChord.getDirectionOf(inChord):
            boneAngle = boneAngle - math.pi
        return self.inOutBoneCommands(obj, inChord, outChord, boneAngle, self.toolRadius, smooth)

    def tboneLongEdge(self, obj, inChord, outChord, smooth):
        inChordIsLonger = inChord.getLength() > outChord.getLength()
        return self.tboneEdgeCommands(obj, inChord, outChord, inChordIsLonger, smooth)

    def tboneShortEdge(self, obj, inChord, outChord, smooth):
        inChordIsShorter = inChord.getLength() < outChord.getLength()
        return self.tboneEdgeCommands(obj, inChord, outChord, inChordIsShorter, smooth)

    def boneIsBlacklisted(self, obj, boneId, loc):
        blacklisted = False
        parentConsumed = False
        if boneId in obj.BoneBlacklist:
            blacklisted = True
        elif loc in self.locationBlacklist:
            obj.BoneBlacklist.append(boneId)
            blacklisted = True
        elif hasattr(obj.Base, 'BoneBlacklist'):
            parentConsumed = boneId not in obj.Base.BoneBlacklist
            blacklisted = parentConsumed
        if blacklisted:
            self.locationBlacklist.add(loc)
        return (blacklisted, parentConsumed)

    # Generate commands necessary to execute the dogbone
    def boneCommands(self, obj, enabled, inChord, outChord, smooth):
        if enabled:
            if obj.Style == Style.Dogbone:
                return self.dogbone(obj, inChord, outChord, smooth)
            if obj.Style == Style.Tbone_H:
                return self.tboneHorizontal(obj, inChord, outChord, smooth)
            if obj.Style == Style.Tbone_V:
                return self.tboneVertical(obj, inChord, outChord, smooth)
            if obj.Style == Style.Tbone_L:
                return self.tboneLongEdge(obj, inChord, outChord, smooth)
            if obj.Style == Style.Tbone_S:
                return self.tboneShortEdge(obj, inChord, outChord, smooth)
        else:
            return [ inChord.g1Command(), outChord.g1Command() ]

    def insertBone(self, boneId, obj, inChord, outChord, commands, smooth):
        debugPrint(">----------------------------------- %d --------------------------------------" % boneId)
        self.boneShapes = []
        loc = (inChord.End.x, inChord.End.y)
        blacklisted, inaccessible = self.boneIsBlacklisted(obj, boneId, loc)
        enabled = not blacklisted
        self.bones.append((boneId, loc, enabled, inaccessible))

        self.boneId = boneId
        if debugDogbone and boneId == 0:
            bones = self.boneCommands(obj, False, inChord, outChord, smooth)
        else:
            bones = self.boneCommands(obj, enabled, inChord, outChord, smooth)
        commands.extend(bones[:-1])
        self.shapes[boneId] = self.boneShapes
        debugPrint("<----------------------------------- %d --------------------------------------" % boneId)
        return boneId + 1, bones[-1]

    def execute(self, obj):
        if not obj.Base:
            return
        if not obj.Base.isDerivedFrom("Path::Feature"):
            return
        if not obj.Base.Path:
            return
        if not obj.Base.Path.Commands:
            return

        self.setup(obj)

        commands = []           # the dressed commands
        lastChord = Chord()     # the last chord
        lastCommand = None      # the command that generated the last chord
        oddsAndEnds = []        # track chords that are connected to plunges - in case they form a loop

        boneId = 1
        self.bones = []
        self.locationBlacklist = set()

        for thisCmd in obj.Base.Path.Commands:
            if thisCmd.Name in movecommands:
                thisChord = lastChord.moveToParameters(thisCmd.Parameters)
                thisIsACandidate = self.canAttachDogbone(thisCmd, thisChord)

                if thisIsACandidate and lastCommand and self.shouldInsertDogbone(obj, lastChord, thisChord):
                    boneId, lastCommand = self.insertBone(boneId, obj, lastChord, thisChord, commands, Smooth.InAndOut)
                elif lastCommand and thisChord.isAPlungeMove():
                    for chord in (chord for chord in oddsAndEnds if lastChord.connectsTo(chord)):
                        if self.shouldInsertDogbone(obj, lastChord, chord):
                            boneId, lastCommand = self.insertBone(boneId, obj, lastChord, chord, commands, Smooth.In)
                    lastCommand = None
                    commands.append(thisCmd)
                elif thisIsACandidate:
                    lastCommand = thisCmd
                else:
                    if lastCommand:
                        commands.append(lastCommand)
                        lastCommand = None
                    commands.append(thisCmd)

                if lastChord.isAPlungeMove() and thisIsACandidate:
                    oddsAndEnds.append(thisChord)

                lastChord = thisChord
            else:
                if lastCommand:
                    commands.append(lastCommand)
                    lastCommand = None
                commands.append(thisCmd)
        #for cmd in commands:
        #    debugPrint("cmd = '%s'" % cmd)
        path = Path.Path(commands)
        obj.Path = path

    def setup(self, obj):
        if not hasattr(self, 'toolRadius'):
            debugPrint("Here we go ... ")
            if hasattr(obj.Base, "BoneBlacklist"):
                # dressing up a bone dressup
                obj.Side = obj.Base.Side
            else:
                # otherwise dogbones are opposite of the base path's side
                if obj.Base.Side == Side.Left:
                    obj.Side = Side.Right
                elif obj.Base.Side == Side.Right:
                    obj.Side = Side.Left
                else:
                    # This will cause an error, which is fine for now 'cause I don't know what to do here
                    obj.Side = 'On'

        self.toolRadius = 5
        toolLoad = PathUtils.getLastToolLoad(obj)
        if toolLoad is None or toolLoad.ToolNumber == 0:
            self.toolRadius = 5
        else:
            tool = PathUtils.getTool(obj, toolLoad.ToolNumber)
            if not tool or tool.Diameter == 0:
                self.toolRadius = 5
            else:
                self.toolRadius = tool.Diameter / 2

    def boneStateList(self, obj):
        state = {}
        # If the receiver was loaded from file, then it never generated the bone list.
        if not hasattr(self, 'bones'):
            self.execute(obj)
        for (id, loc, enabled, inaccessible) in self.bones:
            item = state.get(loc)
            if item:
                item[2].append(id)
            else:
                state[loc] = (enabled, inaccessible, [id])
        return state

class TaskPanel:
    DataIds = QtCore.Qt.ItemDataRole.UserRole
    DataKey = QtCore.Qt.ItemDataRole.UserRole + 1

    def __init__(self, obj):
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/DogboneEdit.ui")
        FreeCAD.ActiveDocument.openTransaction(translate("Dogbone_Dressup", "Edit Dogbone Dress-up"))

    def reject(self):
        FreeCAD.ActiveDocument.abortTransaction()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def accept(self):
        self.getFields()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)
        FreeCAD.ActiveDocument.recompute()

    def getFields(self):
        self.obj.Style = str(self.form.styleCombo.currentText())
        self.obj.Side  = str(self.form.sideCombo.currentText())
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
        for loc, (enabled, inaccessible, ids) in self.obj.Proxy.boneStateList(self.obj).iteritems():
            lbl = '(%.2f, %.2f): %s' % (loc[0], loc[1], ','.join(str(id) for id in ids))
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
            itemList.append(item)
        self.form.bones.clear()
        for item in sorted(itemList, key=lambda item: item.data(self.DataKey)):
            self.form.bones.addItem(item)

    def updateUI(self):
        customSelected = self.obj.Incision == Incision.Custom
        self.form.custom.setEnabled(customSelected)
        self.form.customLabel.setEnabled(customSelected)
        self.updateBoneList()

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

        if hasattr(self.obj.Proxy, "shapes"):
            print("showing shapes attribute")
            for shapes in self.obj.Proxy.shapes.itervalues():
                for shape in shapes:
                    Part.show(shape)
        else:
            print("no shapes attribute found")


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

class SelObserver:
    def __init__(self):
        import PathScripts.PathSelection as PST
        PST.eselect()

    def __del__(self):
        import PathScripts.PathSelection as PST
        PST.clear()

    def addSelection(self, doc, obj, sub, pnt):
        FreeCADGui.doCommand('Gui.Selection.addSelection(FreeCAD.ActiveDocument.' + obj + ')')
        FreeCADGui.updateGui()

class ViewProviderDressup:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def claimChildren(self):
        for i in self.Object.Base.InList:
            if hasattr(i, "Group"):
                group = i.Group
                for g in group:
                    if g.Name == self.Object.Base.Name:
                        group.remove(g)
                i.Group = group
                print i.Group
        #FreeCADGui.ActiveDocument.getObject(obj.Base.Name).Visibility = False
        return [self.Object.Base]

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        panel = TaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(panel)
        panel.setupUi()
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onDelete(self, arg1=None, arg2=None):
        '''this makes sure that the base operation is added back to the project and visible'''
        FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
        PathUtils.addToJob(arg1.Object.Base)
        return True

class CommandDogboneDressup:

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Dogbone_Dressup", "Dogbone Dress-up"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Dogbone_Dressup", "Creates a Dogbone Dress-up object from a selected path")}

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
            FreeCAD.Console.PrintError(translate("Dogbone_Dressup", "Please select one path object\n"))
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(translate("Dogbone_Dressup", "The selected object is not a path\n"))
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            FreeCAD.Console.PrintError(translate("Dogbone_Dressup", "Please select a Profile or Dogbone Dressup object"))
            return
        if not hasattr(baseObject, "Side"):
            FreeCAD.Console.PrintError(translate("Dogbone_Dressup", "Please select a Profile or Dogbone Dressup object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate("Dogbone_Dressup", "Create Dogbone Dress-up"))
        FreeCADGui.addModule("PathScripts.DogboneDressup")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "DogboneDressup")')
        FreeCADGui.doCommand('dbo = PathScripts.DogboneDressup.ObjectDressup(obj)')
        FreeCADGui.doCommand('obj.Base = FreeCAD.ActiveDocument.' + baseObject.Name)
        FreeCADGui.doCommand('PathScripts.DogboneDressup.ViewProviderDressup(obj.ViewObject)')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('Gui.ActiveDocument.getObject(obj.Base.Name).Visibility = False')
        FreeCADGui.doCommand('dbo.setup(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Dogbone_Dressup', CommandDogboneDressup())

FreeCAD.Console.PrintLog("Loading DogboneDressup... done\n")
