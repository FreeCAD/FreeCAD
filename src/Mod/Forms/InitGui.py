# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
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

"""GUI initialization for the Forms workbench."""

import os

import FreeCAD as App
import FreeCADGui as Gui
from PySide import QtCore


class FormsWorkbench(Gui.Workbench):
    """Tools for creating and editing free-form product geometry."""

    def __init__(self):
        self.__class__.Icon = (
            App.getResourceDir() + "Mod/Forms/Resources/icons/Forms_Workbench.svg"
        )
        self.__class__.MenuText = "Forms"
        self.__class__.ToolTip = "Forms workbench"

    def Initialize(self):
        icon_path = os.path.join(App.getResourceDir(), "Mod", "Forms", "Resources", "icons")
        Gui.addIconPath(icon_path)

        import CommandPrimitives  # noqa: F401
        import CommandEdit  # noqa: F401
        import CommandTopology  # noqa: F401

        translate = App.Qt.translate
        create_commands = [
            "Forms_CreateBox",
            "Forms_CreateCylinder",
            "Forms_CreateQuadball",
            "Forms_CreateSphere",
            "Forms_CreatePipe",
            "Forms_CreateFace",
            "Forms_CreateTorus",
            "Forms_CreateTube",
            "Forms_Edit",
        ]
        modify_commands = [
            "Forms_InsertEdge",
            "Forms_InsertPoint",
            "Forms_Subdivide",
            "Forms_Thicken",
            "Forms_DeleteFaces",
            "Forms_DeleteEdges",
            "Forms_FillHole",
            "Forms_Bridge",
            "Forms_EraseAndFill",
            "Forms_Unweld",
            "Forms_Weld",
            "Forms_Match",
            "Forms_Crease",
            "Forms_Uncrease",
            "Forms_Straighten",
            "Forms_Flatten",
        ]
        modify_toolbar_commands = [
            command
            for command in modify_commands
            if command not in ("Forms_DeleteFaces", "Forms_DeleteEdges")
        ]
        self.appendToolbar(translate("Workbench", "Forms Create"), create_commands)
        self.appendToolbar(
            translate("Workbench", "Forms Modify"), modify_toolbar_commands
        )
        self.appendMenu(
            [translate("Workbench", "&Forms"), translate("Workbench", "&Create")],
            create_commands,
        )
        self.appendMenu(
            [translate("Workbench", "&Forms"), translate("Workbench", "&Modify")],
            modify_commands,
        )

    def Activated(self):
        from Forms.edit import active_form_session
        from Forms.toolbar import set_forms_toolbar_mode

        set_forms_toolbar_mode(active_form_session() is not None)

    def GetClassName(self):
        return "Gui::PythonWorkbench"


Gui.addWorkbench(FormsWorkbench())
