# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2026 CCNUdhj                                            *
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

"""GUI tests for the BIM Reorder Children command."""

from PySide import QtCore

from bimcommands import BimReorder
from bimtests.TestArchBaseGui import TestArchBaseGui


class TestBimReorderGui(TestArchBaseGui):
    """Test the Reorder Children task panel."""

    def test_sort_connection_has_no_qt_warning(self):
        """Opening the task panel should not register an invalid Qt method."""
        # Regression test for:
        # https://github.com/FreeCAD/FreeCAD/issues/29817
        group = self.document.addObject("App::DocumentObjectGroup", "Group")
        child_z = self.document.addObject("App::FeaturePython", "ChildZ")
        child_z.Label = "Z"
        child_a = self.document.addObject("App::FeaturePython", "ChildA")
        child_a.Label = "A"
        group.Group = [child_z, child_a]

        messages = []
        old_handler = QtCore.qInstallMessageHandler(
            lambda _msg_type, _context, message: messages.append(message)
        )
        try:
            panel = BimReorder.BIM_Reorder_TaskPanel(group)
        finally:
            QtCore.qInstallMessageHandler(old_handler)

        self.assertFalse(any("addMetaMethod" in message for message in messages), messages)

        panel.form.pushButton.click()
        labels = [
            panel.form.listWidget.item(index).text()
            for index in range(panel.form.listWidget.count())
        ]
        self.assertEqual(labels, ["A", "Z"])
