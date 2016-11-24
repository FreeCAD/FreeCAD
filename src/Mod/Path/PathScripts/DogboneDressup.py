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

debugDressup = False

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
movecw =       ['G2', 'G02']
moveccw =      ['G3', 'G03']
movearc = movecw + moveccw

def debugPrint(msg):
    if debugDressup:
        print(msg)

def debugMarker(vector, label, color = None, radius = 0.5):
    if debugDressup:
        obj = FreeCAD.ActiveDocument.addObject("Part::Sphere", label)
        obj.Label = label
        obj.Radius = radius
        obj.Placement = FreeCAD.Placement(vector, FreeCAD.Rotation(FreeCAD.Vector(0,0,1), 0))
        if color:
            obj.ViewObject.ShapeColor = color

def debugCircle(vector, r, label, color = None):
    if debugDressup:
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

def getAngle(v):
    a = v.getAngle(FreeCAD.Vector(1,0,0))
    if v.y < 0:
        return -a
    return a

def pointFromCommand(cmd, pt, X='X', Y='Y', Z='Z'):
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
            if cmd.Name in movestraight:
                edges.append(Part.Edge(Part.Line(lastPt, pt)))
            elif cmd.Name in movearc:
                center = lastPt + pointFromCommand(cmd, FreeCAD.Vector(0,0,0), 'I', 'J', 'K')
                A = lastPt - center
                B = pt - center
                d = -B.x * A.y + B.y * A.x

                if d == 0:
                    # we're dealing with half a circle here
                    angle = getAngle(A) + math.pi/2
                    if cmd.Name in movecw:
                        angle -= math.pi
                else:
                    C = A + B
                    angle = getAngle(C)

                R = (lastPt - center).Length
                ptm = center + FreeCAD.Vector(math.cos(angle), math.sin(angle), 0) * R

                edges.append(Part.Edge(Part.Arc(lastPt, ptm, pt)))
            lastPt = pt
    return edges

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

    def g1Command(self):
        return Path.Command("G1", {"X": self.End.x, "Y": self.End.y, "Z": self.End.z})

    def arcCommand(self, cmd, center):
        d = center - self.Start
        return Path.Command(cmd, {"X": self.End.x, "Y": self.End.y, "Z": self.End.z, "I": d.x, "J": d.y, "K": 0})

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

