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

import re
import os
import FreeCAD as App

from pivy import coin
from Part import LineSegment, Compound

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui, QtWidgets
    from PySide.QtWidgets import QPushButton, QMenu

import UtilsAssembly
import Preferences

# translate = App.Qt.translate

__title__ = "Assembly Command Create Exploded View"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


class CommandCreateView:
    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Assembly_ExplodedView",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateView", "Create Exploded View"),
            "Accel": "V",
            "ToolTip": "<p>"
            + QT_TRANSLATE_NOOP(
                "Assembly_CreateView",
                "Create an exploded view of the current assembly.",
            )
            + "</p>",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return (
            UtilsAssembly.isAssemblyCommandActive()
            and UtilsAssembly.assembly_has_at_least_n_parts(2)
        )

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return

        self.panel = TaskAssemblyCreateView()
        Gui.Control.showDialog(self.panel)


######### Exploded View Object ###########
class ExplodedView:
    def __init__(self, expView):
        expView.addProperty(
            "App::PropertyLinkList", "Moves", "Exploded View", "Move objects of the exploded view."
        )
        expView.Proxy = self

        self.stepsChangedCallback = None

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, viewObj, prop):
        if prop == "Moves" and hasattr(self, "stepsChangedCallback"):
            if self.stepsChangedCallback is not None:
                self.stepsChangedCallback()

    def setMovesChangedCallback(self, callback):
        self.stepsChangedCallback = callback

    def execute(self, fp):
        """Do something when doing a recomputation, this method is mandatory"""
        # App.Console.PrintMessage("Recompute Python Box feature\n")
        pass

    def applyMoves(self, viewObj, com=None, size=None):
        positions = []  # [[p1start, p1end], [p2start, p2end], ...]
        if com is None:
            com, size = UtilsAssembly.getComAndSize(self.getAssembly(viewObj))
        for move in viewObj.Moves:
            positions = positions + move.Proxy.applyStep(move, com, size)

        return positions

    def getAssembly(self, viewObj):
        for obj in viewObj.InList:
            if obj.isDerivedFrom("Assembly::AssemblyObject"):
                return obj
        return None

    def saveAssemblyAndExplode(self, viewObj):
        self.initialPlcs = UtilsAssembly.saveAssemblyPartsPlacements(self.getAssembly(viewObj))

        self.positions = self.applyMoves(viewObj)

        lines = []

        for startPos, endPos in self.positions:
            line = LineSegment(startPos, endPos).toShape()
            lines.append(line)
        if lines:
            return Compound(lines)

        return None

    def restoreAssembly(self, viewObj):
        if self.initialPlcs is None:
            return

        UtilsAssembly.restoreAssemblyPartsPlacements(self.getAssembly(viewObj), self.initialPlcs)


class ViewProviderExplodedView:
    def __init__(self, vobj):
        """Set this object to the proxy object of the actual view provider"""
        vobj.Proxy = self

    def attach(self, vobj):
        """Setup the scene sub-graph of the view provider, this method is mandatory"""
        self.app_obj = vobj.Object

        self.display_mode = coin.SoType.fromName("SoFCSelection").createInstance()

        vobj.addDisplayMode(self.display_mode, "Wireframe")

    def updateData(self, joint, prop):
        """If a property of the handled feature has changed we have the chance to handle this here"""
        # joint is the handled feature, prop is the name of the property that has changed
        pass

    def getDisplayModes(self, obj):
        """Return a list of display modes."""
        return ["Wireframe"]

    def getDefaultDisplayMode(self):
        """Return the name of the default display mode. It must be defined in getDisplayModes."""
        return "Wireframe"

    def onChanged(self, vp, prop):
        """Here we can do something when a single property got changed"""
        # App.Console.PrintMessage("Change property: " + str(prop) + "\n")
        pass

    def getIcon(self):
        return ":/icons/Assembly_ExplodedView.svg"

    def dumps(self):
        """When saving the document this object gets stored using Python's json module.\
                Since we have some un-serializable parts here -- the Coin stuff -- we must define this method\
                to return a tuple of all serializable objects or None."""
        return None

    def loads(self, state):
        """When restoring the serialized object from document we have the chance to set some internals here.\
                Since no data were serialized nothing needs to be done here."""
        return None

    def claimChildren(self):
        return self.app_obj.Moves

    def doubleClicked(self, vobj):
        task = Gui.Control.activeTaskDialog()
        if task:
            task.reject()

        assembly = vobj.Object.Proxy.getAssembly(vobj.Object)

        if assembly is None:
            return False

        if UtilsAssembly.activeAssembly() != assembly:
            Gui.ActiveDocument.setEdit(assembly)

        panel = TaskAssemblyCreateView(vobj.Object)
        Gui.Control.showDialog(panel)

        return True


