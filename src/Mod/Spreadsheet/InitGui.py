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


class SpreadsheetWorkbench ( Workbench ):
        "Spreadsheet workbench object"
        Icon = """
            /* XPM */
            static char * Spreadsheet_xpm[] = {
            "16 16 5 1",
            "   c None",
            ".  c #151614",
            "+  c #575956",
            "@  c #969895",
            "#  c #F7F9F6",
            "                ",
            "                ",
            " ...............",
            ".@##@+########@.",
            ".+@@+.@@@@@@@@+.",
            "..+++.+++++++++.",
            ".@##@+########@.",
            ".+@@+.@@@@@@@@+.",
            "..+++.+++++++++.",
            ".@##@+########@.",
            ".+@@+.@@@@@@@@+.",
            "..+++.+++++++++.",
            ".@##@+########@.",
            "..+++.+++++++++.",
            "                ",
            "                "};"""
        MenuText = "Spreadsheet"
        ToolTip = "Spreadsheet workbench"

	def Initialize(self):
		# load the module
                import SpreadsheetGui

	def GetClassName(self):
                return "SpreadsheetGui::Workbench"

Gui.addWorkbench(SpreadsheetWorkbench())

# Append the open handler
FreeCAD.EndingAdd("Spreadsheet formats (*.csv)","SpreadsheetGui")
