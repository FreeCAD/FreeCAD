# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   © 2022 Werner Mayer <wmayer@users.sourceforge.net>                         #
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################


__title__ = "FreeCAD Points command definitions"
__author__ = "Werner Mayer"
__url__ = "https://www.freecad.org"

## @package commands
#  \ingroup Points
#  \brief FreeCAD Points command definitions

import FreeCAD
import Points


def make_points_from_geometry(geometries, distance):
    for geom in geometries:
        global_plm = geom.getGlobalPlacement()
        local_plm = geom.Placement
        plm = global_plm * local_plm.inverse()

        prop = geom.getPropertyOfGeometry()
        if prop is None:
            continue

        points_and_normals = prop.getPoints(distance)
        if len(points_and_normals[0]) == 0:
            continue

        points = geom.Document.addObject("Points::Feature", "Points")

        kernel = Points.Points()
        kernel.addPoints(points_and_normals[0])

        points.Points = kernel
        points.Placement = plm

        if len(points_and_normals[1]) > 0 and len(points_and_normals[0]) == len(
            points_and_normals[1]
        ):
            points.addProperty("Points::PropertyNormalList", "Normal", locked=True)
            points.Normal = points_and_normals[1]

        points.purgeTouched()
