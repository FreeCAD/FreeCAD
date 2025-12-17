# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   Copyright (c) 2018 Gaël Écorchard <galou_breizh@yahoo.fr>                  #
#   Copyright (c) 2022 FreeCAD Project Association                             #
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import platform
import shutil
from pathlib import Path
import FreeCAD


def get_python_exe() -> str:
    """
    Find Python. In preference order.

    A) The value of the BaseApp/Preferences/PythonConsole/PathToPythonExecutable user preference
    B) The executable located in the same bin directory as FreeCAD and called "python3"
    C) The executable located in the same bin directory as FreeCAD and called "python"
    D) The result of a shutil search for your system's "python3" executable
    E) The result of a shutil search for your system's "python" executable
    """

    preferences = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/PythonConsole")

    if python_exe := preferences.GetString("PathToPythonExecutable", ""):
        python_exe = Path(python_exe)
        if python_exe.exists():
            return str(python_exe)

    windows = "Windows" in platform.system()
    fc_dir = Path(FreeCAD.getHomePath()) / "bin"

    python_exe = fc_dir / ("python3.exe" if windows else "python3")
    if python_exe.exists():
        return str(python_exe)

    python_exe = fc_dir / ("python.exe" if windows else "python")
    if python_exe.exists():
        return str(python_exe)

    if python_exe := shutil.which("python3"):
        python_exe = Path(python_exe)
        if python_exe.exists():
            return str(python_exe)

    if python_exe := shutil.which("python"):
        python_exe = Path(python_exe)
        if python_exe.exists():
            return str(python_exe)

    return ""
