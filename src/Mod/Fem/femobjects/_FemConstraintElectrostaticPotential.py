# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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

__title__ = "FreeCAD FEM constraint electrostatic potential document object"
__author__ = "Markus Hovorka, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package FemConstraintElectrostaticPotential
#  \ingroup FEM
#  \brief FreeCAD FEM constraint electrostatic potential object

from . import FemConstraint


class Proxy(FemConstraint.Proxy):

    Type = "Fem::ConstraintElectrostaticPotential"

    def __init__(self, obj):
        super(Proxy, self).__init__(obj)
        obj.addProperty(
            "App::PropertyFloat", "Potential",
            "Parameter", "Potential"),
        obj.addProperty(
            "App::PropertyBool", "PotentialEnabled",
            "Parameter", "Potential Enabled"),
        obj.addProperty(
            "App::PropertyBool", "PotentialConstant",
            "Parameter", "Potential Constant")
