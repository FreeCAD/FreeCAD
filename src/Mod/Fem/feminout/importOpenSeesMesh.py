# ***************************************************************************
# *   Copyright (c) 2020 Raeyat Roknabadi Ebrahim               *
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

__title__ = "FreeCAD OpenSees Mesh reader and writer"
__author__ = "Raeyat Roknabadi Ebrahim"
__url__ = "http://www.freecadweb.org"

## @package importOpenSeesMesh
#  \ingroup FEM
#  \brief FreeCAD OpenSees Mesh reader and writer for FEM workbench

import os
import FreeCAD
from FreeCAD import Console

# ************************************************************************************************
# ********* generic FreeCAD import and export methods ********************************************
# names are fix given from FreeCAD, these methods are called from FreeCAD
# they are set in FEM modules Init.py

if open.__module__ == "__builtin__":
    # because we'll redefine open below (Python2)
    pyopen = open
elif open.__module__ == "io":
    # because we'll redefine open below (Python3)
    pyopen = open


def open(
    filename
):
    """called when freecad opens a file
    a FEM mesh object is created in a new document"""

    docname = os.path.splitext(os.path.basename(filename))[0]
    return insert(filename, docname)


def insert(
    filename,
    docname
):
    """called when freecad wants to import a file
    a FEM mesh object is created in a existing document"""

    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc

    import_opensees_mesh(filename, docname)
    return doc



def import_opensees_mesh(
    filename,
    analysis=None,
    docname=None
):
    """read a FEM mesh from a OpenSees mesh file and
    insert a FreeCAD FEM Mesh object in the ActiveDocument
    """

    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        try:
            doc = FreeCAD.ActiveDocument
        except NameError:
            doc = FreeCAD.newDocument()
    FreeCAD.ActiveDocument = doc

    mesh_name = os.path.basename(os.path.splitext(filename)[0])

    femmesh = read(filename)
    if femmesh:
        mesh_object = doc.addObject("Fem::FemMeshObject", mesh_name)
        mesh_object.FemMesh = femmesh

    return mesh_object


# ********* writer *******************************************************************************
def write(
    fem_mesh,
    filename
):
    """directly write a FemMesh to a OpenSees mesh file format
    fem_mesh: a FemMesh"""

    if not fem_mesh.isDerivedFrom("Fem::FemMesh"):
        Console.PrintError("Not a FemMesh was given as parameter.\n")
        return
    femnodes_mesh = fem_mesh.Nodes
    import femmesh.meshtools as FemMeshTools
    femelement_table = FemMeshTools.get_femelement_table(fem_mesh)
    # opensees_element_type = get_opensees_element_type(fem_mesh, femelement_table)
    f = pyopen(filename, "w")
    write_opensees_mesh_to_file(femnodes_mesh, femelement_table, opensees_element_type, f)
    f.close()


