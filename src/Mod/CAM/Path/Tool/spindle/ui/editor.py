# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
from PySide import QtGui, QtCore
import FreeCAD
from ..models import Spindle
from .properties import SpindlePropertiesWidget


class SpindleEditorDialog(QtGui.QDialog):
    """Dialog for adding or editing a spindle's properties."""

    def __init__(self, spindle: Spindle, parent=None):
        super().__init__(parent)
        self.setWindowTitle(FreeCAD.Qt.translate("CAM", "Spindle Editor"))
        self.spindle = spindle
        self.layout = QtGui.QVBoxLayout(self)

        # Spindle properties widget
        self.props_widget = SpindlePropertiesWidget(self.spindle, self)
        self.layout.addWidget(self.props_widget)

        # Buttons
        buttons = QtGui.QDialogButtonBox(
            QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel, QtCore.Qt.Horizontal, self
        )
        buttons.accepted.connect(self.on_accepted)
        buttons.rejected.connect(self.reject)
        self.layout.addWidget(buttons)

    def on_accepted(self):
        self.props_widget.update_spindle()
        self.accept()
