# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest
from unittest.mock import MagicMock, patch

import FreeCADGui
from PySide import QtWidgets

import Path.Main.Gui.Job as PathJobGui


class TestPathJobGui(unittest.TestCase):
    def test_assign_material_dialog_has_main_window_parent(self):
        panel = PathJobGui.TaskPanel.__new__(PathJobGui.TaskPanel)
        panel._currentStockMaterial = MagicMock(return_value=(None, None))

        dialog = MagicMock()
        dialog.exec_.return_value = QtWidgets.QDialog.Rejected

        with patch.object(PathJobGui, "MaterialDialog", return_value=dialog) as dialog_class:
            panel.assignMaterial()

        dialog_class.assert_called_once_with(parent=FreeCADGui.getMainWindow(), current_uuid=None)
