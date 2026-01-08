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


# Append the open handler
# FreeCAD.addImportType("STEP 214 (*.step *.stp)","ImportGui")
# FreeCAD.addExportType("STEP 214 (*.step *.stp)","ImportGui")
# FreeCAD.addExportType("IGES files (*.iges *.igs)","ImportGui")
FreeCAD.addImportType("PLMXML files (*.plmxml *.PLMXML)", "PlmXmlParser")
FreeCAD.addImportType("STEPZ Zip File Type (*.stpZ *.stpz *.STPZ)", "stepZ")
FreeCAD.addImportType("glTF (*.gltf *.GLTF *.glb *.GLB)", "ImportGui")
FreeCAD.addExportType("STEPZ zip File Type (*.stpZ *.stpz)", "stepZ")
FreeCAD.addExportType("glTF (*.gltf *.glb)", "ImportGui")
