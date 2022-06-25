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

from __future__ import print_function

import FreeCAD
import FreeCADGui
import Path
import PathScripts.PathDressup as PathDressup
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import PathScripts.PathUtils as PathUtils
import math
import copy

__doc__ = """LeadInOut Dressup MASHIN-CRC USE ROLL-ON ROLL-OFF to profile"""

from PySide.QtCore import QT_TRANSLATE_NOOP

from PathPythonGui.simple_edit_panel import SimpleEditPanel

translate = FreeCAD.Qt.translate

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


movecommands = ["G1", "G01", "G2", "G02", "G3", "G03"]
rapidcommands = ["G0", "G00"]
arccommands = ["G2", "G3", "G02", "G03"]
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
            "App::PropertyBool",
            "UseMachineCRC",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "Use Machine Cutter Radius Compensation /Tool Path Offset G41/G42",
            ),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "Length",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Length or Radius of the approach"),
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
            "App::PropertyEnumeration",
            "RadiusCenter",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property", "The Mode of Point Radiusoffset or Center"
            ),
        )
        obj.RadiusCenter = [
            QT_TRANSLATE_NOOP("Path_DressupLeadInOut", "Radius"),
            QT_TRANSLATE_NOOP("Path_DressupLeadInOut", "Center"),
        ]
        obj.Proxy = self
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

        self.wire = None
        self.rapids = None

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def setup(self, obj):
        obj.Length = obj.Base.ToolController.Tool.Diameter * 0.75
        obj.LeadIn = True
        obj.LeadOut = True
        obj.KeepToolDown = False
        obj.UseMachineCRC = False
        obj.StyleOn = "Arc"
        obj.StyleOff = "Arc"
        obj.RadiusCenter = "Radius"
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
        if obj.Length < 0:
            PathLog.error(
                translate("Path_DressupLeadInOut", "Length/Radius positive not Null")
                + "\n"
            )
            obj.Length = 0.1
        self.wire, self.rapids = PathGeom.wireForPath(obj.Base.Path)
        obj.Path = self.generateLeadInOutCurve(obj)

    def getDirectionOfPath(self, obj):
        op = PathDressup.baseOp(obj.Base)

        if hasattr(op, "Side") and op.Side == "Outside":
            if hasattr(op, "Direction") and op.Direction == "CW":
                return "left"
            else:
                return "right"
        else:
            if hasattr(op, "Direction") and op.Direction == "CW":
                return "right"
        return "left"

    def getSideOfPath(self, obj):
        op = PathDressup.baseOp(obj.Base)
        if hasattr(op, "Side"):
            return op.Side

        return ""

    def normalize(self, Vector):
        vx = 0
        vy = 0

        x = Vector.x
        y = Vector.y
        length = math.sqrt(x * x + y * y)
        if (math.fabs(length)) > 0.0000000000001:
            vx = round(x / length, 3)
            vy = round(y / length, 3)

        return FreeCAD.Vector(vx, vy, 0)

    def invert(self, Vector):
        x = Vector.x * -1
        y = Vector.y * -1
        z = Vector.z * -1
        return FreeCAD.Vector(x, y, z)

    def multiply(self, Vector, len):
        x = Vector.x * len
        y = Vector.y * len
        z = Vector.z * len
        return FreeCAD.Vector(x, y, z)

    def rotate(self, Vector, angle):
        s = math.sin(math.radians(angle))
        c = math.cos(math.radians(angle))
        xnew = Vector.x * c - Vector.y * s
        ynew = Vector.x * s + Vector.y * c
        return FreeCAD.Vector(xnew, ynew, Vector.z)

    def getLeadStart(self, obj, queue, action):
        """returns Lead In G-code."""
        # Modified March 2022 by lcorley to support leadin extension
        results = []
        op = PathDressup.baseOp(obj.Base)
        tc = PathDressup.toolController(obj.Base)
        horizFeed = tc.HorizFeed.Value
        vertFeed = tc.VertFeed.Value
        toolnummer = tc.ToolNumber
        arcs_identical = False

        # Set the correct twist command
        if self.getDirectionOfPath(obj) == "left":
            arcdir = "G3"
        else:
            arcdir = "G2"

        R = obj.Length.Value  # Radius of roll or length
        if queue[1].Name == "G1":  # line
            p0 = queue[0].Placement.Base
            p1 = queue[1].Placement.Base
            v = self.normalize(p1.sub(p0))
            PathLog.debug(" CURRENT_IN Line : P0 Z:{} p1 Z:{}".format(p0.z, p1.z))
        else:
            p0 = queue[0].Placement.Base
            p1 = queue[1].Placement.Base
            v = self.normalize(p1.sub(p0))
            PathLog.debug(
                " CURRENT_IN ARC : P0 X:{} Y:{} P1 X:{} Y:{} ".format(
                    p0.x, p0.y, p1.x, p1.y
                )
            )

        # Calculate offset vector (will be overwritten for arcs)
        if self.getDirectionOfPath(obj) == "right":
            off_v = FreeCAD.Vector(v.y * R, -v.x * R, 0.0)
        else:
            off_v = FreeCAD.Vector(-v.y * R, v.x * R, 0.0)

        # Check if we enter at line or arc command
        if queue[1].Name in movecommands and queue[1].Name not in arccommands:
            # We have a line move
            vec = p1.sub(p0)
            vec_n = self.normalize(vec)
            vec_inv = self.invert(vec_n)
            vec_off = self.multiply(vec_inv, obj.ExtendLeadIn)
            PathLog.debug(
                "LineCMD: {}, Vxinv: {}, Vyinv: {}, Vxoff: {}, Vyoff: {}".format(
                    queue[0].Name, vec_inv.x, vec_inv.y, vec_off.x, vec_off.y
                )
            )
        else:
            # We have an arc move
            # Calculate coordinates for middle of circle
            pij = copy.deepcopy(p0)
            pij.x += queue[1].Parameters["I"]
            pij.y += queue[1].Parameters["J"]

            # Check if lead in and operation go in same direction (usually for inner circles)
            if arcdir == queue[1].Name:
                arcs_identical = True

            # Calculate vector circle start -> circle middle
            vec_circ = pij.sub(p0)

            # Rotate vector to get direction for lead in
            if arcdir == "G2":
                vec_rot = self.rotate(vec_circ, 90)
            else:
                vec_rot = self.rotate(vec_circ, -90)

            # Normalize and invert vector
            vec_n = self.normalize(vec_rot)

            v = self.invert(vec_n)

            # Calculate offset of lead in
            if arcdir == "G3":
                off_v = FreeCAD.Vector(-v.y * R, v.x * R, 0.0)
            else:
                off_v = FreeCAD.Vector(v.y * R, -v.x * R, 0.0)

        offsetvector = FreeCAD.Vector(v.x * R, v.y * R, 0)  # IJ

        if obj.RadiusCenter == "Radius":
            leadstart = (p0.add(off_v)).sub(offsetvector)  # Rmode
            if arcs_identical:
                t = p0.sub(leadstart)
                t = p0.add(t)
                leadstart = t
                offsetvector = self.multiply(offsetvector, -1)
        else:
            leadstart = p0.add(off_v)  # Dmode
        # At this point leadstart is the beginning of the leadin arc
        # and offsetvector points from leadstart to the center of the leadin arc
        # so the offsetvector is a radius of the leadin arc at its start
        # The extend line should be tangent to the leadin arc at this point, or perpendicular to the radius
        if arcdir == "G2":
            tangentvec = self.rotate(offsetvector, -90)
        else:
            tangentvec = self.rotate(offsetvector, 90)
        # Normalize the tangent vector
        tangentvecNorm = self.normalize(tangentvec)
        # Multiply tangentvecNorm by LeadIn length
        leadlinevec = self.multiply(tangentvecNorm, obj.ExtendLeadIn)
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

        if obj.UseMachineCRC:
            if self.getDirectionOfPath(obj) == "right":
                results.append(Path.Command("G42", {"D": toolnummer}))
            else:
                results.append(Path.Command("G41", {"D": toolnummer}))

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
        elif obj.StyleOn == "Tangent":
            extendcommand = Path.Command("G1", {"X": p0.x, "Y": p0.y, "F": horizFeed})
            results.append(extendcommand)
        else:
            PathLog.debug(" CURRENT_IN Perp")

        currLocation.update(results[-1].Parameters)
        currLocation["Z"] = p1.z

        return results

    def getLeadEnd(self, obj, queue, action):
        """returns the Gcode of LeadOut."""
        results = []
        horizFeed = PathDressup.toolController(obj.Base).HorizFeed.Value
        R = obj.Length.Value  # Radius of roll or length
        arcs_identical = False

        # Set the correct twist command
        if self.getDirectionOfPath(obj) == "right":
            arcdir = "G2"
        else:
            arcdir = "G3"

        if queue[1].Name == "G1":  # line
            p0 = queue[0].Placement.Base
            p1 = queue[1].Placement.Base
            v = self.normalize(p1.sub(p0))
        else:  # dealing with a circle
            p0 = queue[0].Placement.Base
            p1 = queue[1].Placement.Base
            v = self.normalize(p1.sub(p0))

        if self.getDirectionOfPath(obj) == "right":
            off_v = FreeCAD.Vector(v.y * R, -v.x * R, 0.0)
        else:
            off_v = FreeCAD.Vector(-v.y * R, v.x * R, 0.0)

        # Check if we leave at line or arc command
        if queue[1].Name in movecommands and queue[1].Name not in arccommands:
            # We have a line move
            vec = p1.sub(p0)
            vec_n = self.normalize(vec)
            vec_inv = self.invert(vec_n)
            vec_off = self.multiply(vec_inv, obj.ExtendLeadOut)
            PathLog.debug(
                "LineCMD: {}, Vxinv: {}, Vyinv: {}, Vxoff: {}, Vyoff: {}".format(
                    queue[0].Name, vec_inv.x, vec_inv.y, vec_off.x, vec_off.y
                )
            )
        else:
            # We have an arc move
            pij = copy.deepcopy(p0)
            pij.x += queue[1].Parameters["I"]
            pij.y += queue[1].Parameters["J"]
            ve = pij.sub(p1)

            if arcdir == queue[1].Name:
                arcs_identical = True

            if arcdir == "G2":
                vec_rot = self.rotate(ve, -90)
            else:
                vec_rot = self.rotate(ve, 90)

            vec_n = self.normalize(vec_rot)
            v = vec_n

            if arcdir == "G3":
                off_v = FreeCAD.Vector(-v.y * R, v.x * R, 0.0)
            else:
                off_v = FreeCAD.Vector(v.y * R, -v.x * R, 0.0)

            vec_inv = self.invert(vec_rot)

        offsetvector = FreeCAD.Vector(v.x * R, v.y * R, 0.0)
        if obj.RadiusCenter == "Radius":
            leadend = (p1.add(off_v)).add(offsetvector)  # Rmode
            if arcs_identical:
                t = p1.sub(leadend)
                t = p1.add(t)
                leadend = t
                off_v = self.multiply(off_v, -1)
        else:
            leadend = p1.add(off_v)  # Dmode

        IJ = off_v
        # At this point leadend is the location of the end of the leadout arc
        # IJ is an offset from the beginning of the leadout arc to its center.
        # It is parallel to a tangent line at the end of the leadout arc
        # Create the normalized tangent vector
        tangentvecNorm = self.normalize(IJ)
        leadlinevec = self.multiply(tangentvecNorm, obj.ExtendLeadOut)
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
        elif obj.StyleOff == "Tangent":
            extendcommand = Path.Command(
                "G1", {"X": leadend.x, "Y": leadend.y, "F": horizFeed}
            )
            results.append(extendcommand)
        else:
            PathLog.debug(" CURRENT_IN Perp")

        if obj.UseMachineCRC:  # crc off
            results.append(Path.Command("G40", {}))

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
        for curCommand in obj.Base.Path.Commands:
            PathLog.debug("CurCMD: {}".format(curCommand))
            if curCommand.Name not in movecommands + rapidcommands:
                # Don't worry about non-move commands, just add to output
                newpath.append(curCommand)
                continue

            if curCommand.Name in rapidcommands:
                # We don't care about rapid moves
                prevCmd = curCommand
                currLocation.update(curCommand.Parameters)
                continue

            if curCommand.Name in movecommands:
                if (
                    prevCmd.Name in rapidcommands
                    and curCommand.Name in movecommands
                    and len(queue) > 0
                ):
                    # Layer changed: Save current layer cmds and prepare next layer
                    layers.append(queue)
                    queue = []
                if (
                    obj.IncludeLayers
                    and curCommand.z < currLocation["Z"]
                    and prevCmd.Name in movecommands
                ):
                    # Layer change within move cmds
                    PathLog.debug(
                        "Layer change in move: {}->{}".format(
                            currLocation["Z"], curCommand.z
                        )
                    )
                    layers.append(queue)
                    queue = []

                # Save all move commands
                queue.append(curCommand)

            currLocation.update(curCommand.Parameters)
            prevCmd = curCommand

        # Add last layer
        if len(queue) > 0:
            layers.append(queue)

        # Go through each layer and add leadIn/Out
        idx = 0
        for layer in layers:
            PathLog.debug("Layer {}".format(idx))

            if obj.LeadIn:
                temp = self.getLeadStart(obj, layer, action)
                newpath.extend(temp)

            for cmd in layer:
                PathLog.debug("CurLoc: {}, NewCmd: {}!!".format(currLocation, cmd))
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
        self.connectWidget("Length", self.form.dsbLen)
        self.connectWidget("ExtendLeadIn", self.form.dsbExtendIn)
        self.connectWidget("ExtendLeadOut", self.form.dsbExtendOut)
        self.connectWidget("StyleOn", self.form.cboStyleIn)
        self.connectWidget("StyleOff", self.form.cboStyleOut)
        self.connectWidget("RadiusCenter", self.form.cboRadius)
        self.connectWidget("RapidPlunge", self.form.chkRapidPlunge)
        self.connectWidget("IncludeLayers", self.form.chkLayers)
        self.connectWidget("KeepToolDown", self.form.chkKeepToolDown)
        self.connectWidget("UseMachineCRC", self.form.chkUseCRC)
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
                    print(i.Group)
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
        PathLog.debug("Deleting Dressup")
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(self.obj)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def clearTaskPanel(self):
        self.panel = None


