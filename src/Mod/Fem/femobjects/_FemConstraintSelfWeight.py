# ***************************************************************************
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM constraint self weight document object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package FemConstraintSelfWeight
#  \ingroup FEM
#  \brief FreeCAD FEM constraint self weight object

from . import FemConstraint


class _FemConstraintSelfWeight(FemConstraint.Proxy):
    """
    The FemConstraintSelfWeight object"
    """

    Type = "Fem::ConstraintSelfWeight"

    def __init__(self, obj):
        super(_FemConstraintSelfWeight, self).__init__(obj)

        obj.addProperty(
            "App::PropertyFloat",
            "Gravity_x",
            "Gravity",
            "Gravity direction: set the x-component of the normalized gravity vector"
        )

        obj.addProperty(
            "App::PropertyFloat",
            "Gravity_y",
            "Gravity",
            "Gravity direction: set the y-component of the normalized gravity vector"
        )

        obj.addProperty(
            "App::PropertyFloat",
            "Gravity_z",
            "Gravity",
            "Gravity direction: set the z-component of the normalized gravity vector"
        )

        obj.Gravity_x = 0.0
        obj.Gravity_y = 0.0
        obj.Gravity_z = -1.0
