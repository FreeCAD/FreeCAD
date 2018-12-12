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

__title__ = "FreeCAD .inp file reader"
__author__ = "Frantisek Loeffelmann, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"
__date__ = "04/08/2016"

## @package importInpMesh
#  \ingroup FEM
#  \brief FreeCAD INP file reader for FEM workbench

import FreeCAD
import os


# ********* generic FreeCAD import and export methods *********
if open.__module__ == '__builtin__':
    # because we'll redefine open below (Python2)
    pyopen = open
elif open.__module__ == 'io':
    # because we'll redefine open below (Python3)
    pyopen = open


def open(filename):
    "called when freecad opens a file"
    docname = os.path.splitext(os.path.basename(filename))[0]
    insert(filename, docname)


def insert(filename, docname):
    "called when freecad wants to import a file"
    try:
        doc = FreeCAD.getDocument(docname)
    except NameError:
        doc = FreeCAD.newDocument(docname)
    FreeCAD.ActiveDocument = doc
    import_inp(filename)


# ********* module specific methods *********
def read(filename):
    '''read a FemMesh from a inp mesh file and return the FemMesh
    '''
    # no document object is created, just the FemMesh is returned
    mesh_data = read_inp(filename)
    from . import importToolsFem
    return importToolsFem.make_femmesh(mesh_data)


def import_inp(filename):
    '''read a FEM mesh from a Z88 mesh file and insert a FreeCAD FEM Mesh object in the ActiveDocument
    '''
    femmesh = read(filename)
    mesh_name = os.path.splitext(os.path.basename(filename))[0]
    if femmesh:
        mesh_object = FreeCAD.ActiveDocument.addObject('Fem::FemMeshObject', mesh_name)
        mesh_object.FemMesh = femmesh


def read_inp(file_name):
    '''read .inp file '''
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
    error_seg3 = False  # to print "not supported"
    nodes = {}
    model_definition = True

    f = pyopen(file_name, "r")
    line = "\n"
    include = ""
    f_include = None
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
                include_path = os.path.normpath(include)
                if os.path.isfile(include_path) is True:
                    f_include = pyopen(include_path, "r")
                else:
                    path_start = os.path.split(file_name)[0]
                    include_full_path = os.path.join(path_start, include_path)
                    f_include = pyopen(include_full_path, "r")
                continue
            read_node = False
            elm_category = []
            elm_2nd_line = False

        # reading nodes
        if (line[:5].upper() == "*NODE") and (model_definition is True):
            read_node = True
        elif read_node is True:
            line_list = line.split(',')
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
                elm_category = elements.tria3
                number_of_nodes = 3
            elif elm_type in ["S6", "CPS6", "CPE6", "CAX6"]:
                elm_category = elements.tria6
                number_of_nodes = 6
            elif elm_type in ["S4", "S4R", "CPS4", "CPS4R", "CPE4", "CPE4R",
                              "CAX4", "CAX4R"]:
                elm_category = elements.quad4
                number_of_nodes = 4
            elif elm_type in ["S8", "S8R", "CPS8", "CPS8R", "CPE8", "CPE8R",
                              "CAX8", "CAX8R"]:
                elm_category = elements.quad8
                number_of_nodes = 8
            elif elm_type == "C3D4":
                elm_category = elements.tetra4
                number_of_nodes = 4
            elif elm_type == "C3D10":
                elm_category = elements.tetra10
                number_of_nodes = 10
            elif elm_type in ["C3D8", "C3D8R", "C3D8I"]:
                elm_category = elements.hexa8
                number_of_nodes = 8
            elif elm_type in ["C3D20", "C3D20R", "C3D20RI"]:
                elm_category = elements.hexa20
                number_of_nodes = 20
            elif elm_type == "C3D6":
                elm_category = elements.penta6
                number_of_nodes = 6
            elif elm_type == "C3D15":
                elm_category = elements.penta15
                number_of_nodes = 15
            elif elm_type in ["B31", "B31R", "T3D2"]:
                elm_category = elements.seg2
                number_of_nodes = 2
            elif elm_type in ["B32", "B32R", "T3D3"]:
                elm_category = elements.seg3
                number_of_nodes = 3
                error_seg3 = True  # to print "not supported"

        elif elm_category != []:
            line_list = line.split(',')
            if elm_2nd_line is False:
                number = int(line_list[0])
                elm_category[number] = []
                pos = 1
            else:
                pos = 0
                elm_2nd_line = False
            for en in range(pos, pos + number_of_nodes - len(elm_category[number])):
                try:
                    enode = int(line_list[en])
                    elm_category[number].append(enode)
                except:
                    elm_2nd_line = True
                    break

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
    for en in elements.seg3:
        n = elements.seg3[en]
        elements.seg3[en] = [n[0], n[2], n[1]]

    return {
        'Nodes': nodes,
        'Seg2Elem': elements.seg2,
        'Seg3Elem': elements.seg3,
        'Tria3Elem': elements.tria3,
        'Tria6Elem': elements.tria6,
        'Quad4Elem': elements.quad4,
        'Quad8Elem': elements.quad8,
        'Tetra4Elem': elements.tetra4,
        'Tetra10Elem': elements.tetra10,
        'Hexa8Elem': elements.hexa8,
        'Hexa20Elem': elements.hexa20,
        'Penta6Elem': elements.penta6,
        'Penta15Elem': elements.penta15
    }