class Bone:
    def __init__(self, boneId, obj, lastCommand, inChord, outChord, smooth):
        self.obj = obj
        self.boneId = boneId
        self.lastCommand = lastCommand
        self.inChord = inChord
        self.outChord = outChord
        self.smooth = smooth

    def angle(self):
        if not hasattr(self, 'cAngle'):
            baseAngle = self.inChord.getAngleXY()
            turnAngle = self.outChord.getAngle(self.inChord)
            angle = addAngle(baseAngle, (turnAngle - math.pi)/2)
            if self.obj.Side == Side.Left:
                angle = addAngle(angle, math.pi)
            self.tAngle = turnAngle
            self.cAngle = angle
        return self.cAngle

    def distance(self, toolRadius):
        if not hasattr(self, 'cDist'):
            self.angle() # make sure the angles are initialized
            self.cDist = toolRadius / math.cos(self.tAngle/2)
        return self.cDist

    def corner(self, toolRadius):
        if not hasattr(self, 'cPt'):
            self.cPt = self.inChord.move(self.distance(toolRadius), self.angle()).End
        return self.cPt

    def location(self):
        return (self.inChord.End.x, self.inChord.End.y)

    def adaptiveLength(self, boneAngle, toolRadius):
        angle = self.angle()
        distance = self.distance(toolRadius)
        # there is something weird happening if the boneAngle came from a horizontal/vertical t-bone
        # for some reason pi/2 is not equal to pi/2
        if math.fabs(angle - boneAngle) < 0.00001:
            # moving directly towards the corner
            debugPrint("adaptive - on target: %.2f - %.2f" % (distance, toolRadius))
            return distance - toolRadius
        debugPrint("adaptive - angles: corner=%.2f  bone=%.2f diff=%.12f" % (angle/math.pi, boneAngle/math.pi, angle - boneAngle))

        # The bones root and end point form a triangle with the intersection of the tool path
        # with the toolRadius circle around the bone end point.
        # In case the math looks questionable, look for "triangle ssa"
        # c = distance
        # b = self.toolRadius
        # beta = fabs(boneAngle - angle)
        beta = math.fabs(addAngle(boneAngle, -angle))
        D = (distance / toolRadius) * math.sin(beta)
        if D > 1: # no intersection
            debugPrint("adaptive - no intersection - no bone")
            return 0
        gamma = math.asin(D)
        alpha = math.pi - beta - gamma
        length = toolRadius * math.sin(alpha) / math.sin(beta)
        if D < 1 and toolRadius < distance: # there exists a second solution
            beta2 = beta
            gamma2 = math.pi - gamma
            alpha2 = math.pi - beta2 - gamma2
            length2 = toolRadius * math.sin(alpha2) / math.sin(beta2)
            length = min(length, length2)

        debugPrint("adaptive corner=%.2f * %.2f˚ -> bone=%.2f * %.2f˚" % (distance, angle, length, boneAngle))
        return length

    def edges(self):
        if not hasattr(self, 'e'):
            self.e = edgesForCommands(self.commands, self.inChord.Start)
        return self.e

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
        self.dbg = []

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

    def findPivotIntersection(self, pivot, pivotEdge, edge, refPt, d, color):
        debugPrint("Intersection (%.2f, %.2f)^%.2f  - [(%.2f, %.2f), (%.2f, %.2f)]" % (pivotEdge.Curve.Center.x, pivotEdge.Curve.Center.y, pivotEdge.Curve.Radius, edge.Vertexes[0].Point.x, edge.Vertexes[0].Point.y, edge.Vertexes[1].Point.x, edge.Vertexes[1].Point.y))
        ppt = None
        pptDistance = 0
        for pt in DraftGeomUtils.findIntersection(edge, pivotEdge, dts=False):
            #debugMarker(pt, "pti.%d-%s.in" % (self.boneId, d), color, 0.2)
            distance = (pt - refPt).Length
            debugPrint("        -->  (%.2f, %.2f): %.2f" % (pt.x, pt.y, distance))
            if not ppt or pptDistance < distance:
                ppt = pt
                pptDistance = distance
        if not ppt:
            tangent = DraftGeomUtils.findDistance(pivot, edge)
            if tangent:
                debugPrint("Taking tangent as intersect %s" % tangent)
                ppt = pivot + tangent
            else:
                debugPrint("Taking chord start as intersect %s" % inChordStart)
                ppt = inChord.Start
            #debugMarker(ppt, "ptt.%d-%s.in" % (self.boneId, d), color, 0.2)
            debugPrint("        -->  (%.2f, %.2f)" % (ppt.x, ppt.y))
        return ppt

    def smoothChordCommands(self, bone, inChord, outChord, edge, wire, corner, smooth, color = None):
        if smooth == 0:
            debugPrint(" No smoothing requested")
            return [ bone.lastCommand, outChord.g1Command() ]

        d = 'in'
        refPoint = inChord.Start
        if smooth == Smooth.Out:
            d = 'out'
            refPoint = outChord.End

        if DraftGeomUtils.areColinear(inChord.asEdge(), outChord.asEdge()):
            debugPrint(" straight edge %s" % d)
            return [ outChord.g1Command() ]

        pivot = None
        pivotDistance = 0

        debugPrint("smooth:  (%.2f, %.2f)-(%.2f, %.2f)" % (edge.Vertexes[0].Point.x, edge.Vertexes[0].Point.y, edge.Vertexes[1].Point.x, edge.Vertexes[1].Point.y))
        for e in wire.Edges:
            self.dbg.append(e)
            if type(e.Curve) == Part.Line:
                debugPrint("         (%.2f, %.2f)-(%.2f, %.2f)" % (e.Vertexes[0].Point.x, e.Vertexes[0].Point.y, e.Vertexes[1].Point.x, e.Vertexes[1].Point.y))
            else:
                debugPrint("         (%.2f, %.2f)^%.2f" % (e.Curve.Center.x, e.Curve.Center.y, e.Curve.Radius))
            for pt in DraftGeomUtils.findIntersection(edge, e, True, findAll=True):
                if pt != corner:
                    debugPrint("         -> candidate")
                    distance = (pt - refPoint).Length
                    if not pivot or pivotDistance > distance:
                        pivot = pt
                        pivotDistance = distance
                else:
                    debugPrint("         -> corner intersect")

        if pivot:
            debugCircle(pivot, self.toolRadius, "pivot.%d-%s" % (self.boneId, d), color)

            pivotEdge = Part.Edge(Part.Circle(pivot, FreeCAD.Vector(0,0,1), self.toolRadius))
            t1 = self.findPivotIntersection(pivot, pivotEdge, inChord.asEdge(), inChord.End, d, color)
            t2 = self.findPivotIntersection(pivot, pivotEdge, outChord.asEdge(), inChord.End, d, color)

            commands = []
            if t1 != inChord.Start:
                debugPrint("  add lead in")
                commands.append(Chord(inChord.Start, t1).g1Command())
            if bone.obj.Side == Side.Left:
                debugPrint("  add g3 command")
                commands.append(Chord(t1, t2).g3Command(pivot))
            else:
                debugPrint("  add g2 command center=(%.2f, %.2f) -> from (%2f, %.2f) to (%.2f, %.2f" % (pivot.x, pivot.y, t1.x, t1.y, t2.x, t2.y))
                commands.append(Chord(t1, t2).g2Command(pivot))
            if t2 != outChord.End:
                debugPrint("  add lead out")
                commands.append(Chord(t2, outChord.End).g1Command())

            debugMarker(pivot, "pivot.%d-%s"     % (self.boneId, d), color, 0.2)
            debugMarker(t1,    "pivot.%d-%s.in"  % (self.boneId, d), color, 0.1)
            debugMarker(t2,    "pivot.%d-%s.out" % (self.boneId, d), color, 0.1)

            return commands

        debugPrint(" no pivot found - straight command")
        return [ inChord.g1Command(), outChord.g1Command() ]

    def inOutBoneCommands(self, bone, boneAngle, fixedLength):
        corner = bone.corner(self.toolRadius)

        bone.tip = bone.inChord.End # in case there is no bone

        debugPrint("corner = (%.2f, %.2f)" % (corner.x, corner.y))
        debugMarker(corner, 'corner', (1., 0., 1.), 0.3)

        length = fixedLength
        if bone.obj.Incision == Incision.Custom:
            length = bone.obj.Custom
        if bone.obj.Incision == Incision.Adaptive:
            length = bone.adaptiveLength(boneAngle, self.toolRadius)

        if length == 0:
            # no bone after all ..
            return [ bone.lastCommand, bone.outChord.g1Command() ]

        boneInChord = bone.inChord.move(length, boneAngle)
        boneOutChord = boneInChord.moveTo(bone.outChord.Start)

        #debugCircle(boneInChord.Start, self.toolRadius, 'boneStart')
        #debugCircle(boneInChord.End, self.toolRadius, 'boneEnd')

        bone.tip = boneInChord.End

        if bone.smooth == 0:
            return [ bone.lastCommand, boneInChord.g1Command(), boneOutChord.g1Command(), outChord.g1Command()]

        # reconstruct the corner and convert to an edge
        offset = corner - bone.inChord.End
        iChord = Chord(bone.inChord.Start + offset, bone.inChord.End + offset)
        oChord = Chord(bone.outChord.Start + offset, bone.outChord.End + offset)
        iLine = iChord.asLine()
        oLine = oChord.asLine()
        cornerShape = Part.Shape([iLine, oLine])

        # construct a shape representing the cut made by the bone
        vt0 = FreeCAD.Vector(     0,  self.toolRadius, 0)
        vt1 = FreeCAD.Vector(length,  self.toolRadius, 0)
        vb0 = FreeCAD.Vector(     0, -self.toolRadius, 0)
        vb1 = FreeCAD.Vector(length, -self.toolRadius, 0)
        vm2 = FreeCAD.Vector(length + self.toolRadius, 0, 0)

        boneBot = Part.Line(vb1, vb0)
        boneLid = Part.Line(vb0, vt0)
        boneTop = Part.Line(vt0, vt1)

        # what we actually want is an Arc - but findIntersect only returns the coincident if one exists
        # which really sucks because that's the one we're probably not interested in ....
        boneArc = Part.Arc(vt1, vm2, vb1)
        #boneArc = Part.Circle(FreeCAD.Vector(length, 0, 0), FreeCAD.Vector(0,0,1), self.toolRadius)
        boneWire = Part.Shape([boneTop, boneArc, boneBot, boneLid])
        boneWire.rotate(FreeCAD.Vector(0,0,0), FreeCAD.Vector(0,0,1), boneAngle * 180 / math.pi)
        boneWire.translate(bone.inChord.End)
        self.boneShapes = [cornerShape, boneWire]

        bone.inCommands  = self.smoothChordCommands(bone, bone.inChord,   boneInChord, Part.Edge(iLine), boneWire, corner, bone.smooth & Smooth.In,  (1., 0., 0.))
        bone.outCommands = self.smoothChordCommands(bone, boneOutChord, bone.outChord, Part.Edge(oLine), boneWire, corner, bone.smooth & Smooth.Out, (0., 1., 0.))
        return bone.inCommands + bone.outCommands

    def dogbone(self, bone):
        boneAngle = bone.angle()
        length = self.toolRadius * 0.41422 # 0.41422 = 2/sqrt(2) - 1 + (a tiny bit)
        return self.inOutBoneCommands(bone, boneAngle, length)

    def tboneHorizontal(self, bone):
        angle = bone.angle()
        boneAngle = 0
        if angle == math.pi or math.fabs(angle) > math.pi/2:
            boneAngle = -math.pi
        return self.inOutBoneCommands(bone, boneAngle, self.toolRadius)

    def tboneVertical(self, bone):
        angle = bone.angle()
        boneAngle = math.pi/2
        if angle == math.pi or angle < 0:
            boneAngle = -boneAngle
        return self.inOutBoneCommands(bone, boneAngle, self.toolRadius)

    def tboneEdgeCommands(self, bone, onIn):
        if onIn:
            boneAngle = bone.inChord.getAngleXY()
        else:
            boneAngle = bone.outChord.getAngleXY()

        if Side.Right == bone.outChord.getDirectionOf(bone.inChord):
            boneAngle = boneAngle - math.pi/2
        else:
            boneAngle = boneAngle + math.pi/2

        onInString = 'out'
        if onIn:
            onInString = 'in'
        debugPrint("tboneEdge boneAngle[%s]=%.2f   (in=%.2f, out=%.2f)" % (onInString, boneAngle/math.pi, bone.inChord.getAngleXY()/math.pi, bone.outChord.getAngleXY()/math.pi))
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
        elif hasattr(bone.obj.Base, 'BoneBlacklist'):
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
            return [ bone.lastCommand, bone.outChord.g1Command() ]

    def insertBone(self, bone):
        debugPrint(">----------------------------------- %d --------------------------------------" % bone.boneId)
        self.boneShapes = []
        blacklisted, inaccessible = self.boneIsBlacklisted(bone)
        enabled = not blacklisted
        self.bones.append((bone.boneId, bone.location(), enabled, inaccessible))

        self.boneId = bone.boneId
        if debugDressup and bone.boneId < 11:
            commands = self.boneCommands(bone, False)
        else:
            commands = self.boneCommands(bone, enabled)
        bone.commands = commands

        self.shapes[bone.boneId] = self.boneShapes
        debugPrint("<----------------------------------- %d --------------------------------------" % bone.boneId)
        return commands

    def removePathCrossing(self, commands, bone1, bone2):
        commands.append(bone2.lastCommand)
        bones = bone2.commands
        if True and hasattr(bone1, "outCommands") and hasattr(bone2, "inCommands"):
            inEdges  = edgesForCommands(bone1.outCommands, bone1.tip)
            outEdges = edgesForCommands(bone2.inCommands,  bone2.inChord.Start)
            for i in range(len(inEdges)):
                e1 = inEdges[i]
                for j in range(len(outEdges) -1, -1, -1):
                    e2 = outEdges[j]
                    cutoff = DraftGeomUtils.findIntersection(e1, e2)
                    for pt in cutoff:
                        #debugCircle(e1.Curve.Center, e1.Curve.Radius, "bone.%d-1" % (self.boneId), (1.,0.,0.))
                        #debugCircle(e2.Curve.Center, e2.Curve.Radius, "bone.%d-2" % (self.boneId), (0.,1.,0.))
                        if pt == e1.valueAt(e1.LastParameter) or pt == e2.valueAt(e2.FirstParameter):
                            continue
                        debugMarker(pt, "it", (0.0, 1.0, 1.0))
                        # 1. remove all redundant commands
                        commands = commands[:-(len(inEdges) - i)]
                        # 2., correct where c1 ends
                        c1 = bone1.outCommands[i]
                        c1Params = c1.Parameters
                        c1Params.update({'X': pt.x, 'Y': pt.y, 'Z': pt.z})
                        c1 = Path.Command(c1.Name, c1Params)
                        commands.append(c1)
                        # 3. change where c2 starts, this depends on the command itself
                        c2 = bone2.inCommands[j]
                        if c2.Name in movearc:
                            center = e2.Curve.Center
                            offset = center - pt
                            c2Params = c2.Parameters
                            c2Params.update({'I': offset.x, 'J': offset.y, 'K': offset.z})
                            c2 = Path.Command(c2.Name, c2Params)
                            bones = [c2]
                            bones.extend(bone2.commands[j+1:])
                        else:
                            bones = bone2.commands[j:]
                        # there can only be the one ...
                        return commands, bones

        return commands, bones

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
        lastBone = None         # track last bone for optimizations
        oddsAndEnds = []        # track chords that are connected to plunges - in case they form a loop

        boneId = 1
        self.bones = []
        self.locationBlacklist = set()
        boneIserted = False

        for thisCommand in obj.Base.Path.Commands:
            if thisCommand.Name in movecommands:
                thisChord = lastChord.moveToParameters(thisCommand.Parameters)
                thisIsACandidate = self.canAttachDogbone(thisCommand, thisChord)

                if thisIsACandidate and lastCommand and self.shouldInsertDogbone(obj, lastChord, thisChord):
                    bone = Bone(boneId, obj, lastCommand, lastChord, thisChord, Smooth.InAndOut)
                    bones = self.insertBone(bone)
                    boneId += 1
                    if lastBone:
                        #debugMarker(thisChord.Start, "it", (1.0, 0.0, 1.0))
                        commands, bones = self.removePathCrossing(commands, lastBone, bone)
                    commands.extend(bones[:-1])
                    lastCommand = bones[-1]
                    lastBone = bone
                elif lastCommand and thisChord.isAPlungeMove():
                    for chord in (chord for chord in oddsAndEnds if lastChord.connectsTo(chord)):
                        if self.shouldInsertDogbone(obj, lastChord, chord):
                            bone = Bone(boneId, obj, lastCommand, lastChord, chord, Smooth.In)
                            bones = self.insertBone(bone)
                            boneId += 1
                            if lastBone:
                                #debugMarker(chord.Start, "it", (0.0, 1.0, 1.0))
                                commands, bones = self.removePathCrossing(commands, lastBone, bone)
                            commands.extend(bones[:-1])
                            lastCommand = bones[-1]
                    lastCommand = None
                    commands.append(thisCommand)
                    lastBone = None
                elif thisIsACandidate:
                    if lastCommand:
                        commands.append(lastCommand)
                    lastCommand = thisCommand
                    lastBone = None
                else:
                    if lastCommand:
                        commands.append(lastCommand)
                        lastCommand = None
                    commands.append(thisCommand)
                    lastBone = None

                if lastChord.isAPlungeMove() and thisIsACandidate:
                    oddsAndEnds.append(thisChord)

                lastChord = thisChord
            else:
                if lastCommand:
                    commands.append(lastCommand)
                    lastCommand = None
                commands.append(thisCommand)
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

        if debugDressup:
            for self.obj in FreeCAD.ActiveDocument.Objects:
                if self.obj.Name.startswith('Shape'):
                    FreeCAD.ActiveDocument.removeObject(self.obj.Name)
            if hasattr(self.obj.Proxy, "shapes"):
                debugPrint("showing shapes attribute")
                for shapes in self.obj.Proxy.shapes.itervalues():
                    for shape in shapes:
                        Part.show(shape)
            else:
                debugPrint("no shapes attribute found")


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
        FreeCADGui.doCommand('obj.Base.ViewObject.Visibility = False')
        FreeCADGui.doCommand('dbo.setup(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Dogbone_Dressup', CommandDogboneDressup())

FreeCAD.Console.PrintLog("Loading DogboneDressup... done\n")
