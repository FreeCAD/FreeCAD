# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "Mesh export for Nastran mesh file format"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package exportPyNastranMesh
#  \ingroup FEM
#  \brief FreeCAD pyNastran Mesh writer for FEM workbench

import FreeCAD

# we need to import FreeCAD before the non FreeCAD library because of the print
try:
    from pyNastran.bdf.bdf import BDF
except Exception:
    FreeCAD.Console.PrintError(
        "Module pyNastran not found. Writing Mystran solver input will not be work.\n"
    )

from FreeCAD import Console

from femmesh import meshtools


# ************************************************************************************************
# ********* generic FreeCAD export methods *******************************************************
# names are fix given from FreeCAD, these methods are called from FreeCAD
# they are set in FEM modules Init.py

def export(
    objectslist,
    filename
):
    "called when freecad exports a file"
    if len(objectslist) != 1:
        Console.PrintError("This exporter can only export one object.\n")
        return
    obj = objectslist[0]
    if not obj.isDerivedFrom("Fem::FemMeshObject"):
        Console.PrintError("No FEM mesh object selected.\n")
        return
    write(obj.FemMesh, filename)


# ************************************************************************************************
# ********* module specific methods **************************************************************
# writer:
# - a method directly writes a FemMesh to the mesh file
# - a method generates the pyNastran code


# ********* writer *******************************************************************************
def write(
    fem_mesh,
    filename
):
    """directly write a FemMesh to a pyNastran mesh file format
    fem_mesh: a FemMesh"""

    if not fem_mesh.isDerivedFrom("Fem::FemMesh"):
        Console.PrintError("Not a FemMesh was given as parameter.\n")
        return
    femnodes_mesh = fem_mesh.Nodes
    femelement_table = meshtools.get_femelement_table(fem_mesh)
    export_element_type = get_export_element_type(fem_mesh, femelement_table)

    model = BDF()
    mesh_pynas_code = get_pynastran_mesh(femnodes_mesh, femelement_table, export_element_type)
    mesh_pynas_code += missing_code_pnynasmesh

    # pynas file
    basefilename = filename[:len(filename) - 4]  # TODO basename is more failsafe
    pynasf = open(basefilename + ".py", "w")
    pynasf.write("# written by FreeCAD\n\n\n")
    pynasf.write("from pyNastran.bdf.bdf import BDF\n")
    pynasf.write("model = BDF()\n\n\n")
    pynasf.write(mesh_pynas_code)

    pynasf.write(
        "model.write_bdf('{}', enddata=True)\n"
        .format(basefilename + "_pyNas.bdf")
    )
    pynasf.close()

    # execute pyNastran code to add grid to the model
    # print(model)
    # print(model.get_bdf_stats())
    exec(mesh_pynas_code)
    # print(model)
    # print(model.get_bdf_stats())

    # write Nastran mesh file
    model.write_bdf(filename, enddata=True)  # TODO FIXME "BEGIN BULK" is missing


def get_pynastran_mesh(
    femnodes_mesh,
    femelement_table,
    export_element_type,
):
    if export_element_type is None:
        Console.PrintError("Error: wrong export_element_type.\n")
        return

    # nodes
    pynas_nodes = "# grid cards, geometric mesh points\n"
    for node in femnodes_mesh:
        vec = femnodes_mesh[node]
        pynas_nodes += "model.add_grid({}, [{}, {}, {}])\n".format(node, vec.x, vec.y, vec.z)
    # print(pynas_nodes)

    # elements
    # Nastran seams to have the same node order as SMESH (FreeCAD) has
    # thus just write the nodes at once
    pynas_elements = "# elements cards\n"
    for element in femelement_table:
        nodes = femelement_table[element]
        # print(element)  #  eleid
        # print(n)  # tuple of nodes
        if export_element_type == "cbar":
            pynas_elements += (
                "model.add_{ele_keyword}({eid}, {pid}, {nodes}, "
                "{orientation_vec}, {gnull})\n"
                .format(
                    ele_keyword=export_element_type,
                    eid=element,
                    pid=1,
                    nodes=nodes,
                    orientation_vec="x=[0.0, 0.0, 1.0]",
                    gnull="g0=None"
                )
            )
        else:
            if export_element_type == "ctetra4":
                ele_keyword = "ctetra"
                # N1, N3, N2, N4
                the_nodes = [nodes[0], nodes[2], nodes[1], nodes[3]]
            elif export_element_type == "ctetra10":
                ele_keyword = "ctetra"
                # N1, N3, N2, N4, N7, N6, N5, N8, N10, N9
                the_nodes = [
                    nodes[0], nodes[2], nodes[1], nodes[3],
                    nodes[6], nodes[5], nodes[4],
                    nodes[7], nodes[9], nodes[8],
                ]
            else:
                ele_keyword = export_element_type
                the_nodes = nodes
            pynas_elements += (
                "model.add_{ele_keyword}({eid}, {pid}, {nodes})\n"
                .format(ele_keyword=ele_keyword, eid=element, pid=1, nodes=the_nodes)
            )
    # print(pynas_elements)

    mesh_pynas_code = "{}\n\n{}\n\n".format(pynas_nodes, pynas_elements)
    return mesh_pynas_code


# Helper
def get_export_element_type(
    femmesh,
    femelement_table=None
):
    return nastran_ele_types[meshtools.get_femmesh_eletype(femmesh, femelement_table)]


nastran_ele_types = {
    "tetra4": "ctetra4",
    "tetra10": "ctetra10",
    "hexa8": None,
    "hexa20": None,
    "tria3": "ctria3",
    "tria6": None,
    "quad4": "cquad4",
    "quad8": None,
    "seg2": "cbar",
    "seg3": None,
    "None": None,
}


missing_code_pnynasmesh = """
model.sol = 101  # is this needed?

# case control
from pyNastran.bdf.bdf import CaseControlDeck
cc = CaseControlDeck([
    #"ECHO = NONE",
    "TITLE = pyNastran for generating solverinput for for Mystran",
    #"SUBCASE 1",
])
model.case_control_deck = cc\n\n
"""
