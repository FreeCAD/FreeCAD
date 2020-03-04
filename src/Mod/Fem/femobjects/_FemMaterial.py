# ***************************************************************************
# *   Copyright (c) 2013 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM material document object"
__author__ = "Juergen Riegel, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package FemMaterial
#  \ingroup FEM
#  \brief FEM material

from . import FemConstraint


class _FemMaterial(FemConstraint.Proxy):
    """
    The FEM Material object
    """

    Type = "Fem::Material"

    def __init__(self, obj):
        super(_FemMaterial, self).__init__(obj)

        obj.addProperty(
            "App::PropertyLinkSubList",
            "References",
            "Material",
            "List of material shapes"
        )

        obj.addProperty(
            "App::PropertyEnumeration",
            "Category",
            "Material",
            "Material type: fluid or solid"
        )

        obj.Category = ["Solid", "Fluid"]  # used in TaskPanel
