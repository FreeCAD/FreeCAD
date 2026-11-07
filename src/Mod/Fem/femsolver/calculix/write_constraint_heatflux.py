# SPDX-License-Identifier: LGPL-2.1-or-later

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


from femtools import loadtable

from .calculixutils import define_masks

# Node count of a volume element and the name of its face msaks in define_masks
_FACE_MASKS = {4: "tetra4", 10: "tetra10", 8: "hexa8", 20: "hexa20", 6: "penta6", 15: "penta15"}


def face_centre(getter, masks, element, face_number):
    """Mean of the nodes of a CalculiX element face, a shell element is its own face"""
    nodes = getter.femelement_table[element]
    if getter.femmesh.VolumeCount:
        face_masks = masks[_FACE_MASKS[len(nodes)]]
        mask = next(key for key, value in face_masks.items() if value == face_number)
        # Bit i of the mask is node i of the element, see get_femnodes_ele_table
        nodes = [node for i, node in enumerate(nodes) if mask >> i & 1]
    points = [getter.femnodes_mesh[node] for node in nodes]
    return [sum(p[k] for p in points) / len(points) for k in range(3)]


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
        heatflux_facesubtype = ""
        heatflux_values = "{:.13G},{:.13G}".format(
            heatflux_obj.AmbientTemp.getValueAs("K").Value,
            heatflux_obj.FilmCoef.getValueAs("t/s^3/K").Value,
        )

    elif heatflux_obj.ConstraintType == "Radiation":
        heatflux_facetype = "R"
        amb_temp = heatflux_obj.AmbientTemp.getValueAs("K").Value
        if heatflux_obj.CavityRadiation:
            heatflux_key_word = f"RADIATE, CAVITY={heatflux_obj.CavityName}"
            heatflux_facesubtype = "CR"
            # set negative temperature for closed cavity
            if heatflux_obj.ClosedCavity:
                amb_temp = -1.0
        else:
            heatflux_key_word = "RADIATE"
            heatflux_facesubtype = ""
        heatflux_values = "{:.13G},{:.13G}".format(amb_temp, heatflux_obj.Emissivity)

    elif heatflux_obj.ConstraintType == "Flux":
        heatflux_key_word = "DFLUX"
        heatflux_facetype = "S"
        heatflux_facesubtype = ""
        heatflux_values = "{:.13G}".format(
            heatflux_obj.DistributedHeatFlux.getValueAs("t/s^3").Value
        )

    else:
        return

    # A flux given at points is interpolated at the centre of each element face
    face_values = {}
    if heatflux_obj.ConstraintType == "Flux" and heatflux_obj.FluxDistribution == "Table":
        points = loadtable.points_from_object(heatflux_obj, "t/s^3")
        faces = [pair for _, surf, _ in femobj["HeatFluxFaces"] for pair in surf]
        getter = ccxwriter.meshdatagetter
        masks = define_masks(ccxwriter.solver_obj)
        centres = [face_centre(getter, masks, face, fno) for face, fno in faces]
        # The face sets hlod [element, face] lists, a tuple is needed as key
        for (face, fno), value in zip(faces, loadtable.interpolate(points, centres)):
            face_values[(face, fno)] = f"{value:.13G}"

    if heatflux_obj.EnableAmplitude:
        heatflux_amplitude = f", AMPLITUDE={heatflux_obj.Name}"
    else:
        heatflux_amplitude = ""

    f.write(f"*{heatflux_key_word}{heatflux_amplitude}\n")
    for feat, surf, is_sub_el in femobj["HeatFluxFaces"]:
        f.write("** {0.Name}.{1[0]}\n".format(*feat))
        for face, fno in surf:
            values = face_values.get((face, fno), heatflux_values)
            f.write(f"{face},{heatflux_facetype}{fno}{heatflux_facesubtype},{values}\n")