######### Exploded View Move #########
ExplodedViewStepTypes = [
    "Normal",
    "Radial",
]


class ExplodedViewStep:
    def __init__(self, evStep, type_index=0):
        evStep.Proxy = self

        # we cannot use "App::PropertyLinkList" for objs because they can be external
        evStep.addProperty(
            "App::PropertyStringList",
            "ObjNames",
            "Exploded Move",
            QT_TRANSLATE_NOOP("App::Property", "The object moved by the move"),
        )

        evStep.addProperty(
            "App::PropertyLinkList",
            "Parts",
            "Exploded Move",
            QT_TRANSLATE_NOOP("App::Property", "The containing parts of objects moved by the move"),
        )

        evStep.addProperty(
            "App::PropertyPlacement",
            "MovementTransform",
            "Exploded Move",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "This is the movement of the move. The end placement is the result of the start placement * this placement.",
            ),
        )

        evStep.addProperty(
            "App::PropertyEnumeration",
            "MoveType",
            "Exploded Move",
            QT_TRANSLATE_NOOP("App::Property", "The type of the move"),
        )
        evStep.MoveType = ExplodedViewStepTypes  # sets the list
        evStep.MoveType = ExplodedViewStepTypes[type_index]  # set the initial value

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, joint, prop):
        """Do something when a property has changed"""
        pass

    def execute(self, fp):
        """Do something when doing a recomputation, this method is mandatory"""
        # App.Console.PrintMessage("Recompute Python Box feature\n")
        pass

    def applyStep(self, move, com=App.Vector(), size=100):
        positions = []
        if move.MoveType == "Radial":
            distance = move.MovementTransform.Base.Length
            factor = 4 * distance / size

        for objName, part in zip(move.ObjNames, move.Parts):
            if not objName:
                continue
            obj = UtilsAssembly.getObjectInPart(objName, part)
            if not obj:
                continue

            if move.ViewObject:
                startPos = UtilsAssembly.getCenterOfBoundingBox([obj], [part])

            if move.MoveType == "Radial":
                objCom, objSize = UtilsAssembly.getComAndSize(obj)
                init_vec = objCom - com
                obj.Placement.Base = obj.Placement.Base + init_vec * factor
            else:
                obj.Placement = move.MovementTransform * obj.Placement

            if move.ViewObject:
                endPos = UtilsAssembly.getCenterOfBoundingBox([obj], [part])
                positions.append([startPos, endPos])

        if move.ViewObject:
            move.ViewObject.Proxy.redrawLines(move, positions)

        return positions

    def getMovingobjects(self, move):
        movingObjs = []
        for objName, part in zip(move.ObjNames, move.Parts):
            obj = UtilsAssembly.getObjectInPart(objName, part)
            if obj is not None:
                movingObjs.append(obj)
        return movingObjs


