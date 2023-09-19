# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
#                                                                           *
#    Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# ***************************************************************************/

# Get the Parameter Group of this module
ParGrp = App.ParamGet("System parameter:Modules").GetGroup("Assembly")

# Set the needed information
ParGrp.SetString("HelpIndex", "Assembly/Help/index.html")
ParGrp.SetString("WorkBenchName", "Assembly")
ParGrp.SetString("WorkBenchModule", "AssemblyWorkbench.py")

FreeCAD.__unit_test__ += ["TestAssemblyWorkbench"]

# This adds a custom import type to the FreeCAD import dialog.
# The correct format for assembly interoperability is a research topic. ASMT is a placeholder.
FreeCAD.addImportType("Assembly Format (*.asmt)", "AssemblyImport")
# FreeCAD.addExportType()
