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
import sys

import FreeCAD


def get_python_exe() -> str:
    """Find Python. In preference order
    A) The value of the BaseApp/Preferences/PythonConsole/PathToPythonExecutable user preference
    B) The executable associated with the linked libpython*.so/.dll/.dylib
    C) The system installed executable
    """
    preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/PythonConsole")
    python_exe = preferences.GetString("PathToPythonExecutable", "").replace("/", os.path.sep)
    if not python_exe or not os.path.exists(python_exe):
        return sys.executable
    return python_exe
