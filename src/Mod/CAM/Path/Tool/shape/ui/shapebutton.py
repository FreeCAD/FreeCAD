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


class ShapeButton(QtGui.QToolButton):
    def __init__(self, shape, parent=None):
        super(ShapeButton, self).__init__(parent)
        self.shape = shape

        self.setToolButtonStyle(QtCore.Qt.ToolButtonTextUnderIcon)
        self.setText(shape.label)

        self.setFixedSize(128, 128)
        self.setBaseSize(128, 128)
        self.icon_size = QtCore.QSize(71, 100)
        self.setIconSize(self.icon_size)

        self._update_icon()

    def set_text(self, text):
        self.label.setText(text)

    def _update_icon(self):
        icon = self.shape.get_icon()
        if icon:
            pixmap = icon.get_qpixmap(self.icon_size)
            self.setIcon(QtGui.QIcon(pixmap))
