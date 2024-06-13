# ***************************************************************************
# *   Copyright (c) 2016 Frantisek Loeffelmann <LoffF@email.cz>             *
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

__title__ = "Mesh import for .msh file format"
__url__ = "https://www.freecad.org"

## @package importMshMesh
#  \ingroup FEM
#  \brief FreeCAD Msh file reader for FEM workbench

import os

import FreeCAD
from FreeCAD import Console
from builtins import open as pyopen




# ********* module specific methods *********

def read(filename):
    """read a FemMesh from a .msh mesh file and return the FemMesh
    """

    mesh_data = read_msh(filename)
    from . import importToolsFem
    return importToolsFem.make_femmesh(mesh_data)

def read_msh(file_name):
    """read .msh file """
    # ATM only mesh reading is supported (no boundary conditions)

    class elements():

        tria3 = {}
        tria6 = {}
        quad4 = {}
        quad8 = {}
        tetra4 = {}
        tetra10 = {}
        hexa8 = {}
        hexa20 = {}
        penta6 = {}
        penta15 = {}
        seg2 = {}
        seg3 = {}
    nodes = {}
    contigous_numbered = False

    f = pyopen(file_name, "r")
    line = "\n"
    include = ""
    f_include = None
    total_num_nodes = 0
    node_ind = 1
    node_tag_array = []
    tag_ind = 0
    node_coord = False
    num_element = 0
    number_of_nodes = 0
    read_elements = False
    read_node = False
    elm_category = []
    point_elem = False
    number_of_nodes = 0
    while line != "":
        line = f.readline()
        if line.strip() == "":
            continue
        
        elm_type = ""
        error_seg3 = False
        error_not_supported_elemtype = False

        # reading nodes
        if (line[:6].upper() == "$NODES"):
            read_node = True
            line = f.readline()
            line_list = line.split(" ")
            total_num_nodes = int(line_list[1])
            node_ind = int(line_list[2])
            if(int(line_list[3])- int(line_list[2]) + 1 == total_num_nodes):
                contigous_numbered = True
        elif (read_node is True) and (len(nodes) < total_num_nodes):
            line_list = line.split(" ")
            if contigous_numbered is True:
                if(len(line_list) == 3):
                    x = float(line_list[0])
                    y = float(line_list[1])
                    z = float(line_list[2])
                    nodes[node_ind] = [x, y, z]
                    node_ind += 1
                    
            else:
                if(len(line_list) == 4):
                    node_tag_array = [0]*int(line_list[3])
                    tag_ind = 0
                elif(len(line_list) == 1):
                    node_tag_array[tag_ind] = int(line_list[0])
                    tag_ind += 1
                    
                else:
                    if(node_coord is False):
                        node_coord = True
                        node_ind = 0
                    x = float(line_list[0])
                    y = float(line_list[1])
                    z = float(line_list[2])
                    nodes[node_tag_array[node_ind]] = [x, y, z]
                    nod_ind += 1


        # reading elements
        elif line[:9].upper() == "$ELEMENTS":
            read_elements = True
            line = f.readline()
            continue;
        elif line[:12].upper() == "$ENDELEMENTS":
            break
        elif (read_elements is True) and (num_element == 0):
            point_elem = False
            line_list = line.split(" ")
            num_element = int(line_list[3])
            elm_type = int(line_list[2])
            
            if elm_type == 1:
                elm_category = elements.seg2
                number_of_nodes = 2
            elif elm_type == 2:
                elm_category = elements.tria3
                number_of_nodes = 3
            elif elm_type == 3:
                elm_category = elements.quad4
                number_of_nodes = 4
            elif elm_type == 4:
                elm_category = elements.tetra4
                number_of_nodes = 4
            elif elm_type == 5:
                elm_category = elements.hexa8
                number_of_nodes = 8
            elif elm_type == 6:
                elm_category = elements.penta6
                number_of_nodes = 6
            elif elm_type == 8:
                elm_category = elements.seg3
                number_of_nodes = 3
            elif elm_type == 9:
                elm_category = elements.tria6
                number_of_nodes = 6
            elif elm_type == 11:
                elm_category = elements.tetra10
                number_of_nodes = 10
            elif elm_type == 16:
                elm_category = elements.quad8
                number_of_nodes = 15
            elif elm_type == 17:
                elm_category = elements.hexa20
                number_of_nodes = 20
            elif elm_type == 18:
                elm_category = elements.penta15
                number_of_nodes = 15
                error_seg3 = True  # to print "not supported"
            elif elm_type == 15:
                point_elem = True
            else:
                error_not_supported_elemtype = True
        elif elm_category != [] and num_element > 0:
           
            line_list = line.split(" ")
            element_tag = int(line_list[0])
            elm_category[element_tag] = []
            ind = 1
            for i in range(number_of_nodes):
                if ind >= len(line_list):
                    line = f.readline()
                    line_list = line.split(" ")
                    ind = 0
                elm_category[element_tag].append(int(line_list[ind]))
                ind += 1
            num_element -= 1
        elif point_elem is True:
            num_element -= 1

        
    if error_seg3 is True:  # to print "not supported"
        Console.PrintError("Error: seg3 (3-node beam element type) not supported, yet.\n")
    elif error_not_supported_elemtype is True:
        Console.PrintError("Error: {} not supported.\n".format(elm_type))
    f.close()


    # switch from the .msh node numbering to the FreeCAD node numbering
    for en in elements.tetra10:
        n = elements.tetra10[en]
        elements.tetra10[en] = [n[0], n[1], n[2], n[3], n[4], n[5], n[6],
                                n[7], n[9], n[8]]
    for en in elements.hexa20:
        n = elements.hexa20[en]
        elements.hexa20[en] = [n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7],
                               n[8], n[11], n[13], n[9], n[16], n[18], n[19],
                               n[17],n[10], n[12], n[14], n[15]]
    
    for en in elements.penta6:
        
        n = elements.penta6[en]
        elements.penta6[en] = [n[1], n[0], n[2], n[4], n[3], n[5]]
   
    for en in elements.penta15:
        
        n = elements.penta15[en]
        elements.penta15[en] = [n[1], n[0], n[2], n[4], n[3], n[5],
                                n[6], n[7], n[9], n[12], n[13], n[14], n[10],
                                n[8], n[11]]
     
    return {
        "Nodes": nodes,
        "Seg2Elem": elements.seg2,
        "Seg3Elem": elements.seg3,
        "Tria3Elem": elements.tria3,
        "Tria6Elem": elements.tria6,
        "Quad4Elem": elements.quad4,
        "Quad8Elem": elements.quad8,
        "Tetra4Elem": elements.tetra4,
        "Tetra10Elem": elements.tetra10,
        "Hexa8Elem": elements.hexa8,
        "Hexa20Elem": elements.hexa20,
        "Penta6Elem": elements.penta6,
        "Penta15Elem": elements.penta15
    }
