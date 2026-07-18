# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2026 CCNUdhj                                            *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""Unit tests for the Draft Clone GUI command."""

from unittest import mock

import FreeCADGui as Gui
from draftguitools import gui_clone
from drafttests import test_base


class DraftGuiClone(test_base.DraftTestCaseDoc):
    """Test the Clone command's GUI-specific behavior."""

    def setUp(self):
        """Create a document and initialize the Draft GUI."""
        super().setUp()
        Gui.activateWorkbench("DraftWorkbench")
        Gui.activateView("Gui::View3DInventor", True)

    def tearDown(self):
        """Reset GUI state and close the temporary document."""
        Gui.Selection.clearSelection()
        Gui.activeView().setActiveObject("part", None)
        super().tearDown()

    def test_clone_is_added_to_active_part(self):
        """A clone created inside an active Part should stay in that Part."""
        # Regression test for:
        # https://github.com/FreeCAD/FreeCAD/issues/30013
        part = self.doc.addObject("App::Part", "Part")
        body = self.doc.addObject("PartDesign::Body", "Body")
        part.addObject(body)
        Gui.activeView().setActiveObject("part", part)
        Gui.Selection.addSelection(body)

        command = gui_clone.Clone()
        with mock.patch.object(command, "finish"):
            command.proceed()
        Gui.updateGui()

        clone = self.doc.getObject("Clone")
        self.assertIsNotNone(clone)
        self.assertIn(clone, part.Group)