class ViewProviderExplodedViewStep:
    def __init__(self, vobj):
        """Set this object to the proxy object of the actual view provider"""
        vobj.Proxy = self

    def attach(self, vobj):
        """Setup the scene sub-graph of the view provider, this method is mandatory"""
        self.app_obj = vobj.Object

        pref = Preferences.preferences()

        self.line_thickness = pref.GetInt("StepLineThickness", 3)

        param_step_line_color = pref.GetUnsigned("StepLineColor", 0xCC333300)
        self.so_color = coin.SoBaseColor()
        self.so_color.rgb.setValue(UtilsAssembly.color_from_unsigned(param_step_line_color))

        self.draw_style = coin.SoDrawStyle()
        self.draw_style.style = coin.SoDrawStyle.LINES
        self.draw_style.lineWidth = self.line_thickness
        self.draw_style.linePattern = 0xF0F0  # Dashed line pattern

        # Create a separator to hold all dashed lines
        self.lineSetGroup = coin.SoSeparator()

        self.display_mode = coin.SoType.fromName("SoFCSelection").createInstance()
        self.display_mode.addChild(self.lineSetGroup)  # Add the group to the display mode
        vobj.addDisplayMode(self.display_mode, "Wireframe")

    def updateData(self, stepObj, prop):
        """If a property of the handled feature has changed we have the chance to handle this here"""
        # stepObj is the handled feature, prop is the name of the property that has changed
        pass

    def redrawLines(self, stepObj, positions):
        # Clear existing lines
        self.lineSetGroup.removeAllChildren()

        for startPos, endPos in positions:
            # Create the line
            line = coin.SoLineSet()
            line.numVertices.setValue(2)
            coords = coin.SoCoordinate3()
            coords.point.setValues(0, [startPos, endPos])

            # Create separator for this line to apply the style
            line_sep = coin.SoSeparator()
            line_sep.addChild(self.draw_style)
            line_sep.addChild(self.so_color)
            line_sep.addChild(coords)
            line_sep.addChild(line)

            # Add to the group
            self.lineSetGroup.addChild(line_sep)

    def getDisplayModes(self, obj):
        """Return a list of display modes."""
        modes = []
        modes.append("Wireframe")
        return modes

    def getDefaultDisplayMode(self):
        """Return the name of the default display mode. It must be defined in getDisplayModes."""
        return "Wireframe"

    def onChanged(self, vp, prop):
        """Here we can do something when a single property got changed"""
        # App.Console.PrintMessage("Change property: " + str(prop) + "\n")
        pass

    def getIcon(self):
        return ":/icons/button_add_all.svg"

    def dumps(self):
        """When saving the document this object gets stored using Python's json module.\
                Since we have some un-serializable parts here -- the Coin stuff -- we must define this method\
                to return a tuple of all serializable objects or None."""
        return None

    def loads(self, state):
        """When restoring the serialized object from document we have the chance to set some internals here.\
                Since no data were serialized nothing needs to be done here."""
        return None


class ExplodedViewSelGate:
    def __init__(self, assembly, viewObj):
        self.assembly = assembly
        self.viewObj = viewObj

    def allow(self, doc, obj, sub):
        if (obj.Name == self.assembly.Name and sub) or self.assembly.hasObject(obj, True):
            # Objects within the assembly.
            return True

        if obj in self.viewObj.Moves:
            # Enable selection of steps object
            return True

        return False