def write_opensees_mesh_to_file(
    femnodes_mesh,
    femelement_table,
    opensees_element_type,
    f
):
    node_dimension = 3  # 2 for 2D not supported
    node_dof = 3
    # if (
    #     opensees_element_type == 4
    #     or opensees_element_type == 17
    #     or opensees_element_type == 16
    #     or opensees_element_type == 1
    #     or opensees_element_type == 10
    # ):
    #     node_dof = 3
    # elif (
    #     opensees_element_type == 23
    #     or opensees_element_type == 24
    # ):
    #     node_dof = 6  # schalenelemente
    # else:
    #     Console.PrintError("Error: wrong opensees_element_type.\n")
    #     return
    node_count = len(femnodes_mesh)
    element_count = len(femelement_table)
    dofs = node_dof * node_count
    unknown_flag = 0
    written_by = "written by FreeCAD"

    # nodes
    for node in femnodes_mesh:
        vec = femnodes_mesh[node]
        f.write(
            "node {0} {1:.6f}, {2:.6f}, {3:.6f}\n"
            .format(node, vec.x, vec.y, vec.z)
        )
    # elements
    # for element in femelement_table:
    #     # opensees_element_type is checked for every element
    #     # but mixed elements are not supported up to date
    #     n = femelement_table[element]
    #     if (
    #         opensees_element_type == 2
    #         or opensees_element_type == 4
    #         or opensees_element_type == 5
    #         or opensees_element_type == 9
    #         or opensees_element_type == 13
    #         or opensees_element_type == 25
    #     ):
    #         # seg2 FreeCAD --> stab4 Z88
    #         # N1, N2
    #         f.write("{0} {1}\n".format(element, opensees_element_type))
    #         f.write("{0} {1}\n".format(
    #                 n[0], n[1]))
    #     elif opensees_element_type == 3 or opensees_element_type == 14 or opensees_element_type == 24:
    #         # tria6 FreeCAD --> schale24 Z88
    #         # N1, N2, N3, N4, N5, N6
    #         f.write("{0} {1}\n".format(element, opensees_element_type))
    #         f.write("{0} {1} {2} {3} {4} {5}\n".format(
    #                 n[0], n[1], n[2], n[3], n[4], n[5]))
    #     elif opensees_element_type == 7 or opensees_element_type == 20 or opensees_element_type == 23:
    #         # quad8 FreeCAD --> schale23 Z88
    #         # N1, N2, N3, N4, N5, N6, N7, N8
    #         f.write("{0} {1}\n".format(element, opensees_element_type))
    #         f.write("{0} {1} {2} {3} {4} {5} {6} {7}\n".format(
    #                 n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7]))
    #     elif opensees_element_type == 17:
    #         # tetra4 FreeCAD --> volume17 Z88
    #         # N4, N2, N3, N1
    #         f.write("{0} {1}\n".format(element, opensees_element_type))
    #         f.write("{0} {1} {2} {3}\n".format(
    #                 n[3], n[1], n[2], n[0]))
    #     elif opensees_element_type == 16:
    #         # tetra10 FreeCAD --> volume16 Z88
    #         # N1, N2, N4, N3, N5, N9, N8, N6, N10, N7, FC to Z88 is different as Z88 to FC
    #         f.write("{0} {1}\n".format(element, opensees_element_type))
    #         f.write("{0} {1} {2} {3} {4} {5} {6} {7} {8} {9}\n".format(
    #                 n[0], n[1], n[3], n[2], n[4], n[8], n[7], n[5], n[9], n[6]))
    #     elif opensees_element_type == 1:
    #         # hexa8 FreeCAD --> volume1 Z88
    #         # N1, N2, N3, N4, N5, N6, N7, N8
    #         f.write("{0} {1}\n".format(element, opensees_element_type))
    #         f.write("{0} {1} {2} {3} {4} {5} {6} {7}\n".format(
    #                 n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7]))
    #     elif opensees_element_type == 10:
    #         # hexa20 FreeCAD --> volume10 Z88
    #         # N2, N3, N4, N1, N6, N7, N8, N5, N10, N11
    #         # N12, N9,  N14, N15, N16, N13, N18, N19, N20, N17
    #         # or turn by 90 degree and they match !
    #         # N1, N2, N3, N4, N5, N6, N7, N8, N9, N10
    #         # N11, N12, N13, N14, N15, N16, N17, N18, N19, N20
    #         f.write("{0} {1}\n".format(element, opensees_element_type))
    #         f.write(
    #             "{0} {1} {2} {3} {4} {5} {6} {7} {8} {9} "
    #             "{10} {11} {12} {13} {14} {15} {16} {17} {18} {19}\n"
    #             .format(
    #                 n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7], n[8], n[9],
    #                 n[10], n[11], n[12], n[13], n[14], n[15], n[16], n[17], n[18], n[19]
    #             )
    #         )
    #     else:
    #         Console.PrintError(
    #             "Writing of Z88 elementtype {0} not supported.\n".format(opensees_element_type)
    #         )
    #         # TODO support schale12 (made from prism15) and schale16 (made from hexa20)
    #         return
