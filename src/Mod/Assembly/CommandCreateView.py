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

import math
import FreeCAD as App

from pivy import coin
from Part import LineSegment, Compound, Precision

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui
    from PySide import QtCore, QtGui, QtWidgets
    from PySide.QtWidgets import QPushButton, QMenu

import UtilsAssembly
import Preferences
from SoSwitchMarker import SoSwitchMarker

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

    def unsetupObject(self, obj):
        obj.removeObjectsFromDocument()

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
        if com is None:
            com, size = UtilsAssembly.getComAndSize(self.getAssembly(viewObj))
        finals, drawn = explodedMoves(viewObj, com, size)
        for obj, plc in finals.items():
            obj.Placement = plc
            obj.purgeTouched()
        if App.GuiUp:
            for owner, pairs in drawn:
                view = getattr(owner, "ViewObject", None)
                proxy = getattr(view, "Proxy", None) if view else None
                if proxy is not None and hasattr(proxy, "redrawLines"):
                    proxy.redrawLines(owner, pairs)
        return drawn

    def explodeTemporarily(self, viewObj):
        self.initialPlcs = UtilsAssembly.saveAssemblyPartsPlacements(self.getAssembly(viewObj))
        self.applyMoves(viewObj)
        self.setMoveVisibility(viewObj, True)

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

    def restoreAssembly(self, viewObj):
        if self.initialPlcs is None:
            return

        UtilsAssembly.restoreAssemblyPartsPlacements(self.getAssembly(viewObj), self.initialPlcs)
        self.setMoveVisibility(viewObj, False)

    def setMoveVisibility(self, viewObj, visible):
        for move in getattr(viewObj, "Group", []) or []:
            if hasattr(move, "Visibility"):
                move.Visibility = visible
            for child in getattr(move, "Group", None) or []:
                if hasattr(child, "Visibility"):
                    child.Visibility = visible

    def _calculateExplodedPlacements(self, viewObj):
        """
        Internal helper to calculate final placements for an exploded view without
        applying them.
        Returns:
            - A dictionary mapping {part_object: final_placement}.
            - A list of [start_pos, end_pos] for explosion lines.
        """
        assembly = self.getAssembly(viewObj)
        com, size = UtilsAssembly.getComAndSize(assembly)
        finals, drawn = explodedMoves(viewObj, com, size)
        positions = []
        for _owner, pairs in drawn:
            positions.extend(pairs)
        return finals, positions

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

        self.movePositionMarker = SoSwitchMarker(vobj)
        self.display_mode = coin.SoType.fromName("SoFCSelection").createInstance()
        self.display_mode.addChild(self.movePositionMarker)

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

    def showMovePositionMarker(self, visible, placement=None, ref=None):
        if visible:
            assembly = self.app_obj.Proxy.getAssembly(self.app_obj)
            if assembly and ref and placement:
                asm_global_plc = assembly.getGlobalPlacement()
                if asm_global_plc != App.Placement():
                    placement = asm_global_plc.inverse() * UtilsAssembly.getJcsGlobalPlc(placement, ref)
                    ref = None
            self.movePositionMarker.whichChild = coin.SO_SWITCH_ALL
            self.movePositionMarker.set_marker_placement(placement, ref)
        else:
            self.movePositionMarker.whichChild = coin.SO_SWITCH_NONE

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
        # Issue #32494: re-opening while a task is live re-applies explosion moves
        # and openCommand may commit the create/edit transaction into the assembly.
        if Gui.Control.activeDialog():
            return True

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


######### Exploded View Move #########
ExplodedViewStepTypes = [
    "Normal",
    "Radial",
]


# SoTransformDragger stores its position in an SbVec3f.
SINGLE_PRECISION_EPSILON = 2**-23


def setMovementDistance(move, distance, direction=None):
    """Set a move's translation length while preserving its direction and rotation."""
    transform = App.Placement(move.MovementTransform)
    base = transform.Base

    if base.Length > 0:
        direction = App.Vector(base)
    elif direction is not None and direction.Length > 0:
        direction = App.Vector(direction)
    elif move.MoveType == "Radial":
        # Radial moves only use the vector length, not its direction.
        direction = App.Vector(1, 0, 0)
    else:
        return None

    direction.normalize()
    transform.Base = direction * max(0.0, distance)
    move.MovementTransform = transform
    return direction


def movementEditMode(move):
    if move.MoveType == "Radial":
        return "Distance"
    if isPureRotationMovement(move.MovementTransform):
        return "Angle"
    return "Distance"


def movementLabel(move):
    if move.MoveType == "Radial":
        return QtWidgets.QApplication.translate("Assembly", "Radial Translation")
    if movementEditMode(move) == "Angle":
        return QtWidgets.QApplication.translate("Assembly", "Rotation")
    return QtWidgets.QApplication.translate("Assembly", "Translation")


def isPureRotationMovement(transform):
    """Return True when a placement is a rotation without translation along its axis."""
    rotation = transform.Rotation
    if rotation.Angle <= Precision.angular():
        return False

    axis = App.Vector(rotation.Axis)
    if axis.Length <= Precision.confusion():
        return False
    axis.normalize()

    # A pure rotation around an arbitrary center usually has a non-zero Base
    # term (center - R * center). The meaningful test is whether the transform
    # also translates along the rotation axis. If it does, it is a screw/mixed
    # movement, so the distance editor is more appropriate.
    translation = App.Vector(transform.Base)
    return abs(translation.dot(axis)) <= Precision.confusion()


def draggerTranslationTolerance(initialPlacement, currentPlacement):
    """Tolerance for translation noise from the dragger's single-precision position."""
    coordinateScale = max(
        abs(initialPlacement.Base.x),
        abs(initialPlacement.Base.y),
        abs(initialPlacement.Base.z),
        abs(currentPlacement.Base.x),
        abs(currentPlacement.Base.y),
        abs(currentPlacement.Base.z),
        1.0,
    )
    return max(
        Precision.confusion(),
        4 * SINGLE_PRECISION_EPSILON * coordinateScale,
    )


