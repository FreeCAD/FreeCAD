# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Samuel Abels
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

from PySide import QtGui, QtCore
from .shapewidget import ShapeWidget


class ShapeButton(QtGui.QToolButton):
    def __init__(self, shape, parent=None):
        super(ShapeButton, self).__init__(parent)
        self.shape = shape

        self.vbox = QtGui.QVBoxLayout(self)
        self.vbox.setAlignment(
            QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter
        )  # Align all items centrally
        self.vbox.setContentsMargins(0, 0, 0, 0)
        self.vbox.setSpacing(0)

        self.icon_widget = ShapeWidget(self.shape, QtCore.QSize(71, 70), dimensions=False)

        self.label_widget = QtGui.QLabel(shape.label)
        self.label_widget.setAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter)

        self.id_widget = QtGui.QLabel(shape.id)
        self.id_widget.setAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter)
        self.id_widget.setContentsMargins(0, 0, 0, 0)
        font = self.id_widget.font()
        font.setPointSize(font.pointSize() * 0.9)
        self.id_widget.setFont(font)

        # Add widgets to the new layout
        self.vbox.addWidget(self.icon_widget)
        self.vbox.addWidget(self.label_widget)
        self.vbox.addWidget(self.id_widget)

        self.setLayout(self.vbox)

        self.setFixedSize(128, 128)
        self.setBaseSize(128, 128)

    def set_text(self, text):
        # Update the text of the label widget
        self.label_widget.setText(text)
