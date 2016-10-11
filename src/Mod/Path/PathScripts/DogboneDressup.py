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

class Shape:
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

class Length:
    Fixed = 'fixed'
    Adaptive = 'adaptive'
    Custom = 'custom'
    All = [Adaptive, Fixed, Custom]

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

    def offsetBy(self, distance):
        angle = self.getAngleXY() - math.pi/2
        dx = distance * math.cos(angle)
        dy = distance * math.sin(angle)
        d = FreeCAD.Vector(dx, dy, 0)
        return Chord(self.Start + d, self.End + d)

    def g1Command(self):
        return Path.Command("G1", {"X": self.End.x, "Y": self.End.y})

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

        return (x, y)

class ObjectDressup:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLink", "Base","Base", "The base path to modify")
        obj.addProperty("App::PropertyEnumeration", "Side", "Dressup", "The side of path to insert bones")
        obj.Side = [Side.Left, Side.Right]
        obj.Side = Side.Right
        obj.addProperty("App::PropertyEnumeration", "Shape", "Dressup", "The shape of boness")
        obj.Shape = Shape.All
        obj.Shape = Shape.Dogbone
        obj.addProperty("App::PropertyIntegerList", "BoneBlacklist", "Dressup", "Bones that aren't dressed up")
        obj.BoneBlacklist = []
        obj.setEditorMode('BoneBlacklist', 2)  # hide this one
        obj.addProperty("App::PropertyEnumeration", "Length", "Dressup", "The algorithm to determine the bone length")
        obj.Length = Length.All
        obj.Length = Length.Fixed
        obj.addProperty("App::PropertyFloat", "Custom", "Dressup", "Dressup length if lenght == custom")
        obj.Custom = 0.0
        obj.Proxy = self

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

    # draw circles where dogbones go, easier to spot during testing
    def debugCircleBone(self, obj,inChord, outChord):
        di = 0.
        dj = 0.5
        if inChord.Start.x < outChord.End.x:
            dj = -dj
        circle = Path.Command("G1", {"I": di, "J": dj}).Parameters
        circle.update({"X": inChord.End.x, "Y": inChord.End.y})
        return [ Path.Command("G3", circle) ]

    def adaptiveBoneLength(self, obj, inChord, outChord, angle):
        iChord = inChord.offsetBy(self.toolRadius)
        oChord = outChord.offsetBy(self.toolRadius)
        x,y = iChord.intersection(oChord, self.toolRadius)
        dest = inChord.moveTo(FreeCAD.Vector(x, y, inChord.End.z))
        destAngle = dest.getAngleXY()
        distance = dest.getLength() - self.toolRadius * math.fabs(math.cos(destAngle - angle))
        #print("adapt")
        #print("  in  = %s -> %s" % (inChord, iChord))
        #print("  out = %s -> %s" % (outChord, oChord))
        #print("      = (%.2f, %.2f) -> %.2f (%.2f %.2f) -> %.2f" % (x, y, dest.getLength(), destAngle/math.pi, angle/math.pi, distance))
        return distance

    def inOutBoneCommands(self, obj, inChord, outChord, angle, fixedLength):
        length = fixedLength
        if obj.Length == Length.Custom:
            length = obj.Custom
        if obj.Length == Length.Adaptive:
            length = self.adaptiveBoneLength(obj, inChord, outChord, angle)
        x = length * math.cos(angle);
        y = length * math.sin(angle);
        boneChordIn = inChord.moveBy(x, y, 0)
        boneChordOut = boneChordIn.moveTo(outChord.Start)
        return [ boneChordIn.g1Command(), boneChordOut.g1Command() ]

    def dogboneAngle(self, obj, inChord, outChord):
        baseAngle = inChord.getAngleXY()
        turnAngle = outChord.getAngle(inChord)
        boneAngle = baseAngle + (turnAngle - math.pi)/2
        if obj.Side == Side.Left:
            boneAngle = boneAngle + math.pi
        while boneAngle < -math.pi:
            boneAngle += 2*math.pi
        while boneAngle > math.pi:
            boneAngle -= 2*math.pi
        #print("base=%+3.2f turn=%+3.2f bone=%+3.2f" % (baseAngle/math.pi, turnAngle/math.pi, boneAngle/math.pi))
        return boneAngle

    def dogbone(self, obj, inChord, outChord):
        boneAngle = self.dogboneAngle(obj, inChord, outChord)
        length = self.toolRadius * 0.41422 # 0.41422 = 2/sqrt(2) - 1 + (a tiny bit)
        return self.inOutBoneCommands(obj, inChord, outChord, boneAngle, length)

    def tboneHorizontal(self, obj, inChord, outChord):
        angle = self.dogboneAngle(obj, inChord, outChord)
        boneAngle = 0
        if angle == math.pi or math.fabs(angle) > math.pi/2:
            boneAngle = -math.pi
        return self.inOutBoneCommands(obj, inChord, outChord, boneAngle, self.toolRadius)

    def tboneVertical(self, obj, inChord, outChord):
        angle = self.dogboneAngle(obj, inChord, outChord)
        boneAngle = math.pi/2
        if angle == math.pi or angle < 0:
            boneAngle = -boneAngle
        return self.inOutBoneCommands(obj, inChord, outChord, boneAngle, self.toolRadius)

    def tboneEdgeCommands(self, obj, inChord, outChord, onIn):
        boneAngle = outChord.getAngleXY()
        if onIn:
            boneAngle = inChord.getAngleXY()
        boneAngle = boneAngle + math.pi/2
        if Side.Right == outChord.getDirectionOf(inChord):
            boneAngle = boneAngle - math.pi
        return self.inOutBoneCommands(obj, inChord, outChord, boneAngle, self.toolRadius)

    def tboneLongEdge(self, obj, inChord, outChord):
        inChordIsLonger = inChord.getLength() > outChord.getLength()
        return self.tboneEdgeCommands(obj, inChord, outChord, inChordIsLonger)

    def tboneShortEdge(self, obj, inChord, outChord):
        inChordIsShorter = inChord.getLength() < outChord.getLength()
        return self.tboneEdgeCommands(obj, inChord, outChord, inChordIsShorter)

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
    def boneCommands(self, obj, boneId, inChord, outChord):
        loc = (inChord.End.x, inChord.End.y)
        blacklisted, inaccessible = self.boneIsBlacklisted(obj, boneId, loc)
        enabled = not blacklisted
        self.bones.append((boneId, loc, enabled, inaccessible))

        if enabled:
            if obj.Shape == Shape.Dogbone:
                return self.dogbone(obj, inChord, outChord)
            if obj.Shape == Shape.Tbone_H:
                return self.tboneHorizontal(obj, inChord, outChord)
            if obj.Shape == Shape.Tbone_V:
                return self.tboneVertical(obj, inChord, outChord)
            if obj.Shape == Shape.Tbone_L:
                return self.tboneLongEdge(obj, inChord, outChord)
            if obj.Shape == Shape.Tbone_S:
                return self.tboneShortEdge(obj, inChord, outChord)
            return self.debugCircleBone(obj, inChord, outChord)
        else:
            return []

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
                    commands.extend(self.boneCommands(obj, boneId, lastChord, thisChord))
                    boneId = boneId + 1

                if lastCommand and thisChord.isAPlungeMove():
                    for chord in (chord for chord in oddsAndEnds if lastChord.connectsTo(chord)):
                        if self.shouldInsertDogbone(obj, lastChord, chord):
                            commands.extend(self.boneCommands(obj, boneId,lastChord, chord))
                            boneId = boneId + 1

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
                item[1].append(id)
            else:
                state[loc] = (enabled, inaccessible, [id])
        return state

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

