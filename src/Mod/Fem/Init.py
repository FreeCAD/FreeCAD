# ***************************************************************************
# *   Copyright (c) 2001 Juergen Riegel <juergen.riegel@web.de>             *
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""FEM module App init script

Gathering all the information to start FreeCAD.
This is the first one of three init scripts.
The third one runs when the gui is up.

The script is executed using exec().
This happens inside srd/Gui/FreeCADGuiInit.py
All imports made there are available here too.
Thus no need to import them here.
But the import code line is used anyway to get flake8 quired.
Since they are cached they will not be imported twice.
"""

__title__ = "FEM module App init script"
__author__ = "Juergen Riegel, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

# imports to get flake8 quired
import sys
import FreeCAD

# needed imports
from femtools.migrate_app import FemMigrateApp


# migrate old FEM App objects
sys.meta_path.append(FemMigrateApp())


# add FEM App unit tests
FreeCAD.__unit_test__ += ["TestFemApp"]


# add import and export file types
FreeCAD.addExportType("FEM mesh Python (*.meshpy)", "feminout.importPyMesh")

FreeCAD.addExportType("FEM mesh TetGen (*.poly)", "feminout.convert2TetGen")

# see FemMesh::read() and FemMesh::write() methods in src/Mod/Fem/App/FemMesh.cpp
FreeCAD.addImportType(
    "FEM mesh formats (*.bdf *.dat *.inp *.med *.unv *.vtk *.vtu *.pvtu *.z88)", "Fem"
)
FreeCAD.addExportType(
    "FEM mesh formats (*.dat *.inp *.med *.stl *.unv *.vtk *.vtu *.z88)", "Fem"
)

FreeCAD.addExportType("FEM mesh Nastran (*.bdf)", "feminout.exportNastranMesh")

FreeCAD.addImportType("FEM result CalculiX (*.frd)", "feminout.importCcxFrdResults")

FreeCAD.addImportType("FEM mesh Fenics (*.xml *.xdmf)", "feminout.importFenicsMesh")
FreeCAD.addExportType("FEM mesh Fenics (*.xml *.xdmf)", "feminout.importFenicsMesh")

FreeCAD.addImportType(
    "FEM mesh YAML/JSON (*.meshyaml *.meshjson *.yaml *.json)", "feminout.importYamlJsonMesh"
)
FreeCAD.addExportType(
    "FEM mesh YAML/JSON (*.meshyaml *.meshjson *.yaml *.json)", "feminout.importYamlJsonMesh"
)

FreeCAD.addImportType("FEM mesh Z88 (*i1.txt)", "feminout.importZ88Mesh")
FreeCAD.addExportType("FEM mesh Z88 (*i1.txt)", "feminout.importZ88Mesh")

FreeCAD.addImportType("FEM result Z88 displacements (*o2.txt)", "feminout.importZ88O2Results")

if "BUILD_FEM_VTK" in FreeCAD.__cmake__:
    FreeCAD.addImportType("FEM result VTK (*.vtk *.vtu *.pvtu)", "feminout.importVTKResults")
    FreeCAD.addExportType("FEM result VTK (*.vtk *.vtu)", "feminout.importVTKResults")
