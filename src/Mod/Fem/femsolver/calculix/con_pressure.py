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

__title__ = "FreeCAD FEM calculix constraint pressure"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"


def get_analysis_types():
    return ["buckling", "static", "thermomech"]


def get_sets_name():
    return "constraints_pressure_element_face_loads"


def write_pressure(f, femobj, prs_obj, ccxwriter):
    rev = -1 if prs_obj.Reversed else 1
    f.write("*DLOAD\n")
    for ref_shape in femobj["PressureFaces"]:
        # the loop is needed for compatibility reason
        # in deprecated method get_pressure_obj_faces_depreciated
        # the face ids where per ref_shape
        f.write("** " + ref_shape[0] + "\n")
        for face, fno in ref_shape[1]:
            if fno > 0:  # solid mesh face
                f.write("{},P{},{}\n".format(face, fno, rev * prs_obj.Pressure))
            # on shell mesh face: fno == 0
            # normal of element face == face normal
            elif fno == 0:
                f.write("{},P,{}\n".format(face, rev * prs_obj.Pressure))
            # on shell mesh face: fno == -1
            # normal of element face opposite direction face normal
            elif fno == -1:
                f.write("{},P,{}\n".format(face, -1 * rev * prs_obj.Pressure))
