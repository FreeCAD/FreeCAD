# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import os
import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui, QtWidgets

import JointObject
from JointObject import TaskAssemblyCreateJoint
import UtilsAssembly
import Assembly_rc

# translate = App.Qt.translate

__title__ = "Assembly Commands to Create Joints"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


def noOtherTaskActive():
    return UtilsAssembly.isAssemblyCommandActive() or JointObject.activeTask is not None


def isCreateJointActive():
    return UtilsAssembly.assembly_has_at_least_n_parts(1) and noOtherTaskActive()


def activateJoint(index):
    if JointObject.activeTask:
        JointObject.activeTask.reject()

    Gui.addModule("JointObject")  # NOLINT
    Gui.doCommand(f"panel = JointObject.TaskAssemblyCreateJoint({index})")
    Gui.doCommandGui("dialog = Gui.Control.showDialog(panel)")
    dialog = Gui.doCommandEval("dialog")
    if dialog is not None:
        dialog.setAutoCloseOnTransactionChange(True)
        dialog.setDocumentName(App.ActiveDocument.Name)


class CommandCreateJointFixed:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointFixed",
            "MenuText": QT_TRANSLATE_NOOP(
                "Assembly_CreateJointFixed",
                "Create a Fixed Joint",
            ),
            "Accel": "F",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointFixed",
                "1 - If an assembly is active : Create a joint permanently locking two parts together, preventing any movement or rotation.",
            )
            + "</p>"
            + "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointFixed",
                "2 - If a part is active : Position sub parts by matching selected coordinate systems. The second part selected will move.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        if UtilsAssembly.activePart() is not None:
            return UtilsAssembly.assembly_has_at_least_n_parts(2)

        return isCreateJointActive()

    def Activated(self):
        activateJoint(0)


class CommandCreateJointRevolute:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointRevolute",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointRevolute", "Create Revolute Joint"),
            "Accel": "R",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointRevolute",
                "Create a Revolute Joint: Allows rotation around a single axis between selected parts.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()

    def Activated(self):
        activateJoint(1)


class CommandCreateJointCylindrical:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointCylindrical",
            "MenuText": QT_TRANSLATE_NOOP(
                "Assembly_CreateJointCylindrical", "Create Cylindrical Joint"
            ),
            "Accel": "C",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointCylindrical",
                "Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()

    def Activated(self):
        activateJoint(2)


class CommandCreateJointSlider:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointSlider",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointSlider", "Create Slider Joint"),
            "Accel": "S",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointSlider",
                "Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()

    def Activated(self):
        activateJoint(3)


class CommandCreateJointBall:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointBall",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointBall", "Create Ball Joint"),
            "Accel": "B",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointBall",
                "Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()

    def Activated(self):
        activateJoint(4)


class CommandCreateJointDistance:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointDistance",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointDistance", "Create Distance Joint"),
            "Accel": "D",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointDistance",
                "Create a Distance Joint: Fix the distance between the selected objects.",
            )
            + "</p><p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointDistance",
                "Create one of several different joints based on the selection."
                "For example, a distance of 0 between a plane and a cylinder creates a tangent joint. A distance of 0 between planes will make them co-planar.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()

    def Activated(self):
        activateJoint(5)


class CommandCreateJointParallel:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointParallel",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointParallel", "Create Parallel Joint"),
            "Accel": "N",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointParallel",
                "Create an Parallel Joint: Make the Z axis of selected coordinate systems parallel.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()

    def Activated(self):
        activateJoint(6)


class CommandCreateJointPerpendicular:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointPerpendicular",
            "MenuText": QT_TRANSLATE_NOOP(
                "Assembly_CreateJointPerpendicular", "Create Perpendicular Joint"
            ),
            "Accel": "M",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointPerpendicular",
                "Create an Perpendicular Joint: Make the Z axis of selected coordinate systems perpendicular.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()

    def Activated(self):
        activateJoint(7)


class CommandCreateJointAngle:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointAngle",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointAngle", "Create Angle Joint"),
            "Accel": "X",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointAngle",
                "Create an Angle Joint: Fix the angle between the Z axis of selected coordinate systems.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()

    def Activated(self):
        activateJoint(8)


class CommandCreateJointRackPinion:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointRackPinion",
            "MenuText": QT_TRANSLATE_NOOP(
                "Assembly_CreateJointRackPinion", "Create Rack and Pinion Joint"
            ),
            "Accel": "Q",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointRackPinion",
                "Create a Rack and Pinion Joint: Links a part with a sliding joint with a part with a revolute joint.",
            )
            + "</p><p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointRackPinion",
                "Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rack and the pinion.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()

    def Activated(self):
        activateJoint(9)


class CommandCreateJointScrew:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointScrew",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointScrew", "Create Screw Joint"),
            "Accel": "W",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointScrew",
                "Create a Screw Joint: Links a part with a sliding joint with a part with a revolute joint.",
            )
            + "</p><p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointScrew",
                "Select the same coordinate systems as the revolute and sliding joints. The pitch radius defines the movement ratio between the rotating screw and the sliding part.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()

    def Activated(self):
        activateJoint(10)


