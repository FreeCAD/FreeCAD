# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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

__title__ = "FreeCAD FEM solver job control task panel"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

from PySide import QtCore
from PySide import QtGui

import FreeCADGui as Gui
import femsolver.run
import femsolver.report


_UPDATE_INTERVAL = 50
_REPORT_TITLE = "Run Report"
_REPORT_ERR = (
    "Failed to run. Please try again after all"
    "of the following errors are resolved.")


class ControlTaskPanel(QtCore.QObject):

    machineChanged = QtCore.Signal(object)
    machineStarted = QtCore.Signal(object)
    machineStopped = QtCore.Signal(object)
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
        self.machineStopped.connect(self._timer.stop)
        self.machineStopped.connect(self._displayReport)
        self.machineStopped.connect(self.form.updateState)
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
        self.machine.target = femsolver.run.PREPARE
        self.machine.start()

    @QtCore.Slot()
    def run(self):
        self.machine.reset(femsolver.run.SOLVE)
        self.machine.target = femsolver.run.RESULTS
        self.machine.start()

    @QtCore.Slot()
    def edit(self):
        self.machine.reset(femsolver.run.SOLVE)
        self.machine.solver.Proxy.edit(
            self.machine.directory)

    @QtCore.Slot()
    def abort(self):
        self.machine.abort()

    @QtCore.Slot()
    def updateWidget(self):
        self.form.setDirectory(self.machine.directory)
        self.form.setStatus(self.machine.status)
        self.form.setTime(self.machine.time)
        self.form.updateState(self.machine)

    @QtCore.Slot()
    def updateMachine(self):
        if self.form.directory() != self.machine.directory:
            self.machine = femsolver.run.getMachine(
                self.machine.solver, self.form.directory())

    @QtCore.Slot()
    def _updateTimer(self):
        if self.machine.running:
            self._timer.start()

    @QtCore.Slot(object)
    def _displayReport(self, machine):
        text = _REPORT_ERR if machine.failed else None
        femsolver.report.display(machine.report, _REPORT_TITLE, text)

    def getStandardButtons(self):
        return int(QtGui.QDialogButtonBox.Close)

    def reject(self):
        Gui.ActiveDocument.resetEdit()

    def _connectMachine(self, machine):
        self._disconnectMachine()
        machine.signalStatus.add(self._statusProxy)
        machine.signalStatusCleared.add(self._statusClearedProxy)
        machine.signalStarted.add(self._startedProxy)
        machine.signalStopped.add(self._stoppedProxy)
        machine.signalState.add(self._stateProxy)

    def _disconnectMachine(self):
        if self.machine is not None:
            self.machine.signalStatus.remove(self._statusProxy)
            self.machine.signalStatusCleared.add(self._statusClearedProxy)
            self.machine.signalStarted.remove(self._startedProxy)
            self.machine.signalStopped.remove(self._stoppedProxy)
            self.machine.signalState.remove(self._stateProxy)

    def _startedProxy(self):
        self.machineStarted.emit(self.machine)

    def _stoppedProxy(self):
        self.machineStopped.emit(self.machine)

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

    def __init__(self, parent=None):
        super(ControlWidget, self).__init__(parent)
        self._setupUi()
        self._inputFileName = ""

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

    @QtCore.Slot(int)
    def updateState(self, machine):
        if machine.state <= femsolver.run.PREPARE:
            self._writeBtt.setText(self.tr("Write"))
            self._editBtt.setText(self.tr("Edit"))
            self._runBtt.setText(self.tr("Run"))
        elif machine.state <= femsolver.run.SOLVE:
            self._writeBtt.setText(self.tr("Re-write"))
            self._editBtt.setText(self.tr("Edit"))
            self._runBtt.setText(self.tr("Run"))
        else:
            self._writeBtt.setText(self.tr("Re-write"))
            self._editBtt.setText(self.tr("Edit"))
            self._runBtt.setText(self.tr("Re-run"))
        if machine.running:
            self._runBtt.setText(self.tr("Abort"))
        self.setRunning(machine)

    @QtCore.Slot()
    def _selectDirectory(self):
        path = QtGui.QFileDialog.getExistingDirectory(self)
        self.setDirectory(path)
        self.directoryChanged.emit()

    def setRunning(self, machine):
        if machine.running:
            self._runBtt.clicked.connect(self.runClicked)
            self._runBtt.clicked.disconnect()
            self._runBtt.clicked.connect(self.abortClicked)
            self._directoryGrp.setDisabled(True)
            self._writeBtt.setDisabled(True)
            self._editBtt.setDisabled(True)
        else:
            self._runBtt.clicked.connect(self.abortClicked)
            self._runBtt.clicked.disconnect()
            self._runBtt.clicked.connect(self.runClicked)
            self._directoryGrp.setDisabled(False)
            self._writeBtt.setDisabled(False)
            self._editBtt.setDisabled(
                not machine.solver.Proxy.editSupported()
                or machine.state < femsolver.run.PREPARE
            )

##  @}
