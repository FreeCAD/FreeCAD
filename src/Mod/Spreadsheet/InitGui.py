# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   © 2002 Juergen Riegel <juergen.riegel@web.de>                              #
#   © 2013 Eivind Kvedalen <eivind@kvedalen.name>                              #
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


# Spreadsheet gui init module
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up


class SpreadsheetWorkbench(Workbench):
    "Spreadsheet workbench object"

    def __init__(self):
        self.__class__.Icon = (
            FreeCAD.getResourceDir() + "Mod/Spreadsheet/Resources/icons/SpreadsheetWorkbench.svg"
        )
        self.__class__.MenuText = "Spreadsheet"
        self.__class__.ToolTip = "Spreadsheet workbench"

    def Initialize(self):
        # load the module
        import SpreadsheetGui

    def GetClassName(self):
        return "SpreadsheetGui::Workbench"


Gui.addWorkbench(SpreadsheetWorkbench())

# Append the open handler
FreeCAD.addImportType("Spreadsheet formats (*.csv *.CSV)", "SpreadsheetGui")
