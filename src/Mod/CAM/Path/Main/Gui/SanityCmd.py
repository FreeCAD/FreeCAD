# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""
This file has the GUI command for checking and catching common errors in FreeCAD
CAM projects.
"""

from Path.Main.Sanity import Sanity
from PySide.QtCore import QT_TRANSLATE_NOOP
from PySide.QtGui import QFileDialog
import FreeCAD
import FreeCADGui
import Path
import Path.Log
import os
import webbrowser

translate = FreeCAD.Qt.translate

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class CommandCAMSanity:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Sanity",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Sanity", "Sanity Check"),
            "Accel": "P, S",
            "ToolTip": QT_TRANSLATE_NOOP("CAM_Sanity", "Checks the CAM job for common errors"),
        }

    def IsActive(self):
        selection = FreeCADGui.Selection.getSelectionEx()
        if len(selection) == 0:
            return False
        obj = selection[0].Object
        return isinstance(obj.Proxy, Path.Main.Job.ObjectJob)

    def Activated(self):
        FreeCADGui.addIconPath(":/icons")
        obj = FreeCADGui.Selection.getSelectionEx()[0].Object

        # Ask the user for a filename to save the report to

        defaultDir = os.path.split(FreeCAD.ActiveDocument.getFileName())[0]

        if defaultDir == "":
            defaultDir = os.path.expanduser("~")

        file_location = QFileDialog.getSaveFileName(
            None,
            translate("Path", "Save Sanity Check Report"),
            defaultDir,
            "HTML files (*.html)",
        )[0]

        if file_location == "":
            return

        sanity_checker = Sanity.CAMSanity(obj, file_location)
        html = sanity_checker.get_output_report()

        if html is None:
            Path.Log.error("Sanity check failed. No report generated.")
            return

        with open(file_location, "w") as fp:
            fp.write(html)

        FreeCAD.Console.PrintMessage("Sanity check report written to: {}\n".format(file_location))

        webbrowser.open_new_tab(file_location)


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Sanity", CommandCAMSanity())
