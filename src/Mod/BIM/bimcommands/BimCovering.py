# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD Arch workbench.
# You can find the full license text in the LICENSE file in the root directory.

"""Command to create and configure Arch Covering objects.

This command creates an Arch Covering object, used to represent surface finishes like flooring, wall
cladding, or ceiling tiles. It opens a task panel allowing the user to select a base face or object
and configure parameters for solid tiles, parametric patterns, or standard hatch patterns.
"""

import FreeCAD
import Arch

if FreeCAD.GuiUp:
    import FreeCADGui

    QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
    translate = FreeCAD.Qt.translate
else:

    def translate(ctxt, txt):
        return txt

    def QT_TRANSLATE_NOOP(ctxt, txt):
        return txt


class BIM_Covering:
    """
    The Command object for the Arch Covering tool.

    This class handles the high-level orchestration for creating surface finishes. It acts as the
    controller that manages the tool's resources, determines its availability, and launches the
    task-specific user interface.
    """

    def GetResources(self):
        return {
            "Pixmap": "BIM_Covering",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Covering", "Covering"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Covering", "Creates a covering (floor finish, cladding) on a selected face"
            ),
            "Accel": "C, V",
        }

    def IsActive(self):
        return hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")

    def Activated(self):
        """
        Opens a "Create Covering" transaction and shows the creation task panel.

        The selection is read and normalized before the task panel opens so it can pre-populate
        which face(s) to cover. The transaction is opened before the task panel is constructed
        so that the buffer object's creation falls inside it from the start.
        """
        if not FreeCAD.ActiveDocument:
            return

        import ArchCoveringGui

        raw_selection = FreeCADGui.Selection.getSelectionEx()
        selection_list = []
        if raw_selection:
            for sel_item in raw_selection:
                obj = Arch.resolve_pd_object(sel_item.Object)

                face_found_in_subelements = False
                if sel_item.SubElementNames:
                    for sub_name in sel_item.SubElementNames:
                        if sub_name.startswith("Face"):
                            selection_list.append((obj, [sub_name]))
                            face_found_in_subelements = True

                if not face_found_in_subelements:
                    selection_list.append(obj)

        FreeCAD.ActiveDocument.openTransaction(QT_TRANSLATE_NOOP("Command", "Create Covering"))
        self.task_panel = ArchCoveringGui.ArchCoveringTaskPanel(
            command=self, selection=selection_list
        )
        FreeCADGui.Control.showDialog(self.task_panel)

        if not selection_list:
            self.task_panel.setPicking(True)

    def finish(self):
        """Terminates the command session and cleans up the interface."""
        FreeCADGui.Control.closeDialog()


# Register the command
FreeCADGui.addCommand("BIM_Covering", BIM_Covering())
