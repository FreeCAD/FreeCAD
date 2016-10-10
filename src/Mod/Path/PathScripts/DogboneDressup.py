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

    def getLength(self):
        return self.asVector().Length

    def getDirectionOfVector(self, B):
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
            return self.getDirectionOfVector(chordOrVector.asVector())
        return self.getDirectionOfVector(chordOrVector)

    def getAngleOfVector(self, ref):
        angle = self.asVector().getAngle(ref)
        # unfortunately they never figure out the sign :(
        # positive angles go up, so when the reference vector is left
        # then the receiver must go down
        if self.getDirectionOfVector(ref) == 'Left':
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

    LabelDogbone = 'Dogbone'
    LabelTbone_H = 'T-bone horizontal'
    LabelTbone_V = 'T-bone vertical'
    LabelTbone_L = 'T-bone long edge'
    LabelTbone_S = 'T-bone short edge'

    def __init__(self, obj):
        obj.addProperty("App::PropertyLink", "Base","Base", "The base path to modify")
        obj.addProperty("App::PropertyEnumeration", "Side", "Dressup", "The side of path to insert dog-bones")
        obj.Side = ['Left', 'Right']
        obj.Side = 'Right'
        obj.addProperty("App::PropertyEnumeration", "Shape", "Dressup", "The shape of dogboness")
        obj.Shape = [ObjectDressup.LabelDogbone, ObjectDressup.LabelTbone_H, ObjectDressup.LabelTbone_V, ObjectDressup.LabelTbone_L, ObjectDressup.LabelTbone_S]
        obj.Shape = ObjectDressup.LabelDogbone
        obj.addProperty("App::PropertyIntegerList", "BoneBlacklist", "", "Bones that aren't dressed up")
        obj.setEditorMode('BoneBlacklist', 2)  # hide this one
        obj.BoneBlacklist = []
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
    def debugCircleBone(self, obj,inChord, outChord):
        di = 0.
        dj = 0.5
        if inChord.Start.x < outChord.End.x:
            dj = -dj
        circle = Path.Command("G1", {"I": di, "J": dj}).Parameters
        circle.update({"X": inChord.End.x, "Y": inChord.End.y})
        return [ Path.Command("G3", circle) ]

    def inOutBoneCommands(self, obj, inChord, outChord, angle, length):
        x = length * math.cos(angle);
        y = length * math.sin(angle);
        boneChordIn = inChord.moveBy(x, y, 0)
        boneChordOut = boneChordIn.moveTo(outChord.Start)
        return [ boneChordIn.g1Command(), boneChordOut.g1Command() ]

    def dogboneAngle(self, obj, inChord, outChord):
        baseAngle = inChord.getAngleXY()
        turnAngle = outChord.getAngle(inChord)
        boneAngle = baseAngle + (turnAngle - math.pi)/2
        if obj.Side == 'Left':
            boneAngle = boneAngle + math.pi
        while boneAngle < -math.pi:
            boneAngle += 2*math.pi
        while boneAngle > math.pi:
            boneAngle -= 2*math.pi
        #print("base=%+3.2f turn=%+3.2f bone=%+3.2f" % (baseAngle/math.pi, turnAngle/math.pi, boneAngle/math.pi))
        return boneAngle

    def dogbone(self, obj, inChord, outChord):
        boneAngle = self.dogboneAngle(obj, inChord, outChord)
        length = self.toolRadius * 0.2929 # 0.2929 = 1 - 1/sqrt(2) + (a tiny bit)
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
        if 'Right' == outChord.getDirectionOf(inChord):
            boneAngle = boneAngle - math.pi
        return self.inOutBoneCommands(obj, inChord, outChord, boneAngle, self.toolRadius)

    def tboneLongEdge(self, obj, inChord, outChord):
        inChordIsLonger = inChord.getLength() > outChord.getLength()
        return self.tboneEdgeCommands(obj, inChord, outChord, inChordIsLonger)

    def tboneShortEdge(self, obj, inChord, outChord):
        inChordIsShorter = inChord.getLength() < outChord.getLength()
        return self.tboneEdgeCommands(obj, inChord, outChord, inChordIsShorter)

    def isBoneBlacklisted(self, obj, boneId, loc):
        blacklisted = False
        if boneId in obj.BoneBlacklist:
            blacklisted = True
        elif loc in self.locationBlacklist:
            obj.BoneBlacklist.append(boneId)
            blacklisted = True
        if blacklisted:
            self.locationBlacklist.add(loc)
        return blacklisted

    # Generate commands necessary to execute the dogbone
    def boneCommands(self, obj, boneId, inChord, outChord):
        loc = (inChord.End.x, inChord.End.y)
        enabled = not self.isBoneBlacklisted(obj, boneId, loc)
        self.bones.append((boneId, loc, enabled))

        if enabled:
            if obj.Shape == ObjectDressup.LabelDogbone:
                return self.dogbone(obj, inChord, outChord)
            if obj.Shape == ObjectDressup.LabelTbone_H:
                return self.tboneHorizontal(obj, inChord, outChord)
            if obj.Shape == ObjectDressup.LabelTbone_V:
                return self.tboneVertical(obj, inChord, outChord)
            if obj.Shape == ObjectDressup.LabelTbone_L:
                return self.tboneLongEdge(obj, inChord, outChord)
            if obj.Shape == ObjectDressup.LabelTbone_S:
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
            # By default the side for dogbones is opposite of the base path side
            if obj.Base.Side == 'Left':
                obj.Side = 'Right'
            elif obj.Base.Side == 'Right':
                obj.Side = 'Left'
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
        for (id, loc, enabled) in self.bones:
            item = state.get(loc)
            if item:
                item[1].append(id)
            else:
                state[loc] = (enabled, [id])
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
        blacklist = []
        for i in range(0, self.form.bones.count()):
            item = self.form.bones.item(i)
            if item.checkState() == QtCore.Qt.CheckState.Unchecked:
                blacklist.extend(item.data(self.DataIds))
        self.obj.BoneBlacklist = sorted(blacklist)
        self.obj.Proxy.execute(self.obj)

    def updateModel(self):
        self.getFields()
        FreeCAD.ActiveDocument.recompute()

    def comboSelectText(self, combo, text):
        index = combo.findText(text, QtCore.Qt.MatchFixedString)
        if index >= 0:
            combo.setCurrentIndex(index)

    def setFields(self):
        # If the dressup was loaded from disk the Proxy might not be seupt properly
        self.comboSelectText(self.form.shape, self.obj.Shape)
        self.comboSelectText(self.form.side, self.obj.Side)
        self.form.bones.clear()
        itemList = []
        for loc, state in self.obj.Proxy.boneStateList(self.obj).iteritems():
            lbl = '(%.2f, %.2f): %s' % (loc[0], loc[1], ','.join(str(id) for id in state[1]))
            item = QtGui.QListWidgetItem(lbl)
            item.setFlags(QtCore.Qt.ItemFlag.ItemIsEnabled | QtCore.Qt.ItemFlag.ItemIsSelectable | QtCore.Qt.ItemFlag.ItemIsUserCheckable)
            if state[0]:
                item.setCheckState(QtCore.Qt.CheckState.Checked)
            else:
                item.setCheckState(QtCore.Qt.CheckState.Unchecked)
            item.setData(self.DataIds, state[1])
            item.setData(self.DataKey, state[1][0])
            itemList.append(item)
        for item in sorted(itemList, key=lambda item: item.data(self.DataKey)):
            self.form.bones.addItem(item)

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
