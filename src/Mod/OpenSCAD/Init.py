#***************************************************************************
#*   Copyright (c) 2001,2002 Juergen Riegel <juergen.riegel@web.de>        *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/

# FreeCAD init script of the OpenSCAD module

import os
import FreeCAD

FreeCAD.addImportType("OpenSCAD CSG Format (*.csg)", "importCSG")

param = FreeCAD.ParamGet(\
        "User parameter:BaseApp/Preferences/Mod/OpenSCAD")
openscadfilename = param.GetString('openscadexecutable')
openscadbin = openscadfilename and os.path.isfile(openscadfilename)

if openscadbin:
    FreeCAD.addImportType("OpenSCAD Format (*.scad)", "importCSG")
    FreeCAD.__unit_test__ += ["TestOpenSCADApp"]

FreeCAD.addExportType("OpenSCAD CSG Format (*.csg)", "exportCSG")
FreeCAD.addExportType("OpenSCAD Format (*.scad)", "exportCSG")
