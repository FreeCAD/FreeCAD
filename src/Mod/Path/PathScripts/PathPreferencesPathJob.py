# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD
import FreeCADGui
from PySide import QtCore, QtGui
from PathScripts.PathPostProcessor import PostProcessor


class Page:
    def __init__(self, parent=None):
        self.form = FreeCADGui.PySideUic.loadUi(":preferences/PathJob.ui")

    def saveSettings(self):
        print("saveSettings")
        processor = str(self.form.defaultPostProcessor.currentText())
        args = str(self.form.defaultPostProcessorArgs.text())
        blacklist = []
        for i in range(0, self.form.postProcessorList.count()):
            item = self.form.postProcessorList.item(i)
            if item.checkState() == QtCore.Qt.CheckState.Unchecked:
                blacklist.append(item.text())
        PostProcessor.saveDefaults(processor, args, blacklist)

    def loadSettings(self):
        print("loadSettings")
        self.form.defaultPostProcessor.addItem("")
        blacklist = PostProcessor.blacklist()
        for processor in PostProcessor.all():
            self.form.defaultPostProcessor.addItem(processor)
            item = QtGui.QListWidgetItem(processor)
            if processor in blacklist:
                item.setCheckState(QtCore.Qt.CheckState.Unchecked)
            else:
                item.setCheckState(QtCore.Qt.CheckState.Checked)
            item.setFlags( QtCore.Qt.ItemFlag.ItemIsSelectable | QtCore.Qt.ItemFlag.ItemIsEnabled | QtCore.Qt.ItemFlag.ItemIsUserCheckable)
            self.form.postProcessorList.addItem(item)

        postindex = self.form.defaultPostProcessor.findText(PostProcessor.default(), QtCore.Qt.MatchFixedString)

        if postindex >= 0:
            self.form.defaultPostProcessor.blockSignals(True)
            self.form.defaultPostProcessor.setCurrentIndex(postindex)
            self.form.defaultPostProcessor.blockSignals(False)

        self.form.defaultPostProcessorArgs.setText(PostProcessor.defaultArgs())