def rotationCenterFromTransform(transform):
    """Return a point on the axis of a rotation transform."""
    angle = transform.Rotation.Angle
    if angle <= Precision.angular():
        return App.Vector()

    axis = App.Vector(transform.Rotation.Axis)
    axis.normalize()
    translation = App.Vector(transform.Base)
    perpendicular = translation - axis * translation.dot(axis)
    cotangent = 1.0 / math.tan(angle / 2.0)
    return (perpendicular + axis.cross(perpendicular) * cotangent) * 0.5


def placementIsIdentity(transform):
    return (
        transform.Base.Length <= Precision.confusion()
        and transform.Rotation.Angle <= Precision.angular()
    )


def moveObjectsAndReferences(move):
    if not hasattr(move, "References"):
        return [], []
    if not UtilsAssembly.isRefValid(move.References, 1):
        return [], []

    objs = []
    refs = []
    for sub in move.References[1]:
        ref = [move.References[0], [sub]]
        obj = UtilsAssembly.getObject(ref)
        if obj is None or not hasattr(obj, "Placement"):
            continue
        objs.append(obj)
        refs.append(ref)
    return objs, refs


def childMoves(obj):
    return [
        member
        for member in (getattr(obj, "Group", None) or [])
        if hasattr(member, "MovementTransform")
    ]


def movementStages(viewObj):
    """Yield (group, move). The group owns the parts. Each child is one move."""
    for child in list(getattr(viewObj, "Group", []) or []):
        for transform in childMoves(child):
            yield child, transform


def referencePartKeys(references):
    """Part names in a reference path, ignoring the feature picked under the part."""
    keys = set()
    if references and len(references) > 1:
        for sub in references[1]:
            head = sub.split(".")[0] if sub else ""
            if head:
                keys.add(head)
    return keys


def applyStage(transform, current, obj, com, size):
    """Placement after one move. Same rule as a single exploded step."""
    if getattr(transform, "MoveType", "Normal") == "Radial":
        factor = 4 * transform.MovementTransform.Base.Length / size if size else 0
        obj_com, _obj_size = UtilsAssembly.getComAndSize(obj)
        return App.Placement(current.Base + (obj_com - com) * factor, current.Rotation)
    return transform.MovementTransform * current


def chainAfterMovements(start_point, start_placement, transforms, obj, com, size):
    """Return the final placement and the points along its move chain."""
    point = App.Vector(start_point)
    current = App.Placement(start_placement)
    points = [App.Vector(point)]
    for transform in transforms:
        new_placement = applyStage(transform, current, obj, com, size)
        point = (new_placement * current.inverse()).multVec(point)
        points.append(App.Vector(point))
        current = new_placement
    return current, points


def explodedMoves(viewObj, com, size):
    """Final part placements and one line per part.

    Each part keeps its own line start. Parts without one start at their origin.
    """
    finals = {}
    drawn = []
    for owner in viewObj.Group:
        transforms = childMoves(owner)
        if not transforms:
            continue
        objs, _refs = moveObjectsAndReferences(owner)
        if not objs:
            continue
        starts = list(getattr(owner, "LineStarts", []) or [])
        positions = []
        for i, obj in enumerate(objs):
            current = finals.get(obj, obj.Placement)
            local = starts[i] if i < len(starts) else App.Placement()
            final, points = chainAfterMovements((current * local).Base, current, transforms, obj, com, size)
            finals[obj] = final
            positions.extend(list(pair) for pair in zip(points, points[1:]))
        drawn.append((owner, positions))
    return finals, drawn


def setMovementAngle(move, angle):
    """Set a move's rotation angle while preserving its axis and center."""
    transform = App.Placement(move.MovementTransform)
    if not isPureRotationMovement(transform):
        return False

    axis = App.Vector(transform.Rotation.Axis)
    if axis.Length <= Precision.confusion():
        return False
    axis.normalize()

    center = rotationCenterFromTransform(transform)
    rotation = App.Rotation(axis, angle)
    pivot = App.Placement(center, App.Rotation())
    move.MovementTransform = App.Placement(center, rotation) * pivot.inverse()
    return True


class ExplodedViewStep:
    def __init__(self, evStep, type_index=0):
        evStep.Proxy = self
        if not hasattr(evStep, "Group"):
            evStep.addExtension("App::GroupExtensionPython")

        self.createProperties(evStep)

        evStep.MoveType = ExplodedViewStepTypes  # sets the list
        evStep.MoveType = ExplodedViewStepTypes[type_index]  # set the initial value

    def onDocumentRestored(self, evStep):
        if not hasattr(evStep, "Group"):
            evStep.addExtension("App::GroupExtensionPython")
        self.createProperties(evStep)

    def unsetupObject(self, obj):
        obj.removeObjectsFromDocument()

    def createProperties(self, evStep):
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

    def claimChildren(self):
        obj = getattr(self, "app_obj", None)
        if obj is None:
            return []
        return list(getattr(obj, "Group", []) or [])

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
    def __init__(self, task):
        self.task = task

    def allow(self, doc, obj, sub):
        if self.task.selectingFeature:
            comp, new_sub = UtilsAssembly.getComponentReference(self.task.assembly, obj, sub)
            return bool(comp) and self.task._referenceMoves([comp, [new_sub]])

        assembly = self.task.assembly
        viewObj = self.task.viewObj
        comp, new_sub = UtilsAssembly.getComponentReference(assembly, obj, sub)
        if comp:
            # Objects within the assembly.
            return True

        if obj == viewObj:
            return True

        if obj in viewObj.Group:
            return True

        for group in viewObj.Group:
            if obj in (getattr(group, "Group", None) or []):
                return True

        return False


