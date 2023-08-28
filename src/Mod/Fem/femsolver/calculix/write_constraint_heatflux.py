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

__title__ = "FreeCAD FEM calculix constraint heatflux"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"


def get_analysis_types():
    return ["thermomech"]


def get_sets_name():
    return "constraints_heatflux_element_face_heatflux"


def get_before_write_meshdata_constraint():
    return ""


def get_after_write_meshdata_constraint():
    return ""


def write_meshdata_constraint(f, femobj, heatflux_obj, ccxwriter):

    # floats read from ccx should use {:.13G}, see comment in writer module

    if heatflux_obj.ConstraintType == "Convection":
        heatflux_key_word = "FILM"
        heatflux_facetype = "F"
        # SvdW: add factor to force heatflux to units system of t/mm/s/K
        heatflux_values = "{:.13G},{:.13G}".format(
            heatflux_obj.AmbientTemp,
            heatflux_obj.FilmCoef * 0.001
        )
    elif heatflux_obj.ConstraintType == "DFlux":
        heatflux_key_word = "DFLUX"
        heatflux_facetype = "S"
        heatflux_values = "{:.13G}".format(heatflux_obj.DFlux * 0.001)

    f.write("*{}\n".format(heatflux_key_word))
    for ref_shape in femobj["HeatFluxFaceTable"]:
        elem_string = ref_shape[0]
        face_table = ref_shape[1]
        f.write("** Heat flux on face {}\n".format(elem_string))
        for i in face_table:
            # OvG: Only write out the VolumeIDs linked to a particular face
            f.write("{},{}{},{}\n".format(
                i[0],
                heatflux_facetype,
                i[1],
                heatflux_values
            ))
