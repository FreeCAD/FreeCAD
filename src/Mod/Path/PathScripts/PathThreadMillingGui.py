# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
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
import PathGui as PGui # ensure Path/Gui/Resources are loaded
import PathScripts.PathCircularHoleBaseGui as PathCircularHoleBaseGui
import PathScripts.PathThreadMilling as PathThreadMilling
import PathScripts.PathGui as PathGui
import PathScripts.PathLog as PathLog
import PathScripts.PathOpGui as PathOpGui
import csv

from PySide import QtCore

__title__ = "Path Thread Milling Operation UI."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "UI and Command for Path Thread Milling Operation."

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule(PathLog.thisModule())

def setupCombo(combo, selections):
    combo.clear()
    for item in selections:
        combo.addItem(item)

def fillThreads(combo, dataFile):
    combo.blockSignals(True)
    combo.clear()
    with open("{}Mod/Path/Data/Threads/{}.csv".format(FreeCAD.getHomePath(), dataFile)) as fp:
        reader = csv.DictReader(fp)
        for row in reader:
            combo.addItem(row['name'], row)
    combo.setEnabled(True)
    combo.blockSignals(False)

class TaskPanelOpPage(PathCircularHoleBaseGui.TaskPanelOpPage):
    '''Controller for the thread milling operation's page'''

    def initPage(self, obj):
        self.majorDia = PathGui.QuantitySpinBox(self.form.threadMajor, obj, 'MajorDiameter') # pylint: disable=attribute-defined-outside-init
        self.minorDia = PathGui.QuantitySpinBox(self.form.threadMinor, obj, 'MinorDiameter') # pylint: disable=attribute-defined-outside-init
        self.pitch    = PathGui.QuantitySpinBox(self.form.threadPitch, obj, 'Pitch') # pylint: disable=attribute-defined-outside-init

        setupCombo(self.form.threadOrientation, obj.Proxy.ThreadOrientations)
        setupCombo(self.form.threadType, obj.Proxy.ThreadTypes)
        setupCombo(self.form.opDirection, obj.Proxy.Directions)

    def getForm(self):
        '''getForm() ... return UI'''
        return FreeCADGui.PySideUic.loadUi(":/panels/PageOpThreadMillingEdit.ui")

    def getFields(self, obj):
        '''getFields(obj) ... update obj's properties with values from the UI'''
        PathLog.track()

        self.majorDia.updateProperty()
        self.minorDia.updateProperty()
        self.pitch.updateProperty()

        obj.ThreadOrientation = self.form.threadOrientation.currentText()
        obj.ThreadType = self.form.threadType.currentText()
        obj.ThreadName = self.form.threadName.currentText()
        obj.Direction = self.form.opDirection.currentText()
        obj.Passes = self.form.opPasses.value()
        obj.LeadInOut = self.form.leadInOut.checkState() == QtCore.Qt.Checked
        obj.TPI = self.form.threadTPI.value()

        self.updateToolController(obj, self.form.toolController)

    def setFields(self, obj):
        '''setFields(obj) ... update UI with obj properties' values'''
        PathLog.track()

        self.form.threadOrientation.setCurrentText(obj.ThreadOrientation)

        self.form.threadType.blockSignals(True)
        self.form.threadName.blockSignals(True)
        self.form.threadType.setCurrentText(obj.ThreadType)
        self._updateFromThreadType()
        self.form.threadName.setCurrentText(obj.ThreadName)
        self.form.threadType.blockSignals(False)
        self.form.threadName.blockSignals(False)
        self.form.threadTPI.setValue(obj.TPI)

        self.form.opPasses.setValue(obj.Passes)
        self.form.opDirection.setCurrentText(obj.Direction)
        self.form.leadInOut.setCheckState(QtCore.Qt.Checked if obj.LeadInOut else QtCore.Qt.Unchecked)

        self.majorDia.updateSpinBox()
        self.minorDia.updateSpinBox()
        self.pitch.updateSpinBox()

        self.setupToolController(obj, self.form.toolController)


    def _isThreadMetric(self):
        return self.form.threadType.currentText() == PathThreadMilling.ObjectThreadMilling.ThreadTypeMetricInternal

    def _isThreadImperial(self):
        return self.form.threadType.currentText() == PathThreadMilling.ObjectThreadMilling.ThreadTypeImperialInternal

    def _updateFromThreadType(self):

        if self.form.threadType.currentText() == PathThreadMilling.ObjectThreadMilling.ThreadTypeCustom:
            self.form.threadName.setEnabled(False)
            self.form.threadFit.setEnabled(False)
            self.form.threadFitLabel.setEnabled(False)
            self.form.threadPitch.setEnabled(True)
            self.form.threadPitchLabel.setEnabled(True)
            self.form.threadTPI.setEnabled(True)
            self.form.threadTPILabel.setEnabled(True)

        if self._isThreadMetric():
            self.form.threadFit.setEnabled(True)
            self.form.threadFitLabel.setEnabled(True)
            self.form.threadPitch.setEnabled(True)
            self.form.threadPitchLabel.setEnabled(True)
            self.form.threadTPI.setEnabled(False)
            self.form.threadTPILabel.setEnabled(False)
            self.form.threadTPI.setValue(0)
            fillThreads(self.form.threadName, 'metric-internal')

        if self._isThreadImperial():
            self.form.threadFit.setEnabled(True)
            self.form.threadFitLabel.setEnabled(True)
            self.form.threadPitch.setEnabled(False)
            self.form.threadPitchLabel.setEnabled(False)
            self.form.threadTPI.setEnabled(True)
            self.form.threadTPILabel.setEnabled(True)
            self.pitch.updateSpinBox(0)
            fillThreads(self.form.threadName, 'imperial-internal')

    def _updateFromThreadName(self):
        thread = self.form.threadName.currentData()
        fit = float(self.form.threadFit.value()) / 100
        mamin = float(thread['dMajorMin'])
        mamax = float(thread['dMajorMax'])
        major = mamin + (mamax - mamin) * fit
        mimin = float(thread['dMinorMin'])
        mimax = float(thread['dMinorMax'])
        minor = mimin + (mimax - mimin) * fit

        if self._isThreadMetric():
            pitch = float(thread['pitch'])
            self.pitch.updateSpinBox(pitch)

        if self._isThreadImperial():
            tpi = int(thread['tpi'])
            self.form.threadTPI.setValue(tpi)
            minor = minor * 25.4
            major = major * 25.4

        self.majorDia.updateSpinBox(major)
        self.minorDia.updateSpinBox(minor)

        self.setDirty()

    def getSignalsForUpdate(self, obj):
        '''getSignalsForUpdate(obj) ... return list of signals which cause the receiver to update the model'''
        signals = []

        signals.append(self.form.threadMajor.editingFinished)
        signals.append(self.form.threadMinor.editingFinished)
        signals.append(self.form.threadPitch.editingFinished)
        signals.append(self.form.threadOrientation.currentIndexChanged)
        signals.append(self.form.threadTPI.editingFinished)
        signals.append(self.form.opDirection.currentIndexChanged)
        signals.append(self.form.opPasses.editingFinished)
        signals.append(self.form.leadInOut.stateChanged)

        signals.append(self.form.toolController.currentIndexChanged)

        return signals

    def registerSignalHandlers(self, obj):
        self.form.threadType.currentIndexChanged.connect(self._updateFromThreadType)
        self.form.threadName.currentIndexChanged.connect(self._updateFromThreadName)
        self.form.threadFit.valueChanged.connect(self._updateFromThreadName)


Command = PathOpGui.SetupOperation('Thread Milling',
        PathThreadMilling.Create,
        TaskPanelOpPage,
        'Path_ThreadMilling',
        QtCore.QT_TRANSLATE_NOOP("PathThreadMilling", "Thread Milling"),
        QtCore.QT_TRANSLATE_NOOP("PathThreadMilling", "Creates a Path Thread Milling operation from features of a base object"),
        PathThreadMilling.SetupProperties)

FreeCAD.Console.PrintLog("Loading PathThreadMillingGui ... done\n")
