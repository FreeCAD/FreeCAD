# ***************************************************************************
# *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
            points.addProperty("Points::PropertyNormalList", "Normal")
            points.Normal = points_and_normals[1]

        points.purgeTouched()
