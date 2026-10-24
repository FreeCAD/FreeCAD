# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD Project Association
# SPDX-FileNotice: Part of the FreeCAD project.
################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Unit tests for Draft snapping behavior."""

## @package test_snapper
# \ingroup drafttests
# \brief Unit tests for Draft snapping behavior.

## \addtogroup drafttests
# @{

from unittest.mock import patch

import FreeCADGui as Gui
from drafttests import test_base


class DraftSnapper(test_base.DraftTestCaseNoDoc):
    """Tests for Draft Snapper lifecycle behavior."""

    def test_cancel_point_request_resets_edit_mode(self):
        """Canceling a point request should leave Draft edit mode."""
        from draftguitools.gui_snapper import Snapper

        class FakeToolbar:
            def __init__(self):
                self.off_ui_calls = 0

            def offUi(self):
                self.off_ui_calls += 1

        class FakeGuiDocument:
            def __init__(self):
                self.in_edit = object()
                self.reset_edit_calls = 0

            def getInEdit(self):
                return self.in_edit

            def resetEdit(self):
                self.reset_edit_calls += 1
                self.in_edit = None

        snapper = Snapper()
        snapper.view = object()
        toolbar = FakeToolbar()
        gui_doc = FakeGuiDocument()

        with (
            patch.object(Gui, "draftToolBar", toolbar, create=True),
            patch.object(Gui, "ActiveDocument", gui_doc, create=True),
        ):
            snapper.cancelPointRequest()

        self.assertEqual(toolbar.off_ui_calls, 1)
        self.assertEqual(gui_doc.reset_edit_calls, 1)
        self.assertIsNone(gui_doc.getInEdit())


## @}
