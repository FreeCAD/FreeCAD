# ***************************************************************************
# *   Copyright (c) 2020 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FEM geometry tools"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import FreeCAD

from . import femutils


# ************************************************************************************************
def get_vertexes_by_element(
    aShape,
    anElement
):
    # we're going to extend the method find_element_in_shape and return the vertexes
    # import Part
    ele_vertexes = []
    ele_st = anElement.ShapeType
    if ele_st == "Solid" or ele_st == "CompSolid":
        for index, solid in enumerate(aShape.Solids):
            if is_same_geometry(solid, anElement):
                for vele in aShape.Solids[index].Vertexes:
                    for i, v in enumerate(aShape.Vertexes):
                        if vele.isSame(v):  # use isSame, because orientation could be different
                            ele_vertexes.append(i)
                # FreeCAD.Console.PrintMessage("  " + str(sorted(ele_vertexes)), "\n")
                return ele_vertexes
        FreeCAD.Console.PrintError(
            "Error, Solid " + str(anElement) + " not found in: " + str(aShape) + "\n"
        )
    elif ele_st == "Face" or ele_st == "Shell":
        for index, face in enumerate(aShape.Faces):
            if is_same_geometry(face, anElement):
                for vele in aShape.Faces[index].Vertexes:
                    for i, v in enumerate(aShape.Vertexes):
                        if vele.isSame(v):  # use isSame, because orientation could be different
                            ele_vertexes.append(i)
                # FreeCAD.Console.PrintMessage("  " + str(sorted(ele_vertexes)) + "\n")
                return ele_vertexes
    elif ele_st == "Edge" or ele_st == "Wire":
        for index, edge in enumerate(aShape.Edges):
            if is_same_geometry(edge, anElement):
                for vele in aShape.Edges[index].Vertexes:
                    for i, v in enumerate(aShape.Vertexes):
                        if vele.isSame(v):  # use isSame, because orientation could be different
                            ele_vertexes.append(i)
                # FreeCAD.Console.PrintMessage("  " + str(sorted(ele_vertexes)) + "\n")
                return ele_vertexes
    elif ele_st == "Vertex":
        for index, vertex in enumerate(aShape.Vertexes):
            if is_same_geometry(vertex, anElement):
                ele_vertexes.append(index)
                # FreeCAD.Console.PrintMessage("  " + str(sorted(ele_vertexes)) + "\n")
                return ele_vertexes
    elif ele_st == "Compound":
        FreeCAD.Console.PrintError("Compound is not supported.\n")


# ************************************************************************************************
def get_rectangular_coords(
    obj
):
    from math import cos, sin, radians
    A = [1, 0, 0]
    B = [0, 1, 0]
    a_x = A[0]
    a_y = A[1]
    a_z = A[2]
    b_x = B[0]
    b_y = B[1]
    b_z = B[2]
    x_rot = radians(obj.X_rot)
    y_rot = radians(obj.Y_rot)
    z_rot = radians(obj.Z_rot)
    if obj.X_rot != 0:
        a_y = A[1] * cos(x_rot) + A[2] * sin(x_rot)
        a_z = A[2] * cos(x_rot) - A[1] * sin(x_rot)
        b_y = B[1] * cos(x_rot) + B[2] * sin(x_rot)
        b_z = B[2] * cos(x_rot) - B[1] * sin(x_rot)
    if obj.Y_rot != 0:
        a_x = A[0] * cos(y_rot) - A[2] * sin(y_rot)
        a_z = A[2] * cos(y_rot) + A[0] * sin(y_rot)
        b_x = B[0] * cos(y_rot) - B[2] * sin(y_rot)
        b_z = B[2] * cos(y_rot) + B[0] * sin(z_rot)
    if obj.Z_rot != 0:
        a_x = A[0] * cos(z_rot) + A[1] * sin(z_rot)
        a_y = A[1] * cos(z_rot) - A[0] * sin(z_rot)
        b_x = B[0] * cos(z_rot) + B[1] * sin(z_rot)
        b_y = B[1] * cos(z_rot) - B[0] * sin(z_rot)
    A = [a_x, a_y, a_z]
    B = [b_x, b_y, b_z]
    A_coords = str(round(A[0], 4)) + "," + str(round(A[1], 4)) + "," + str(round(A[2], 4))
    B_coords = str(round(B[0], 4)) + "," + str(round(B[1], 4)) + "," + str(round(B[2], 4))
    coords = A_coords + "," + B_coords
    return coords


# ************************************************************************************************
def get_cylindrical_coords(
    obj
):
    vec = obj.Axis
    base = obj.BasePoint
    Ax = base[0] + 10 * vec[0]
    Ay = base[1] + 10 * vec[1]
    Az = base[2] + 10 * vec[2]
    Bx = base[0] - 10 * vec[0]
    By = base[1] - 10 * vec[1]
    Bz = base[2] - 10 * vec[2]
    A = [Ax, Ay, Az]
    B = [Bx, By, Bz]
    A_coords = str(A[0]) + "," + str(A[1]) + "," + str(A[2])
    B_coords = str(B[0]) + "," + str(B[1]) + "," + str(B[2])
    coords = A_coords + "," + B_coords
    return coords


