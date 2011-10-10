# FreeCAD init script of the Mesh module
# (c) 2004 Werner Mayer

#***************************************************************************
#*   (c) Werner Mayer <werner.wm.mayer@gmx.de> 2004                        *
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
#*   Werner Mayer 2004                                                     *
#***************************************************************************/



# Get the Parameter Group of this module
ParGrp = FreeCAD.ParamGet("System parameter:Modules").GetGroup("Mesh")

# Append the open handler
FreeCAD.addImportType("Mesh formats (*.stl *.ast *.bms *.obj *.ply)","Mesh")
FreeCAD.addExportType("Mesh formats (*.stl *.ast *.bms *.obj *.off *.ply)","Mesh")


# Set the needed information
ParGrp.SetString("HelpIndex",        "Mesh/Help/index.html")
ParGrp.SetString("WorkBenchName",    "Mesh Design")
ParGrp.SetString("WorkBenchModule",  "MeshWorkbench.py")
