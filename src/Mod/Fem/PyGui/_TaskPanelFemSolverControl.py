# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
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


__title__ = "Solver Job Control Task Panel"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


from PySide import QtCore
from PySide import QtGui

import FreeCADGui as Gui
import FemSolve
import FemReport


_UPDATE_INTERVAL = 50
_REPORT_TITLE = "Run Report"
_REPORT_ERR = (
    "Failed to run. Please try again after all"
    "of the following errors are resolved.")


class ControlTaskPanel(QtCore.QObject):

    machineChanged = QtCore.Signal(object)
    machineStarted = QtCore.Signal(object)
    machineStoped = QtCore.Signal(object)
    machineStatusChanged = QtCore.Signal(str)
    machineStatusCleared = QtCore.Signal()
    machineTimeChanged = QtCore.Signal(float)
    machineStateChanged = QtCore.Signal(float)

    def __init__(self, machine):
        super(ControlTaskPanel, self).__init__()
        self.form = ControlWidget()
        self._machine = None

        # Timer that updates the duration indicator.
        self._timer = QtCore.QTimer()
        self._timer.setInterval(_UPDATE_INTERVAL)
        self._timer.timeout.connect(self._timeProxy)

        # Connect object to widget.
        self.form.writeClicked.connect(self.write)
        self.form.editClicked.connect(self.edit)
        self.form.runClicked.connect(self.run)
        self.form.abortClicked.connect(self.abort)
        self.form.directoryChanged.connect(self.updateMachine)
        self.form.analysisTypeChanged.connect(self.updateType)

        # Seems that the task panel doesn't get destroyed. Disconnect
        # as soon as the widget of the task panel gets destroyed.
        self.form.destroyed.connect(self._disconnectMachine)
        self.form.destroyed.connect(self._timer.stop)
        self.form.destroyed.connect(
            lambda: self.machineStatusChanged.disconnect(
                self.form.appendStatus))

        # Connect all proxy signals.
        self.machineStarted.connect(self._timer.start)
        self.machineStarted.connect(self.form.updateState)
        self.machineStoped.connect(self._timer.stop)
        self.machineStoped.connect(self._displayReport)
        self.machineStoped.connect(self.form.updateState)
        self.machineStatusChanged.connect(self.form.appendStatus)
        self.machineStatusCleared.connect(self.form.clearStatus)
        self.machineTimeChanged.connect(self.form.setTime)
        self.machineStateChanged.connect(
                lambda: self.form.updateState(self.machine))
        self.machineChanged.connect(self._updateTimer)

        # Set initial machine. Signal updates the widget.
        self.machineChanged.connect(self.updateWidget)
        self.form.destroyed.connect(
            lambda: self.machineChanged.disconnect(self.updateWidget))

        self.machine = machine

    @property
    def machine(self):
        return self._machine

    @machine.setter
    def machine(self, value):
        self._connectMachine(value)
        self._machine = value
        self.machineChanged.emit(value)

    @QtCore.Slot()
    def write(self):
        self.machine.reset()
        self.machine.target = FemSolve.PREPARE
        self.machine.start()

    @QtCore.Slot()
    def run(self):
        self.machine.reset(FemSolve.SOLVE)
        self.machine.target = FemSolve.RESULTS
        self.machine.start()

    @QtCore.Slot()
    def edit(self):
        pass

    @QtCore.Slot()
    def abort(self):
        self.machine.abort()

    @QtCore.Slot()
    def updateType(self):
        if self.machine.solver.AnalysisType != self.form.analysisType():
            self.machine.solver.AnalysisType = self.form.analysisType()

    @QtCore.Slot()
    def updateWidget(self):
        self.form.setDirectory(self.machine.directory)
        self.form.setSupportedTypes(self.machine.solver.SupportedTypes)
        self.form.setAnalysisType(self.machine.solver.AnalysisType)
        self.form.setStatus(self.machine.status)
        self.form.setTime(self.machine.time)
        self.form.updateState(self.machine)

    @QtCore.Slot()
    def updateMachine(self):
        path = self.form.directory()
        self.machine = FemSolve.getMachine(self.machine.solver, path)

    @QtCore.Slot()
    def _updateTimer(self):
        if self.machine.running:
            self._timer.start()

    @QtCore.Slot(object)
    def _displayReport(self, machine):
        text = _REPORT_ERR if machine.failed else None
        FemReport.display(machine.report, _REPORT_TITLE, text)

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def reject(self):
        Gui.ActiveDocument.resetEdit()

    def _connectMachine(self, machine):
        self._disconnectMachine()
        machine.signalStatus.add(self._statusProxy)
        machine.signalStatusCleared.add(self._statusClearedProxy)
        machine.signalStarted.add(self._startedProxy)
        machine.signalStoped.add(self._stopedProxy)
        machine.signalState.add(self._stateProxy)

    def _disconnectMachine(self):
        if self.machine is not None:
            self.machine.signalStatus.remove(self._statusProxy)
            self.machine.signalStatusCleared.add(self._statusClearedProxy)
            self.machine.signalStarted.remove(self._startedProxy)
            self.machine.signalStoped.remove(self._stopedProxy)
            self.machine.signalState.remove(self._stateProxy)

    def _startedProxy(self):
        self.machineStarted.emit(self.machine)

    def _stopedProxy(self):
        self.machineStoped.emit(self.machine)

    def _statusProxy(self, line):
        self.machineStatusChanged.emit(line)

    def _statusClearedProxy(self):
        self.machineStatusCleared.emit()

    def _timeProxy(self):
        time = self.machine.time
        self.machineTimeChanged.emit(time)

    def _stateProxy(self):
        state = self.machine.state
        self.machineStateChanged.emit(state)


