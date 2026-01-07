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


__title__ = "Assembly Command Create Exploded View"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"


class CommandCreateView:
    def __init__(self):
        pass

    def GetResources(self):
        return {
            "Pixmap": "Assembly_ExplodedView",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_CreateView", "Exploded View"),
            "Accel": "E",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_CreateView",
                "Creates an exploded view of the current assembly",
            ),
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

        Gui.addModule("CommandCreateView")  # NOLINT
        Gui.doCommand("panel = CommandCreateView.TaskAssemblyCreateView()")
        self.panel = Gui.doCommandEval("panel")
        Gui.doCommandGui("dialog = Gui.Control.showDialog(panel)")
        dialog = Gui.doCommandEval("dialog")
        if dialog is not None:
            dialog.setAutoCloseOnDeletedDocument(True)
            dialog.setDocumentName(App.ActiveDocument.Name)


######### Exploded View Object ###########
class ExplodedView:
    def __init__(self, expView):
        expView.Proxy = self
        expView.addExtension("App::GroupExtensionPython")

        self.stepsChangedCallback = None

    def onDocumentRestored(self, expView):
        self.migrationScript(expView)

    def migrationScript(self, expView):
        if hasattr(expView, "Moves"):
            expView.addExtension("App::GroupExtensionPython")
            expView.Group = expView.Moves
            expView.removeProperty("Moves")

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, viewObj, prop):
        if prop == "Group" and hasattr(self, "stepsChangedCallback"):
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
        for move in viewObj.Group:
            positions = positions + move.Proxy.applyStep(move, com, size)

        return positions

    def getAssembly(self, viewObj):
        for obj in viewObj.InList:
            if obj.isDerivedFrom("Assembly::AssemblyObject"):
                return obj
        return None

    def _createSafeLine(self, start, end):
        """Creates a LineSegment shape only if points are not coincident."""
        from Part import Precision

        if (start - end).Length > Precision.confusion():
            return LineSegment(start, end).toShape()
        return None

    def saveAssemblyAndExplode(self, viewObj):
        self.initialPlcs = UtilsAssembly.saveAssemblyPartsPlacements(self.getAssembly(viewObj))

        self.positions = self.applyMoves(viewObj)

        lines = []

        for startPos, endPos in self.positions:
            line = self._createSafeLine(startPos, endPos)
            if line:
                lines.append(line)
        if lines:
            return Compound(lines)

        return None

    def restoreAssembly(self, viewObj):
        if self.initialPlcs is None:
            return

        UtilsAssembly.restoreAssemblyPartsPlacements(self.getAssembly(viewObj), self.initialPlcs)

    def _calculateExplodedPlacements(self, viewObj):
        """
        Internal helper to calculate final placements for an exploded view without
        applying them.
        Returns:
            - A dictionary mapping {part_object: final_placement}.
            - A list of [start_pos, end_pos] for explosion lines.
        """
        final_placements = {}
        line_positions = []
        factor = 1

        assembly = self.getAssembly(viewObj)
        # Get a snapshot of the assembly's current, un-exploded state
        calculated_placements = UtilsAssembly.saveAssemblyPartsPlacements(assembly)

        com, size = UtilsAssembly.getComAndSize(assembly)

        for move in viewObj.Group:
            if not UtilsAssembly.isRefValid(move.References, 1):
                continue

            if move.MoveType == "Radial":
                distance = move.MovementTransform.Base.Length
                factor = 4 * distance / size

            subs = move.References[1]
            for sub in subs:
                ref = [move.References[0], [sub]]
                obj = UtilsAssembly.getObject(ref)
                if not obj or not hasattr(obj, "Placement"):
                    continue

                # Use the placement from our calculation dictionary, which tracks
                # changes from previous steps.
                current_placement = calculated_placements.get(obj.Name, obj.Placement)

                # The part's shape is already placed, so its BBox.Center is the
                # correct global starting position for the explosion line.
                start_pos = obj.Shape.BoundBox.Center

                if move.MoveType == "Radial":
                    obj_com, obj_size = UtilsAssembly.getComAndSize(obj)
                    init_vec = obj_com - com
                    new_base = current_placement.Base + init_vec * factor
                    new_placement = App.Placement(new_base, current_placement.Rotation)
                else:
                    new_placement = move.MovementTransform * current_placement

                # Store the newly calculated placement for this part
                calculated_placements[obj.Name] = new_placement
                final_placements[obj] = new_placement

                # To find the end_pos, calculate the transformation that takes the part
                # from its current_placement to its new_placement...
                delta_transform = new_placement * current_placement.inverse()
                # ...and apply that same transformation to the start_pos.
                end_pos = delta_transform.multVec(start_pos)
                line_positions.append([start_pos, end_pos])

        return final_placements, line_positions

    def getExplodedShape(self, viewObj):
        """
        Generates a compound shape of the exploded assembly in memory
        without modifying the document. Returns a single Part.Compound.
        """
        final_placements, line_positions = self._calculateExplodedPlacements(viewObj)

        exploded_shapes = []

        # We need to include ALL parts of the assembly, not just the moved ones.
        assembly = self.getAssembly(viewObj)
        all_parts = UtilsAssembly.getMovablePartsWithin(assembly, True)
        visible_parts = [
            part for part in all_parts if hasattr(part, "Visibility") and part.Visibility
        ]

        for part in visible_parts:
            # Get the shape. It's crucial to use .copy()
            shape_copy = part.Shape.copy()

            # If the part was moved, use its calculated final placement.
            # Otherwise, use its current placement from the document.
            final_plc = final_placements.get(part, part.Placement)

            shape_copy.Placement = final_plc
            exploded_shapes.append(shape_copy)

        # Add shapes for the explosion lines
        for start_pos, end_pos in line_positions:
            line = self._createSafeLine(start_pos, end_pos)
            if line:
                exploded_shapes.append(line)

        if exploded_shapes:
            return Compound(exploded_shapes)

        return None


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
        return self.app_obj.Group

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
        dialog = Gui.Control.showDialog(panel)
        if dialog is not None:
            dialog.setAutoCloseOnDeletedDocument(True)
            dialog.setDocumentName(App.ActiveDocument.Name)

        return True

    def onDelete(self, vobj, subelements):
        for obj in self.claimChildren():
            obj.Document.removeObject(obj.Name)
        return True


