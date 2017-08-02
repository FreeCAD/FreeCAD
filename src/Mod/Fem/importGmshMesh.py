# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Qingfeng Xia <ox.ac.uk>             *
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
from __future__ import print_function

__title__ = "FreeCAD Gmsh supported format mesh export"
__author__ = "Qingfeng Xia, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package importGmshMesh
#  \ingroup FEM
#  \brief FreeCAD gmsh supported format mesh export from FemMeshGmsh object

import os
import FreeCAD
import importToolsFem

import FemGui
import FemGmshTools


########## generic FreeCAD import and export methods ##########
# only export are supported, and boundary mesh might also be exported

def export(objectslist, fileString):
    "called when freecad exports a mesh file supprted by gmsh generation"
    if len(objectslist) != 1:
        FreeCAD.Console.PrintError("This exporter can only export one object.\n")
        return
    obj = objectslist[0]
    if not obj.isDerivedFrom("Fem::FemMeshObject"):
        FreeCAD.Console.PrintError("No FEM mesh object selected.\n")
        return
    #if not obj.Type == 'FemMeshGmsh':
    #    FreeCAD.Console.PrintError("Object selected is not a FemMeshGmsh type\n")
    #    return

    gmsh = FemGmshTools.FemGmshTools(obj, FemGui.getActiveAnalysis())
    if fileString != "":
        fileName, fileExtension = os.path.splitext(fileString)
        for k in FemGmshTools.FemGmshTools.output_format_suffix:
            if FemGmshTools.FemGmshTools.output_format_suffix[k] == fileExtension.lower():
                ret = gmsh.export_mesh(k, fileString)
                if not ret:
                    FreeCAD.Console.PrintError("Mesh is written to `{}` by Gmsh\n".format(ret))
                return
        FreeCAD.Console.PrintError("Export mesh format with suffix `{}` is not supported by Gmsh\n".format(fileExtension.lower()) )
