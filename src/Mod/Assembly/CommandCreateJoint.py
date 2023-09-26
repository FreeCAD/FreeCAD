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
import UtilsAssembly
import Assembly_rc

# translate = App.Qt.translate

__title__ = "Assembly Commands to Create Joints"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


class CommandCreateJointFixed:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointFixed",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointFixed", "Create Fixed Joint"),
            "Accel": "F",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreateJointFixed",
                "<p>Create a Fixed Joint: Permanently locks two parts together, preventing any movement or rotation.</p>",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.activeAssembly() is not None

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return
        view = Gui.activeDocument().activeView()

        self.panel = TaskAssemblyCreateJoint(assembly, view, 0)
        Gui.Control.showDialog(self.panel)


class CommandCreateJointRevolute:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointRevolute",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointRevolute", "Create Revolute Joint"),
            "Accel": "R",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreateJointRevolute",
                "<p>Create a Revolute Joint: Allows rotation around a single axis between selected parts.</p>",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.activeAssembly() is not None

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return
        view = Gui.activeDocument().activeView()

        self.panel = TaskAssemblyCreateJoint(assembly, view, 1)
        Gui.Control.showDialog(self.panel)


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
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreateJointCylindrical",
                "<p>Create a Cylindrical Joint: Enables rotation along one axis while permitting movement along the same axis between assembled parts.</p>",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.activeAssembly() is not None

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return
        view = Gui.activeDocument().activeView()

        self.panel = TaskAssemblyCreateJoint(assembly, view, 2)
        Gui.Control.showDialog(self.panel)


class CommandCreateJointSlider:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointSlider",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointSlider", "Create Slider Joint"),
            "Accel": "S",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreateJointSlider",
                "<p>Create a Slider Joint: Allows linear movement along a single axis but restricts rotation between selected parts.</p>",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.activeAssembly() is not None

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return
        view = Gui.activeDocument().activeView()

        self.panel = TaskAssemblyCreateJoint(assembly, view, 3)
        Gui.Control.showDialog(self.panel)


class CommandCreateJointBall:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointBall",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointBall", "Create Ball Joint"),
            "Accel": "B",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreateJointBall",
                "<p>Create a Ball Joint: Connects parts at a point, allowing unrestricted movement as long as the connection points remain in contact.</p>",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.activeAssembly() is not None

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return
        view = Gui.activeDocument().activeView()

        self.panel = TaskAssemblyCreateJoint(assembly, view, 4)
        Gui.Control.showDialog(self.panel)


class CommandCreateJointPlanar:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointPlanar",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointPlanar", "Create Planar Joint"),
            "Accel": "P",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreateJointPlanar",
                "<p>Create a Planar Joint: Ensures two selected features are in the same plane, restricting movement to that plane.</p>",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.activeAssembly() is not None

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return
        view = Gui.activeDocument().activeView()

        self.panel = TaskAssemblyCreateJoint(assembly, view, 5)
        Gui.Control.showDialog(self.panel)


class CommandCreateJointParallel:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointParallel",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointParallel", "Create Parallel Joint"),
            "Accel": "L",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreateJointParallel",
                "<p>Create a Parallel Joint: Aligns two features to be parallel, constraining relative movement to parallel translations.</p>",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.activeAssembly() is not None

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return
        view = Gui.activeDocument().activeView()

        self.panel = TaskAssemblyCreateJoint(assembly, view, 6)
        Gui.Control.showDialog(self.panel)


