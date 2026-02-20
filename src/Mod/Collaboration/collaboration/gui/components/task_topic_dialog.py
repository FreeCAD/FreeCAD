# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

from PySide.QtWidgets import QDialogButtonBox

import FreeCADGui as Gui

import Collaboration_rc  # noqa: F401

import collaboration.app.util as util


logger = util.get_logger(__name__)


class TaskTopicDialog:
    def __init__(self, topic) -> None:
        self.topic = topic
        self.form = Gui.PySideUic.loadUi(":/ui/task_topic_dialog.ui")
        self.initialize_form()

    def initialize_form(self):
        self.form.setWindowTitle("Topic")
        self.form.setWindowIcon(Gui.getIcon("Tree_Annotation"))

        self.form.labelLineEdit.setText(self.topic.Label)
        self.form.labelLineEdit.textChanged.connect(self.onLabelChanged)

        self.form.titleLineEdit.setText(self.topic.LabelText[0])
        self.form.titleLineEdit.textChanged.connect(self.onTitleChanged)

    def open(self):
        logger.debug("Opening Task Topic Dialog")

    def getStandardButtons(self):
        return QDialogButtonBox.Close

    def onLabelChanged(self, labelText):
        self.topic.Label = labelText

    def onTitleChanged(self, title):
        labelText = self.topic.LabelText
        labelText[0] = title
        self.topic.LabelText = labelText
