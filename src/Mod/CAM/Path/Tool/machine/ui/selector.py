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
import FreeCAD
from ..models.machine import Machine
from ..models.lathe import Lathe
from ..models.mill import Mill


translate = FreeCAD.Qt.translate


class MachineButton(QtGui.QToolButton):
    def __init__(self, label: str, icon: QtGui.QIcon, parent=None):
        super(MachineButton, self).__init__(parent)
        self.icon = icon

        self.vbox = QtGui.QVBoxLayout(self)
        self.vbox.setAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter)
        self.vbox.setContentsMargins(0, 0, 0, 0)
        self.vbox.setSpacing(0)

        self.icon_widget = QtGui.QLabel()
        self.icon_widget.setAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter)

        self.label_widget = QtGui.QLabel(label)
        self.label_widget.setAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter)

        self.vbox.addWidget(self.icon_widget)
        self.vbox.addWidget(self.label_widget)
        self.setLayout(self.vbox)

        self.setFixedSize(128, 128)
        self.setBaseSize(128, 128)
        self.icon_size = QtCore.QSize(71, 70)

        self._update_icon()

    def _update_icon(self):
        if self.icon:
            pixmap = self.icon.pixmap(self.icon_size)
            self.icon_widget.setPixmap(pixmap)
        else:
            self.icon_widget.clear()


class MachineSelector(QtGui.QDialog):
    def __init__(self, parent=None):
        super(MachineSelector, self).__init__(parent)
        self.machine: Optional[Machine] = None
        self.setWindowTitle("Select Machine Type")

        main_layout = QtGui.QVBoxLayout(self)
        main_layout.setContentsMargins(20, 20, 20, 20)

        buttons_layout = QtGui.QHBoxLayout()
        buttons_layout.setContentsMargins(10, 10, 10, 10)

        self.mill_button = MachineButton("3 Axis Mill", QtGui.QIcon(":/icons/mill.svg"))
        self.lathe_button = MachineButton("Lathe", QtGui.QIcon(":/icons/lathe.svg"))

        buttons_layout.addWidget(self.mill_button)
        buttons_layout.addWidget(self.lathe_button)

        main_layout.addLayout(buttons_layout)

        self.setMinimumWidth(300)

        self.mill_button.clicked.connect(self._on_mill_clicked)
        self.lathe_button.clicked.connect(self._on_lathe_clicked)

    def _on_mill_clicked(self):
        self.machine = Mill(translate("CAM", "My 3-Axis CNC"))
        self.accept()

    def _on_lathe_clicked(self):
        self.machine = Lathe(translate("CAM", "My Lathe"))
        self.accept()

    def show(self) -> Optional[Machine]:
        if self.exec_() == QtGui.QDialog.Accepted:
            return self.machine
        return None
