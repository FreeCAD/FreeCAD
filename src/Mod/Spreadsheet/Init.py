# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   © 2001 Juergen Riegel <juergen.riegel@web.de>                              #
#   © 2013 Yorik van Havre <yorik@uncreated.net>                               #
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


# FreeCAD init script of the Spreadsheet module


# Get the Parameter Group of this module
ParGrp = App.ParamGet("System parameter:Modules").GetGroup("Spreadsheet")

# Set the needed information
ParGrp.SetString("HelpIndex", "Spreadsheet/Help/index.html")
ParGrp.SetString("WorkBenchName", "Spreadsheet")
ParGrp.SetString("WorkBenchModule", "SpreadsheetWorkbench.py")

# add Import/Export types
App.addImportType("Excel spreadsheet (*.xlsx *.XLSX)", "importXLSX")

App.__unit_test__ += ["TestSpreadsheet"]
