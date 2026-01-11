# SPDX-License-Identifier: LGPL-2.1-or-later
#
# this script is meant to be called by nsis installer scripts, it gets version information
# from freecad and writes version.nsh file in the directory the script is located at
import FreeCAD
import datetime
import os

filepath=os.path.join(os.path.dirname(os.path.abspath(__file__)),"version.nsh")
v=FreeCAD.Version()
content=f'''\
!define COPYRIGHT_YEAR {datetime.date.today().year}
!define APP_VERSION_MAJOR "{v[0]}"
!define APP_VERSION_MINOR "{v[1]}"
!define APP_VERSION_PATCH "{v[2]}"
!define APP_VERSION_REVISION "{v[3].split()[0]}"
'''

with open(filepath, "w", encoding="utf-8") as file:
    file.writelines(content)
