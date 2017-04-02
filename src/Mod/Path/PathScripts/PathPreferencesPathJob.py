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
from FreeCAD import Units
from PySide import QtCore, QtGui
from PathScripts.PathPreferences import PathPreferences
from PathScripts.PathPostProcessor import PostProcessor


class JobPreferencesPage:
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
        geometryTolerance = Units.Quantity(self.form.geometryTolerance.text())
        PathPreferences.savePostProcessorDefaults(processor, args, blacklist, geometryTolerance)

        path = str(self.form.leOutputFile.text())
        policy = str(self.form.cboOutputPolicy.currentText())
        PathPreferences.saveOutputFileDefaults(path, policy)

    def selectComboEntry(self, widget, text):
        index = widget.findText(text, QtCore.Qt.MatchFixedString)
        if index >= 0:
            widget.blockSignals(True)
            widget.setCurrentIndex(index)
            widget.blockSignals(False)

    def verifyAndUpdateDefaultPostProcessorWith(self, processor):
        defaultIsValid = False
        self.form.defaultPostProcessor.blockSignals(True)
        self.form.defaultPostProcessor.clear()
        self.form.defaultPostProcessor.addItem("")
        for i in range(0, self.form.postProcessorList.count()):
            item = self.form.postProcessorList.item(i)
            if item.checkState() == QtCore.Qt.CheckState.Checked:
                self.form.defaultPostProcessor.addItem(item.text())
                if item.text() == processor :
                    defaultIsValid = True
        # if we get here the default processor was disabled
        if not defaultIsValid:
            self.form.defaultPostProcessorArgs.setText('')
            processor = ''
        self.selectComboEntry(self.form.defaultPostProcessor, processor)
        self.form.defaultPostProcessor.blockSignals(False)

    def verifyAndUpdateDefaultPostProcessor(self):
        self.verifyAndUpdateDefaultPostProcessorWith(str(self.form.defaultPostProcessor.currentText()))

    def loadSettings(self):
        blacklist = PathPreferences.postProcessorBlacklist()
        for processor in PathPreferences.allAvailablePostProcessors():
            item = QtGui.QListWidgetItem(processor)
            if processor in blacklist:
                item.setCheckState(QtCore.Qt.CheckState.Unchecked)
            else:
                item.setCheckState(QtCore.Qt.CheckState.Checked)
            item.setFlags( QtCore.Qt.ItemFlag.ItemIsSelectable | QtCore.Qt.ItemFlag.ItemIsEnabled | QtCore.Qt.ItemFlag.ItemIsUserCheckable)
            self.form.postProcessorList.addItem(item)
        self.verifyAndUpdateDefaultPostProcessorWith(PathPreferences.defaultPostProcessor())

        self.form.defaultPostProcessorArgs.setText(PathPreferences.defaultPostProcessorArgs())

        geomTol = Units.Quantity(PathPreferences.defaultGeometryTolerance(), Units.Length)
        self.form.geometryTolerance.setText(geomTol.UserString)

        self.form.leOutputFile.setText(PathPreferences.defaultOutputFile())
        self.selectComboEntry(self.form.cboOutputPolicy, PathPreferences.defaultOutputPolicy())

        self.form.postProcessorList.itemEntered.connect(self.setProcessorListTooltip)
        self.form.postProcessorList.itemChanged.connect(self.verifyAndUpdateDefaultPostProcessor)
        self.form.defaultPostProcessor.currentIndexChanged.connect(self.updateDefaultPostProcessorToolTip)
        self.form.pbBrowseFileSystem.clicked.connect(self.browseFileSystem)

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
        foo = QtGui.QFileDialog.getExistingDirectory(QtGui.qApp.activeWindow(), "Path - Output File/Directory", self.form.leOutputFile.text())
        if foo:
            self.form.leOutputFile.setText(foo)
