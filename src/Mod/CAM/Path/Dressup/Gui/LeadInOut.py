#  -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2017 LTS <SammelLothar@gmx.de> under LGPL               *
# *   Copyright (c) 2020-2021 Schildkroet                                   *
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


import FreeCAD as App
import FreeCADGui
import Path
import Path.Base.Language as PathLanguage
import Path.Dressup.Utils as PathDressup
import PathScripts.PathUtils as PathUtils
from Path.Geom import wireForPath
import math

__doc__ = """LeadInOut Dressup USE ROLL-ON ROLL-OFF to profile"""

from PySide.QtCore import QT_TRANSLATE_NOOP

from PathPythonGui.simple_edit_panel import SimpleEditPanel

translate = App.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectDressup:
    def __init__(self, obj):
        lead_styles = [
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Arc"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Tangent"),
            QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "Perpendicular"),
        ]
        self.obj = obj
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base toolpath to modify"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "LeadIn",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Calculate roll-on to toolpath"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "LeadOut",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Calculate roll-off from toolpath"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "KeepToolDown",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Keep the Tool Down in toolpath"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "Length",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Length or Radius of the approach"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "LengthOut",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Length or Radius of the exit"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "StyleOn",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The Style of motion into the toolpath"),
        )
        obj.StyleOn = lead_styles
        obj.addProperty(
            "App::PropertyEnumeration",
            "StyleOff",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The Style of motion out of the toolpath"),
        )
        obj.StyleOff = lead_styles
        obj.addProperty(
            "App::PropertyDistance",
            "ExtendLeadIn",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Extends LeadIn distance"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "ExtendLeadOut",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Extends LeadOut distance"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "RapidPlunge",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Perform plunges with G0"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "IncludeLayers",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "Apply LeadInOut to layers within an operation"
            ),
        )
        obj.Proxy = self

        self.wire = None
        self.rapids = None

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def setup(self, obj):
        obj.Length = PathDressup.toolController(obj.Base).Tool.Diameter * 0.75
        obj.LengthOut = PathDressup.toolController(obj.Base).Tool.Diameter * 0.75
        obj.LeadIn = True
        obj.LeadOut = True
        obj.KeepToolDown = False
        obj.StyleOn = "Arc"
        obj.StyleOff = "Arc"
        obj.ExtendLeadIn = 0
        obj.ExtendLeadOut = 0
        obj.RapidPlunge = False
        obj.IncludeLayers = True

    def execute(self, obj):
        if not obj.Base:
            return
        if not obj.Base.isDerivedFrom("Path::Feature"):
            return
        if not obj.Base.Path:
            return

        if obj.Length <= 0:
            Path.Log.error(
                translate("CAM_DressupLeadInOut", "Length/Radius positive not Null")
                + "\n"
            )
            obj.Length = 0.1

        if obj.LengthOut <= 0:
            Path.Log.error(
                translate("CAM_DressupLeadInOut", "Length/Radius positive not Null")
                + "\n"
            )
            obj.LengthOut = 0.1

        self.wire, self.rapids = wireForPath(PathUtils.getPathWithPlacement(obj.Base))
        obj.Path = self.generateLeadInOutCurve(obj)

    def getDirectionOfPath(self, obj):
        op = PathDressup.baseOp(obj.Base)
        side = op.Side if hasattr(op, "Side") else "Inside"
        direction = op.Direction if hasattr(op, "Direction") else "CCW"

        if side == "Outside":
            return "left" if direction == "CW" else "right"
        else:
            return "right" if direction == "CW" else "left"

    def getArcDirection(self, obj):
        direction = self.getDirectionOfPath(obj)
        return math.pi / 2 if direction == "left" else -math.pi / 2

    def getTravelStart(self, obj, pos, first):
        op = PathDressup.baseOp(obj.Base)
        vertfeed = PathDressup.toolController(obj.Base).VertFeed.Value
        travel = []

        # begin positions for travel and plunge moves are not used anywhere,
        # skipping them makes our life a lot easier

        # move to clearance height
        if first:
            travel.append(PathLanguage.MoveStraight(
                None, "G0", {"Z": op.ClearanceHeight.Value}))

        # move to correct xy-position
        travel.append(PathLanguage.MoveStraight(
            None, "G0", {"X": pos.x, "Y": pos.y}))

        # move to correct z-position (either rapidly or in two steps)
        if obj.RapidPlunge:
            travel.append(PathLanguage.MoveStraight(None, "G0", {"Z": pos.z}))
        else:
            if first or not obj.KeepToolDown:
                travel.append(PathLanguage.MoveStraight(
                    None, "G0", {"Z": op.SafeHeight.Value}))
            travel.append(PathLanguage.MoveStraight(
                None, "G1", {"Z": pos.z, "F": vertfeed}))

        return travel

    def getTravelEnd(self, obj, pos, last):
        op = PathDressup.baseOp(obj.Base)
        travel = []

        # move to clearance height
        if last or not obj.KeepToolDown:
            travel.append(PathLanguage.MoveStraight(
                None, "G0", {"Z": op.ClearanceHeight.Value}))

        return travel

    def angleToVector(self, angle):
        return App.Vector(math.cos(angle), math.sin(angle), 0)

    def createArcMove(self, obj, begin, end, c):
        horizfeed = PathDressup.toolController(obj.Base).HorizFeed.Value

        param = {"X": end.x, "Y": end.y, "I": c.x, "J": c.y, "F": horizfeed}
        if self.getArcDirection(obj) > 0:
            return PathLanguage.MoveArcCCW(begin, "G3", param)
        else:
            return PathLanguage.MoveArcCW(begin, "G2", param)

    def createStraightMove(self, obj, begin, end):
        horizfeed = PathDressup.toolController(obj.Base).HorizFeed.Value

        param = {"X": end.x, "Y": end.y, "F": horizfeed}
        return PathLanguage.MoveStraight(begin, "G1", param)

    def getLeadStart(self, obj, move, first):
        lead = []
        begin = move.positionBegin()

        def prepend(instr):
            nonlocal lead
            nonlocal begin
            lead.insert(0, instr)
            begin = lead[0].positionBegin()

        #    tangent  begin      move
        #    <----_-----x-------------------x
        #       /       |
        #     /         | normal
        #    |          |
        #    x          v

        if obj.LeadIn:
            length = obj.Length.Value
            angle = move.anglesOfTangents()[0]
            tangent = -self.angleToVector(angle) * length
            normal = self.angleToVector(
                angle + self.getArcDirection(obj)) * length

            # prepend the selected lead-in
            if obj.StyleOn == "Arc":
                arcbegin = begin + tangent + normal
                prepend(self.createArcMove(obj, arcbegin, begin, -tangent))
            elif obj.StyleOn == "Tangent":
                prepend(self.createStraightMove(obj, begin + tangent, begin))
            else:  # obj.StyleOn == "Perpendicular"
                prepend(self.createStraightMove(obj, begin + normal, begin))

            extend = obj.ExtendLeadIn.Value
            if extend != 0:
                # prepend extension
                extendbegin = begin + normal / length * extend
                prepend(self.createStraightMove(obj, extendbegin, begin))

        # prepend travel moves
        lead = self.getTravelStart(obj, begin, first) + lead

        return lead

    def getLeadEnd(self, obj, move, last):
        lead = []
        end = move.positionEnd()

        def append(instr):
            nonlocal lead
            nonlocal end
            lead.append(instr)
            end = lead[-1].positionEnd()

        #            move       end   tangent
        #    x-------------------x-----_---->
        #                        |       \
        #                 normal |         \
        #                        |          |
        #                        v          x

        if obj.LeadOut:
            length = obj.LengthOut.Value
            angle = move.anglesOfTangents()[1]
            tangent = self.angleToVector(angle) * length
            normal = self.angleToVector(
                angle + self.getArcDirection(obj)) * length

            # append the selected lead-out
            if obj.StyleOff == "Arc":
                arcend = end + tangent + normal
                append(self.createArcMove(obj, end, arcend, normal))
            elif obj.StyleOff == "Tangent":
                append(self.createStraightMove(obj, end, end + tangent))
            else:  # obj.StyleOff == "Perpendicular"
                append(self.createStraightMove(obj, end, end + normal))

            extend = obj.ExtendLeadOut.Value 
            if extend != 0:
                # append extension
                extendend = end + normal / length * extend
                append(self.createStraightMove(obj, end, extendend))

        # append travel moves
        lead += self.getTravelEnd(obj, end, last)

        return lead

    def isCuttingMove(self, obj, instr):
        return (instr.isMove()
                and not instr.isRapid()
                and (not obj.IncludeLayers or not instr.isPlunge()))

    def findLastCuttingMoveIndex(self, obj, source):
        for i in range(len(source) - 1, -1, -1):
            if self.isCuttingMove(obj, source[i]):
                return i
        return None

    def generateLeadInOutCurve(self, obj):
        source = PathLanguage.Maneuver.FromPath(
            PathUtils.getPathWithPlacement(obj.Base)).instr
        maneuver = PathLanguage.Maneuver()

        # Knowing weather a given instruction is the first cutting move is easy,
        # we just use a flag and set it to false afterwards. To find the last
        # cutting move we need to search the list in reverse order.
        first = True
        lastCuttingMoveIndex = self.findLastCuttingMoveIndex(obj, source)

        for i, instr in enumerate(source):
            if not self.isCuttingMove(obj, instr):
                # non-move instructions get added verbatim
                if not instr.isMove():
                    maneuver.addInstruction(instr)

                # skip travel and plunge moves, travel moves will be added in
                # getLeadStart and getLeadEnd
                continue

            if first or not self.isCuttingMove(obj, source[i - 1]):
                # add lead start and travel moves
                maneuver.addInstructions(self.getLeadStart(obj, instr, first))
                first = False

            # add current move
            maneuver.addInstruction(instr)

            last = i == lastCuttingMoveIndex
            if last or not self.isCuttingMove(obj, source[i + 1]):
                # add lead end and travel moves
                maneuver.addInstructions(self.getLeadEnd(obj, instr, last))

        return maneuver.toPath()


