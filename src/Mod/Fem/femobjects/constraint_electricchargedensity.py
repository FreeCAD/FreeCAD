# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM constraint electric charge density document object"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package constraint_electricchargedensity
#  \ingroup FEM
#  \brief constraint electric charge density object

from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class ConstraintElectricChargeDensity(base_fempythonobject.BaseFemPythonObject):

    Type = "Fem::ConstraintElectricChargeDensity"

    def __init__(self, obj):
        super().__init__(obj)

        for prop in self._get_properties():
            prop.add_to_object(obj)

    def _get_properties(self):
        prop = []

        prop.append(
            _PropHelper(
                type="App::PropertyVolumeChargeDensity",
                name="SourceChargeDensity",
                group="Electric Charge Density",
                doc="Free electric charge per unit volume at the sources",
                value="0 C/mm^3",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertySurfaceChargeDensity",
                name="InterfaceChargeDensity",
                group="Electric Charge Density",
                doc="Free electric charge per unit surface at the boundaries",
                value="0 C/mm^2",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyElectricCharge",
                name="TotalCharge",
                group="Electric Charge Density",
                doc="Total free electric charge",
                value="0 C",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="Mode",
                group="Electric Charge Density",
                doc="Switch quantity input mode",
                value=["Interface", "Source", "Total Interface", "Total Source"],
            )
        )

        return prop
