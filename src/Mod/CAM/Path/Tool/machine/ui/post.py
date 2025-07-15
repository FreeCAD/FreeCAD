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
from PySide.QtGui import QComboBox, QTextEdit
import FreeCAD
import Path
from ..models.machine import Machine


translate = FreeCAD.Qt.translate


class PostProcessorSettingsDialog(QtGui.QDialog):
    def __init__(self, machine: Machine, parent=None):
        super().__init__(parent)
        self.machine = machine
        self.setWindowTitle(translate("CAM", "Post Processor Settings"))
        self.layout = QtGui.QVBoxLayout(self)

        general_group = QtGui.QGroupBox(translate("CAM", "Post Processor"))
        general_layout = QtGui.QFormLayout()

        self.post_processor_combo = QComboBox()
        available_post_processors = Path.Preferences.allAvailablePostProcessors()
        self.post_processor_combo.addItems(available_post_processors)
        if machine.post_processor in available_post_processors:
            self.post_processor_combo.setCurrentIndex(
                available_post_processors.index(machine.post_processor)
            )
        general_layout.addRow(translate("CAM", "Post processor"), self.post_processor_combo)

        self.post_processor_args = QtGui.QLineEdit(machine.post_processor_args)
        general_layout.addRow(
            translate("CAM", "Post processor arguments"), self.post_processor_args
        )

        self.supported_args = QTextEdit()
        self.supported_args.setReadOnly(True)
        font = QtGui.QFont("Courier New")
        self.supported_args.setFont(font)
        self.supported_args.setMinimumHeight(350)
        general_layout.addRow(translate("CAM", "Supported arguments"), self.supported_args)

        self.post_processor_combo.currentIndexChanged.connect(self._update_post_processor_args)
        self._update_post_processor_args(self.post_processor_combo.currentIndex())

        general_group.setLayout(general_layout)
        self.layout.addWidget(general_group)

        buttons = QtGui.QDialogButtonBox(
            QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel, QtCore.Qt.Horizontal
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        self.layout.addWidget(buttons)

        self.resize(600, self.sizeHint().height())

    def _update_post_processor_args(self, index):
        post_processor_name = self.post_processor_combo.itemText(index)
        self.setWindowTitle(translate("CAM", f"{post_processor_name} - Post Processor Settings"))
        post = Path.Post.Processor.PostProcessorFactory.get_post_processor(
            None, post_processor_name
        )
        if not post:
            return
        args = post.tooltipArgs or translate("CAM", "No arguments found")
        self.supported_args.setText(args)

    def accept(self):
        self.machine.post_processor = self.post_processor_combo.currentText()
        self.machine.post_processor_args = self.post_processor_args.text()
        super().accept()
