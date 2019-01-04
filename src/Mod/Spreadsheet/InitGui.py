# Spreadsheet gui init module
# (c) 2003 Juergen Riegel
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up

#***************************************************************************
#*   (c) Juergen Riegel (juergen.riegel@web.de) 2002                       *
#*   Copyright (c) 2013 Eivind Kvedalen (eivind@kvedalen.name)             *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#*   Juergen Riegel 2002                                                   *
#***************************************************************************/

import FreeCAD


class SpreadsheetWorkbench ( Workbench ):
    "Spreadsheet workbench object"
    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Spreadsheet/Resources/icons/SpreadsheetWorkbench.svg"
        self.__class__.MenuText = "Spreadsheet"
        self.__class__.ToolTip = "Spreadsheet workbench"

    def Initialize(self):
        # load the module
        import SpreadsheetGui

    def GetClassName(self):
        return "SpreadsheetGui::Workbench"

Gui.addWorkbench(SpreadsheetWorkbench())

# Append the open handler
FreeCAD.EndingAdd("Spreadsheet formats (*.csv)","SpreadsheetGui")