class TaskDressupLeadInOut(SimpleEditPanel):
    _transaction_name = "Edit LeadInOut Dress-up"
    _ui_file = ":/panels/DressUpLeadInOutEdit.ui"

    def setupUi(self):
        self.connectWidget("LeadIn", self.form.chkLeadIn)
        self.connectWidget("LeadOut", self.form.chkLeadOut)
        self.connectWidget("Length", self.form.dspLenIn)
        self.connectWidget("LengthOut", self.form.dspLenOut)
        self.connectWidget("ExtendLeadIn", self.form.dspExtendIn)
        self.connectWidget("ExtendLeadOut", self.form.dspExtendOut)
        self.connectWidget("StyleOn", self.form.cboStyleIn)
        self.connectWidget("StyleOff", self.form.cboStyleOut)
        self.connectWidget("RapidPlunge", self.form.chkRapidPlunge)
        self.connectWidget("IncludeLayers", self.form.chkLayers)
        self.connectWidget("KeepToolDown", self.form.chkKeepToolDown)
        self.setFields()


class ViewProviderDressup:
    def __init__(self, vobj):
        self.obj = vobj.Object
        self.setEdit(vobj)

    def attach(self, vobj):
        self.obj = vobj.Object
        self.panel = None

    def claimChildren(self):
        if hasattr(self.obj.Base, "InList"):
            for i in self.obj.Base.InList:
                if hasattr(i, "Group"):
                    group = i.Group
                    for g in group:
                        if g.Name == self.obj.Base.Name:
                            group.remove(g)
                    i.Group = group
        return [self.obj.Base]

    def setEdit(self, vobj, mode=0):
        FreeCADGui.Control.closeDialog()
        panel = TaskDressupLeadInOut(vobj.Object, self)
        FreeCADGui.Control.showDialog(panel)
        return True

    def unsetEdit(self, vobj, mode=0):
        if self.panel:
            self.panel.abort()

    def onDelete(self, arg1=None, arg2=None):
        """this makes sure that the base operation is added back to the project and visible"""
        Path.Log.debug("Deleting Dressup")
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(self.obj)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def clearTaskPanel(self):
        self.panel = None


