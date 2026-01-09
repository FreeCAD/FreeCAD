# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   © 2001 Juergen Riegel <juergen.riegel@web.de>                              #
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


# FreeCAD init script of the OpenSCAD module

import os
import FreeCAD

FreeCAD.addImportType("OpenSCAD CSG Format (*.csg *.CSG)", "importCSG")

param = FreeCAD.ParamGet(\
        "User parameter:BaseApp/Preferences/Mod/OpenSCAD")
openscadfilename = param.GetString('openscadexecutable')
openscadbin = openscadfilename and os.path.isfile(openscadfilename)

if openscadbin:
    FreeCAD.addImportType("OpenSCAD Format (*.scad *.SCAD)", "importCSG")
    FreeCAD.__unit_test__ += ["TestOpenSCADApp"]

FreeCAD.addExportType("OpenSCAD CSG Format (*.csg)", "exportCSG")
FreeCAD.addExportType("OpenSCAD Format (*.scad)", "exportCSG")
