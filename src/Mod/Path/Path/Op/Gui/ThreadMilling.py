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
import Path
import Path.Base.Gui.Util as PathGuiUtil
import Path.Op.Gui.Base as PathOpGui
import Path.Op.Gui.CircularHoleBase as PathCircularHoleBaseGui
import Path.Op.ThreadMilling as PathThreadMilling
import PathGui
import csv

from PySide.QtCore import QT_TRANSLATE_NOOP


from PySide import QtCore

__title__ = "Path Thread Milling Operation UI."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecad.org"
__doc__ = "UI and Command for Path Thread Milling Operation."

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


def fillThreads(form, dataFile, defaultSelect):
    form.threadName.blockSignals(True)
    select = form.threadName.currentText()
    Path.Log.debug("select = '{}'".format(select))
    form.threadName.clear()
    with open(
        "{}Mod/Path/Data/Threads/{}".format(FreeCAD.getHomePath(), dataFile)
    ) as fp:
        reader = csv.DictReader(fp)
        for row in reader:
            form.threadName.addItem(row["name"], row)
    if select:
        form.threadName.setCurrentText(select)
    elif defaultSelect:
        form.threadName.setCurrentText(defaultSelect)
    form.threadName.setEnabled(True)
    form.threadName.blockSignals(False)


