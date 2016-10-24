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
from PathScripts.PathPost import CommandPathPost as PathPost


class Page:
    def __init__(self, parent=None):
        self.form = FreeCADGui.PySideUic.loadUi(":preferences/PathJob.ui")

        self.postProcessorDefaultTooltip = self.form.defaultPostProcessor.toolTip()
        self.postProcessorArgsDefaultTooltip = self.form.defaultPostProcessorArgs.toolTip()
        self.processor = { }

    def saveSettings(self):
        processor = str(self.form.defaultPostProcessor.currentText())
        args = str(self.form.defaultPostProcessorArgs.text())
        blacklist = []
        for i in range(0, self.form.postProcessorList.count()):
            item = self.form.postProcessorList.item(i)
            if item.checkState() == QtCore.Qt.CheckState.Unchecked:
                blacklist.append(item.text())
        PostProcessor.saveDefaults(processor, args, blacklist)

        path = str(self.form.leOutputFile.text())
        policy = str(self.form.cboOutputPolicy.currentText())
        PathPost.saveDefaults(path, policy)

    def selectComboEntry(self, widget, text):
        index = widget.findText(text, QtCore.Qt.MatchFixedString)
        if index >= 0:
            widget.blockSignals(True)
            widget.setCurrentIndex(index)
            widget.blockSignals(False)

    def loadSettings(self):
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
        self.selectComboEntry(self.form.defaultPostProcessor, PostProcessor.default())

        self.form.defaultPostProcessorArgs.setText(PostProcessor.defaultArgs())
        self.form.leOutputFile.setText(PathPost.defaultOutputFile())
        self.selectComboEntry(self.form.cboOutputPolicy, PathPost.defaultOutputPolicy())

        self.form.postProcessorList.itemEntered.connect(self.setProcessorListTooltip)
        self.form.defaultPostProcessor.currentIndexChanged.connect(self.updateDefaultPostProcessorToolTip)
        self.form.browseFileSystem.clicked.connect(self.browseFileSystem)

    def getPostProcessor(self, name):
        if not name in self.processor.keys():
            processor = PostProcessor.load(name)
            self.processor[name] = processor
            return processor
        return self.processor[name]

    def setPostProcessorTooltip(self, widget, name, default):
        processor = self.getPostProcessor(name)
        if processor.tooltip:
            widget.setToolTip(processor.tooltip)
        else:
            widget.setToolTip(default)

    def setProcessorListTooltip(self, item):
        self.setPostProcessorTooltip(self.form.postProcessorList, item.text(), '')

    def updateDefaultPostProcessorToolTip(self):
        name = str(self.form.defaultPostProcessor.currentText())
        if name:
            self.setPostProcessorTooltip(self.form.defaultPostProcessor, name, self.postProcessorDefaultTooltip)
            processor = self.getPostProcessor(name)
            if processor.tooltipArgs:
                self.form.defaultPostProcessorArgs.setToolTip(processor.tooltipArgs)
            else:
                self.form.defaultPostProcessorArgs.setToolTip(self.postProcessorArgsDefaultTooltip)
        else:
            self.form.defaultPostProcessor.setToolTip(self.postProcessorDefaultTooltip)
            self.form.defaultPostProcessorArgs.setToolTip(self.postProcessorArgsDefaultTooltip)

    def browseFileSystem(self):
        foo = QtGui.QFileDialog.getExistingDirectory(QtGui.qApp.activeWindow(), "Path - Output File/Directory", self.form.defaultOutputPath.text())
        if foo:
            self.form.defaultOutputPath.setText(foo)
