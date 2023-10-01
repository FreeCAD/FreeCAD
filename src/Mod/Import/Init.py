# FreeCAD init script of the Import module
# (c) 2001 Juergen Riegel

# ***************************************************************************
# *   Copyright (c) 2002 Juergen Riegel <juergen.riegel@web.de>             *
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
# ***************************************************************************/


# Append the open handler
# FreeCAD.addImportType("STEP 214 (*.step *.stp)","ImportGui")
# FreeCAD.addExportType("STEP 214 (*.step *.stp)","ImportGui")
# FreeCAD.addExportType("IGES files (*.iges *.igs)","ImportGui")
FreeCAD.addImportType("PLMXML files (*.plmxml)", "PlmXmlParser")
FreeCAD.addImportType("STEPZ Zip File Type (*.stpZ *.stpz)", "stepZ")
FreeCAD.addImportType("glTF (*.gltf *.glb)", "ImportGui")
FreeCAD.addExportType("STEPZ zip File Type (*.stpZ *.stpz)", "stepZ")
FreeCAD.addExportType("glTF (*.gltf *.glb)", "ImportGui")
