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
import Path.Dressup.Utils as PathDressup
import PathScripts.PathUtils as PathUtils
from Path.Geom import CmdMoveRapid, CmdMoveStraight, CmdMoveArc, wireForPath

__doc__ = """LeadInOut Dressup USE ROLL-ON ROLL-OFF to profile"""

from PySide.QtCore import QT_TRANSLATE_NOOP

from PathPythonGui.simple_edit_panel import SimpleEditPanel

translate = App.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


movecommands = CmdMoveStraight + CmdMoveArc
currLocation = {}


class ObjectDressup:
    def __init__(self, obj):
        lead_styles = [
            QT_TRANSLATE_NOOP("Path_DressupLeadInOut", "Arc"),
            QT_TRANSLATE_NOOP("Path_DressupLeadInOut", "Tangent"),
            QT_TRANSLATE_NOOP("Path_DressupLeadInOut", "Perpendicular"),
        ]
        self.obj = obj
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base path to modify"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "LeadIn",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Calculate roll-on to path"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "LeadOut",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Calculate roll-off from path"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "KeepToolDown",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Keep the Tool Down in Path"),
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
            QT_TRANSLATE_NOOP("App::Property", "The Style of motion into the Path"),
        )
        obj.StyleOn = lead_styles
        obj.addProperty(
            "App::PropertyEnumeration",
            "StyleOff",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The Style of motion out of the Path"),
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
        obj.Length = obj.Base.ToolController.Tool.Diameter * 0.75
        obj.LengthOut = obj.Base.ToolController.Tool.Diameter * 0.75
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
                translate("Path_DressupLeadInOut", "Length/Radius positive not Null")
                + "\n"
            )
            obj.Length = 0.1

        if obj.LengthOut <= 0:
            Path.Log.error(
                translate("Path_DressupLeadInOut", "Length/Radius positive not Null")
                + "\n"
            )
            obj.LengthOut = 0.1

        self.wire, self.rapids = wireForPath(PathUtils.getPathWithPlacement(obj.Base))
        obj.Path = self.generateLeadInOutCurve(obj)

    def getDirectionOfPath(self, obj):
        op = PathDressup.baseOp(obj.Base)

        if hasattr(op, "Side") and op.Side == "Outside":
            return (
                "left" if hasattr(op, "Direction") and op.Direction == "CW" else "right"
            )
        else:
            return (
                "right" if hasattr(op, "Direction") and op.Direction == "CW" else "left"
            )

    def getSideOfPath(self, obj):
        op = PathDressup.baseOp(obj.Base)
        return op.Side if hasattr(op, "Side") else ""

    def getLeadStart(self, obj, queue, action):
        """returns Lead In G-code."""
        # Modified March 2022 by lcorley to support leadin extension
        results = []
        op = PathDressup.baseOp(obj.Base)
        horizFeed = PathDressup.toolController(obj.Base).HorizFeed.Value
        vertFeed = PathDressup.toolController(obj.Base).VertFeed.Value

        arcs_identical = False

        # Set the correct twist command
        arcdir = "G3" if self.getDirectionOfPath(obj) == "left" else "G2"

        if queue[1].Name == "G1":  # line
            p0 = queue[0].Placement.Base
            p1 = queue[1].Placement.Base
            v = App.Vector(p1.sub(p0)).normalize()
            Path.Log.debug(" CURRENT_IN Line : P0 Z:{} p1 Z:{}".format(p0.z, p1.z))
        else:
            p0 = queue[0].Placement.Base
            p1 = queue[1].Placement.Base
            Path.Log.track()
            v = App.Vector(p1.sub(p0)).normalize()
            Path.Log.debug(
                " CURRENT_IN ARC : P0 X:{} Y:{} P1 X:{} Y:{} Z:{}".format(
                    p0.x, p0.y, p1.x, p1.y, p1.z
                )
            )

        # Calculate offset vector (will be overwritten for arcs)
        if self.getDirectionOfPath(obj) == "right":
            off_v = App.Vector(v.y * obj.Length.Value, -v.x * obj.Length.Value, 0.0)
        else:
            off_v = App.Vector(-v.y * obj.Length.Value, v.x * obj.Length.Value, 0.0)

        # Check if we enter at line or arc command
        if queue[1].Name in movecommands and queue[1].Name not in CmdMoveArc:
            # We have a line move
            vec = p1.sub(p0)
            vec_n = App.Vector(vec).normalize()
            vec_inv = vec_n
            vec_inv.multiply(-1)
            vec_off = vec_inv
            vec_off.multiply(obj.ExtendLeadIn)
            Path.Log.debug(
                "LineCMD: {}, Vxinv: {}, Vyinv: {}, Vxoff: {}, Vyoff: {}".format(
                    queue[0].Name, vec_inv.x, vec_inv.y, vec_off.x, vec_off.y
                )
            )
        else:
            # We have an arc move
            # Calculate coordinates for middle of circle
            pij = App.Vector(p0)
            pij.x += queue[1].Parameters["I"]
            pij.y += queue[1].Parameters["J"]

            # Check if lead in and operation go in same direction (usually for inner circles)
            if arcdir == queue[1].Name:
                arcs_identical = True

            # Calculate vector circle start -> circle middle
            vec_circ = pij.sub(p0)

            angle = 90 if arcdir == "G2" else -90
            vec_rot = App.Rotation(App.Vector(0, 0, 1), angle).multVec(vec_circ)

            # Normalize and invert vector
            vec_n = App.Vector(vec_rot).normalize()
            v = App.Vector(vec_n).multiply(-1)

            # Calculate offset of lead in
            if arcdir == "G3":
                off_v = App.Vector(-v.y * obj.Length.Value, v.x * obj.Length.Value, 0.0)
            else:
                off_v = App.Vector(v.y * obj.Length.Value, -v.x * obj.Length.Value, 0.0)

        offsetvector = App.Vector(
            v.x * obj.Length.Value, v.y * obj.Length.Value, 0
        )

        if obj.StyleOn == "Arc":
            leadstart = (p0.add(off_v)).sub(offsetvector)
            if arcs_identical:
                t = p0.sub(leadstart)
                t = p0.add(t)
                leadstart = t
                offsetvector = offsetvector.multiply(-1)
        elif obj.StyleOn == "Tangent":
            # This is wrong.  please fix
            leadstart = (p0.add(off_v)).sub(offsetvector)
            if arcs_identical:
                t = p0.sub(leadstart)
                t = p0.add(t)
                leadstart = t
                offsetvector = offsetvector.multiply(-1)
        else:  # perpendicular
            leadstart = p0.add(off_v)

        # At this point leadstart is the beginning of the leadin arc
        # and offsetvector points from leadstart to the center of the leadin arc
        # so the offsetvector is a radius of the leadin arc at its start
        # The extend line should be tangent to the leadin arc at this point, or perpendicular to the radius

        angle = -90 if arcdir == "G2" else 90
        tangentvec = App.Rotation(App.Vector(0, 0, 1), angle).multVec(offsetvector)

        # Normalize the tangent vector
        tangentvecNorm = App.Vector(tangentvec).normalize()
        leadlinevec = App.Vector(tangentvecNorm).multiply(obj.ExtendLeadIn)

        # leadlinevec provides the offset from the beginning of the lead arc to the beginning of the extend line
        extendstart = leadstart.add(leadlinevec)

        if action == "start":
            if obj.ExtendLeadIn != 0:
                # Rapid move to beginning of extend line
                extendcommand = Path.Command(
                    "G0",
                    {
                        "X": extendstart.x,
                        "Y": extendstart.y,
                        "Z": op.ClearanceHeight.Value,
                    },
                )
            else:
                # Rapid move to beginning of leadin arc
                extendcommand = Path.Command(
                    "G0",
                    {
                        "X": extendstart.x,
                        "Y": extendstart.y,
                        "Z": op.ClearanceHeight.Value,
                    },
                )
            results.append(extendcommand)
            extendcommand = Path.Command("G0", {"Z": op.SafeHeight.Value})
            results.append(extendcommand)

        if action == "layer":
            if not obj.KeepToolDown:
                extendcommand = Path.Command("G0", {"Z": op.SafeHeight.Value})
                results.append(extendcommand)

            extendcommand = Path.Command("G0", {"X": extendstart.x, "Y": extendstart.y})
            results.append(extendcommand)

        if obj.RapidPlunge:
            extendcommand = Path.Command("G0", {"Z": p1.z})
        else:
            extendcommand = Path.Command("G1", {"Z": p1.z, "F": vertFeed})

        results.append(extendcommand)

        if obj.StyleOn == "Arc":
            if obj.ExtendLeadIn != 0:
                # Insert move to beginning of leadin arc
                extendcommand = Path.Command(
                    "G1", {"X": leadstart.x, "Y": leadstart.y, "F": horizFeed}
                )
                results.append(extendcommand)
            arcmove = Path.Command(
                arcdir,
                {
                    "X": p0.x,
                    "Y": p0.y,
                    "Z": p0.z,
                    "I": offsetvector.x,
                    "J": offsetvector.y,
                    "K": offsetvector.z,
                    "F": horizFeed,
                },
            )  # add G2/G3 move
            results.append(arcmove)
        # elif obj.StyleOn in ["Tangent", "Perpendicular"]:
        else:
            extendcommand = Path.Command("G1", {"X": p0.x, "Y": p0.y, "F": horizFeed})
            results.append(extendcommand)

        currLocation.update(results[-1].Parameters)
        currLocation["Z"] = p1.z

        return results

    def getLeadEnd(self, obj, queue, action):
        """returns the Gcode of LeadOut."""
        results = []
        horizFeed = PathDressup.toolController(obj.Base).HorizFeed.Value
        arcs_identical = False

        # Set the correct twist command
        if self.getDirectionOfPath(obj) == "right":
            arcdir = "G2"
        else:
            arcdir = "G3"

        if queue[1].Name == "G1":  # line
            p0 = queue[0].Placement.Base
            p1 = queue[1].Placement.Base
            v = App.Vector(p1.sub(p0)).normalize()
        else:  # dealing with a circle
            p0 = queue[0].Placement.Base
            p1 = queue[1].Placement.Base
            v = App.Vector(p1.sub(p0)).normalize()

        if self.getDirectionOfPath(obj) == "right":
            off_v = App.Vector(
                v.y * obj.LengthOut.Value, -v.x * obj.LengthOut.Value, 0.0
            )
        else:
            off_v = App.Vector(
                -v.y * obj.LengthOut.Value, v.x * obj.LengthOut.Value, 0.0
            )

        # Check if we leave at line or arc command
        if queue[1].Name in movecommands and queue[1].Name not in CmdMoveArc:
            # We have a line move
            vec_n = App.Vector(p1.sub(p0)).normalize()
            vec_inv = vec_n
            vec_inv.multiply(-1)
            vec_off = App.Vector(vec_inv).multiply(obj.ExtendLeadOut)
            Path.Log.debug(
                "LineCMD: {}, Vxinv: {}, Vyinv: {}, Vxoff: {}, Vyoff: {}".format(
                    queue[0].Name, vec_inv.x, vec_inv.y, vec_off.x, vec_off.y
                )
            )
        else:
            # We have an arc move
            pij = App.Vector(p0)
            pij.x += queue[1].Parameters["I"]
            pij.y += queue[1].Parameters["J"]
            ve = pij.sub(p1)

            if arcdir == queue[1].Name:
                arcs_identical = True

            angle = -90 if arcdir == "G2" else 90
            vec_rot = App.Rotation(App.Vector(0, 0, 1), angle).multVec(ve)

            vec_n = App.Vector(vec_rot).normalize()
            v = vec_n

            if arcdir == "G3":
                off_v = App.Vector(
                    -v.y * obj.LengthOut.Value, v.x * obj.LengthOut.Value, 0.0
                )
            else:
                off_v = App.Vector(
                    v.y * obj.LengthOut.Value, -v.x * obj.LengthOut.Value, 0.0
                )

            vec_inv = vec_rot
            vec_inv.multiply(-1)

        offsetvector = App.Vector(
            v.x * obj.LengthOut.Value, v.y * obj.LengthOut.Value, 0.0
        )
        # if obj.RadiusCenter == "Radius":
        if obj.StyleOff == "Arc":
            leadend = (p1.add(off_v)).add(offsetvector)
            if arcs_identical:
                t = p1.sub(leadend)
                t = p1.add(t)
                leadend = t
                off_v.multiply(-1)

        elif obj.StyleOff == "Tangent":
            # This is WRONG.  Please fix
            leadend = (p1.add(off_v)).add(offsetvector)
            if arcs_identical:
                t = p1.sub(leadend)
                t = p1.add(t)
                leadend = t
                off_v.multiply(-1)
        else:
            leadend = p1.add(off_v)

        IJ = off_v
        # At this point leadend is the location of the end of the leadout arc
        # IJ is an offset from the beginning of the leadout arc to its center.
        # It is parallel to a tangent line at the end of the leadout arc
        # Create the normalized tangent vector
        tangentvecNorm = App.Vector(IJ).normalize()
        leadlinevec = App.Vector(tangentvecNorm).multiply(obj.ExtendLeadOut)

        extendleadoutend = leadend.add(leadlinevec)

        if obj.StyleOff == "Arc":
            arcmove = Path.Command(
                arcdir,
                {
                    "X": leadend.x,
                    "Y": leadend.y,
                    "Z": leadend.z,
                    "I": IJ.x,
                    "J": IJ.y,
                    "K": IJ.z,
                    "F": horizFeed,
                },
            )  # add G2/G3 move
            results.append(arcmove)
            if obj.ExtendLeadOut != 0:
                extendcommand = Path.Command(
                    "G1",
                    {"X": extendleadoutend.x, "Y": extendleadoutend.y, "F": horizFeed},
                )
                results.append(extendcommand)
        else:
            extendcommand = Path.Command(
                "G1", {"X": leadend.x, "Y": leadend.y, "F": horizFeed}
            )
            results.append(extendcommand)

        return results

    def generateLeadInOutCurve(self, obj):
        global currLocation
        firstmove = Path.Command("G0", {"X": 0, "Y": 0, "Z": 0})
        op = PathDressup.baseOp(obj.Base)
        currLocation.update(firstmove.Parameters)
        newpath = []
        queue = []
        action = "start"
        prevCmd = ""
        layers = []

        # Read in all commands
        for curCommand in PathUtils.getPathWithPlacement(obj.Base).Commands:
            Path.Log.debug("CurCMD: {}".format(curCommand))
            if curCommand.Name not in movecommands + CmdMoveRapid:
                # Don't worry about non-move commands, just add to output
                newpath.append(curCommand)
                continue

            if curCommand.Name in CmdMoveRapid:
                # We don't care about rapid moves
                prevCmd = curCommand
                currLocation.update(curCommand.Parameters)
                continue

            if curCommand.Name in movecommands:
                if (
                    prevCmd.Name in CmdMoveRapid
                    and curCommand.Name in movecommands
                    and len(queue) > 0
                ):
                    # Layer changed: Save current layer cmds and prepare next layer
                    layers.append(queue)
                    queue = []
                if (
                    obj.IncludeLayers
                    and curCommand.z < currLocation["Z"] and not Path.Geom.isRoughly(curCommand.z, currLocation["Z"])
                    and prevCmd.Name in movecommands
                ):
                    # Layer change within move cmds
                    Path.Log.debug(
                        "Layer change in move: {}->{}".format(
                            currLocation["Z"], curCommand.z
                        )
                    )
                    layers.append(queue)
                    queue = []

                # Save all move commands
                # getLeadStart and getLeadEnd incorrectly treat missing axis words as being 0.
                currXYZ = { k: currLocation[k] for k in "XYZ" if k in currLocation }
                tmp = Path.Command(curCommand.Name, currXYZ | curCommand.Parameters)
                queue.append(tmp)

            currLocation.update(curCommand.Parameters)
            prevCmd = curCommand

        # Add last layer
        if len(queue) > 0:
            layers.append(queue)

        # Go through each layer and add leadIn/Out
        idx = 0
        for layer in layers:
            Path.Log.debug("Layer {}".format(idx))

            if obj.LeadIn:
                temp = self.getLeadStart(obj, layer, action)
                newpath.extend(temp)

            for cmd in layer:
                Path.Log.debug("CurLoc: {}, NewCmd: {}!!".format(currLocation, cmd))
                newpath.append(cmd)

            if obj.LeadOut:
                tmp = []
                tmp.append(layer[-2])
                tmp.append(layer[-1])
                temp = self.getLeadEnd(obj, tmp, action)
                newpath.extend(temp)

            if not obj.KeepToolDown or idx == len(layers) - 1:
                extendcommand = Path.Command("G0", {"Z": op.ClearanceHeight.Value})
                newpath.append(extendcommand)
            else:
                action = "layer"

            idx += 1

        commands = newpath
        return Path.Path(commands)


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
            "Pixmap": "Path_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("Path_DressupLeadInOut", "LeadInOut"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Path_DressupLeadInOut",
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
                translate("Path_DressupLeadInOut", "Please select one path object")
                + "\n"
            )
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            Path.Log.error(
                translate("Path_DressupLeadInOut", "The selected object is not a path")
                + "\n"
            )
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            Path.Log.error(
                translate("Path_DressupLeadInOut", "Please select a Profile object")
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
    FreeCADGui.addCommand("Path_DressupLeadInOut", CommandPathDressupLeadInOut())

Path.Log.notice("Loading Path_DressupLeadInOut... done\n")
