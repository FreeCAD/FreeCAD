#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#***************************************************************************
#*   update_translations.py                                                *
#*   generate master .ts file for FreeCAD translations                     *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import glob
import subprocess # nosec


updater = "lupdate"
workbench = "SheetMetal"
translation_files = ["../../*.py", "../panels/*.ui"]

files = []
for pattern in translation_files:
    files.extend(glob.glob(pattern))

print(f"Found {len(files)} files to process:")
for f in files:
    print(f" - {f}")

cmd = [updater] + files + ["-ts", f"{workbench}.ts"]
result = subprocess.run( # nosec
    cmd,
    capture_output=True,
    text=True,
    check=False
)
if result.returncode != 0:
    print(f"Error running {updater}: {result.stderr}")
else:
    print(f"{updater} ran successfully.")
    print(result.stdout)
