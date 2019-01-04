#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2011                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import FreeCAD

# add import/export types
FreeCAD.addImportType("Industry Foundation Classes (*.ifc)","importIFC")
FreeCAD.addExportType("Industry Foundation Classes (*.ifc)","importIFC")
FreeCAD.addImportType("Wavefront OBJ - Arch module (*.obj)","importOBJ")
FreeCAD.addExportType("Wavefront OBJ - Arch module (*.obj)","importOBJ")
FreeCAD.addExportType("WebGL file (*.html)","importWebGL")
FreeCAD.addExportType("JavaScript Object Notation (*.json)","importJSON")
FreeCAD.addImportType("Collada (*.dae)","importDAE")
FreeCAD.addExportType("Collada (*.dae)","importDAE")
FreeCAD.addImportType("3D Studio mesh (*.3ds)","import3DS")
FreeCAD.addImportType("SweetHome3D XML export (*.zip)","importSH3D")
