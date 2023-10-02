# ***************************************************************************
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

__title__ = "FreeCAD Python Mesh reader and writer"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package importPyMesh
#  \ingroup FEM
#  \brief FreeCAD Python Mesh reader and writer for FEM workbench

import FreeCAD

from femmesh import meshtools

# ************************************************************************************************
# ********* generic FreeCAD import and export methods ********************************************
# names are fix given from FreeCAD, these methods are called from FreeCAD
# they are set in FEM modules Init.py

pyopen = open


# export mesh to python
def export(
    objectslist,
    filename
):
    "called when freecad exports a file"
    if len(objectslist) != 1:
        FreeCAD.Console.PrintError("This exporter can only export one object.\n")
        return
    obj = objectslist[0]
    if not obj.isDerivedFrom("Fem::FemMeshObject"):
        FreeCAD.Console.PrintError("No FEM mesh object selected.\n")
        return
    femnodes_mesh = obj.FemMesh.Nodes
    femelement_table = meshtools.get_femelement_table(obj.FemMesh)
    if meshtools.is_solid_femmesh(obj.FemMesh):
        fem_mesh_type = "Solid"
    elif meshtools.is_face_femmesh(obj.FemMesh):
        fem_mesh_type = "Face"
    elif meshtools.is_edge_femmesh(obj.FemMesh):
        fem_mesh_type = "Edge"
    else:
        FreeCAD.Console.PrintError("Export of this FEM mesh to Python not supported.\n")
        return
    f = pyopen(filename, "w")
    write_python_mesh_to_file(femnodes_mesh, femelement_table, fem_mesh_type, f)
    f.close()


# ************************************************************************************************
# ********* module specific methods **************************************************************
# writer:
# - a method directly writes a FemMesh to the mesh file
# - a method takes a file handle, mesh data and writes to the file handle

# ********* writer *******************************************************************************

def write(
    fem_mesh,
    filename
):
    """directly write a FemMesh to a Python mesh file
    fem_mesh: a FemMesh"""

    if not fem_mesh.isDerivedFrom("Fem::FemMesh"):
        FreeCAD.Console.PrintError("Not a FemMesh was given as parameter.\n")
        return
    femnodes_mesh = fem_mesh.Nodes
    femelement_table = meshtools.get_femelement_table(fem_mesh)
    if meshtools.is_solid_femmesh(fem_mesh):
        fem_mesh_type = "Solid"
    elif meshtools.is_face_femmesh(fem_mesh):
        fem_mesh_type = "Face"
    elif meshtools.is_edge_femmesh(fem_mesh):
        fem_mesh_type = "Edge"
    else:
        FreeCAD.Console.PrintError("Export of this FEM mesh to Python not supported.\n")
        return
    f = pyopen(filename, "w")
    write_python_mesh_to_file(femnodes_mesh, femelement_table, fem_mesh_type, f)
    f.close()


def write_python_mesh_to_file(femnodes_mesh, femelement_table, fem_mesh_type, f):

    mesh_name = "femmesh"

    # nodes
    f.write("def create_nodes(femmesh):\n")
    f.write("    # nodes\n")
    for node in femnodes_mesh:
        # print(node, ' --> ', femnodes_mesh[node])
        vec = femnodes_mesh[node]
        f.write(
            "    {0}.addNode({1}, {2}, {3}, {4})\n"
            .format(mesh_name, vec.x, vec.y, vec.z, node)
        )
    f.write("    return True\n")
    f.write("\n\n")

    # elements
    f.write("def create_elements(femmesh):\n")
    f.write("    # elements\n")
    for element in femelement_table:
        # print(element, ' --> ', femelement_table[element])
        if fem_mesh_type == "Solid":
            f.write(
                "    {0}.addVolume({1}, {2})\n"
                .format(mesh_name, list(femelement_table[element]), element)
            )
        elif fem_mesh_type == "Face":
            f.write(
                "    {0}.addFace({1}, {2})\n"
                .format(mesh_name, list(femelement_table[element]), element)
            )
        elif fem_mesh_type == "Edge":
            f.write(
                "    {0}.addEdge({1}, {2})\n"
                .format(mesh_name, list(femelement_table[element]), element)
            )
    f.write("    return True\n")
