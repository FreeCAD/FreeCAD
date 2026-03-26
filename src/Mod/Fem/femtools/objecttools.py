# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "Abstract base class for the work with solvers and meshers"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"


from PySide.QtCore import QProcess
from abc import ABC, abstractmethod
import os
import tempfile

import FreeCAD


class ObjectTools(ABC):
    """Abstract base class for the work with solvers and meshers"""

    def __init__(self, obj):
        obj.Tool = self
        self.obj = obj
        self.model_file = ""
        self.process = QProcess()
        self.analysis = obj.getParentGroup()
        self.fem_param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        self._create_working_directory()

        self.process.finished.connect(self._process_finished)

    def _create_working_directory(self):
        """
        Create working directory according to preferences
        """
        if not os.path.isdir(self.obj.WorkingDirectory):
            gen_param = self.fem_param.GetGroup("General")
            if gen_param.GetBool("UseTempDirectory"):
                self.obj.WorkingDirectory = tempfile.mkdtemp(prefix="fem_")
            elif gen_param.GetBool("UseBesideDirectory"):
                root, ext = os.path.splitext(self.obj.Document.FileName)
                if root:
                    self.obj.WorkingDirectory = os.path.join(root, self.obj.Label)
                    os.makedirs(self.obj.WorkingDirectory, exist_ok=True)
                else:
                    # file not saved, use temporary
                    self.obj.WorkingDirectory = tempfile.mkdtemp(prefix="fem_")
            elif gen_param.GetBool("UseCustomDirectory"):
                sub_dir = self.obj.Document.Name + "-" + self.obj.Label
                base_dir = gen_param.GetString("CustomDirectoryPath")
                # no custom directory, use home directory
                if not base_dir:
                    base_dir = FreeCAD.ConfigGet("UserHomePath")
                self.obj.WorkingDirectory = os.path.join(base_dir, sub_dir)
                os.makedirs(self.obj.WorkingDirectory, exist_ok=True)

    @abstractmethod
    def prepare(self):
        pass

    @abstractmethod
    def compute(self):
        pass

    @abstractmethod
    def update_properties(self):
        pass

    def run(self, blocking=False):
        self.prepare()
        self.compute()
        if blocking:
            return self.process.waitForFinished(-1)
        return None

    def _process_finished(self, code, status):
        if status == QProcess.ExitStatus.NormalExit and code == 0:
            self.update_properties()