class CommandPathDressupLeadInOut:
    def GetResources(self):
        return {
            "Pixmap": "Path_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("Path_DressupLeadInOut", "LeadInOut Dressup"),
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
            PathLog.error(
                translate("Path_DressupLeadInOut", "Please select one path object")
                + "\n"
            )
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            PathLog.error(
                translate("Path_DressupLeadInOut", "The selected object is not a path")
                + "\n"
            )
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            PathLog.error(
                translate("Path_DressupLeadInOut", "Please select a Profile object")
            )
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create LeadInOut Dressup")
        FreeCADGui.addModule("PathScripts.PathDressupLeadInOut")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "LeadInOutDressup")'
        )
        FreeCADGui.doCommand(
            "dbo = PathScripts.PathDressupLeadInOut.ObjectDressup(obj)"
        )
        FreeCADGui.doCommand("base = FreeCAD.ActiveDocument." + selection[0].Name)
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(base)")
        FreeCADGui.doCommand("obj.Base = base")
        FreeCADGui.doCommand("job.Proxy.addOperation(obj, base)")
        FreeCADGui.doCommand("dbo.setup(obj)")
        FreeCADGui.doCommand(
            "obj.ViewObject.Proxy = PathScripts.PathDressupLeadInOut.ViewProviderDressup(obj.ViewObject)"
        )
        FreeCADGui.doCommand(
            "Gui.ActiveDocument.getObject(base.Name).Visibility = False"
        )
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("Path_DressupLeadInOut", CommandPathDressupLeadInOut())

PathLog.notice("Loading Path_DressupLeadInOut... done\n")
