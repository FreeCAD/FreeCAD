# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
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
# ***************************************************************************/

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


def isCreateJointActive():
    return UtilsAssembly.isAssemblyGrounded() and UtilsAssembly.assembly_has_at_least_n_parts(2)


def activateJoint(index):
    if JointObject.activeTask:
        JointObject.activeTask.reject()

    panel = TaskAssemblyCreateJoint(index)
    Gui.Control.showDialog(panel)


class CommandCreateJointFixed:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointFixed",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointFixed", "Create Fixed Joint"),
            "Accel": "J",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateJointFixed",
                "Create a Fixed Joint: Permanently locks two parts together, preventing any movement or rotation.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
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
                "Create a Distance Joint: Depending on your selection this tool will apply different constraints.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        # return False
        return isCreateJointActive()

    def Activated(self):
        activateJoint(5)


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

                full_element_name = UtilsAssembly.getFullElementName(sel.ObjectName, sub)
                obj = UtilsAssembly.getObject(full_element_name)
                part_containing_obj = UtilsAssembly.getContainingPart(full_element_name, obj)

                # Only objects within the assembly.
                objs_names, element_name = UtilsAssembly.getObjsNamesAndElement(sel.ObjectName, sub)
                if assembly.Name not in objs_names:
                    continue

                # Check if part is grounded and if so delete the joint.
                for joint in joint_group.Group:
                    if (
                        hasattr(joint, "ObjectToGround")
                        and joint.ObjectToGround == part_containing_obj
                    ):
                        # Remove grounded tag.
                        if part_containing_obj.Label.endswith(" 🔒"):
                            part_containing_obj.Label = part_containing_obj.Label[:-2]
                        doc = App.ActiveDocument
                        doc.removeObject(joint.Name)
                        doc.recompute()
                        return

                # Create groundedJoint.

                part_containing_obj.Label = part_containing_obj.Label + " 🔒"
                ground = joint_group.newObject("App::FeaturePython", "GroundedJoint")
                JointObject.GroundedJoint(ground, part_containing_obj)
                JointObject.ViewProviderGroundedJoint(ground.ViewObject)
        App.closeActiveTransaction()


if App.GuiUp:
    Gui.addCommand("Assembly_ToggleGrounded", CommandToggleGrounded())
    Gui.addCommand("Assembly_CreateJointFixed", CommandCreateJointFixed())
    Gui.addCommand("Assembly_CreateJointRevolute", CommandCreateJointRevolute())
    Gui.addCommand("Assembly_CreateJointCylindrical", CommandCreateJointCylindrical())
    Gui.addCommand("Assembly_CreateJointSlider", CommandCreateJointSlider())
    Gui.addCommand("Assembly_CreateJointBall", CommandCreateJointBall())
    Gui.addCommand("Assembly_CreateJointDistance", CommandCreateJointDistance())
