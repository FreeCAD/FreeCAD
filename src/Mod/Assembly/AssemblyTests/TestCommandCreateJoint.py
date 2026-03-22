# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
#                                                                           *
#    Copyright (c) 2026 Weston Schmidt <weston_schmidt@alumni.purdue.edu>   *
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

import unittest
from unittest.mock import MagicMock, patch

import FreeCAD as App

if App.GuiUp:
    import CommandCreateJoint


def _msg(text, end="\n"):
    """Write messages to the console including the line ending."""
    App.Console.PrintMessage(text + end)


@unittest.skipIf(not App.GuiUp, "GUI tests require FreeCAD GUI mode")
class TestCommandCreateJoint(unittest.TestCase):
    """Unit tests for CommandCreateJoint module."""

    def setUp(self):
        """Create a temporary assembly document for each test."""
        doc_name = self.__class__.__name__
        if App.ActiveDocument:
            if App.ActiveDocument.Name != doc_name:
                App.newDocument(doc_name)
        else:
            App.newDocument(doc_name)
        App.setActiveDocument(doc_name)
        self.doc = App.ActiveDocument

        self.assembly = self.doc.addObject("Assembly::AssemblyObject", "Assembly")
        self.part = self.assembly.newObject("Part::Box", "Box")

        _msg(f"  Temporary document '{self.doc.Name}'")

    def tearDown(self):
        """Clean up the temporary document."""
        App.closeDocument(self.doc.Name)

    def test_toggle_grounded_uses_gui_document_transaction(self):
        """Toggle grounded should open a GUI document command before editing."""
        operation = "Toggle grounded uses GUI document transaction"
        _msg(f"  Test '{operation}'")

        gui_doc = type("GuiDocument", (), {})()
        gui_doc.openCommand = MagicMock()
        gui_doc.commitCommand = MagicMock()

        selection = MagicMock()
        selection.SubElementNames = [f"{self.part.Name}."]
        selection.Object.resolveSubElement.return_value = None

        joint_group = type("JointGroup", (), {"Group": []})()

        with (
            patch.object(CommandCreateJoint.Gui, "ActiveDocument", gui_doc),
            patch.object(
                CommandCreateJoint.Gui.Selection, "getSelectionEx", return_value=[selection]
            ),
            patch.object(
                CommandCreateJoint.UtilsAssembly, "activeAssembly", return_value=self.assembly
            ),
            patch.object(
                CommandCreateJoint.UtilsAssembly, "getJointGroup", return_value=joint_group
            ),
            patch.object(
                CommandCreateJoint.UtilsAssembly,
                "getComponentReference",
                return_value=(self.part, selection.SubElementNames[0]),
            ),
            patch.object(CommandCreateJoint, "createGroundedJoint") as create_grounded_joint,
        ):
            CommandCreateJoint.CommandToggleGrounded().Activated()

        gui_doc.openCommand.assert_called_once_with("Toggle grounded")
        create_grounded_joint.assert_called_once_with(self.part)
        gui_doc.commitCommand.assert_called_once_with()