######### Exploded View Move #########
ExplodedViewStepTypes = [
    "Normal",
    "Radial",
]


class ExplodedViewStep:
    def __init__(self, evStep, type_index=0):
        evStep.Proxy = self

        self.createProperties(evStep)

        evStep.MoveType = ExplodedViewStepTypes  # sets the list
        evStep.MoveType = ExplodedViewStepTypes[type_index]  # set the initial value

    def onDocumentRestored(self, evStep):
        self.createProperties(evStep)

    def createProperties(self, evStep):
        self.migrationScript(evStep)

        if not hasattr(evStep, "References"):
            evStep.addProperty(
                "App::PropertyXLinkSubHidden",
                "References",
                "Exploded Move",
                QT_TRANSLATE_NOOP("App::Property", "The objects moved by the move"),
                locked=True,
            )

        if not hasattr(evStep, "MovementTransform"):
            evStep.addProperty(
                "App::PropertyPlacement",
                "MovementTransform",
                "Exploded Move",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the movement of the move. The end placement is the result of the start placement * this placement.",
                ),
                locked=True,
            )

        if not hasattr(evStep, "MoveType"):
            evStep.addProperty(
                "App::PropertyEnumeration",
                "MoveType",
                "Exploded Move",
                QT_TRANSLATE_NOOP("App::Property", "The type of the move"),
                locked=True,
            )

    def migrationScript(self, evStep):
        if hasattr(evStep, "Parts"):
            objNames = evStep.ObjNames
            parts = evStep.Parts

            evStep.removeProperty("ObjNames")
            evStep.removeProperty("Parts")

            evStep.addProperty(
                "App::PropertyXLinkSubHidden",
                "References",
                "Exploded Move",
                QT_TRANSLATE_NOOP("App::Property", "The objects moved by the move"),
                locked=True,
            )

            rootObj = None
            paths = []

            for objName, part in zip(objNames, parts):
                # now we need to get the 'selection-root-obj' and the global path
                obj = UtilsAssembly.getObjectInPart(objName, part)
                rootObj, path = UtilsAssembly.getRootPath(obj, part)
                if rootObj is None:
                    continue
                paths.append(path)
                # Note: all the parts should have the same rootObj.

            evStep.References = [rootObj, paths]

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, evStep, prop):
        """Do something when a property has changed"""
        pass

    def execute(self, fp):
        """Do something when doing a recomputation, this method is mandatory"""
        # App.Console.PrintMessage("Recompute Python Box feature\n")
        pass

    def applyStep(self, move, com=App.Vector(), size=100):
        if not UtilsAssembly.isRefValid(move.References, 1):
            return

        positions = []
        if move.MoveType == "Radial":
            distance = move.MovementTransform.Base.Length
            factor = 4 * distance / size

        subs = move.References[1]
        for sub in subs:
            ref = [move.References[0], [sub]]
            obj = UtilsAssembly.getObject(ref)
            if not obj:
                continue

            if move.ViewObject:
                startPos = UtilsAssembly.getCenterOfBoundingBox([obj], [ref])

            if move.MoveType == "Radial":
                objCom, objSize = UtilsAssembly.getComAndSize(obj)
                init_vec = objCom - com
                obj.Placement.Base = obj.Placement.Base + init_vec * factor
            else:
                obj.Placement = move.MovementTransform * obj.Placement

            if move.ViewObject:
                endPos = UtilsAssembly.getCenterOfBoundingBox([obj], [ref])
                positions.append([startPos, endPos])

        if move.ViewObject:
            move.ViewObject.Proxy.redrawLines(move, positions)

        return positions


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

        if obj in self.viewObj.Group:
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
            for move in self.viewObj.Group:
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
        self.presel_ref = None

        self.blockSetDragger = False
        self.blockDraggerMove = True
        self.currentStep = None

    def accept(self):
        self.deactivate()
        UtilsAssembly.restoreAssemblyPartsPlacements(self.assembly, self.initialPlcs)
        for move in self.viewObj.Group:
            move.Visibility = False
        commands = ""
        for move in self.viewObj.Group:
            more = UtilsAssembly.generatePropertySettings(move)
            commands = commands + more
        Gui.doCommand(commands[:-1])  # Don't use the last \n
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
        self.selectedRefs = []
        self.selectedObjs = []
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
                ref = [sel.Object, [sub_name]]
                obj = UtilsAssembly.getObject(ref)
                moving_part = UtilsAssembly.getMovingPart(self.assembly, ref)
                element_name = UtilsAssembly.getElementName(sub_name)

                # Only objects within the assembly, not the assembly and not elements.
                if obj is None or moving_part is None or obj == self.assembly or element_name != "":
                    Gui.Selection.removeSelection(sel.Object, sub_name)
                    continue

                partAsSolid = self.form.CheckBox_PartsAsSingleSolid.isChecked()
                if partAsSolid:
                    obj = moving_part

                # truncate the sub name at obj.Name
                if partAsSolid:
                    # We handle both cases separately because with external files there
                    # can be several times the same name. For containing part we are sure it's
                    # the first instance, for the object we are sure it's the last.
                    ref[1][0] = UtilsAssembly.truncateSubAtLast(ref[1][0], obj.Name)
                else:
                    ref[1][0] = UtilsAssembly.truncateSubAtFirst(ref[1][0], obj.Name)

                if not obj in self.selectedObjs and hasattr(obj, "Placement"):
                    self.selectedRefs.append(ref)
                    self.selectedObjs.append(obj)
                    self.selectedObjsInitPlc.append(App.Placement(obj.Placement))

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
        for move in self.viewObj.Group:
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
        # clears selection before trying to select the whole assembly
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
            plc = UtilsAssembly.getGlobalPlacement(self.selectedRefs[0], self.selectedObjs[0])
            self.initialDraggerPlc = App.Placement(plc)
            if self.alignMode == "Center":
                self.initialDraggerPlc.Base = UtilsAssembly.getCenterOfBoundingBox(
                    self.selectedObjs, self.selectedRefs
                )

    def setDraggerObjectPlc(self):
        self.findDraggerInitialPlc()

        self.blockDraggerMove = True
        self.assembly.ViewObject.DraggerPlacement = self.initialDraggerPlc
        self.blockDraggerMove = False

    def createExplodedViewObject(self):

        Gui.addModule("UtilsAssembly")
        commands = (
            f'assembly = App.ActiveDocument.getObject("{self.assembly.Name}")\n'
            "view_group = UtilsAssembly.getViewGroup(assembly)\n"
            'viewObj = view_group.newObject("App::FeaturePython", "Exploded View")\n'
            "CommandCreateView.ExplodedView(viewObj)"
        )
        Gui.doCommand(commands)
        self.viewObj = Gui.doCommandEval("viewObj")
        Gui.doCommandGui("CommandCreateView.ViewProviderExplodedView(viewObj.ViewObject)")

    def createExplodedStepObject(self, moveType_index=0):
        commands = (
            f'assembly = App.ActiveDocument.getObject("{self.assembly.Name}")\n'
            'currentStep = assembly.newObject("App::FeaturePython", "Move")\n'
            f"CommandCreateView.ExplodedViewStep(currentStep, {moveType_index})"
        )
        Gui.doCommand(commands)
        self.currentStep = Gui.doCommandEval("currentStep")
        Gui.doCommandGui("CommandCreateView.ViewProviderExplodedViewStep(currentStep.ViewObject)")

        self.currentStep.MovementTransform = App.Placement()

        # Note: the rootObj of all our refs must be the same since all the
        # objects are within assembly. So we put all the sub in a single ref.
        listOfSubs = []
        for ref in self.selectedRefs:
            listOfSubs.append(ref[1][0])
        self.currentStep.References = [self.selectedRefs[0][0], listOfSubs]

        # Note: self.viewObj.Group.append(self.currentStep) does not work
        listOfMoves = self.viewObj.Group
        listOfMoves.append(self.currentStep)
        self.viewObj.Group = listOfMoves

    def dismissCurrentStep(self):
        if self.currentStep is None:
            return

        for obj, init_plc in zip(self.selectedObjs, self.selectedObjsInitPlc):
            obj.Placement = init_plc

        Gui.doCommand(f'App.ActiveDocument.removeObject("{self.currentStep.Name}")')
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
        isRadial = self.currentStep.MoveType == "Radial"
        self.currentStep = None

        if isRadial:
            Gui.Selection.clearSelection()
            return

        # Reset the initial placements
        self.findDraggerInitialPlc()

        for i, obj in enumerate(self.selectedObjs):
            self.selectedObjsInitPlc[i] = App.Placement(obj.Placement)

    def moveMouse(self, info):
        if not self.selectingFeature:
            return

        view = Gui.activeDocument().activeView()
        cursor_info = view.getObjectInfo(view.getCursorPos())

        if not cursor_info or not self.presel_ref:
            self.assembly.ViewObject.DraggerVisibility = False
            return

        ref = self.presel_ref
        element_name = UtilsAssembly.getElementName(ref[1][0])

        if element_name == "":
            vertex_name = ""
        else:
            newPos = App.Vector(cursor_info["x"], cursor_info["y"], cursor_info["z"])
            vertex_name = UtilsAssembly.findElementClosestVertex(self.assembly, ref, newPos)

        ref = UtilsAssembly.addVertexToReference(ref, vertex_name)

        plc = UtilsAssembly.findPlacement(ref)
        global_plc = UtilsAssembly.getGlobalPlacement(ref)
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
                        if row < len(self.viewObj.Group):
                            move = self.viewObj.Group[row]
                            # First remove the link from the viewObj
                            self.viewObj.Group.remove(move)
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
            ref = [App.getDocument(doc_name).getObject(obj_name), [sub_name]]
            obj = UtilsAssembly.getObject(ref)
            moving_part = UtilsAssembly.getMovingPart(self.assembly, ref)

            if obj is None or moving_part is None:
                return

            if self.form.CheckBox_PartsAsSingleSolid.isChecked():
                part = moving_part
            else:
                part = obj

            element_name = UtilsAssembly.getElementName(sub_name)

            if element_name != "":
                # When selecting, we do not want to select an element, but only the containing part.
                Gui.Selection.removeSelection(doc_name, obj_name, sub_name)
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

        element_name = UtilsAssembly.getElementName(sub_name)
        if element_name == "":
            self.setDragger()
            pass

    def setPreselection(self, doc_name, obj_name, sub_name):
        if not self.selectingFeature or not sub_name:
            self.presel_ref = None
            return

        self.presel_ref = [App.getDocument(doc_name).getObject(obj_name), [sub_name]]

    def clearSelection(self, doc_name):
        self.form.stepList.clearSelection()
        self.setDragger()


if App.GuiUp:
    Gui.addCommand("Assembly_CreateView", CommandCreateView())
