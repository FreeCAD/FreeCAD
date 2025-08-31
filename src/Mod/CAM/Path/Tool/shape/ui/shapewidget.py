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
from typing import Optional
from PySide import QtGui, QtCore
from ..models.base import ToolBitShape


def _png2qpixmap(data, icon_size):
    pixmap = QtGui.QPixmap()
    pixmap.loadFromData(data, "PNG")
    # Scale the pixmap if the requested size is different
    if pixmap.size() != icon_size:
        pixmap = pixmap.scaled(
            icon_size,
            QtCore.Qt.KeepAspectRatio,
            QtCore.Qt.SmoothTransformation,
        )
    return pixmap


class ShapeWidget(QtGui.QWidget):
    def __init__(self, shape: ToolBitShape, icon_size: Optional[QtCore.QSize] = None, parent=None):
        super(ShapeWidget, self).__init__(parent)
        self.layout = QtGui.QVBoxLayout(self)
        self.layout.setAlignment(QtCore.Qt.AlignHCenter)

        self.shape = shape
        self.icon_size = icon_size or QtCore.QSize(200, 235)
        self.icon_widget = QtGui.QLabel()
        self.layout.addWidget(self.icon_widget)

        self._update_icon()

    def _update_icon(self):
        ratio = self.devicePixelRatioF()
        size = self.icon_size * ratio
        icon = self.shape.get_icon()
        if icon:
            pixmap = icon.get_qpixmap(size)
            self.icon_widget.setPixmap(pixmap)
            return

        thumbnail = self.shape.get_thumbnail()
        if thumbnail:
            pixmap = _png2qpixmap(thumbnail, size)
            self.icon_widget.setPixmap(pixmap)
            return

        self.icon_widget.clear()  # Clear pixmap if no icon
