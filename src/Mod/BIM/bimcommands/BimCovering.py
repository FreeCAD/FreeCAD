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

if FreeCAD.GuiUp:
    from PySide import QtCore, QtGui
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
        """
        Provides metadata required by the FreeCAD GUI.

        Returns
        -------
        dict
            A dictionary containing the icon path (Pixmap), localized menu text
            (MenuText), descriptive tooltip (ToolTip), and keyboard shortcut (Accel).
        """
        return {
            "Pixmap": "BIM_Covering",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Covering", "Covering"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Covering", "Creates a covering (floor finish, cladding) on a selected face"
            ),
            "Accel": "C, V",
        }

    def IsActive(self):
        """
        Determines if the current GUI environment supports the tool's requirements.

        This C++ callback validates the contextual eligibility of the command. It ensures the tool
        is only enabled when a 3D viewer is active, preventing activation in non-geometric contexts
        like spreadsheets or 2D pages.

        Returns
        -------
        bool
            True if the 3D scene graph is available; False otherwise.

        Notes
        -----
        The C++ command manager automatically handles state-based locking (disabling the button
        while the tool's Task Panel is already open), so this method only needs to verify
        environmental requirements.
        """
        return hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")

    def Activated(self):
        """
        Initializes the Covering tool session.

        Captures references to the active document and viewer, instantiates the Task Panel logic,
        and registers the panel with the FreeCAD Task Manager to begin the user interaction.
        """
        # Check for pre-selection and fill in the task panel with it, if the user has already
        # selected something before executing the command
        sel = FreeCADGui.Selection.getSelectionEx()
        selection_list = []
        if sel:
            for s in sel:
                if s.SubElementNames:
                    # Iterate through all selected sub-elements (faces) This ensures that selecting
                    # Face1 AND Face2 of the same object results in two distinct targets for the
                    # batch operation.
                    for sub in s.SubElementNames:
                        if "Face" in sub:
                            selection_list.append((s.Object, [sub]))
                else:
                    # Whole object selected
                    selection_list.append(s.Object)

        # Launch the task panel
        import ArchCovering

        self.task_panel = ArchCovering.ArchCoveringTaskPanel(command=self, selection=selection_list)
        FreeCADGui.Control.showDialog(self.task_panel)

        if not selection_list:
            # Auto-enable picking if nothing selected
            self.task_panel.setPicking(True)

    def finish(self):
        """
        Terminates the command session and cleans up the interface.

        Closes the docked Task Panel via the FreeCAD Task Manager, returning the GUI to its standard
        state.
        """
        FreeCADGui.Control.closeDialog()


# Register the command: hand the Python class instance over to the C++ GUI Manager.
FreeCADGui.addCommand("BIM_Covering", BIM_Covering())