class ControlWidget(QtGui.QWidget):

    writeClicked = QtCore.Signal()
    editClicked = QtCore.Signal()
    runClicked = QtCore.Signal()
    abortClicked = QtCore.Signal()
    directoryChanged = QtCore.Signal()
    analysisTypeChanged = QtCore.Signal()

    def __init__(self, analysisTypes=[], parent=None):
        super(ControlWidget, self).__init__(parent)
        self._setupUi()
        self._typeRadioButtons = {}
        self._analysisTypes = []
        self._inputFileName = ""
        self.setSupportedTypes(analysisTypes)

    def _setupUi(self):
        self.setWindowTitle(self.tr("Solver Control"))
        # Working directory group box
        self._directoryTxt = QtGui.QLineEdit()
        self._directoryTxt.editingFinished.connect(self.directoryChanged)
        directoryBtt = QtGui.QToolButton()
        directoryBtt.setText("...")
        directoryBtt.clicked.connect(self._selectDirectory)
        directoryLyt = QtGui.QHBoxLayout()
        directoryLyt.addWidget(self._directoryTxt)
        directoryLyt.addWidget(directoryBtt)
        self._directoryGrp = QtGui.QGroupBox()
        self._directoryGrp.setTitle(self.tr("Working Directory"))
        self._directoryGrp.setLayout(directoryLyt)

        # Analysis types group box - can be filled with
        # setSupportedTypes method.
        self._analysisTypeGrp = QtGui.QGroupBox()
        self._analysisTypeGrp.setTitle(self.tr("Analysis Type"))
        self._analysisTypeLyt = QtGui.QVBoxLayout()
        self._analysisTypeGrp.setLayout(self._analysisTypeLyt)

        # Action buttons (Write, Edit, Run)
        self._writeBtt = QtGui.QPushButton(self.tr("Write"))
        self._editBtt = QtGui.QPushButton(self.tr("Edit"))
        self._runBtt = QtGui.QPushButton()
        self._writeBtt.clicked.connect(self.writeClicked)
        self._editBtt.clicked.connect(self.editClicked)
        actionLyt = QtGui.QGridLayout()
        actionLyt.addWidget(self._writeBtt, 0, 0)
        actionLyt.addWidget(self._editBtt, 0, 1)
        actionLyt.addWidget(self._runBtt, 1, 0, 1, 2)

        # Solver status log
        self._statusEdt = QtGui.QPlainTextEdit()
        self._statusEdt.setReadOnly(True)

        # Elapsed time indicator
        timeHeaderLbl = QtGui.QLabel(self.tr("Elapsed Time:"))
        self._timeLbl = QtGui.QLabel()
        timeLyt = QtGui.QHBoxLayout()
        timeLyt.addWidget(timeHeaderLbl)
        timeLyt.addWidget(self._timeLbl)
        timeLyt.addStretch()
        timeLyt.setContentsMargins(0, 0, 0, 0)
        self._timeWid = QtGui.QWidget()
        self._timeWid.setLayout(timeLyt)

        # Main layout
        layout = QtGui.QVBoxLayout()
        layout.addWidget(self._directoryGrp)
        layout.addWidget(self._analysisTypeGrp)
        layout.addLayout(actionLyt)
        layout.addWidget(self._statusEdt)
        layout.addWidget(self._timeWid)
        self.setLayout(layout)

    @QtCore.Slot(str)
    def setStatus(self, text):
        if text is None:
            text = ""
        self._statusEdt.setPlainText(text)
        self._statusEdt.moveCursor(QtGui.QTextCursor.End)

    def status(self):
        return self._statusEdt.plainText()

    @QtCore.Slot(str)
    def appendStatus(self, line):
        self._statusEdt.moveCursor(QtGui.QTextCursor.End)
        self._statusEdt.insertPlainText(line)
        self._statusEdt.moveCursor(QtGui.QTextCursor.End)

    @QtCore.Slot(str)
    def clearStatus(self):
        self._statusEdt.setPlainText("")

    @QtCore.Slot(float)
    def setTime(self, time):
        timeStr = "<b>%05.1f</b>" % time if time is not None else ""
        self._timeLbl.setText(timeStr)

    def time(self):
        if (self._timeLbl.text() == ""):
            return None
        return float(self._timeLbl.text())

    @QtCore.Slot(float)
    def setDirectory(self, directory):
        self._directoryTxt.setText(directory)

    def directory(self):
        return self._directoryTxt.text()

    @QtCore.Slot(object)
    def setSupportedTypes(self, types):
        for button in self._typeRadioButtons.itervalues():
            button.deleteLater()
        self._typeRadioButtons = {}
        for t in types:
            button = QtGui.QRadioButton(t)

            def changed(checked):
                if checked:
                    self.analysisTypeChanged.emit()
            button.toggled.connect(changed)
            self._typeRadioButtons[t] = button
            self._analysisTypeLyt.addWidget(button)
        self._analysisTypes = types

    def supportedTypes(self):
        return self._analysisTypes

    @QtCore.Slot(int)
    def updateState(self, machine):
        if machine.state <= FemSolve.PREPARE:
            self._writeBtt.setText(self.tr("Write"))
            self._editBtt.setText(self.tr("Edit"))
            self._runBtt.setText(self.tr("Run"))
        elif machine.state <= FemSolve.SOLVE:
            self._writeBtt.setText(self.tr("Re-write"))
            self._editBtt.setText(self.tr("Edit"))
            self._runBtt.setText(self.tr("Run"))
        else:
            self._writeBtt.setText(self.tr("Re-write"))
            self._editBtt.setText(self.tr("Edit"))
            self._runBtt.setText(self.tr("Re-run"))
        if machine.running:
            self._runBtt.setText(self.tr("Abort"))
        self.setRunning(machine.running)

    @QtCore.Slot(str)
    def setAnalysisType(self, value):
        if value in self._typeRadioButtons:
            self._typeRadioButtons[value].setChecked(True)

    @QtCore.Slot()
    def _selectDirectory(self):
        path = QtGui.QFileDialog.getExistingDirectory(self)
        self.setDirectory(path)

    def analysisType(self):
        for t, button in self._typeRadioButtons.iteritems():
            if button.isChecked():
                return t
        return None

    def setRunning(self, isRunning):
        if isRunning:
            self._runBtt.clicked.connect(self.runClicked)
            self._runBtt.clicked.disconnect()
            self._runBtt.clicked.connect(self.abortClicked)
            self._directoryGrp.setDisabled(True)
            self._analysisTypeGrp.setDisabled(True)
            self._writeBtt.setDisabled(True)
            self._editBtt.setDisabled(True)
        else:
            self._runBtt.clicked.connect(self.abortClicked)
            self._runBtt.clicked.disconnect()
            self._runBtt.clicked.connect(self.runClicked)
            self._directoryGrp.setDisabled(False)
            self._analysisTypeGrp.setDisabled(False)
            self._writeBtt.setDisabled(False)
            self._editBtt.setDisabled(False)

    def _createRadioParent(self):
        if self._radioParent is not None:
            self._radioParent.deleteLater()
        self._radioParent = QtGui.QWidget(self._analysisTypeGrp)
        self._radioParent.setLayout(QtGui.QVBoxLayout())
        return self._radioParent
