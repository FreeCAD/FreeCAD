# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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


class _FemConstraintSelfWeight:
    "The FemConstraintSelfWeight object"
    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyFloat",
            "Gravity_x",
            "Gravity",
            "set the gravity component in the x direction"
        )
        obj.addProperty(
            "App::PropertyFloat",
            "Gravity_y",
            "Gravity",
            "set the gravity component in the y direction"
        )
        obj.addProperty(
            "App::PropertyFloat",
            "Gravity_z",
            "Gravity",
            "set the gravity component in the z direction"
        )
        obj.Gravity_x = 0.0
        obj.Gravity_y = 0.0
        obj.Gravity_z = -1.0
        obj.Proxy = self
        self.Type = "Fem::ConstraintSelfWeight"

    def execute(self, obj):
        return