class CommandCreateJointGears:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointGears",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointGears", "Create Gears Joint"),
            "Accel": "X",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointGears",
                "Create a Gears Joint: Links two rotating gears together. They will have inverse rotation direction.",
            )
            + "</p><p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointScrew",
                "Select the same coordinate systems as the revolute joints.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()

    def Activated(self):
        activateJoint(11)


class CommandCreateJointBelt:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointPulleys",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointBelt", "Create Belt Joint"),
            "Accel": "P",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointBelt",
                "Create a Belt Joint: Links two rotating objects together. They will have the same rotation direction.",
            )
            + "</p><p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointScrew",
                "Select the same coordinate systems as the revolute joints.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()

    def Activated(self):
        activateJoint(12)


class CommandGroupGearBelt:
    def GetCommands(self):
        return ("Assembly_CreateJointGears", "Assembly_CreateJointBelt")

    def GetResources(self):
        """Set icon, menu and tooltip."""

        return {
            "Pixmap": "Assembly_CreateJointGears",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointGearBelt", "Create Gear/Belt Joint"),
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointGearBelt",
                "Create a Gears/Belt Joint: Links two rotating gears together.",
            )
            + "</p><p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointGearBelt",
                "Select the same coordinate systems as the revolute joints.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return isCreateJointActive()


def createGroundedJoint(obj):
    if not UtilsAssembly.activeAssembly():
        return

    Gui.addModule("UtilsAssembly")
    Gui.addModule("JointObject")
    commands = (
        f'obj = App.ActiveDocument.getObject("{obj.Name}")\n'
        "assembly = UtilsAssembly.activeAssembly()\n"
        "joint_group = UtilsAssembly.getJointGroup(assembly)\n"
        'ground = joint_group.newObject("App::FeaturePython", "GroundedJoint")\n'
        "JointObject.GroundedJoint(ground, obj)"
    )
    Gui.doCommand(commands)
    Gui.doCommandGui("JointObject.ViewProviderGroundedJoint(ground.ViewObject)")
    return Gui.doCommandEval("ground")


class CommandToggleGrounded:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_ToggleGrounded",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_ToggleGrounded", "Toggle grounded"),
            "Accel": "G",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_ToggleGrounded",
                "Grounding a part permanently locks its position in the assembly, preventing any movement or rotation. You need at least one grounded part before starting to assemble.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return (
            UtilsAssembly.isAssemblyCommandActive()
            and UtilsAssembly.assembly_has_at_least_n_parts(1)
        )

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return

        joint_group = UtilsAssembly.getJointGroup(assembly)

        selection = Gui.Selection.getSelectionEx("*", 0)
        if not selection:
            return

        App.setActiveTransaction("Toggle grounded")
        for sel in selection:
            # If you select 2 solids (bodies for example) within an assembly.
            # There'll be a single sel but 2 SubElementNames.
            for sub in sel.SubElementNames:
                ref = [sel.Object, [sub, sub]]
                moving_part = UtilsAssembly.getMovingPart(assembly, ref)

                # Only objects within the assembly.
                if moving_part is None:
                    continue

                # Check if part is grounded and if so delete the joint.
                ungrounded = False
                for joint in joint_group.Group:
                    if hasattr(joint, "ObjectToGround") and joint.ObjectToGround == moving_part:
                        commands = (
                            "doc = App.ActiveDocument\n"
                            f'doc.removeObject("{joint.Name}")\n'
                            "doc.recompute()\n"
                        )
                        Gui.doCommand(commands)
                        ungrounded = True
                        break
                if ungrounded:
                    continue

                # Create groundedJoint.
                createGroundedJoint(moving_part)
        App.closeActiveTransaction()


if App.GuiUp:
    Gui.addCommand("Assembly_ToggleGrounded", CommandToggleGrounded())
    Gui.addCommand("Assembly_CreateJointFixed", CommandCreateJointFixed())
    Gui.addCommand("Assembly_CreateJointRevolute", CommandCreateJointRevolute())
    Gui.addCommand("Assembly_CreateJointCylindrical", CommandCreateJointCylindrical())
    Gui.addCommand("Assembly_CreateJointSlider", CommandCreateJointSlider())
    Gui.addCommand("Assembly_CreateJointBall", CommandCreateJointBall())
    Gui.addCommand("Assembly_CreateJointDistance", CommandCreateJointDistance())
    Gui.addCommand("Assembly_CreateJointParallel", CommandCreateJointParallel())
    Gui.addCommand("Assembly_CreateJointPerpendicular", CommandCreateJointPerpendicular())
    Gui.addCommand("Assembly_CreateJointAngle", CommandCreateJointAngle())
    Gui.addCommand("Assembly_CreateJointRackPinion", CommandCreateJointRackPinion())
    Gui.addCommand("Assembly_CreateJointScrew", CommandCreateJointScrew())
    Gui.addCommand("Assembly_CreateJointGears", CommandCreateJointGears())
    Gui.addCommand("Assembly_CreateJointBelt", CommandCreateJointBelt())
    Gui.addCommand("Assembly_CreateJointGearBelt", CommandGroupGearBelt())
