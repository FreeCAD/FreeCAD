# ***************************************************************************
# *   Copyright (c) 2001 - Juergen Riegel <juergen.riegel@web.de>           *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
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
# *   Juergen Riegel 2002                                                   *
# ***************************************************************************/

# FreeCAD init script of the Fem module

import FreeCAD


FreeCAD.addExportType("FEM mesh TetGen (*.poly)", "feminout.convert2TetGen")

FreeCAD.addImportType("FEM mesh formats (*.unv *.med *.dat *.bdf)", "Fem")
FreeCAD.addExportType("FEM mesh formats (*.unv *.med *.stl *.dat *.inp *.vtk *.vtu)", "Fem")

FreeCAD.addImportType("FEM mesh CalculiX/Abaqus (*.inp)", "feminout.importInpMesh")
FreeCAD.addImportType("FEM result CalculiX (*.frd)", "feminout.importCcxFrdResults")

FreeCAD.addImportType("FEM mesh Fenics (*.xml *.xdmf)", "feminout.importFenicsMesh")
FreeCAD.addExportType("FEM mesh Fenics (*.xml *.xdmf)", "feminout.importFenicsMesh")

FreeCAD.addImportType("FEM mesh Z88 (*i1.txt)", "feminout.importZ88Mesh")
FreeCAD.addExportType("FEM mesh Z88 (*i1.txt)", "feminout.importZ88Mesh")

FreeCAD.addImportType("FEM result Z88 displacements (*o2.txt)", "feminout.importZ88O2Results")

if("BUILD_FEM_VTK" in FreeCAD.__cmake__):
    FreeCAD.addImportType("FEM result VTK (*.vtk *.vtu)", "feminout.importVTKResults")
    FreeCAD.addExportType("FEM result VTK (*.vtk *.vtu)", "feminout.importVTKResults")
