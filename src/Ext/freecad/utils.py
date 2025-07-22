# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2023 FreeCAD Project Association                   *
# *   Copyright (c) 2018 Gaël Écorchard <galou_breizh@yahoo.fr>             *
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

import os
import platform
import shutil

import FreeCAD


def get_python_exe() -> str:
    """Find Python. In preference order
    A) The value of the BaseApp/Preferences/PythonConsole/ExternalPythonExecutable user preference
    B) The executable located in the same bin directory as FreeCAD and called "python3"
    C) The executable located in the same bin directory as FreeCAD and called "python"
    D) The result of a shutil search for your system's "python3" executable
    E) The result of a shutil search for your system's "python" executable"""
    prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/PythonConsole")
    python_exe = prefs.GetString("ExternalPythonExecutable", "Not set")
    fc_dir = FreeCAD.getHomePath()
    if not python_exe or python_exe == "Not set" or not os.path.exists(python_exe):
        python_exe = os.path.join(fc_dir, "bin", "python3")
        if "Windows" in platform.system():
            python_exe += ".exe"

    if not python_exe or not os.path.exists(python_exe):
        python_exe = os.path.join(fc_dir, "bin", "python")
        if "Windows" in platform.system():
            python_exe += ".exe"

    if not python_exe or not os.path.exists(python_exe):
        python_exe = shutil.which("python3")

    if not python_exe or not os.path.exists(python_exe):
        python_exe = shutil.which("python")

    if not python_exe or not os.path.exists(python_exe):
        return ""

    python_exe = python_exe.replace("/", os.path.sep)
    prefs.SetString("ExternalPythonExecutable", python_exe)
    return python_exe
