# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Frantisek Loeffelmann <LoffF@email.cz>           *
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

import FemMeshTools
import FreeCAD
import os
import string

__title__ = "FreeCAD .inp file reader"
__author__ = "Frantisek Loeffelmann "
__url__ = "http://www.freecadweb.org"
__date__ = "04/08/2016"


if open.__module__ == '__builtin__':
    pyopen = open  # because we'll redefine open below 


def read_inp(file_name):
    "read .inp file, currently only the mesh"

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
        # seg3 = {}
    error_seg3 = False  # to print "not supported"
    nodes = {}
    model_definition = True

    def read_elm_nodes(elm_category, number_of_nodes):

        line_list = string.split(line, ',')
        number = int(line_list[0])
        elm_category[number] = []
        for en in range(1, number_of_nodes + 1):
            enode = int(line_list[en])
            elm_category[number].append(enode)

    f = pyopen(file_name, "r")
    line = "\n"
    include = ""
    while line != "":
        if include:
            line = f_include.readline()
            if line == "":
                f_include.close()
                include = ""
                line = f.readline()
        else:
            line = f.readline()
        if line.strip() == '':
            continue
        elif line[0] == '*':  # start/end of a reading set
            if line[0:2] == '**':  # comments
                continue
            if line[:8].upper() == "*INCLUDE":
                start = 1 + line.index("=")
                include = line[start:].strip().strip('"')
                f_include = pyopen(include, "r")
                continue

            read_node = False
            read_tria3 = False
            read_tria6 = False
            read_quad4 = False
            read_quad8 = False
            read_tetra4 = False
            read_tetra10 = False
            read_hexa8 = False
            read_hexa20_line1 = False
            read_hexa20_line2 = False
            read_penta6 = False
            read_penta15 = False
            read_seg2 = False
            read_seg3 = False

        # reading nodes
        if (line[:5].upper() == "*NODE") and (model_definition is True):
            read_node = True
        elif read_node is True:
            line_list = string.split(line, ',')
            number = int(line_list[0])
            x = float(line_list[1])
            y = float(line_list[2])
            z = float(line_list[3])
            nodes[number] = [x, y, z]

        # reading elements
        elif line[:8].upper() == "*ELEMENT":
            line_list = line[8:].upper().split(',')
            for line_part in line_list:
                if line_part.lstrip()[:4] == "TYPE":
                    elm_type = line_part.split('=')[1].strip()

            if elm_type in ["S3", "CPS3", "CPE3", "CAX3"]:
                read_tria3 = True
            elif elm_type in ["S6", "CPS6", "CPE6", "CAX6"]:
                read_tria6 = True
            elif elm_type in ["S4", "S4R", "CPS4", "CPS4R", "CPE4", "CPE4R",
                              "CAX4", "CAX4R"]:
                read_quad4 = True
            elif elm_type in ["S8", "S8R", "CPS8", "CPS8R", "CPE8", "CPE8R",
                              "CAX8", "CAX8R"]:
                read_quad8 = True
            elif elm_type == "C3D4":
                read_tetra4 = True
            elif elm_type == "C3D10":
                read_tetra10 = True
            elif elm_type in ["C3D8", "C3D8R", "C3D8I"]:
                read_hexa8 = True
            elif elm_type in ["C3D20", "C3D20R", "C3D20RI"]:
                read_hexa20_line1 = True
            elif elm_type == "C3D6":
                read_penta6 = True
            elif elm_type == "C3D15":
                read_penta15 = True
            elif elm_type in ["B31", "B31R", "T3D2"]:
                read_seg2 = True
            elif elm_type in ["B32", "B32R", "T3D3"]:
                read_seg3 = True

        elif read_tria3 is True:
            read_elm_nodes(elements.tria3, 3)
        elif read_tria6 is True:
            read_elm_nodes(elements.tria6, 6)
        elif read_quad4 is True:
            read_elm_nodes(elements.quad4, 4)
        elif read_quad8 is True:
            read_elm_nodes(elements.quad8, 8)
        elif read_tetra4 is True:
            read_elm_nodes(elements.tetra4, 4)
        elif read_tetra10 is True:
            read_elm_nodes(elements.tetra10, 10)
        elif read_hexa8 is True:
            read_elm_nodes(elements.hexa8, 8)
        elif read_hexa20_line2 is True:
            line_list = string.split(line, ',')
            if line_list[-1].strip() == "":
                del line_list[-1]
            for en in range(0, len(line_list)):
                enode = int(line_list[en])
                elements.hexa20[number].append(enode)
            read_hexa20_line2 = False
        elif read_hexa20_line1 is True:
            line_list = string.split(line, ',')
            if line_list[-1].strip() == "":
                del line_list[-1]
            number = int(line_list[0])
            elements.hexa20[number] = []
            for en in range(1, len(line_list)):
                enode = int(line_list[en])
                elements.hexa20[number].append(enode)
            read_hexa20_line2 = True
        elif read_penta6 is True:
            read_elm_nodes(elements.penta6, 6)
        elif read_penta15 is True:
            read_elm_nodes(elements.penta15, 15)
        elif read_seg2 is True:
            read_elm_nodes(elements.seg2, 2)
        elif read_seg3 is True:
            error_seg3 = True  # to print "not supported"
            # read_elm_nodes(elements.seg3, 3)
        elif line[:5].upper() == "*STEP":
            model_definition = False
    if error_seg3 is True:  # to print "not supported"
        FreeCAD.Console.PrintError("Error: seg3 (3-node beam element type) not supported, yet.\n")
    f.close()

    # switch from the CalculiX node numbering to the FreeCAD node numbering
    # numbering do not change: tria3, tria6, quad4, quad8, seg2
    for en in elements.tetra4:
        n = elements.tetra4[en]
        elements.tetra4[en] = [n[1], n[0], n[2], n[3]]
    for en in elements.tetra10:
        n = elements.tetra10[en]
        elements.tetra10[en] = [n[1], n[0], n[2], n[3], n[4], n[6], n[5],
                                n[8], n[7], n[9]]
    for en in elements.hexa8:
        n = elements.hexa8[en]
        elements.hexa8[en] = [n[5], n[6], n[7], n[4], n[1], n[2], n[3], n[0]]
    for en in elements.hexa20:
        n = elements.hexa20[en]
        elements.hexa20[en] = [n[5], n[6], n[7], n[4], n[1], n[2], n[3], n[0],
                               n[13], n[14], n[15], n[12], n[9], n[10], n[11],
                               n[8], n[17], n[18], n[19], n[16]]
    for en in elements.penta6:
        n = elements.penta6[en]
        elements.penta6[en] = [n[4], n[5], n[3], n[1], n[2], n[0]]
    for en in elements.penta15:
        n = elements.penta15[en]
        elements.penta15[en] = [n[4], n[5], n[3], n[1], n[2], n[0],
                                n[10], n[11], n[9], n[7], n[8], n[6], n[13],
                                n[14], n[12]]
    # for en in elements.seg3:
        # n = elements.seg3[en]
        # elements.seg3[en] = [n[0], n[1], n[2]]

    return {'Nodes': nodes,
            'Hexa8Elem': elements.hexa8, 'Penta6Elem': elements.penta6, 'Tetra4Elem': elements.tetra4,
            'Tetra10Elem': elements.tetra10, 'Penta15Elem': elements.penta15, 'Hexa20Elem': elements.hexa20,
            'Tria3Elem': elements.tria3, 'Tria6Elem': elements.tria6, 'Quad4Elem': elements.quad4,
            'Quad8Elem': elements.quad8, 'Seg2Elem': elements.seg2}  # , 'Seg3Elem': elements.seg3}


def import_inp(filename):
    "create imported objects in FreeCAD, currently only FemMesh"

    m = read_inp(filename)
    mesh = FemMeshTools.make_femmesh(m)
    mesh_name = os.path.splitext(os.path.basename(filename))[0] 
    mesh_object = FreeCAD.ActiveDocument.addObject('Fem::FemMeshObject', mesh_name)
    mesh_object.FemMesh = mesh


def insert(filename, docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    import_inp(filename)


def open(filename):
    "called when freecad opens a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    insert(filename, docname)