class CommandCreateJointTangent:
    def __init__(self):
        pass

    def GetResources(self):

        return {
            "Pixmap": "Assembly_CreateJointTangent",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateJointTangent", "Create Tangent Joint"),
            "Accel": "T",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreateJointTangent",
                "<p>Create a Tangent Joint: Forces two features to be tangent, restricting movement to smooth transitions along their contact surface.</p>",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return UtilsAssembly.activeAssembly() is not None

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return
        view = Gui.activeDocument().activeView()

        self.panel = TaskAssemblyCreateJoint(assembly, view, 7)
        Gui.Control.showDialog(self.panel)


class MakeJointSelGate:
    def __init__(self, taskbox, assembly):
        self.taskbox = taskbox
        self.assembly = assembly

    def allow(self, doc, obj, sub):
        if not sub:
            return False

        objs_names, element_name = UtilsAssembly.getObjsNamesAndElement(obj.Name, sub)

        if self.assembly.Name not in objs_names or element_name == "":
            # Only objects within the assembly. And not whole objects, only elements.
            return False

        if Gui.Selection.isSelected(obj, sub, Gui.Selection.ResolveMode.NoResolve):
            # If it's to deselect then it's ok
            return True

        if len(self.taskbox.current_selection) >= 2:
            # No more than 2 elements can be selected for basic joints.
            return False

        full_obj_name = ".".join(objs_names)
        for selection_dict in self.taskbox.current_selection:
            if selection_dict["full_obj_name"] == full_obj_name:
                # Can't join a solid to itself. So the user need to select 2 different parts.
                return False

        return True


class TaskAssemblyCreateJoint(QtCore.QObject):
    def __init__(self, assembly, view, jointTypeIndex):
        super().__init__()

        self.assembly = assembly
        self.view = view
        self.doc = App.ActiveDocument

        self.form = Gui.PySideUic.loadUi(":/panels/TaskAssemblyCreateJoint.ui")

        self.form.jointType.addItems(JointObject.JointTypes)
        self.form.jointType.setCurrentIndex(jointTypeIndex)

        Gui.Selection.clearSelection()
        Gui.Selection.addSelectionGate(
            MakeJointSelGate(self, self.assembly), Gui.Selection.ResolveMode.NoResolve
        )
        Gui.Selection.addObserver(self, Gui.Selection.ResolveMode.NoResolve)
        Gui.Selection.setSelectionStyle(Gui.Selection.SelectionStyle.GreedySelection)
        self.current_selection = []
        self.preselection_dict = None

        self.callbackMove = self.view.addEventCallback("SoLocation2Event", self.moveMouse)
        self.callbackKey = self.view.addEventCallback("SoKeyboardEvent", self.KeyboardEvent)

        App.setActiveTransaction("Create joint")
        self.createJointObject()

    def accept(self):
        if len(self.current_selection) != 2:
            App.Console.PrintWarning("You need to select 2 elements from 2 separate parts.")
            return False
        self.deactivate()
        App.closeActiveTransaction()
        return True

    def reject(self):
        self.deactivate()
        App.closeActiveTransaction(True)
        return True

    def deactivate(self):
        Gui.Selection.removeSelectionGate()
        Gui.Selection.removeObserver(self)
        Gui.Selection.setSelectionStyle(Gui.Selection.SelectionStyle.NormalSelection)
        Gui.Selection.clearSelection()
        self.view.removeEventCallback("SoLocation2Event", self.callbackMove)
        self.view.removeEventCallback("SoKeyboardEvent", self.callbackKey)
        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()

    def createJointObject(self):
        type_index = self.form.jointType.currentIndex()

        joint_group = self.assembly.getObject("Joints")

        if not joint_group:
            joint_group = self.assembly.newObject("App::DocumentObjectGroup", "Joints")

        self.joint = joint_group.newObject("App::FeaturePython", "Joint")
        JointObject.Joint(self.joint, type_index)
        JointObject.ViewProviderJoint(self.joint.ViewObject, self.joint)

    def updateJoint(self):
        # First we build the listwidget
        self.form.featureList.clear()
        simplified_names = []
        for sel in self.current_selection:
            # TODO: ideally we probably want to hide the feature name in case of PartDesign bodies. ie body.face12 and not body.pad2.face12
            sname = sel["full_element_name"].split(self.assembly.Name + ".", 1)[-1]
            simplified_names.append(sname)
        self.form.featureList.addItems(simplified_names)

        # Then we pass the new list to the join object
        self.joint.Proxy.setJointConnectors(self.current_selection)

    def moveMouse(self, info):
        if len(self.current_selection) >= 2 or (
            len(self.current_selection) == 1
            and self.current_selection[0]["full_element_name"]
            == self.preselection_dict["full_element_name"]
        ):
            self.joint.ViewObject.Proxy.showPreviewJCS(False)
            return

        cursor_pos = self.view.getCursorPos()
        cursor_info = self.view.getObjectInfo(cursor_pos)
        # cursor_info example  {'x': 41.515, 'y': 7.449, 'z': 16.861, 'ParentObject': <Part object>, 'SubName': 'Body002.Pad.Face5', 'Document': 'part3', 'Object': 'Pad', 'Component': 'Face5'}

        if (
            not cursor_info
            or not self.preselection_dict
            or cursor_info["SubName"] != self.preselection_dict["sub_name"]
        ):
            self.joint.ViewObject.Proxy.showPreviewJCS(False)
            return

        # newPos = self.view.getPoint(*info["Position"]) # This is not what we want, it's not pos on the object but on the focal plane

        newPos = App.Vector(cursor_info["x"], cursor_info["y"], cursor_info["z"])
        self.preselection_dict["mouse_pos"] = newPos

        self.preselection_dict["vertex_name"] = UtilsAssembly.findElementClosestVertex(
            self.preselection_dict
        )

        placement = self.joint.Proxy.findPlacement(
            self.preselection_dict["object"],
            self.preselection_dict["element_name"],
            self.preselection_dict["vertex_name"],
        )
        self.joint.ViewObject.Proxy.showPreviewJCS(True, placement)
        self.previewJCSVisible = True

    # 3D view keyboard handler
    def KeyboardEvent(self, info):
        if info["State"] == "UP" and info["Key"] == "ESCAPE":
            self.reject()

        if info["State"] == "UP" and info["Key"] == "RETURN":
            self.accept()

    # selectionObserver stuff
    def addSelection(self, doc_name, obj_name, sub_name, mousePos):
        full_obj_name = UtilsAssembly.getFullObjName(obj_name, sub_name)
        full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)
        selected_object = UtilsAssembly.getObject(full_element_name)
        element_name = UtilsAssembly.getElementName(full_element_name)

        selection_dict = {
            "object": selected_object,
            "element_name": element_name,
            "full_element_name": full_element_name,
            "full_obj_name": full_obj_name,
            "mouse_pos": App.Vector(mousePos[0], mousePos[1], mousePos[2]),
        }
        selection_dict["vertex_name"] = UtilsAssembly.findElementClosestVertex(selection_dict)

        self.current_selection.append(selection_dict)
        self.updateJoint()

    def removeSelection(self, doc_name, obj_name, sub_name, mousePos=None):
        full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)

        # Find and remove the corresponding dictionary from the combined list
        selection_dict_to_remove = None
        for selection_dict in self.current_selection:
            if selection_dict["full_element_name"] == full_element_name:
                selection_dict_to_remove = selection_dict
                break

        if selection_dict_to_remove is not None:
            self.current_selection.remove(selection_dict_to_remove)

        self.updateJoint()

    def setPreselection(self, doc_name, obj_name, sub_name):
        if not sub_name:
            self.preselection_dict = None
            return

        full_obj_name = UtilsAssembly.getFullObjName(obj_name, sub_name)
        full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)
        selected_object = UtilsAssembly.getObject(full_element_name)
        element_name = UtilsAssembly.getElementName(full_element_name)

        self.preselection_dict = {
            "object": selected_object,
            "sub_name": sub_name,
            "element_name": element_name,
            "full_element_name": full_element_name,
            "full_obj_name": full_obj_name,
        }

    def clearSelection(self, doc_name):
        self.current_selection.clear()
        self.updateJoint()


if App.GuiUp:
    Gui.addCommand("Assembly_CreateJointFixed", CommandCreateJointFixed())
    Gui.addCommand("Assembly_CreateJointRevolute", CommandCreateJointRevolute())
    Gui.addCommand("Assembly_CreateJointCylindrical", CommandCreateJointCylindrical())
    Gui.addCommand("Assembly_CreateJointSlider", CommandCreateJointSlider())
    Gui.addCommand("Assembly_CreateJointBall", CommandCreateJointBall())
    Gui.addCommand("Assembly_CreateJointPlanar", CommandCreateJointPlanar())
    Gui.addCommand("Assembly_CreateJointParallel", CommandCreateJointParallel())
    Gui.addCommand("Assembly_CreateJointTangent", CommandCreateJointTangent())
