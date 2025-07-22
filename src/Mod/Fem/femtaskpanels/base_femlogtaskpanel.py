# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM base task panel with logging"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package base_femlogtaskpanel
#  \ingroup FEM
#  \brief base task panel for logging

from abc import ABC, abstractmethod

from PySide import QtCore
from PySide import QtGui

import FreeCAD

from femtools.femutils import getOutputWinColor

from . import base_femtaskpanel


class _Thread(QtCore.QThread):
    """
    Class for thread and subprocess manipulation
    'tool' argument must be  an object with 'compute', 'prepare', 'update_properties' methods
    and a 'process' attribute of type QProcess object
    """

    def __init__(self, tool):
        super().__init__()
        self.tool = tool
        self.prepare_ok = False

    def run(self):
        try:
            self.tool.prepare()
            self.prepare_ok = True
        except Exception as e:
            self.prepare_ok = False
            FreeCAD.Console.PrintError("{}\n".format(e))


class _BaseLogTaskPanel(base_femtaskpanel._BaseTaskPanel, ABC):
    """
    Abstract base class for TaskPanel with logging
    """

    def __init__(self, obj, tool):
        super().__init__(obj)
        self.tool = tool
        self.timer = QtCore.QTimer()
        self.elapsed = QtCore.QElapsedTimer()
        self._thread = _Thread(self.tool)
        self.text_log = None
        self.text_time = None

    def setup_connections(self):
        QtCore.QObject.connect(self._thread, QtCore.SIGNAL("started()"), self.thread_started)
        QtCore.QObject.connect(self._thread, QtCore.SIGNAL("finished()"), self.thread_finished)
        QtCore.QObject.connect(self.tool.process, QtCore.SIGNAL("started()"), self.process_started)
        QtCore.QObject.connect(
            self.tool.process,
            QtCore.SIGNAL("finished(int,QProcess::ExitStatus)"),
            self.process_finished,
        )
        QtCore.QObject.connect(
            self.tool.process,
            QtCore.SIGNAL("finished(int,QProcess::ExitStatus)"),
            self.stop_timer,
        )
        QtCore.QObject.connect(
            self.tool.process,
            QtCore.SIGNAL("readyReadStandardOutput()"),
            self.write_output,
        )
        QtCore.QObject.connect(
            self.tool.process,
            QtCore.SIGNAL("readyReadStandardError()"),
            self.write_error,
        )
        QtCore.QObject.connect(self.timer, QtCore.SIGNAL("timeout()"), self.update_timer_text)

    def thread_started(self):
        self.text_log.clear()
        self.write_log("Prepare process...\n", QtGui.QColor(getOutputWinColor("Text")))
        QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)

    def thread_finished(self):
        QtGui.QApplication.restoreOverrideCursor()
        if self._thread.prepare_ok:
            self.write_log("Preparation finished\n", QtGui.QColor(getOutputWinColor("Text")))
            self.preparation_finished()
        else:
            self.timer.stop()
            self.write_log("Preparation failed.\n", QtGui.QColor(getOutputWinColor("Error")))
            return None

    def preparation_finished(self):
        self.tool.compute()

    def process_finished(self, code, status):
        if status == QtCore.QProcess.ExitStatus.NormalExit:
            if code != 0:
                self.write_log(
                    "Process finished with errors. Result not updated\n",
                    QtGui.QColor(getOutputWinColor("Error")),
                )
                return
            self.tool.update_properties()
            self.write_log("Process finished\n", QtGui.QColor(getOutputWinColor("Text")))
        else:
            self.write_log("Process crashed\n", QtGui.QColor(getOutputWinColor("Error")))

    def process_started(self):
        QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
        self.write_log("Start process...\n", QtGui.QColor(getOutputWinColor("Text")))

    def write_output(self):
        self.write_log(
            self.tool.process.readAllStandardOutput().data().decode("utf-8"),
            QtGui.QColor(getOutputWinColor("Logging")),
        )

    def write_error(self):
        self.write_log(
            self.tool.process.readAllStandardError().data().decode("utf-8"),
            QtGui.QColor(getOutputWinColor("Error")),
        )

    def write_log(self, data, color):
        cursor = QtGui.QTextCursor(self.text_log.document())
        cursor.beginEditBlock()
        cursor.movePosition(QtGui.QTextCursor.End)
        fmt = QtGui.QTextCharFormat()
        fmt.setForeground(color)
        cursor.mergeCharFormat(fmt)
        cursor.insertText(data)
        cursor.endEditBlock()
        self.text_log.ensureCursorVisible()

    @abstractmethod
    def set_object_params(self):
        pass

    @abstractmethod
    def get_object_params(self):
        pass

    def getStandardButtons(self):
        button_value = (
            QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Cancel
        )
        return button_value

    def accept(self):
        if (
            self._thread.isRunning()
            or self.tool.process.state() != QtCore.QProcess.ProcessState.NotRunning
        ):
            FreeCAD.Console.PrintWarning("Process still running\n")
            return None

        self.timer.stop()
        QtGui.QApplication.restoreOverrideCursor()
        self.set_object_params()
        return super().accept()

    def reject(self):
        # self_thread may be blocking
        if self._thread.isRunning():
            return None
        self.timer.stop()
        QtGui.QApplication.restoreOverrideCursor()
        if self.tool.process.state() != QtCore.QProcess.ProcessState.NotRunning:
            self.tool.process.kill()
            FreeCAD.Console.PrintWarning("Process aborted\n")
        else:
            return super().reject()

    def clicked(self, button):
        if button == QtGui.QDialogButtonBox.Apply:
            if (
                self._thread.isRunning()
                or self.tool.process.state() != QtCore.QProcess.ProcessState.NotRunning
            ):
                FreeCAD.Console.PrintWarning("Process already running\n")
                return None

            self.apply()

    def apply(self):
        self.run_process()

    def update_timer_text(self):
        self.text_time.setText(f"Time: {self.elapsed.elapsed()/1000:4.1f} s")

    def stop_timer(self, code, status):
        self.timer.stop()
        QtGui.QApplication.restoreOverrideCursor()

    def run_process(self):
        self.set_object_params()
        self.elapsed.start()
        self.update_timer_text()
        self.timer.start(100)

        self._thread.start()

    def get_version(self):
        full_message = self.tool.version()
        QtGui.QMessageBox.information(None, "{} - Info".format(self.tool.name), full_message)