class CommandPathDressupLeadInOut:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupLeadInOut", "LeadInOut"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_DressupLeadInOut",
                "Creates a Cutter Radius Compensation G41/G42 Entry Dressup object from a selected path",
            ),
        }

    def IsActive(self):
        op = PathDressup.selection()
        if op:
            return not PathDressup.hasEntryMethod(op)
        return False

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            Path.Log.error(
                translate("CAM_DressupLeadInOut", "Please select one toolpath object")
                + "\n"
            )
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            Path.Log.error(
                translate("CAM_DressupLeadInOut", "The selected object is not a toolpath")
                + "\n"
            )
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            Path.Log.error(
                translate("CAM_DressupLeadInOut", "Please select a Profile object")
            )
            return

        # everything ok!
        App.ActiveDocument.openTransaction("Create LeadInOut Dressup")
        FreeCADGui.addModule("Path.Dressup.Gui.LeadInOut")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "LeadInOutDressup")'
        )
        FreeCADGui.doCommand("dbo = Path.Dressup.Gui.LeadInOut.ObjectDressup(obj)")
        FreeCADGui.doCommand("base = FreeCAD.ActiveDocument." + selection[0].Name)
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(base)")
        FreeCADGui.doCommand("obj.Base = base")
        FreeCADGui.doCommand("job.Proxy.addOperation(obj, base)")
        FreeCADGui.doCommand("dbo.setup(obj)")
        FreeCADGui.doCommand(
            "obj.ViewObject.Proxy = Path.Dressup.Gui.LeadInOut.ViewProviderDressup(obj.ViewObject)"
        )
        FreeCADGui.doCommand(
            "Gui.ActiveDocument.getObject(base.Name).Visibility = False"
        )
        App.ActiveDocument.commitTransaction()
        App.ActiveDocument.recompute()


if App.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupLeadInOut", CommandPathDressupLeadInOut())

Path.Log.notice("Loading CAM_DressupLeadInOut... done\n")
