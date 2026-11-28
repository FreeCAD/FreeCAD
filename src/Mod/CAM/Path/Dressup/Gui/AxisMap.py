# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2018 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import math
import Path
from Path.Base.Gui.Util import QuantitySpinBox
import Path.Dressup.Utils as PathDressup
import Path.Post.Utils as PostUtils
from PathPythonGui.simple_edit_panel import SimpleEditPanel
from PathScripts import PathUtils
from PySide.QtCore import QT_TRANSLATE_NOOP

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


if FreeCAD.GuiUp:
    import FreeCADGui

__doc__ = """Axis remapping Dressup object and FreeCAD command.
This dressup remaps one axis of motion to another.
For example, you can re-map the Y axis to A to control a 4th axis rotary."""


translate = FreeCAD.Qt.translate


class ObjectDressup:
    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The base path to modify"),
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "AxisMap",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The input mapping axis"),
        )
        obj.addProperty(
            "App::PropertyDistance",
            "Radius",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The radius of the wrapped axis"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "Reverse",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Reverse rotary axis direction"),
        )
        obj.addProperty(
            "App::PropertyVectorDistance",
            "Centre",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The centre of rotation.\nAffects only to Path repesentation in 3d view.",
            ),
        )
        obj.addProperty(
            "App::PropertyAngle",
            "MinRotaryMove",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "Minimum rotary movement in degrees"),
        )

        obj.AxisMap = ("X->A", "Y->A", "X->B", "Y->B", "X->C", "Y->C")
        obj.AxisMap = "Y->A"
        obj.Radius = 45
        obj.Proxy = self
        obj.MinRotaryMove = 0.05

        self.defineCylinder(obj)

    def defineCylinder(self, obj):
        """Set default Centre, Radius and AxisMap from surfaces in Job"""
        import Part

        if job := PathUtils.findParentJob(obj):
            cylinders = [
                f.Surface
                for m in job.Model.Group + [job.Stock]
                for f in m.Shape.Faces
                if isinstance(f.Surface, Part.Cylinder)
            ]
            if cylinders:
                cylinder = max(cylinders, key=lambda c: c.Radius)
                obj.Centre = cylinder.Center
                obj.Radius = cylinder.Radius
                if Path.Geom.compareVecs(cylinder.Axis, FreeCAD.Vector(0, 1, 0)):
                    obj.AxisMap = "X->B"

    def dumps(self):
        return

    def loads(self, state):
        return

    def onChanged(self, obj, prop):
        if "Restore" not in obj.State and prop == "Radius":
            job = PathUtils.findParentJob(obj)
            if job:
                job.Proxy.setCenterOfRotation(obj.Centre)

        if prop == "Path" and obj.ViewObject:
            obj.ViewObject.signalChangeIcon()

    def onDocumentRestored(self, obj):
        if not hasattr(obj, "MinRotaryMove"):
            obj.addProperty(
                "App::PropertyAngle",
                "MinRotaryMove",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Minimum rotary movement in degrees",
                ),
            )
            obj.MinRotaryMove = 0.05
        if not hasattr(obj, "Reverse"):
            obj.addProperty(
                "App::PropertyBool",
                "Reverse",
                "Path",
                QT_TRANSLATE_NOOP("App::Property", "Reverse rotary axis direction"),
            )
        if not hasattr(obj, "Centre"):
            obj.addProperty(
                "App::PropertyVectorDistance",
                "Centre",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The centre of rotation.\nAffects only to Path repesentation in 3d view.",
                ),
            )
            obj.Centre = FreeCAD.Vector(0, 0, 0 - obj.Radius.Value)

    def execute(self, obj):

        inAxis = obj.AxisMap[0]
        outAxis = obj.AxisMap[3]

        if (
            not obj.Base
            or not obj.Base.isDerivedFrom("Path::Feature")
            or not obj.Base.Path
            or not obj.Base.Path.Commands
        ):
            obj.Path = Path.Path()
            return

        PathDressup.placeWithBase(obj)
        job = PathUtils.findParentJob(obj)
        deflection = job.GeometryTolerance.Value

        path = obj.Base.Path
        path = PostUtils.splitArcs(path, deflection=deflection)

        newcommandlist = []

        # Track original coordinates before axis mapping
        last_input_pos = {"X": 0.0, "Y": 0.0, "Z": 0.0}

        # Original modal feedrate (mm/s)
        last_input_feed = 0.0

        # Track whether the output feedrate is currently linear or rotary
        last_feed_mode = "linear"

        # Last rotary position actually emitted (degrees)
        last_emitted_rotary = 0.0

        # Cache settings
        radius = obj.Radius.Value
        min_rotary_move = obj.MinRotaryMove
        direction = -1 if obj.Reverse else 1

        epsilon = 1e-12

        for cmd in path.Commands:

            # Original command parameters
            input_params = dict(cmd.Parameters)

            # Update original modal feedrate
            if "F" in input_params:
                last_input_feed = input_params["F"]

            # Calculate current original position
            current_pos = last_input_pos.copy()

            for axis in ("X", "Y", "Z"):
                if axis in input_params:
                    current_pos[axis] = input_params[axis]

            # Calculate original movement vector
            delta = {axis: current_pos[axis] - last_input_pos[axis] for axis in ("X", "Y", "Z")}

            # Original movement distance
            move_length = math.hypot(*delta.values())

            # Start with original command parameters
            newparams = dict(input_params)

            # Extract selected axis
            remapvar = newparams.pop(inAxis, None)

            rotary_move_emitted = False

            if remapvar is not None:

                # Convert the absolute input position to rotary degrees
                target_rotary = math.degrees(direction * remapvar / radius)

                if cmd.Name == "G0":

                    # Rapid moves always map directly
                    last_emitted_rotary = target_rotary
                    newparams[outAxis] = last_emitted_rotary

                elif cmd.Name == "G1":
                    # Calculate movement from the last emitted position
                    rotary_delta = target_rotary - last_emitted_rotary

                    # Only emit rotary movements above the minimum threshold
                    if abs(rotary_delta) >= min_rotary_move:
                        newparams[outAxis] = target_rotary
                        last_emitted_rotary = target_rotary

                        # Calculate angular feedrate in degrees/second
                        if move_length > epsilon and last_input_feed > epsilon:
                            move_time = move_length / last_input_feed
                            newparams["F"] = abs(rotary_delta) / move_time
                            last_feed_mode = "rotary"

                        rotary_move_emitted = True

                else:

                    # Preserve direct mapping for other command types
                    last_emitted_rotary = target_rotary
                    newparams[outAxis] = last_emitted_rotary

            # Restore linear feedrate when no rotary move is emitted
            if (
                cmd.Name == "G1"
                and not rotary_move_emitted
                and last_feed_mode == "rotary"
                and last_input_feed > epsilon
            ):
                newparams["F"] = last_input_feed
                last_feed_mode = "linear"

            # Create transformed command
            newcommand = Path.Command(cmd.Name, newparams)
            newcommand.Annotations = cmd.Annotations

            newcommandlist.append(newcommand)

            # Update original position AFTER processing the move
            last_input_pos = current_pos

        path = Path.Path(newcommandlist)
        path.Center = obj.Centre
        obj.Path = path


