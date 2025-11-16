# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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

import json
import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui

import UtilsAssembly

translate = App.Qt.translate

__title__ = "Assembly Command Create Snapshot"
__author__ = "AsdtoCAD"
__url__ = "https://www.freecad.org"


class CommandCreateSnapshot:
    """
    Command to create a Snapshot object that captures the current state
    of an assembly.
    """

    def GetResources(self):
        return {
            "Pixmap": "Std_ViewScreenShot",
            "MenuText": QT_TRANSLATE_NOOP("Assembly_Snapshot", "Snapshot"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "Assembly_Snapshot",
                "Captures the current assembly state (placements and visibility). Double clicking the Snapshot object restores assembly to that state.",
            ),
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        return (
            UtilsAssembly.activeAssembly() is not None
            and UtilsAssembly.assembly_has_at_least_n_parts(1)
        )

    def Activated(self):
        assembly = UtilsAssembly.activeAssembly()
        if not assembly:
            return

        noTransaction = App.getActiveTransaction() is None
        if noTransaction:
            App.setActiveTransaction("Create Snapshot")

        snapshot_group = UtilsAssembly.getSnapshotGroup(assembly)

        snapshot_obj = snapshot_group.newObject("App::FeaturePython", "Snapshot")
        Snapshot(snapshot_obj)
        ViewProviderSnapshot(snapshot_obj.ViewObject)
        snapshot_group.purgeTouched()

        if noTransaction:
            App.closeActiveTransaction()

class Snapshot:
    """
    The Python proxy for the Snapshot App::FeaturePython object.
    It handles the capturing and restoring of the assembly's state.
    """

    def __init__(self, snapshot_obj):
        snapshot_obj.Proxy = self

        if not hasattr(snapshot_obj, "CapturedState"):
            snapshot_obj.addProperty(
                "App::PropertyString",
                "CapturedState",
                "Snapshot",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Stores the JSON data of the captured assembly state.",
                ),
            )

        # Capture the state immediately upon creation
        self.captureState(snapshot_obj)

    def execute(self, snapshot_obj):
        """Called on recompute. A snapshot is static, so we do nothing."""
        pass

    def getAssembly(self, snapshot_obj):
        """Finds the parent assembly of this snapshot."""
        for obj in snapshot_obj.InList:
            # The group is in the assembly, so we go up two levels.
            for parent in obj.InList:
                if parent.isDerivedFrom("Assembly::AssemblyObject"):
                    return parent
        return None

    def captureState(self, snapshot_obj):
        """
        Captures placements, visibility, and materials of all movable parts
        in the assembly and stores them as a JSON string.
        """
        assembly = self.getAssembly(snapshot_obj)
        if not assembly:
            App.Console.PrintWarning("Could not find parent assembly for snapshot.\n")
            return

        components = UtilsAssembly.getMovablePartsWithin(assembly, partsAsSolid=True)
        state_data = {}

        for part in components:
            if not part or not hasattr(part, "Placement"):
                continue

            part_state = {}

            # Capture Placement
            plc = part.Placement
            part_state["placement"] = {
                "pos": [plc.Base.x, plc.Base.y, plc.Base.z],
                "rot": [plc.Rotation.Q[0], plc.Rotation.Q[1], plc.Rotation.Q[2], plc.Rotation.Q[3]],
            }

            # Capture Visibility
            try:
                part_state["visibility"] = part.Visibility
            except AttributeError:
                # Part might not have a ShapeMaterial
                part_state["visibility"] = None

            state_data[part.Name] = part_state

        snapshot_obj.CapturedState = json.dumps(state_data, indent=2)

        snapshot_obj.purgeTouched()

    def restoreState(self, snapshot_obj):
        """
        Applies the captured state to the assembly.
        """
        assembly = self.getAssembly(snapshot_obj)
        doc = snapshot_obj.Document

        if not assembly:
            App.Console.PrintError("Snapshot cannot find its parent assembly to restore state.\n")
            return

        if not snapshot_obj.CapturedState:
            App.Console.PrintWarning("Snapshot has no captured state to restore.\n")
            return

        try:
            state_data = json.loads(snapshot_obj.CapturedState)
        except json.JSONDecodeError:
            App.Console.PrintError("Snapshot data is corrupt and cannot be restored.\n")
            return

        for part_fullname, part_state in state_data.items():
            part = doc.getObject(part_fullname)

            # If part was deleted, just skip it.
            if not part:
                continue

            # Restore Placement
            if "placement" in part_state:
                pos = part_state["placement"]["pos"]
                rot = part_state["placement"]["rot"]
                part.Placement = App.Placement(App.Vector(*pos), App.Rotation(*rot))

            # Restore Visibility
            try:
                part.Visibility = part_state["visibility"]
            except (AttributeError, KeyError):
                pass  # Ignore if material properties are missing or object changed


class ViewProviderSnapshot:
    """
    The Python proxy for the Snapshot ViewProvider object.
    It handles the double-click event to trigger the state restoration.
    """

    def __init__(self, vp_obj):
        vp_obj.Proxy = self
        self.Object = vp_obj.Object

    def getIcon(self):
        return ":/icons/Std_ViewScreenShot.svg"

    def doubleClicked(self, vp_obj):
        """This is the core function to restore the assembly state."""
        snapshot_obj = vp_obj.Object
        if not snapshot_obj or not hasattr(snapshot_obj, "Proxy"):
            return False
            
        if Gui.Control.activeDialog():
            App.Console.PrintError("Cannot restore snapshot while a task is active.\n")
            return

        App.setActiveTransaction("Restore Snapshot '" + snapshot_obj.Label + "'")
        try:
            snapshot_obj.Proxy.restoreState(snapshot_obj)
        except Exception as e:
            App.Console.PrintError(f"Failed to restore snapshot: {e}\n")
            if noTransaction:
                App.closeActiveTransaction(True)
            return False

        App.closeActiveTransaction()

        return True

    def attach(self, vp_obj):
        """Called when the view provider is attached to the object."""
        pass

    def dumps(self):
        return None

    def loads(self, state):
        return None


if App.GuiUp:
    Gui.addCommand("Assembly_CreateSnapshot", CommandCreateSnapshot())