######### Create Exploded View Task ###########
class TaskAssemblyCreateView(QtCore.QObject):
    def __init__(self, viewObj=None):
        super().__init__()

        self.form = Gui.PySideUic.loadUi(":/panels/TaskAssemblyCreateView.ui")
        self.form.stepList.installEventFilter(self)
        self.form.stepList.viewport().installEventFilter(self)
        self.form.stepList.itemClicked.connect(self.onItemClicked)
        self.form.stepList.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.form.stepList.customContextMenuRequested.connect(self.onStepContextMenu)

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

        self._addPositionMenu(self.form.btnAlignDragger, self.alignDragger, "dragger")
        self.enableDragger(False)
        self.form.btnRadialExplosion.clicked.connect(self.onRadialClicked)

        pref = Preferences.preferences()
        self.form.CheckBox_PartsAsSingleSolid.setChecked(pref.GetBool("PartsAsSingleSolid", True))

        self.initialPlcs = UtilsAssembly.saveAssemblyPartsPlacements(self.assembly)
        self.moveSpinboxes = {}
        self.moveDirections = {}
        self.selectedRefs = []
        self.selectedObjs = []
        self.selectedObjsInitPlc = []
        self._transaction = None
        self._undoCheckpoint = App.ActiveDocument.UndoCount

        if viewObj:
            self.viewObj = viewObj
            viewObj.Proxy.setMoveVisibility(viewObj, True)
            self.onMovesChanged()
        else:
            self._beginEdit("Create Exploded View")
            self.createExplodedViewObject()
            self._commitEdit()

        self.selectingFeature = None
        self._pendingLineStarts = {}
        Gui.Selection.addSelectionGate(
            ExplodedViewSelGate(self), Gui.Selection.ResolveMode.NoResolve
        )
        Gui.Selection.addObserver(self, Gui.Selection.ResolveMode.NoResolve)

        self.viewObj.Proxy.setMovesChangedCallback(self.onMovesChanged)
        self.callbackMove = view.addEventCallback("SoLocation2Event", self.moveMouse)
        self.callbackClick = view.addEventCallback("SoMouseButtonEvent", self.clickMouse)
        self.callbackKey = view.addEventCallback("SoKeyboardEvent", self.KeyboardEvent)

        self.selectingFeature = None
        self.form.LabelAlignDragger.setVisible(False)
        self.presel_ref = None

        self.blockSetDragger = False
        self.blockDraggerMove = True
        self.currentStep = None
        self._dragStartTransform = None
        self.selectedTransform = None
        self.movePositionMove = None
        self.radialExplosion = False

        self.viewObj.purgeTouched()

    def accept(self):
        self._commitEdit()
        UtilsAssembly.restoreAssemblyPartsPlacements(self.assembly, self.initialPlcs)
        self.viewObj.Proxy.setMoveVisibility(self.viewObj, False)
        self.deactivate()
        self.viewObj.purgeTouched()
        return True

    def reject(self):
        self._abortEdit()
        if getattr(self, "initialPlcs", None):
            UtilsAssembly.restoreAssemblyPartsPlacements(self.assembly, self.initialPlcs)
        if getattr(self, "viewObj", None) is not None:
            self.viewObj.Proxy.setMoveVisibility(self.viewObj, False)
        self.deactivate()
        doc = App.ActiveDocument
        checkpoint = getattr(self, "_undoCheckpoint", None)
        if doc is not None and checkpoint is not None:
            while doc.UndoCount > checkpoint:
                doc.undo()
        if doc is not None:
            doc.recompute()
        return True

    def _beginEdit(self, name):
        if getattr(self, "_transaction", None):
            return False
        gui_doc = getattr(Gui, "ActiveDocument", None)
        if gui_doc is None:
            return False
        gui_doc.openCommand(name)
        self._transaction = name
        return True

    def _commitEdit(self):
        if not getattr(self, "_transaction", None):
            return
        if getattr(self, "initialPlcs", None) and getattr(self, "assembly", None):
            UtilsAssembly.restoreAssemblyPartsPlacements(self.assembly, self.initialPlcs)
        Gui.ActiveDocument.commitCommand()
        self._transaction = None

    def _abortEdit(self):
        if not getattr(self, "_transaction", None):
            return
        Gui.ActiveDocument.abortCommand()
        self._transaction = None
        self.currentStep = None
        self._dragStartTransform = None

    def deactivate(self):
        pref = Preferences.preferences()
        pref.SetBool("PartsAsSingleSolid", self.form.CheckBox_PartsAsSingleSolid.isChecked())

        view = Gui.activeDocument().activeView()
        view.removeDraggerCallback(self.asmDragger, "addFinishCallback", self.cbFin)
        view.removeDraggerCallback(self.asmDragger, "addMotionCallback", self.cbMov)

        self.assembly.ViewObject.DraggerVisibility = False
        self.assembly.ViewObject.EnableMovement = True
        self.viewObj.ViewObject.Proxy.showMovePositionMarker(False)

        if self.selectingFeature:
            Gui.Selection.setSelectionStyle(Gui.Selection.SelectionStyle.NormalSelection)
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
        self._pendingLineStarts = {}
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
                moving_part, new_sub = UtilsAssembly.getComponentReference(
                    self.assembly, sel.Object, sub_name
                )
                if not moving_part:
                    continue

                ref = [moving_part, [new_sub]]
                obj = UtilsAssembly.getObject(ref)
                element_name = UtilsAssembly.getElementName(sub_name)

                # Only objects within the assembly, not the assembly and not elements.
                if obj is None or moving_part is None or obj == self.assembly or element_name != "":
                    Gui.Selection.removeSelection(sel.Object, sub_name)
                    continue

                partAsSolid = self.form.CheckBox_PartsAsSingleSolid.isChecked()
                if partAsSolid:
                    obj = moving_part
                    # new_sub starts after the part, so it does not contain the part name.
                    # Cut the original selection at the part. That reference is the part.
                    sub = UtilsAssembly.truncateSubAtLast(sub_name, obj.Name) or (obj.Name + ".")
                    ref = [sel.Object, [sub]]
                else:
                    ref[1][0] = UtilsAssembly.truncateSubAtFirst(ref[1][0], obj.Name)

                if not obj in self.selectedObjs and hasattr(obj, "Placement"):
                    self.selectedRefs.append(ref)
                    self.selectedObjs.append(obj)
                    self.selectedObjsInitPlc.append(App.Placement(obj.Placement))

        if not self.selectedObjs:
            self.enableDragger(False)
            return

        self.enableDragger(True)
        group = getattr(self, "movePositionMove", None)
        if not hasattr(group, "References"):
            group = self._groupForNewMove()
            self.movePositionMove = group
            self.selectedTransform = None
            if group is not None:
                self._selectGroup(group)
        if group is not None:
            self._placeGroupDragger()
        else:
            self._setDraggerPlacement(self._selectedPartPlacement())

    def enableDragger(self, val):
        self.assembly.ViewObject.DraggerVisibility = val
        self.form.btnAlignDragger.setEnabled(val)

    def onMovesChanged(self):
        # First reset positions
        self._previewMoves()
        self.rebuildStepList()

    def _previewMoves(self):
        if getattr(self, "initialPlcs", None) is None or getattr(self, "viewObj", None) is None:
            return
        UtilsAssembly.restoreAssemblyPartsPlacements(self.assembly, self.initialPlcs)
        self.viewObj.Proxy.applyMoves(self.viewObj, self.com, self.size)

    def _refreshMoves(self, move=None):
        """Parts, lines, and the dragger follow the stored moves.

        A drag already holds the dragger, so this leaves it where the pointer put it.
        The arrows and the placement editor do not, so the dragger is moved onto the
        same LCS the part just followed.
        """
        self._previewMoves()
        if getattr(self, "currentStep", None) is not None:
            return

        owner = self._ownerOf(move) if move is not None else getattr(self, "movePositionMove", None)
        if owner is None or not hasattr(owner, "References"):
            return
        self.movePositionMove = owner
        self.selectedObjs, self.selectedRefs = moveObjectsAndReferences(owner)
        self._placeGroupDragger()
        self.syncDraggerBaseline()

    def rebuildStepList(self):
        tree = self.form.stepList
        tree.clear()
        self.moveSpinboxes.clear()
        parent = None
        parent_owner = None
        for owner, move in movementStages(self.viewObj):
            if parent_owner is not owner:
                parent_owner = owner
                parent = QtWidgets.QTreeWidgetItem([getattr(owner, "Label", owner.Name)])
                parent.setData(0, QtCore.Qt.UserRole, owner.Name)
                tree.addTopLevelItem(parent)
                parent.setExpanded(True)

            item = QtWidgets.QTreeWidgetItem([""])
            item.setData(0, QtCore.Qt.UserRole, move.Name)
            parent.addChild(item)

            row = QtWidgets.QWidget(tree)
            layout = QtWidgets.QHBoxLayout(row)
            layout.setContentsMargins(4, 0, 4, 0)
            row.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
            row.customContextMenuRequested.connect(
                lambda position, currentItem=item, widget=row: self.showStepContextMenu(
                    currentItem, widget.mapToGlobal(position)
                )
            )

            label = QtWidgets.QLabel(getattr(move, "Label", getattr(move, "Name", "")), row)
            layout.addWidget(label)
            layout.addStretch()

            mode = movementEditMode(move)
            if mode in ("Distance", "Angle"):
                spinbox = Gui.UiLoader().createWidget("Gui::QuantitySpinBox")
                spinbox.setParent(row)
                spinbox.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
                spinbox.customContextMenuRequested.connect(
                    lambda position, currentItem=item, widget=spinbox: self.showStepContextMenu(
                        currentItem, widget.mapToGlobal(position)
                    )
                )
                if mode == "Angle":
                    spinbox.setProperty("unit", "deg")
                    spinbox.setProperty("minimum", -360.0)
                    spinbox.setProperty("maximum", 360.0)
                    spinbox.setProperty(
                        "rawValue",
                        math.degrees(move.MovementTransform.Rotation.Angle),
                    )
                    spinbox.setToolTip(
                        QtWidgets.QApplication.translate(
                            "Assembly", "Angle of this exploded-view move"
                        )
                    )
                else:
                    spinbox.setProperty("unit", "mm")
                    spinbox.setProperty("minimum", 0.0)
                    spinbox.setProperty("maximum", 1.0e9)
                    spinbox.setProperty("rawValue", move.MovementTransform.Base.Length)
                    spinbox.setToolTip(
                        QtWidgets.QApplication.translate(
                            "Assembly", "Distance of this exploded-view move"
                        )
                    )
                spinbox.valueChanged.connect(
                    lambda _value, currentMove=move: self.onMoveValueChanged(currentMove)
                )
                layout.addWidget(spinbox)
                self.moveSpinboxes[move.Name] = spinbox

            item.setSizeHint(0, row.sizeHint())
            tree.setItemWidget(item, 0, row)
            row.installEventFilter(self)
            for child in row.findChildren(QtWidgets.QWidget):
                child.installEventFilter(self)

            if mode == "Distance" and move.MovementTransform.Base.Length > 0:
                direction = App.Vector(move.MovementTransform.Base)
                direction.normalize()
                self.moveDirections[move.Name] = direction

    def _itemName(self, item):
        return item.data(0, QtCore.Qt.UserRole)

    def _selectGroup(self, group):
        tree = self.form.stepList
        tree.blockSignals(True)
        for index in range(tree.topLevelItemCount()):
            item = tree.topLevelItem(index)
            if item.data(0, QtCore.Qt.UserRole) == group.Name:
                tree.setCurrentItem(item)
                item.setSelected(True)
                break
        tree.blockSignals(False)

    def _groupForNewMove(self):
        groups = self.viewObj.Group
        if not groups:
            return None
        last = groups[-1]
        same_parts = referencePartKeys(last.References) == self._selectedPartKeys()
        if same_parts and any(move.MoveType != "Radial" for move in childMoves(last)):
            return last
        return None

    def _ownerOf(self, move):
        view = getattr(self, "viewObj", None)
        if view is None or move is None:
            return None
        for child in getattr(view, "Group", []) or []:
            if child is move or getattr(child, "Name", None) == getattr(move, "Name", None):
                return child
            for member in getattr(child, "Group", None) or []:
                if member is move or getattr(member, "Name", None) == getattr(move, "Name", None):
                    return child
        return None

    def _clearTaskSelection(self):
        tree = getattr(getattr(self, "form", None), "stepList", None)
        if tree is not None and hasattr(tree, "clearSelection"):
            tree.blockSignals(True)
            tree.clearSelection()
            tree.setCurrentItem(None)
            tree.blockSignals(False)
        self.selectedTransform = None
        self.movePositionMove = None

    def onItemClicked(self, item, column=0):
        move = self.viewObj.Document.getObject(self._itemName(item))
        if move is None:
            return
        group = self._ownerOf(move) or move
        self.selectedTransform = move if move is not group and hasattr(move, "MovementTransform") else None
        owner = group if hasattr(group, "References") else move
        _objs, refs = moveObjectsAndReferences(owner)
        self.blockSetDragger = True
        Gui.Selection.clearSelection()
        for ref in refs:
            Gui.Selection.addSelection(ref[0], ref[1][0] if ref[1] else "")
        self.blockSetDragger = False
        self.movePositionMove = owner
        self.setDragger()

    def onStepContextMenu(self, position):
        item = self.form.stepList.itemAt(position)
        if item is None:
            return
        self.showStepContextMenu(item, self.form.stepList.viewport().mapToGlobal(position))

    def showStepContextMenu(self, item, globalPosition):
        self.form.stepList.setCurrentItem(item)
        move = self.viewObj.Document.getObject(self._itemName(item))
        if move is None or not hasattr(move, "MovementTransform"):
            return

        menu = QMenu(self.form.stepList)
        editPlacement = menu.addAction(
            QtWidgets.QApplication.translate("Assembly", "Edit placement")
        )
        selectedAction = menu.exec_(globalPosition)
        if selectedAction == editPlacement:
            previousTransform = [App.Placement(move.MovementTransform)]

            def onPlacementChanged():
                self.onMovePlacementChanged(move, previousTransform[0])
                previousTransform[0] = App.Placement(move.MovementTransform)

            UtilsAssembly.openEditingPlacementDialog(
                move,
                "MovementTransform",
                onPlacementChanged,
            )

    def onMovePlacementChanged(self, move, oldTransform=None):
        if oldTransform is not None:
            self.adjustDependentMoves(move, oldTransform)
        self._refreshMoves(move)
        self.rebuildStepList()

    def onMoveValueChanged(self, move):
        spinbox = self.moveSpinboxes.get(move.Name)
        if spinbox is None:
            return

        value = spinbox.property("rawValue")
        oldTransform = App.Placement(move.MovementTransform)
        self._beginEdit("Edit exploded transform")
        if movementEditMode(move) == "Angle":
            if not setMovementAngle(move, value):
                self.updateMoveSpinbox(move)
                self._abortEdit()
                return
        else:
            direction = setMovementDistance(move, value, self.moveDirections.get(move.Name))
            if direction is None:
                self.updateMoveSpinbox(move)
                self._abortEdit()
                return
            self.moveDirections[move.Name] = direction

        self.adjustDependentMoves(move, oldTransform)
        self._commitEdit()
        self._refreshMoves(move)

    def adjustDependentMoves(self, changedMove, oldChangedTransform):
        stages = list(movementStages(self.viewObj))
        moves = [transform for _owner, transform in stages]
        owners = {id(transform): owner for owner, transform in stages}
        try:
            changedIndex = moves.index(changedMove)
        except ValueError:
            return

        oldPlacements = {
            name: App.Placement(placement) for name, placement in self.initialPlcs.items()
        }
        newPlacements = oldPlacements.copy()

        for i, move in enumerate(moves):
            objs, _refs = moveObjectsAndReferences(owners.get(id(move), move))
            objNames = [obj.Name for obj in objs]
            transformBeforeAdjustment = App.Placement(move.MovementTransform)

            if i > changedIndex and move.MoveType == "Normal":
                for objName in objNames:
                    oldPlacement = oldPlacements.get(objName)
                    newPlacement = newPlacements.get(objName)
                    if oldPlacement is None or newPlacement is None:
                        continue

                    delta = newPlacement * oldPlacement.inverse()
                    if placementIsIdentity(delta):
                        continue

                    move.MovementTransform = delta * move.MovementTransform * delta.inverse()
                    break

            oldTransform = oldChangedTransform if move == changedMove else transformBeforeAdjustment
            newTransform = move.MovementTransform
            for objName in objNames:
                if objName in oldPlacements:
                    oldPlacements[objName] = oldTransform * oldPlacements[objName]
                if objName in newPlacements:
                    newPlacements[objName] = newTransform * newPlacements[objName]

    def syncDraggerBaseline(self):
        self.initialDraggerPlc = App.Placement(self.assembly.ViewObject.DraggerPlacement)
        self.selectedObjsInitPlc = [
            App.Placement(obj.Placement) for obj in getattr(self, "selectedObjs", [])
        ]

    def updateMoveSpinbox(self, move):
        spinbox = self.moveSpinboxes.get(move.Name)
        if spinbox is None:
            return

        if movementEditMode(move) == "Angle":
            value = math.degrees(move.MovementTransform.Rotation.Angle)
        else:
            base = move.MovementTransform.Base
            value = base.Length
            if base.Length > 0:
                direction = App.Vector(base)
                direction.normalize()
                self.moveDirections[move.Name] = direction

        spinbox.blockSignals(True)
        spinbox.setProperty("rawValue", value)
        spinbox.blockSignals(False)

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

        self.radialExplosion = True

    def _addPositionMenu(self, button, apply, target):
        menu = QMenu(button)
        button.setMenu(menu)
        translate = QtWidgets.QApplication.translate
        menu.addAction(translate("Assembly", "Origin")).triggered.connect(lambda: apply("Origin"))
        menu.addAction(translate("Assembly", "Center")).triggered.connect(lambda: apply("Center"))
        menu.addAction(translate("Assembly", "Reference…")).triggered.connect(
            lambda: self._pickReference(target)
        )

    def _pickReference(self, target):
        self.selectingFeature = target
        # We use greedy selection to prevent that clicking again on the solid
        # clears selection before trying to select the whole assembly
        Gui.Selection.setSelectionStyle(Gui.Selection.SelectionStyle.GreedySelection)
        self.enableDragger(False)
        self.viewObj.ViewObject.Proxy.showMovePositionMarker(False)
        self.form.LabelAlignDragger.setVisible(True)

    def endSelectionMode(self):
        self.selectingFeature = None
        self.presel_ref = None
        if getattr(self, "viewObj", None) is not None:
            self.viewObj.ViewObject.Proxy.showMovePositionMarker(False)
        if getattr(self, "selectedObjs", None):
            self.enableDragger(True)
        Gui.Selection.setSelectionStyle(Gui.Selection.SelectionStyle.NormalSelection)
        self.form.LabelAlignDragger.setVisible(False)

    def _referenceMoves(self, ref):
        if not ref or ref[0] in self.selectedObjs:
            return bool(ref)
        return UtilsAssembly.getObject(ref) in self.selectedObjs

    def _selectedPartPlacement(self):
        return UtilsAssembly.getGlobalPlacement(self.selectedRefs[0], self.selectedObjs[0])

    def _setDraggerPlacement(self, placement):
        self.initialDraggerPlc = App.Placement(placement)
        self.blockDraggerMove = True
        self.assembly.ViewObject.DraggerPlacement = self.initialDraggerPlc
        self.blockDraggerMove = False

    def _alignTarget(self, ref):
        for obj, part_ref in zip(self.selectedObjs, self.selectedRefs):
            if ref[0] is obj or (part_ref and ref[0] is part_ref[0]):
                return obj, part_ref
        picked = UtilsAssembly.getObject(ref)
        for obj, part_ref in zip(self.selectedObjs, self.selectedRefs):
            if picked is obj:
                return obj, part_ref
        return self.selectedObjs[0], self.selectedRefs[0]

    def alignDragger(self, mode, ref=None):
        if not self.selectedObjs:
            return
        if mode == "Reference":
            if not UtilsAssembly.isRefValid(ref, 1) or not self._referenceMoves(ref):
                return
            placement = UtilsAssembly.getGlobalPlacement(ref) * UtilsAssembly.findPlacement(ref)
            targets = [self._alignTarget(ref)]
        else:
            placement = self._selectedPartPlacement()
            targets = list(zip(self.selectedObjs, self.selectedRefs))
            if mode == "Center":
                placement.Base = UtilsAssembly.getCenterOfBoundingBox(
                    self.selectedObjs[:1], self.selectedRefs[:1]
                )

        self._setDraggerPlacement(placement)
        pending = getattr(self, "_pendingLineStarts", None)
        if pending is None:
            self._pendingLineStarts = {}
            pending = self._pendingLineStarts
        for obj, part_ref in targets:
            part_plc = UtilsAssembly.getGlobalPlacement(part_ref, obj)
            if mode == "Center":
                center = UtilsAssembly.getCenterOfBoundingBox([obj], [part_ref])
                local = App.Placement(part_plc.inverse().multVec(center), App.Rotation())
            elif mode == "Reference":
                local = part_plc.inverse() * placement
            else:
                local = App.Placement()
            pending[obj] = local
        group = self.movePositionMove
        if group is not None:
            self._beginEdit("Align dragger")
            self._saveLineStarts(group)
            self._saveDraggerStart(group)
            self._commitEdit()
            self._refreshMoves()

    def _saveLineStarts(self, group):
        pending = getattr(self, "_pendingLineStarts", None)
        if not pending:
            return
        objs, _refs = moveObjectsAndReferences(group)
        starts = [App.Placement(item) for item in getattr(group, "LineStarts", [])] if hasattr(group, "LineStarts") else []
        while len(starts) < len(objs):
            starts.append(App.Placement())
        for i, obj in enumerate(objs):
            local = pending.get(obj)
            if local is not None:
                starts[i] = App.Placement(local)
        if not hasattr(group, "LineStarts"):
            group.addProperty(
                "App::PropertyPlacementList",
                "LineStarts",
                "Exploded Move",
                QT_TRANSLATE_NOOP("App::Property", "Line start relative to each moved part"),
                locked=True,
            )
        group.LineStarts = starts

    def _saveDraggerStart(self, group):
        if not hasattr(group, "DraggerStart"):
            group.addProperty(
                "App::PropertyPlacement",
                "DraggerStart",
                "Exploded Move",
                QT_TRANSLATE_NOOP("App::Property", "Dragger placement relative to the first moved part"),
                locked=True,
            )
        group.DraggerStart = self._selectedPartPlacement().inverse() * self.initialDraggerPlc

    def _placeGroupDragger(self):
        group = self.movePositionMove if hasattr(getattr(self, "movePositionMove", None), "References") else None
        if group is None or not self.selectedObjs:
            return

        start = getattr(group, "DraggerStart", App.Placement())
        self._setDraggerPlacement(self._selectedPartPlacement() * start)

    def createExplodedViewObject(self):
        view_group = UtilsAssembly.getViewGroup(self.assembly)
        self.viewObj = view_group.newObject("App::FeaturePython", "Exploded View")
        ExplodedView(self.viewObj)
        if App.GuiUp and getattr(self.viewObj, "ViewObject", None):
            ViewProviderExplodedView(self.viewObj.ViewObject)

    def _selectedPartKeys(self):
        keys = set()
        for ref, obj in zip(self.selectedRefs, self.selectedObjs):
            keys |= referencePartKeys(ref)
            name = getattr(obj, "Name", None)
            root_name = getattr(ref[0], "Name", None) if ref and ref[0] is not None else None
            if name and (not keys or name == root_name or name in keys):
                keys.add(name)
        return keys

    def createExplodedStepObject(self):
        type_index = 1 if self.radialExplosion else 0
        self.radialExplosion = False
        self._dragStartTransform = None
        if type_index == 0 and self.selectedTransform is not None and hasattr(self.selectedTransform, "MovementTransform"):
            self.currentStep = self.selectedTransform
            self._dragStartTransform = App.Placement(self.currentStep.MovementTransform)
            return
        if not self.selectedRefs:
            return

        root = self.selectedRefs[0][0]
        subs = []
        for ref in self.selectedRefs:
            if getattr(ref[0], "Name", None) != getattr(root, "Name", None):
                continue
            sub = ref[1][0] if ref[1] else ""
            if sub and sub not in subs:
                subs.append(sub)
        if not subs:
            return

        # Reuse only the last group so new moves stay in creation order.
        group = self._groupForNewMove() if type_index == 0 else None

        new_group = group is None
        if new_group:
            group = self.viewObj.Document.addObject("App::FeaturePython", "Move")
            ExplodedViewStep(group)
            if App.GuiUp and getattr(group, "ViewObject", None):
                ViewProviderExplodedViewStep(group.ViewObject)
            group.References = [root, subs]
        self._saveLineStarts(group)
        self._saveDraggerStart(group)

        transform = self.viewObj.Document.addObject("App::FeaturePython", "Move")
        ExplodedViewStep(transform, type_index)
        if App.GuiUp and getattr(transform, "ViewObject", None):
            ViewProviderExplodedViewStep(transform.ViewObject)
        transform.MovementTransform = App.Placement()
        transform.Label = movementLabel(transform)
        group.Group = list(group.Group) + [transform]
        if group not in self.viewObj.Group:
            self.viewObj.Group = list(self.viewObj.Group) + [group]
        self.currentStep = transform
        if type_index == 0:
            self.movePositionMove = group
            self.selectedTransform = None

    def dismissCurrentStep(self):
        if self.currentStep is None:
            return

        for obj, init_plc in zip(self.selectedObjs, self.selectedObjsInitPlc):
            obj.Placement = init_plc

        if getattr(self, "_transaction", None):
            self._abortEdit()
        else:
            move = self.currentStep
            self.currentStep = None
            if self._dragStartTransform is None:
                move.Document.removeObject(move.Name)
            else:
                self._applyDragDelta(move, App.Placement())
                self._dragStartTransform = None

        Gui.Selection.clearSelection()

    def draggerMoved(self, event):
        if self.blockDraggerMove or self.selectingFeature:
            return

        if self.currentStep is None:
            self._beginEdit("Explode")
            self.createExplodedStepObject()
        if self.currentStep is None:
            self._abortEdit()
            return

        move = self.currentStep
        draggerPlc = self.assembly.ViewObject.DraggerPlacement
        previousEditorMode = movementEditMode(move)
        delta = draggerPlc * self.initialDraggerPlc.inverse()
        translation = (draggerPlc.Base - self.initialDraggerPlc.Base).Length
        if translation > draggerTranslationTolerance(self.initialDraggerPlc, draggerPlc):
            delta = App.Placement(draggerPlc.Base - self.initialDraggerPlc.Base, App.Rotation())
        elif move.MoveType != "Radial" and delta.Rotation.Angle > Precision.angular():
            delta.Base = (
                self.initialDraggerPlc.Base
                - delta.Rotation.multVec(self.initialDraggerPlc.Base)
            )

        self._applyDragDelta(move, delta)
        if movementEditMode(move) != previousEditorMode:
            move.Label = movementLabel(move)
            self.rebuildStepList()
        self.updateMoveSpinbox(move)
        self._refreshMoves()

    def _applyDragDelta(self, move, delta):
        start = self._dragStartTransform
        if start is None:
            move.MovementTransform = delta
        else:
            previous = App.Placement(move.MovementTransform)
            move.MovementTransform = delta * start
            self.adjustDependentMoves(move, previous)

    def draggerFinished(self, event):
        finished = self.currentStep
        isRadial = finished is not None and finished.MoveType == "Radial"
        group = self._ownerOf(finished) if finished is not None else None
        self._commitEdit()
        self.currentStep = None
        self._dragStartTransform = None
        self.onMovesChanged()

        if isRadial:
            Gui.Selection.clearSelection()
            return

        if group is not None:
            self.movePositionMove = group
            self._placeGroupDragger()
        self.syncDraggerBaseline()

    def moveMouse(self, info):
        if not self.selectingFeature:
            return

        view = Gui.activeDocument().activeView()
        cursor_info = view.getObjectInfo(view.getCursorPos())

        if not cursor_info:
            self.presel_ref = None
        if not cursor_info or not self.presel_ref or not self._referenceMoves(self.presel_ref):
            self.viewObj.ViewObject.Proxy.showMovePositionMarker(False)
            return

        ref = self.presel_ref
        element_name = UtilsAssembly.getElementName(ref[1][0])

        if element_name == "":
            vertex_name = ""
        else:
            newPos = App.Vector(cursor_info["x"], cursor_info["y"], cursor_info["z"])
            vertex_name = UtilsAssembly.findElementClosestVertex(ref, newPos)

        ref = UtilsAssembly.addVertexToReference(ref, vertex_name)

        plc = UtilsAssembly.findPlacement(ref)
        self.viewObj.ViewObject.Proxy.showMovePositionMarker(True, plc, ref)

    def clickMouse(self, info):
        if not self.selectingFeature or info["State"] != "DOWN":
            return
        if info["Button"] == "BUTTON2" or (info["Button"] == "BUTTON1" and not self.presel_ref):
            self.endSelectionMode()

    # 3D view keyboard handler
    def KeyboardEvent(self, info):
        if info["State"] == "UP" and info["Key"] == "ESCAPE":
            if self.currentStep is None:
                self.reject()
            else:
                self.dismissCurrentStep()

    def _deleteWatched(self, watched):
        if self.form is None or watched is None:
            return False
        tree = self.form.stepList
        while watched is not None and watched is not tree:
            watched = watched.parent()
        return watched is tree

    def _deleteSelected(self):
        tree = self.form.stepList
        items = list(tree.selectedItems())
        if not items and tree.currentItem() is not None:
            items = [tree.currentItem()]
        names = [item.data(0, QtCore.Qt.UserRole) for item in items]
        names = [name for name in names if name]
        if not names:
            return False
        selected_name = getattr(getattr(self, "selectedTransform", None), "Name", None)
        group_name = getattr(getattr(self, "movePositionMove", None), "Name", None)
        if self._transaction:
            self._commitEdit()
        self._beginEdit("Delete exploded move")
        for name in names:
            obj = self.viewObj.Document.getObject(name)
            if obj is not None:
                self._deleteExplodedObject(obj)
        self._commitEdit()
        if selected_name in names:
            self.selectedTransform = None
        kept = self.viewObj.Document.getObject(group_name) if group_name else None
        self.movePositionMove = kept
        self.onMovesChanged()
        if kept is not None and getattr(self, "selectedObjs", None):
            self._placeGroupDragger()
        else:
            self.selectedTransform = None
            self.movePositionMove = None
            self.viewObj.ViewObject.Proxy.showMovePositionMarker(False)
        return True

    def _deleteExplodedObject(self, obj):
        doc = obj.Document
        parent = self._ownerOf(obj)
        if parent == obj:
            self.viewObj.Group = [member for member in self.viewObj.Group if member != obj]
            parent = None
        doc.removeObject(obj.Name)
        if parent is not None and not childMoves(parent):
            self._deleteExplodedObject(parent)

    def _formOwns(self, watched):
        form = getattr(self, "form", None)
        widget = watched
        while form is not None and widget is not None:
            if widget is form:
                return True
            widget = widget.parent() if hasattr(widget, "parent") else None
        return False

    def eventFilter(self, watched, event):
        if (
            getattr(self, "selectingFeature", None)
            and event.type() == QtCore.QEvent.MouseButtonPress
            and self._formOwns(watched)
        ):
            self.endSelectionMode()
        tree = getattr(getattr(self, "form", None), "stepList", None)
        if (
            tree is not None
            and hasattr(tree, "viewport")
            and watched is tree.viewport()
            and event.type() == QtCore.QEvent.MouseButtonPress
            and tree.itemAt(event.pos()) is None
        ):
            self._clearTaskSelection()
            if getattr(self, "viewObj", None) is not None:
                self.viewObj.ViewObject.Proxy.showMovePositionMarker(False)
            self.setDragger()
            return False
        if self._deleteWatched(watched) and event.type() in (QtCore.QEvent.KeyPress, QtCore.QEvent.ShortcutOverride):
            if event.key() == QtCore.Qt.Key_Delete:
                if event.type() == QtCore.QEvent.ShortcutOverride:
                    event.accept()
                else:
                    self._deleteSelected()
                return True
        return super().eventFilter(watched, event)

    # selectionObserver stuff
    def addSelection(self, doc_name, obj_name, sub_name, mousePos):
        if self.blockSetDragger:
            return

        rootObj = App.getDocument(doc_name).getObject(obj_name)
        if rootObj == self.viewObj:
            if self.selectingFeature:
                self.endSelectionMode()
            self.movePositionMove = None
            self.viewObj.ViewObject.Proxy.showMovePositionMarker(False)
            return
        owner = self._ownerOf(rootObj)
        if owner is not None:
            if self.selectingFeature:
                self.endSelectionMode()
            self.selectedTransform = rootObj if rootObj is not owner and hasattr(rootObj, "MovementTransform") else None
            self.movePositionMove = owner
            self._refreshMoves(rootObj)
            self.enableDragger(bool(self.selectedObjs))
            return

        if self.selectingFeature:
            comp, new_sub = UtilsAssembly.getComponentReference(self.assembly, rootObj, sub_name)
            ref = None
            if comp:
                ref = [comp, [new_sub]]
                element_name = UtilsAssembly.getElementName(new_sub)
                if element_name != "":
                    point = App.Vector(mousePos[0], mousePos[1], mousePos[2])
                    vertex_name = UtilsAssembly.findElementClosestVertex(ref, point)
                    ref = UtilsAssembly.addVertexToReference(ref, vertex_name)
            if ref is not None and self._referenceMoves(ref):
                self.endSelectionMode()
                self.alignDragger("Reference", ref)
            self.blockSetDragger = True
            Gui.Selection.removeSelection(doc_name, obj_name, sub_name)
            self.blockSetDragger = False
            return

        else:
            self._clearTaskSelection()
            moving_part, new_sub = UtilsAssembly.getComponentReference(
                self.assembly, rootObj, sub_name
            )
            ref = [moving_part, [new_sub]]
            obj = UtilsAssembly.getObject(ref)

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
        if self.blockSetDragger:
            return

        if self.selectingFeature:
            self.endSelectionMode()
            return

        if App.getDocument(doc_name).getObject(obj_name) in self.viewObj.Group:
            self.movePositionMove = None
            self.viewObj.ViewObject.Proxy.showMovePositionMarker(False)
            return

        element_name = UtilsAssembly.getElementName(sub_name)
        if element_name == "":
            self.setDragger()
            pass

    def setPreselection(self, doc_name, obj_name, sub_name):
        if not self.selectingFeature or not sub_name:
            self.presel_ref = None
            return

        root = App.getDocument(doc_name).getObject(obj_name)
        comp, new_sub = UtilsAssembly.getComponentReference(self.assembly, root, sub_name)
        ref = [comp, [new_sub]] if comp else None
        self.presel_ref = ref if ref is not None and self._referenceMoves(ref) else None

    def clearSelection(self, doc_name):
        if self.blockSetDragger:
            return
        self._clearTaskSelection()
        self.viewObj.ViewObject.Proxy.showMovePositionMarker(False)
        self.setDragger()


if App.GuiUp:
    Gui.addCommand("Assembly_CreateView", CommandCreateView())
