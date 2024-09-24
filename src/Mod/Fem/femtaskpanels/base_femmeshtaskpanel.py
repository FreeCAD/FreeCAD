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

__title__ = "FreeCAD FEM mesh base task panel for mesh object object"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package base_femmeshtaskpanel
#  \ingroup FEM
#  \brief base task panel for mesh object

import time
import threading
from abc import ABC, abstractmethod

from PySide import QtCore
from PySide import QtGui

import FreeCAD

from femtools.femutils import getOutputWinColor

from . import base_femtaskpanel


class _Process(threading.Thread):
    """
    Class for thread and subprocess manipulation
    'tool' argument must be  an object with a 'compute' method
    and a 'process' attribute of type Popen object
    """

    def __init__(self, tool):
        self.tool = tool
        self._timer = QtCore.QTimer()
        self.success = False
        self.update = False
        self.error = ""
        super().__init__(target=self.tool.compute)
        QtCore.QObject.connect(self._timer, QtCore.SIGNAL("timeout()"), self._check)

    def init(self):
        self._timer.start(100)
        self.start()

    def run(self):
        try:
            self.success = self._target(*self._args, **self._kwargs)
        except Exception as e:
            self.error = str(e)

    def finish(self):
        if self.tool.process:
            self.tool.process.terminate()
        self.join()

    def _check(self):
        if not self.is_alive():
            self._timer.stop()
            self.join()
            if self.success:
                try:
                    self.tool.update_properties()
                    self.update = True
                except Exception as e:
                    self.error = str(e)
                    self.success = False


class _BaseMeshTaskPanel(base_femtaskpanel._BaseTaskPanel, ABC):
    """
    Abstract base class for FemMesh object TaskPanel
    """

    def __init__(self, obj):
        super().__init__(obj)

        self.tool = None
        self.form = None
        self.timer = QtCore.QTimer()
        self.process = None
        self.console_message = ""

    @abstractmethod
    def set_mesh_params(self):
        pass

    @abstractmethod
    def get_mesh_params(self):
        pass

    def getStandardButtons(self):
        button_value = (
            QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Apply | QtGui.QDialogButtonBox.Cancel
        )
        return button_value

    def accept(self):
        if self.process and self.process.is_alive():
            FreeCAD.Console.PrintWarning("Process still running\n")
            return None

        self.timer.stop()
        QtGui.QApplication.restoreOverrideCursor()
        self.set_mesh_params()
        return super().accept()

    def reject(self):
        self.timer.stop()
        QtGui.QApplication.restoreOverrideCursor()
        if self.process and self.process.is_alive():
            self.console_log("Process aborted", "#ff6700")
            self.process.finish()
        else:
            return super().reject()

    def clicked(self, button):
        if button == QtGui.QDialogButtonBox.Apply:
            if self.process and self.process.is_alive():
                FreeCAD.Console.PrintWarning("Process already running\n")
                return None

            self.set_mesh_params()
            self.run_mesher()

    def console_log(self, message="", outputwin_color_type=None):
        self.console_message = self.console_message + (
            '<font color="{}"><b>{:4.1f}:</b></font> '.format(
                getOutputWinColor("Logging"), time.time() - self.time_start
            )
        )
        if outputwin_color_type:
            self.console_message += '<font color="{}">{}</font><br>'.format(
                outputwin_color_type, message
            )
        else:
            self.console_message += message + "<br>"
        self.form.te_output.setText(self.console_message)
        self.form.te_output.moveCursor(QtGui.QTextCursor.End)

    def update_timer_text(self):
        if self.process and self.process.is_alive():
            self.form.l_time.setText(f"Time: {time.time() - self.time_start:4.1f}: ")
        else:
            if self.process:
                if self.process.success:
                    if not self.process.update:
                        return None
                    self.console_log("Success!", "#00AA00")
                else:
                    self.console_log(self.process.error, "#AA0000")
            self.timer.stop()
            QtGui.QApplication.restoreOverrideCursor()

    def run_mesher(self):
        self.process = _Process(self.tool)
        self.timer.start(100)
        self.time_start = time.time()
        self.form.l_time.setText(f"Time: {time.time() - self.time_start:4.1f}: ")
        self.console_message = ""
        self.console_log("Start process...")
        QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)

        self.process.init()

    def get_version(self):
        full_message = self.tool.version()
        QtGui.QMessageBox.information(None, "{} - Information".format(self.tool.name), full_message)