class TaskPanel(SimpleEditPanel):
    _transaction_name = "Edit DressupAxisMap"
    _ui_file = ":/panels/AxisMapEdit.ui"

    def setupUi(self):
        self.setupSpinBoxes()
        self.setFields()
        self.pageRegisterSignalHandlers()

    def setupSpinBoxes(self):
        self.connectWidget("Reverse", self.form.reverse)
        self.connectWidget("AxisMap", self.form.axisMap)
        self.radius = QuantitySpinBox(self.form.radius, self.obj, "Radius", setToolTip=True)
        self.radius.updateWidget()

    def getSignalsForUpdate(self):
        signals = []
        signals.append(self.form.radius.editingFinished)
        return signals

    def pageGetFields(self):
        self.radius.updateProperty()

    def pageRegisterSignalHandlers(self):
        for signal in self.getSignalsForUpdate():
            signal.connect(self.pageGetFields)


class ViewProviderDressup:
    def __init__(self, vobj):
        self.obj = vobj.Object
        vobj.Proxy = self

    def attach(self, vobj):
        self.obj = vobj.Object
        self.panel = None

        if self.obj and self.obj.Base:
            for i in self.obj.Base.InList:
                if hasattr(i, "Group") and self.obj.Base.Name in [o.Name for o in i.Group]:
                    i.Group = [o for o in i.Group if o.Name != self.obj.Base.Name]
            if self.obj.Base.ViewObject:
                self.obj.Base.ViewObject.Visibility = False

    def unsetEdit(self, vobj, mode=0):
        if mode == 0 and self.panel:
            self.panel.abort()

    def setEdit(self, vobj, mode=0):
        if mode == 1:
            FreeCADGui.runCommand("Std_TransformManip")
        elif mode == 0:
            FreeCADGui.Control.closeDialog()
            panel = TaskPanel(vobj.Object, self)
            FreeCADGui.Control.showDialog(panel)
        return True

    def claimChildren(self):
        return [self.obj.Base]

    def dumps(self):
        return

    def loads(self, state):
        return

    def onDelete(self, arg1=None, arg2=None):
        """this makes sure that the base operation is added back to the project and visible"""
        if arg1.Object and arg1.Object.Base:
            FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
            job = PathUtils.findParentJob(arg1.Object)
            if job:
                job.Proxy.addOperation(arg1.Object.Base, arg1.Object)
            arg1.Object.Base = None
        return True

    def clearTaskPanel(self):
        self.panel = None

    def getIcon(self):
        if getattr(PathDressup.baseOp(self.obj), "Active", True):
            return ":/icons/CAM_Dressup.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


class CommandPathDressup:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Dressup",
            "MenuText": QT_TRANSLATE_NOOP("CAM_DressupAxisMap", "Axis Map"),
            "Accel": "",
            "ToolTip": QT_TRANSLATE_NOOP("CAM_DressupAxisMap", "Remaps one axis to another"),
        }

    def IsActive(self):
        return bool(PathDressup.selection())

    def Activated(self):
        # check that the selection contains exactly what we want
        op = PathDressup.selection(verbose=True)
        if not op:
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction("Create Dress-up")
        FreeCADGui.addModule("Path.Dressup.Gui.AxisMap")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "AxisMapDressup")'
        )
        FreeCADGui.doCommand(f"base = FreeCAD.ActiveDocument.getObject('{op.Name}')")
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(base)")
        FreeCADGui.doCommand("job.Proxy.addOperation(obj, base)")
        FreeCADGui.doCommand("Path.Dressup.Gui.AxisMap.ObjectDressup(obj)")
        FreeCADGui.doCommand("obj.Base = base")
        FreeCADGui.doCommand("Path.Dressup.Gui.AxisMap.ViewProviderDressup(obj.ViewObject)")
        FreeCADGui.doCommand("base.Visibility = False")
        FreeCADGui.doCommand("obj.ViewObject.Document.setEdit(obj.ViewObject, 0)")
        # FreeCAD.ActiveDocument.commitTransaction()  # Final `commitTransaction()` called via TaskPanel.accept()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_DressupAxisMap", CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup… done\n")
