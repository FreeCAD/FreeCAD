# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Furgo                                              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
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

import Arch
from bimtests import TestArchBase

class TestArchPanel(TestArchBase.TestArchBase):

    def test_makePanel(self):
        """Test the makePanel function."""
        operation = "Testing makePanel function"
        self.printTestMessage(operation)

        panel = Arch.makePanel(name="TestPanel")
        self.assertIsNotNone(panel, "makePanel failed to create a panel object.")
        self.assertEqual(panel.Label, "TestPanel", "Panel label is incorrect.")

    # TODO: remove NOT_ prefix once it is understood why Arch.makePanelCut fails
    def NOT_test_makePanelCut(self):
        """Test the makePanelCut function."""
        operation = "Testing makePanelCut function"
        self.printTestMessage(operation)

        panel = Arch.makePanel(name="TestPanel")
        panel_cut = Arch.makePanelCut(panel, name="TestPanelCut")
        self.assertIsNotNone(panel_cut, "makePanelCut failed to create a panel cut object.")
        self.assertEqual(panel_cut.Label, "TestPanelCut", "Panel cut label is incorrect.")

    # TODO: remove NOT_ prefix once it is understood why Arch.makePanelSheet fails
    def NOT_test_makePanelSheet(self):
        """Test the makePanelSheet function."""
        operation = "Testing makePanelSheet function"
        self.printTestMessage(operation)

        panel_sheet = Arch.makePanelSheet(name="TestPanelSheet")
        self.assertIsNotNone(panel_sheet, "makePanelSheet failed to create a panel sheet object.")
        self.assertEqual(panel_sheet.Label, "TestPanelSheet", "Panel sheet label is incorrect.")