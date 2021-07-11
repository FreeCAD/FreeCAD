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

__title__ = "FreeCAD FEM calculix constraint centrif"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecadweb.org"


import math
import six

import FreeCAD


def get_analysis_types():
    return ["buckling", "static", "thermomech"]


def get_sets_name():
    return "constraints_centrif_element_sets"


def get_constraint_title():
    return "Centrif Constraints"


def write_meshdata_constraint(f, femobj, centrif_obj, ccxwriter):
    f.write("*ELSET,ELSET={}\n".format(centrif_obj.Name))
    # use six to be sure to be Python 2.7 and 3.x compatible
    if isinstance(femobj["FEMElements"], six.string_types):
        f.write("{}\n".format(femobj["FEMElements"]))
    else:
        for e in femobj["FEMElements"]:
            f.write("{},\n".format(e))


def write_constraint(f, femobj, centrif_obj, ccxwriter):

    # get some data from the centrif_obj
    refobj = centrif_obj.RotationAxis[0][0]
    subobj = centrif_obj.RotationAxis[0][1][0]
    axis = refobj.Shape.getElement(subobj)

    if axis.Curve.TypeId == "Part::GeomLine":
        axiscopy = axis.copy()  # apply global placement to copy
        axiscopy.Placement = refobj.getGlobalPlacement()
        direction = axiscopy.Curve.Direction
        location = axiscopy.Curve.Location
    else:  # no line found, set default
        # TODO: No test at all in the writer
        # they should all be before in prechecks
        location = FreeCAD.Vector(0., 0., 0.)
        direction = FreeCAD.Vector(0., 0., 1.)

    # write to file
    f.write("*DLOAD\n")
    # Why {:.13G} ...
    # ccx uses F20.0 FORTRAN input fields, see in dload.f in ccx's source
    # https://forum.freecadweb.org/viewtopic.php?f=18&t=22759&#p176578
    # example "{:.13G}".format(math.sqrt(2.)*-1e100) and count chars
    f.write(
        "{},CENTRIF,{:.13G},{:.13G},{:.13G},{:.13G},{:.13G},{:.13G},{:.13G}\n"
        .format(
            centrif_obj.Name,
            (2. * math.pi * float(centrif_obj.RotationFrequency.getValueAs("1/s"))) ** 2,
            location.x,
            location.y,
            location.z,
            direction.x,
            direction.y,
            direction.z
        )
    )
    f.write("\n")
