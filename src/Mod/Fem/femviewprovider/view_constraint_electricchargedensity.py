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

__title__ = "FreeCAD FEM constraint electric charge view provider"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package view_constraint_electricchargedensity
#  \ingroup FEM
#  \brief view provider for the constraint electric charge density object

from femtaskpanels import task_constraint_electricchargedensity
from . import view_base_femconstraint


class VPConstraintElectricChargeDensity(view_base_femconstraint.VPBaseFemConstraint):

    def __init__(self, vobj):
        super().__init__(vobj)
        mat = vobj.ShapeAppearance[0]
        mat.DiffuseColor = (1.0, 0.0, 0.2, 0.0)
        vobj.ShapeAppearance = mat

    def setEdit(self, vobj, mode=0):
        return view_base_femconstraint.VPBaseFemConstraint.setEdit(
            self, vobj, mode, task_constraint_electricchargedensity._TaskPanel
        )

    def attach(self, vobj):
        super().attach(vobj)
        vobj.loadSymbol(self.resource_symbol_dir + "ConstraintElectricChargeDensity.iv")