class TaskPanelOpPage(PathCircularHoleBaseGui.TaskPanelOpPage):
    """Controller for the thread milling operation's page"""

    def initPage(self, obj):
        self.majorDia = PathGuiUtil.QuantitySpinBox(
            self.form.threadMajor, obj, "MajorDiameter"
        )
        self.minorDia = PathGuiUtil.QuantitySpinBox(
            self.form.threadMinor, obj, "MinorDiameter"
        )
        self.pitch = PathGuiUtil.QuantitySpinBox(self.form.threadPitch, obj, "Pitch")

    def getForm(self):
        """getForm() ... return UI"""
        form = FreeCADGui.PySideUic.loadUi(":/panels/PageOpThreadMillingEdit.ui")
        comboToPropertyMap = [
            ("threadOrientation", "ThreadOrientation"),
            ("threadType", "ThreadType"),
            ("opDirection", "Direction"),
        ]
        enumTups = PathThreadMilling.ObjectThreadMilling.propertyEnumerations(
            dataType="raw"
        )
        self.populateCombobox(form, enumTups, comboToPropertyMap)

        return form

    def getFields(self, obj):
        """getFields(obj) ... update obj's properties with values from the UI"""
        Path.Log.track()

        self.majorDia.updateProperty()
        self.minorDia.updateProperty()
        self.pitch.updateProperty()

        obj.ThreadOrientation = self.form.threadOrientation.currentData()
        obj.ThreadType = self.form.threadType.currentData()
        obj.ThreadName = self.form.threadName.currentText()
        obj.ThreadFit = self.form.threadFit.value()
        obj.Direction = self.form.opDirection.currentData()
        obj.Passes = self.form.opPasses.value()
        obj.LeadInOut = self.form.leadInOut.checkState() == QtCore.Qt.Checked
        obj.TPI = self.form.threadTPI.value()

        self.updateToolController(obj, self.form.toolController)

    def setFields(self, obj):
        """setFields(obj) ... update UI with obj properties' values"""
        Path.Log.track()

        self.selectInComboBox(obj.ThreadOrientation, self.form.threadOrientation)
        self.selectInComboBox(obj.ThreadType, self.form.threadType)
        self.selectInComboBox(obj.Direction, self.form.opDirection)

        self.form.threadName.blockSignals(True)
        self.form.threadName.setCurrentText(obj.ThreadName)
        self.form.threadName.blockSignals(False)
        self.form.threadFit.setValue(obj.ThreadFit)
        self.form.threadTPI.setValue(obj.TPI)

        self.form.opPasses.setValue(obj.Passes)
        self.form.leadInOut.setCheckState(
            QtCore.Qt.Checked if obj.LeadInOut else QtCore.Qt.Unchecked
        )

        self.majorDia.updateSpinBox()
        self.minorDia.updateSpinBox()
        self.pitch.updateSpinBox()

        self.setupToolController(obj, self.form.toolController)
        self._updateFromThreadType()

    def _isThreadCustom(self):
        return self.form.threadType.currentData() in [
            PathThreadMilling.ThreadTypeCustomInternal,
            PathThreadMilling.ThreadTypeCustomExternal,
        ]

    def _isThreadImperial(self):
        return (
            self.form.threadType.currentData() in PathThreadMilling.ThreadTypesImperial
        )

    def _isThreadMetric(self):
        return self.form.threadType.currentData() in PathThreadMilling.ThreadTypesMetric

    def _isThreadInternal(self):
        return (
            self.form.threadType.currentData() in PathThreadMilling.ThreadTypesInternal
        )

    def _isThreadExternal(self):
        return (
            self.form.threadType.currentData() in PathThreadMilling.ThreadTypesExternal
        )

    def _updateFromThreadType(self):

        if self._isThreadCustom():
            self.form.threadName.setEnabled(False)
            self.form.threadFit.setEnabled(False)
            self.form.threadFitLabel.setEnabled(False)
            self.form.threadPitch.setEnabled(True)
            self.form.threadPitchLabel.setEnabled(True)
            self.form.threadTPI.setEnabled(True)
            self.form.threadTPILabel.setEnabled(True)
        else:
            self.form.threadFit.setEnabled(True)
            self.form.threadFitLabel.setEnabled(True)
            if self._isThreadMetric():
                self.form.threadPitch.setEnabled(True)
                self.form.threadPitchLabel.setEnabled(True)
                self.form.threadTPI.setEnabled(False)
                self.form.threadTPILabel.setEnabled(False)
                self.form.threadTPI.setValue(0)
            else:
                self.form.threadPitch.setEnabled(False)
                self.form.threadPitchLabel.setEnabled(False)
                self.form.threadTPI.setEnabled(True)
                self.form.threadTPILabel.setEnabled(True)
                self.pitch.updateSpinBox(0)
            fillThreads(
                self.form,
                PathThreadMilling.ThreadTypeData[self.form.threadType.currentData()],
                self.obj.ThreadName,
            )
        self._updateFromThreadName()

    def _updateFromThreadName(self):
        if not self._isThreadCustom():
            thread = self.form.threadName.currentData()
            fit = float(self.form.threadFit.value()) / 100
            maxmin = float(thread["dMajorMin"])
            maxmax = float(thread["dMajorMax"])
            major = maxmin + (maxmax - maxmin) * fit
            minmin = float(thread["dMinorMin"])
            minmax = float(thread["dMinorMax"])
            minor = minmin + (minmax - minmin) * fit

            if self._isThreadMetric():
                pitch = float(thread["pitch"])
                self.pitch.updateSpinBox(pitch)

            if self._isThreadImperial():
                tpi = int(thread["tpi"])
                self.form.threadTPI.setValue(tpi)
                minor = minor * 25.4
                major = major * 25.4

            self.majorDia.updateSpinBox(major)
            self.minorDia.updateSpinBox(minor)

        self.setDirty()

    def getSignalsForUpdate(self, obj):
        """getSignalsForUpdate(obj) ... return list of signals which cause the receiver to update the model"""
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


Command = PathOpGui.SetupOperation(
    "ThreadMilling",
    PathThreadMilling.Create,
    TaskPanelOpPage,
    "Path_ThreadMilling",
    QT_TRANSLATE_NOOP("Path_ThreadMilling", "Thread Milling"),
    QT_TRANSLATE_NOOP(
        "Path_ThreadMilling",
        "Creates a Path Thread Milling operation from features of a base object",
    ),
    PathThreadMilling.SetupProperties,
)

FreeCAD.Console.PrintLog("Loading PathThreadMillingGui ... done\n")
