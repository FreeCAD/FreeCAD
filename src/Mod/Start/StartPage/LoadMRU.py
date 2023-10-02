# ***************************************************************************
# *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
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

import os
import FreeCAD
import FreeCADGui

# MRU will be given before this script is run
rf = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/RecentFiles")
filename = rf.GetString("MRU" + str(MRU))
ext = os.path.splitext(filename)[1].lower().strip(".")
mod = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Start").GetString(
    "DefaultImport" + ext, ""
)
if ext.lower() in ("fcstd", "stp", "step", "iges", "igs"):
    FreeCAD.loadFile(filename, mod)
elif ext.lower() in (
    "bmp",
    "cur",
    "gif",
    "ico",
    "pbm",
    "pgm",
    "png",
    "jpg",
    "jpeg",
    "ppm",
    "svg",
    "svgz",
    "xbm",
    "xpm",
):
    FreeCAD.newDocument()
    FreeCADGui.insert(filename, FreeCAD.activeDocument().Name)
    FreeCAD.activeDocument().recompute()
else:
    FreeCADGui.loadFile(filename, mod)
FreeCADGui.activeDocument().sendMsgToViews("ViewFit")

from StartPage import StartPage

StartPage.postStart()
