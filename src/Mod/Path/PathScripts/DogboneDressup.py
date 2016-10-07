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
        return "Chord([%g, %g, %g] -> [%g, %g, %g]) -> %s" % (self.Start.x, self.Start.y, self.Start.z, self.End.x, self.Start.y, self.Start.z, self.End - self.Start)

    def moveTo(self, newEnd):
        #print("Chord(%s -> %s)" % (self.End, newEnd))
        return Chord(self.End, newEnd)

    def moveToParameters(self, params):
        x = params.get('X', self.End.x)
        y = params.get('Y', self.End.y)
        z = params.get('Z', self.End.z)
        return self.moveTo(FreeCAD.Vector(x, y, z))

    def moveBy(self, x, y, z):
        return self.moveTo(self.End + FreeCAD.Vector(x, y, z))

    def asVector(self):
        return self.End - self.Start

    def directionOfVector(self, B):
        A = self.asVector()
        # if the 2 vectors are identical, they head in the same direction
        if A == B:
            return 'Straight'
        d = -A.x*B.y + A.y*B.x
        if d < 0:
            return 'Left'
        if d > 0:
            return 'Right'
        # at this point the only direction left is backwards
        return 'Back'

    def getDirectionOf(self, chordOrVector):
        if type(chordOrVector) is Chord:
            return self.directionOfVector(chordOrVector.asVector())
        return self.directionOfVector(chordOrVector)

    def getAngleOfVector(self, ref):
        angle = self.asVector().getAngle(ref)
        # unfortunately they never figure out the sign :(
        # positive angles go up, so when the reference vector is left
        # then the receiver must go down
        if self.directionOfVector(ref) == 'Left':
            return -angle
        return angle

    def getAngle(self, refChordOrVector):
        if type(refChordOrVector) is Chord:
            return self.getAngleOfVector(refChordOrVector.asVector())
        return self.getAngleOfVector(refChordOrVector)

    def getAngleXY(self):
        return self.getAngle(FreeCAD.Vector(1,0,0))

    def g1Command(self):
        return Path.Command("G1", {"X": self.End.x, "Y": self.End.y})

    def isAPlungeMove(self):
        return self.End.z != self.Start.z

    def foldsBackOrTurns(self, chord, side):
        dir = chord.getDirectionOf(self)
        return dir == 'Back' or dir == side

    def connectsTo(self, chord):
        return self.End == chord.Start


class ObjectDressup:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLink", "Base","Path", "The base path to modify")
        obj.addProperty("App::PropertyEnumeration", "Side", "Side", "side of path to insert dog-bones")
        obj.Side = ['Left', 'Right']
        obj.Side = 'Right'
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def theOtherSideOf(self, side):
        if side == 'Left':
            return 'Right'
        return 'Left'

    # Answer true if a dogbone could be on either end of the chord, given its command
    def canAttachDogbone(self, cmd, chord):
        return cmd.Name in movestraight and not chord.isAPlungeMove()

    def shouldInsertDogbone(self, obj, inChord, outChord):
        return outChord.foldsBackOrTurns(inChord, self.theOtherSideOf(obj.Side))

    # draw circles where dogbones go, easier to spot during testing
    def debugCircleBone(self, inChord, outChord):
        di = 0.
        dj = 0.5
        if inChord.Start.x < outChord.End.x:
            dj = -dj
        circle = Path.Command("G1", {"I": di, "J": dj}).Parameters
        circle.update({"X": inChord.End.x, "Y": inChord.End.y})
        return [ Path.Command("G3", circle) ]

    def dogbone(self, obj, inChord, outChord):
        baseAngle = inChord.getAngleXY()
        turnAngle = outChord.getAngle(inChord)
        boneAngle = baseAngle + (turnAngle - math.pi)/2
        if obj.Side == 'Left':
            boneAngle = boneAngle + math.pi
        #print("base=%+3.2f turn=%+3.2f bond=%+3.2f" % (baseAngle/math.pi, turnAngle/math.pi, boneAngle/math.pi))
        x = self.toolRadius * math.cos(boneAngle) * 0.2929 # 0.2929 = 1 - 1/sqrt(2) + (a tiny bit)
        y = self.toolRadius * math.sin(boneAngle) * 0.2929 # 0.2929 = 1 - 1/sqrt(2) + (a tiny bit)
        boneChordIn = inChord.moveBy(x, y, 0)
        boneChordOut = boneChordIn.moveTo(outChord.Start)
        return [ boneChordIn.g1Command(), boneChordOut.g1Command() ]

    # Generate commands necessary to execute the dogbone
    def dogboneCommands(self, obj, inChord, outChord):
        return self.dogbone(obj, inChord, outChord)

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

        for thisCmd in obj.Base.Path.Commands:
            if thisCmd.Name in movecommands:
                thisChord = lastChord.moveToParameters(thisCmd.Parameters)
                thisIsACandidate = self.canAttachDogbone(thisCmd, thisChord)

                if thisIsACandidate and lastCommand and self.shouldInsertDogbone(obj, lastChord, thisChord):
                    dogbone = self.dogboneCommands(obj, lastChord, thisChord)
                    commands.extend(dogbone)

                if lastCommand and thisChord.isAPlungeMove():
                    for chord in (chord for chord in oddsAndEnds if lastChord.connectsTo(chord)):
                        if self.shouldInsertDogbone(obj, lastChord, chord):
                            commands.extend(self.dogboneCommands(obj, lastChord, chord))

                if lastChord.isAPlungeMove() and thisIsACandidate:
                    oddsAndEnds.append(thisChord)

                if thisIsACandidate:
                    lastCommand = thisCmd
                else:
                    lastCommand = None

                lastChord = thisChord
            commands.append(thisCmd)
        path = Path.Path(commands)
        obj.Path = path

    def setup(self, obj):
        if not hasattr(self, 'toolRadius'):
            print("Here we go ... ")
            # By default the side for dogbones is opposite of the base path side
            if obj.Base.Direction == 'CCW':
                obj.Side = obj.Base.Side
            else:
                if obj.Base.Side == 'Left':
                    obj.Side = 'Right'
                elif obj.Base.Side == 'Right':
                    obj.Side = 'Left'
                else:
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
        if not selection[0].isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(translate("Dogbone_Dressup", "The selected object is not a path\n"))
            return
        if selection[0].isDerivedFrom("Path::FeatureCompoundPython"):
            FreeCAD.Console.PrintError(translate("Dogbone_Dressup", "Please select a Path object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate("Dogbone_Dressup", "Create Dress-up"))
        FreeCADGui.addModule("PathScripts.DogboneDressup")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "DogboneDressup")')
        FreeCADGui.doCommand('dbo = PathScripts.DogboneDressup.ObjectDressup(obj)')
        FreeCADGui.doCommand('obj.Base = FreeCAD.ActiveDocument.' + selection[0].Name)
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