class TaskPanel:
    DataIds = QtCore.Qt.ItemDataRole.UserRole
    DataKey = QtCore.Qt.ItemDataRole.UserRole + 1

    def __init__(self, obj):
        self.obj = obj
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/DogboneEdit.ui")
        self.updating = False

    def accept(self):
        self.getFields()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)
        FreeCAD.ActiveDocument.recompute()

    def reject(self):
        FreeCADGui.Control.closeDialog()
        FreeCAD.ActiveDocument.recompute()
        FreeCADGui.Selection.removeObserver(self.s)

    def getFields(self):
        self.obj.Shape = str(self.form.shape.currentText())
        self.obj.Side  = str(self.form.side.currentText())
        self.obj.Length = str(self.form.length.currentText())
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

    def updateModel(self):
        self.getFields()
        self.form.custom.setEnabled(self.obj.Length == Length.Custom)
        self.updateBoneList()
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
        self.setupCombo(self.form.shape, self.obj.Shape, Shape.All)
        self.setupCombo(self.form.side, self.obj.Side, Side.All)
        self.setupCombo(self.form.length, self.obj.Length, Length.All)
        self.form.custom.setMinimum(0.0)
        self.form.custom.setDecimals(3)
        self.form.custom.setValue(self.obj.Custom)
        self.form.custom.setEnabled(self.obj.Length == Length.Custom)
        self.updateBoneList()

    def open(self):
        self.s = SelObserver()
        # install the function mode resident
        FreeCADGui.Selection.addObserver(self.s)

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Ok)

    def setupUi(self):
        self.setFields()
        # now that the form is filled, setup the signal handlers
        self.form.shape.currentIndexChanged.connect(self.updateModel)
        self.form.side.currentIndexChanged.connect(self.updateModel)
        self.form.length.currentIndexChanged.connect(self.updateModel)
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

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Dogbone_Dressup', CommandDogboneDressup())

FreeCAD.Console.PrintLog("Loading DogboneDressup... done\n")
