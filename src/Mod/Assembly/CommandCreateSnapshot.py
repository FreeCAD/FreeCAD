# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD <hello@astocad.com>                         *
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

import FreeCAD as App

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui

import UtilsAssembly

translate = App.Qt.translate

__title__ = "Assembly Command Create Snapshot"
__author__ = "AstoCAD"
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
                "Captures the current assembly state (placements and visibility). Double-clicking the Snapshot object restores the assembly to that state.",
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
            Gui.ActiveDocument.openCommand(translate("Assembly_Snapshot", "Create Snapshot"))

        snapshot_group = UtilsAssembly.getSnapshotGroup(assembly)

        snapshot_obj = snapshot_group.newObject("App::FeaturePython", "Snapshot")
        Snapshot(snapshot_obj)
        ViewProviderSnapshot(snapshot_obj.ViewObject)
        snapshot_group.purgeTouched()

        if noTransaction:
            Gui.ActiveDocument.commitCommand()


class Snapshot:
    """
    The Python proxy for the Snapshot App::FeaturePython object.
    It handles the capturing and restoring of the assembly's state.
    """

    def __init__(self, snapshot_obj):
        snapshot_obj.Proxy = self
        self.ensureProperties(snapshot_obj)
        # Capture the state immediately upon creation
        self.captureState(snapshot_obj)

    def execute(self, snapshot_obj):
        """Called on recompute. A snapshot is static, so we do nothing."""
        pass

    def onDocumentRestored(self, snapshot_obj):
        """Called when a document containing a snapshot is loaded."""
        self.ensureProperties(snapshot_obj)

    def ensureProperties(self, snapshot_obj):
        """Ensures that the required properties are added to the object."""
        if not hasattr(snapshot_obj, "Components"):
            snapshot_obj.addProperty(
                "App::PropertyLinkList",
                "Components",
                "Snapshot",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "List of components captured in this snapshot.",
                ),
            )

        if not hasattr(snapshot_obj, "Placements"):
            snapshot_obj.addProperty(
                "App::PropertyPlacementList",
                "Placements",
                "Snapshot",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "List of corresponding placements for the components.",
                ),
            )

        if not hasattr(snapshot_obj, "Visibilities"):
            snapshot_obj.addProperty(
                "App::PropertyBoolList",
                "Visibilities",
                "Snapshot",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "List of visibility states for the components.",
                ),
            )

        if not hasattr(snapshot_obj, "SolveOnActivation"):
            snapshot_obj.addProperty(
                "App::PropertyBool",
                "SolveOnActivation",
                "Snapshot",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If true, applying the snapshot will solve the assembly after restoring the placements.",
                ),
            )
            snapshot_obj.SolveOnActivation = True

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
        Captures placements and visibility of all movable parts
        in the assembly and stores them inside the lists.
        """
        assembly = self.getAssembly(snapshot_obj)
        if not assembly:
            App.Console.PrintWarning("Could not find parent assembly for snapshot.\n")
            return

        components = UtilsAssembly.getMovablePartsWithin(assembly, partsAsSolid=True)

        captured_objs = []
        captured_placements = []
        captured_visibilities = []

        for part in components:
            if not part or not hasattr(part, "Placement"):
                continue

            captured_objs.append(part)
            captured_placements.append(part.Placement)

            try:
                captured_visibilities.append(bool(part.Visibility))
            except AttributeError:
                captured_visibilities.append(True)  # Fallback value if Visibility is unavailable

        snapshot_obj.Components = captured_objs
        snapshot_obj.Placements = captured_placements
        snapshot_obj.Visibilities = captured_visibilities

        snapshot_obj.purgeTouched()

    def restoreState(self, snapshot_obj):
        """
        Applies the captured state to the assembly.
        """
        assembly = self.getAssembly(snapshot_obj)
        if not assembly:
            App.Console.PrintError("Snapshot cannot find its parent assembly to restore state.\n")
            return

        components = getattr(snapshot_obj, "Components", [])
        placements = getattr(snapshot_obj, "Placements", [])
        visibilities = getattr(snapshot_obj, "Visibilities", [])

        if not components:
            App.Console.PrintWarning("Snapshot has no captured components to restore.\n")
            return

        # Determine limits in case property list lengths do not match
        n = min(len(components), len(placements), len(visibilities))

        for i in range(n):
            part = components[i]
            # Skip if referenced part was deleted
            if not part:
                continue

            # Restore Placement
            part.Placement = placements[i]

            # Restore Visibility
            try:
                part.Visibility = visibilities[i]
            except AttributeError:
                pass

            part.purgeTouched()

        if snapshot_obj.SolveOnActivation:
            assembly.recompute()


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
            return False

        Gui.ActiveDocument.openCommand(
            translate("Assembly_Snapshot", "Restore Snapshot") + " '" + snapshot_obj.Label + "'"
        )
        try:
            snapshot_obj.Proxy.restoreState(snapshot_obj)
        except Exception as e:
            App.Console.PrintError(f"Failed to restore snapshot: {e}\n")
            Gui.ActiveDocument.abortCommand()
            return False

        Gui.ActiveDocument.commitCommand()

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