######### Create Exploded View Task ###########
class TaskAssemblyCreateView(QtCore.QObject):
    def __init__(self, viewObj=None):
        super().__init__()

        self.form = Gui.PySideUic.loadUi(":/panels/TaskAssemblyCreateView.ui")
        self.form.stepList.installEventFilter(self)
        self.form.stepList.itemClicked.connect(self.onItemClicked)

        view = Gui.activeDocument().activeView()

        self.assembly = UtilsAssembly.activeAssembly()
        self.assembly.ViewObject.EnableMovement = False
        self.com, self.size = UtilsAssembly.getComAndSize(self.assembly)
        self.asmDragger = self.assembly.ViewObject.getDragger()
        self.cbFin = view.addDraggerCallback(
            self.asmDragger, "addFinishCallback", self.draggerFinished
        )
        self.cbMov = view.addDraggerCallback(
            self.asmDragger, "addMotionCallback", self.draggerMoved
        )

        Gui.Selection.clearSelection()

        self.form.btnAlignDragger.setMenu(QMenu(self.form.btnAlignDragger))
        actionAlignTo = self.form.btnAlignDragger.menu().addAction("Align to...")
        actionAlignToCenter = self.form.btnAlignDragger.menu().addAction("Align to part center")
        actionAlignToOrigin = self.form.btnAlignDragger.menu().addAction("Align to part origin")

        # Connect actions to the respective functions
        actionAlignTo.triggered.connect(self.onAlignTo)
        actionAlignToCenter.triggered.connect(self.onAlignToCenter)
        actionAlignToOrigin.triggered.connect(self.onAlignToPartOrigin)

        self.form.btnAlignDragger.setEnabled(False)
        self.form.btnAlignDragger.setText("Select a part")
        self.form.btnRadialExplosion.clicked.connect(self.onRadialClicked)

        pref = Preferences.preferences()
        self.form.CheckBox_PartsAsSingleSolid.setChecked(pref.GetBool("PartsAsSingleSolid", True))

        self.initialPlcs = UtilsAssembly.saveAssemblyPartsPlacements(self.assembly)

        if viewObj:
            App.setActiveTransaction("Edit Exploded View")
            self.viewObj = viewObj
            for move in self.viewObj.Moves:
                move.Visibility = True
            self.onMovesChanged()

        else:
            App.setActiveTransaction("Create Exploded View")
            self.createExplodedViewObject()

        Gui.Selection.addSelectionGate(
            ExplodedViewSelGate(self.assembly, self.viewObj), Gui.Selection.ResolveMode.NoResolve
        )
        Gui.Selection.addObserver(self, Gui.Selection.ResolveMode.NoResolve)

        self.viewObj.Proxy.setMovesChangedCallback(self.onMovesChanged)
        self.callbackMove = view.addEventCallback("SoLocation2Event", self.moveMouse)
        self.callbackClick = view.addEventCallback("SoMouseButtonEvent", self.clickMouse)
        self.callbackKey = view.addEventCallback("SoKeyboardEvent", self.KeyboardEvent)

        self.selectingFeature = False
        self.form.LabelAlignDragger.setVisible(False)
        self.preselection_dict = None

        self.blockSetDragger = False
        self.blockDraggerMove = True
        self.currentStep = None

    def accept(self):
        self.deactivate()
        UtilsAssembly.restoreAssemblyPartsPlacements(self.assembly, self.initialPlcs)
        for move in self.viewObj.Moves:
            move.Visibility = False
        App.closeActiveTransaction()
        return True

    def reject(self):
        self.deactivate()
        App.closeActiveTransaction(True)
        return True

    def deactivate(self):
        pref = Preferences.preferences()
        pref.SetBool("PartsAsSingleSolid", self.form.CheckBox_PartsAsSingleSolid.isChecked())

        view = Gui.activeDocument().activeView()
        view.removeDraggerCallback(self.asmDragger, "addFinishCallback", self.cbFin)
        view.removeDraggerCallback(self.asmDragger, "addMotionCallback", self.cbMov)

        self.assembly.ViewObject.DraggerVisibility = False
        self.assembly.ViewObject.EnableMovement = True

        Gui.Selection.removeSelectionGate()
        Gui.Selection.removeObserver(self)
        Gui.Selection.clearSelection()

        self.viewObj.Proxy.setMovesChangedCallback(None)
        view.removeEventCallback("SoLocation2Event", self.callbackMove)
        view.removeEventCallback("SoMouseButtonEvent", self.callbackClick)
        view.removeEventCallback("SoKeyboardEvent", self.callbackKey)

        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()

    def setDragger(self):
        if self.blockSetDragger:
            return

        self.dismissCurrentStep()
        self.selectedObjs = []
        self.selectedParts = []  # containing parts
        self.selectedObjsInitPlc = []
        selection = Gui.Selection.getSelectionEx("*", 0)
        if not selection:
            self.enableDragger(False)
            return
        for sel in selection:
            # If you select 2 solids (bodies for example) within an assembly.
            # There'll be a single sel but 2 SubElementNames.

            if not sel.SubElementNames:
                # no subnames, so its a root assembly itself that is selected.
                Gui.Selection.removeSelection(sel.Object)
                continue

            for sub_name in sel.SubElementNames:
                # Only objects within the assembly.
                objs_names, element_name = UtilsAssembly.getObjsNamesAndElement(
                    sel.ObjectName, sub_name
                )
                if self.assembly.Name not in objs_names:
                    Gui.Selection.removeSelection(sel.Object, sub_name)
                    continue

                obj_name = sel.ObjectName
                full_obj_name = UtilsAssembly.getFullObjName(obj_name, sub_name)
                full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)
                selected_object = UtilsAssembly.getObject(full_element_name)
                if selected_object is None:
                    continue
                element_name = UtilsAssembly.getElementName(full_element_name)
                part = UtilsAssembly.getContainingPart(
                    full_element_name, selected_object, self.assembly
                )

                if selected_object == self.assembly or element_name != "":
                    # do not accept selection of assembly itself or elements
                    Gui.Selection.removeSelection(sel.Object, sub_name)
                    continue

                if self.form.CheckBox_PartsAsSingleSolid.isChecked():
                    selected_object = part

                if not selected_object in self.selectedObjs and hasattr(
                    selected_object, "Placement"
                ):
                    self.selectedObjs.append(selected_object)
                    self.selectedParts.append(part)
                    self.selectedObjsInitPlc.append(App.Placement(selected_object.Placement))

        if len(self.selectedObjs) != 0:
            self.enableDragger(True)
            self.onAlignToCenter()

        else:
            self.enableDragger(False)

    def enableDragger(self, val):
        self.assembly.ViewObject.DraggerVisibility = val
        self.form.btnAlignDragger.setEnabled(val)
        if val:
            self.form.btnAlignDragger.setText("Align dragger to...")
        else:
            self.form.btnAlignDragger.setText("Select a part")

    def onMovesChanged(self):
        # First reset positions
        UtilsAssembly.restoreAssemblyPartsPlacements(self.assembly, self.initialPlcs)

        self.viewObj.Proxy.applyMoves(self.viewObj, self.com, self.size)

        self.form.stepList.clear()
        for move in self.viewObj.Moves:
            self.form.stepList.addItem(move.Name)

    def onItemClicked(self, item):
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(self.viewObj.Document.Name, item.text(), "")
        # we give back the focus to the item as addSelection gave the focus to the 3dview
        self.form.stepList.setCurrentItem(item)

    def onRadialClicked(self):
        self.dismissCurrentStep()

        # Add to selection all the movable parts
        partsAsSolid = self.form.CheckBox_PartsAsSingleSolid.isChecked()
        assemblyParts = UtilsAssembly.getMovablePartsWithin(self.assembly, partsAsSolid)
        self.blockSetDragger = True
        for part in assemblyParts:
            Gui.Selection.addSelection(part, "")
        self.blockSetDragger = False
        self.setDragger()

        self.createExplodedStepObject(1)  # 1 = type_index of "Radial"

    def onAlignTo(self):
        self.alignMode = "Custom"
        self.selectingFeature = True
        # We use greedy selection to prevent that clicking again on the solid
        # clears selection before trying to select the whole assemly
        Gui.Selection.setSelectionStyle(Gui.Selection.SelectionStyle.GreedySelection)
        self.enableDragger(False)
        self.form.LabelAlignDragger.setVisible(True)

    def endSelectionMode(self):
        self.selectingFeature = False
        self.enableDragger(True)
        Gui.Selection.setSelectionStyle(Gui.Selection.SelectionStyle.NormalSelection)
        self.form.LabelAlignDragger.setVisible(False)

    def onAlignToCenter(self):
        self.alignMode = "Center"
        self.setDraggerObjectPlc()

    def onAlignToPartOrigin(self):
        self.alignMode = "PartOrigin"
        self.setDraggerObjectPlc()

    def findDraggerInitialPlc(self):
        if len(self.selectedObjs) == 0:
            return

        if self.alignMode == "Custom":
            self.initialDraggerPlc = App.Placement(self.assembly.ViewObject.DraggerPlacement)
        else:
            plc = UtilsAssembly.getGlobalPlacement(self.selectedObjs[0], self.selectedParts[0])
            self.initialDraggerPlc = App.Placement(plc)
            if self.alignMode == "Center":
                self.initialDraggerPlc.Base = UtilsAssembly.getCenterOfBoundingBox(
                    self.selectedObjs, self.selectedParts
                )

    def setDraggerObjectPlc(self):
        self.findDraggerInitialPlc()

        self.blockDraggerMove = True
        self.assembly.ViewObject.DraggerPlacement = self.initialDraggerPlc
        self.blockDraggerMove = False

    def createExplodedViewObject(self):
        view_group = UtilsAssembly.getViewGroup(self.assembly)
        self.viewObj = view_group.newObject("App::FeaturePython", "Exploded View")

        ExplodedView(self.viewObj)
        ViewProviderExplodedView(self.viewObj.ViewObject)

    def createExplodedStepObject(self, moveType_index=0):
        self.currentStep = self.assembly.newObject("App::FeaturePython", "Move")
        ExplodedViewStep(self.currentStep, moveType_index)
        ViewProviderExplodedViewStep(self.currentStep.ViewObject)

        # Note: self.viewObj.Moves.append(self.currentStep) does not work
        listOfMoves = self.viewObj.Moves
        listOfMoves.append(self.currentStep)
        self.viewObj.Moves = listOfMoves

        objNames = []
        for obj in self.selectedObjs:
            objNames.append(obj.Name)

        self.currentStep.MovementTransform = App.Placement()
        self.currentStep.ObjNames = objNames
        self.currentStep.Parts = self.selectedParts

    def dismissCurrentStep(self):
        if self.currentStep is None:
            return

        for obj, init_plc in zip(self.selectedObjs, self.selectedObjsInitPlc):
            obj.Placement = init_plc

        self.currentStep.Document.removeObject(self.currentStep.Name)
        self.currentStep = None

        Gui.Selection.clearSelection()

    def draggerMoved(self, event):
        if self.blockDraggerMove:
            return

        if self.currentStep is None:
            self.createExplodedStepObject()

        # reset the objects position to their position before the current move.
        for obj, init_plc in zip(self.selectedObjs, self.selectedObjsInitPlc):
            obj.Placement = init_plc

        # we update the move Placement.
        draggerPlc = self.assembly.ViewObject.DraggerPlacement
        self.currentStep.MovementTransform = draggerPlc * self.initialDraggerPlc.inverse()

        # Apply the move
        self.currentStep.Proxy.applyStep(self.currentStep, self.com, self.size)

    def draggerFinished(self, event):
        if self.currentStep.MoveType == "Radial":
            self.currentStep = None
            Gui.Selection.clearSelection()
            return

        self.currentStep = None

        # Reset the initial placements
        self.findDraggerInitialPlc()

        for i, obj in enumerate(self.selectedObjs):
            self.selectedObjsInitPlc[i] = App.Placement(obj.Placement)

    def moveMouse(self, info):
        if not self.selectingFeature:
            return

        view = Gui.activeDocument().activeView()
        cursor_info = view.getObjectInfo(view.getCursorPos())

        if not cursor_info or not self.preselection_dict:
            self.assembly.ViewObject.DraggerVisibility = False
            return

        newPos = App.Vector(cursor_info["x"], cursor_info["y"], cursor_info["z"])
        self.preselection_dict["mouse_pos"] = newPos

        if self.preselection_dict["element_name"] == "":
            self.preselection_dict["vertex_name"] = ""
        else:
            self.preselection_dict["vertex_name"] = UtilsAssembly.findElementClosestVertex(
                self.preselection_dict
            )

        obj = self.preselection_dict["object"]
        part = self.preselection_dict["part"]
        plc = UtilsAssembly.findPlacement(
            obj,
            part,
            self.preselection_dict["element_name"],
            self.preselection_dict["vertex_name"],
        )
        global_plc = UtilsAssembly.getGlobalPlacement(obj, part)
        plc = global_plc * plc

        self.blockDraggerMove = True
        self.assembly.ViewObject.DraggerPlacement = plc
        self.blockDraggerMove = False
        self.assembly.ViewObject.DraggerVisibility = True

    def clickMouse(self, info):
        if info["Button"] == "BUTTON2" and info["State"] == "DOWN":
            if self.selectingFeature:
                self.endSelectionMode()

    # 3D view keyboard handler
    def KeyboardEvent(self, info):
        if info["State"] == "UP" and info["Key"] == "ESCAPE":
            if self.currentStep is None:
                self.reject()
            else:
                if self.selectingFeature:
                    self.endSelectionMode()
                else:
                    self.dismissCurrentStep()

    # Taskbox keyboard event handler
    def eventFilter(self, watched, event):
        if self.form is not None and watched == self.form.stepList:
            if event.type() == QtCore.QEvent.ShortcutOverride:
                if event.key() == QtCore.Qt.Key_Delete:
                    event.accept()
                    return True  # Indicate that the event has been handled
                return False

            elif event.type() == QtCore.QEvent.KeyPress:
                if event.key() == QtCore.Qt.Key_Delete:
                    selected_indexes = self.form.stepList.selectedIndexes()
                    sorted_indexes = sorted(selected_indexes, key=lambda x: x.row(), reverse=True)
                    for index in sorted_indexes:
                        row = index.row()
                        if row < len(self.viewObj.Moves):
                            move = self.viewObj.Moves[row]
                            # First remove the link from the viewObj
                            self.viewObj.Moves.remove(move)
                            # Delete the object
                            move.Document.removeObject(move.Name)

                    return True  # Consume the event

        return super().eventFilter(watched, event)

    # selectionObserver stuff
    def addSelection(self, doc_name, obj_name, sub_name, mousePos):
        if self.selectingFeature:
            Gui.Selection.removeSelection(doc_name, obj_name, sub_name)
            return

        else:
            full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)
            selected_object = UtilsAssembly.getObject(full_element_name)
            if selected_object is None:
                return

            element_name = UtilsAssembly.getElementName(full_element_name)
            part = UtilsAssembly.getContainingPart(
                full_element_name, selected_object, self.assembly
            )

            if not self.form.CheckBox_PartsAsSingleSolid.isChecked():
                part = selected_object

            if element_name != "":
                # When selecting, we do not want to select an element, but only the containing part.
                Gui.Selection.removeSelection(selected_object, element_name)
                if Gui.Selection.isSelected(part, ""):
                    Gui.Selection.removeSelection(part, "")
                else:
                    Gui.Selection.addSelection(part, "")
            else:
                self.setDragger()
                pass

    def removeSelection(self, doc_name, obj_name, sub_name, mousePos=None):
        if self.selectingFeature:
            self.endSelectionMode()
            self.findDraggerInitialPlc()
            return

        full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)
        element_name = UtilsAssembly.getElementName(full_element_name)
        if element_name == "":
            self.setDragger()
            pass

    def setPreselection(self, doc_name, obj_name, sub_name):
        if not self.selectingFeature or not sub_name:
            self.preselection_dict = None
            return

        full_obj_name = UtilsAssembly.getFullObjName(obj_name, sub_name)
        full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)
        selected_object = UtilsAssembly.getObject(full_element_name)
        element_name = UtilsAssembly.getElementName(full_element_name)
        part = UtilsAssembly.getContainingPart(full_element_name, selected_object, self.assembly)

        self.preselection_dict = {
            "object": selected_object,
            "part": part,
            "sub_name": sub_name,
            "element_name": element_name,
            "full_element_name": full_element_name,
            "full_obj_name": full_obj_name,
        }

    def clearSelection(self, doc_name):
        self.form.stepList.clearSelection()
        self.setDragger()


if App.GuiUp:
    Gui.addCommand("Assembly_CreateView", CommandCreateView())
