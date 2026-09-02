# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

__title__ = "FreeCAD FEM solver Z88 view provider"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package view_z88
#  \ingroup FEM
#  \brief solver Z88 view provider

import FreeCADGui

from femtaskpanels import task_solver_z88
from femviewprovider import view_base_femobject


class VPSolverZ88(view_base_femobject.VPBaseFemObject):

    def __init__(self, vobj):
        super().__init__(vobj)

    def getIcon(self):
        return ":/icons/FEM_SolverZ88.svg"

    def setEdit(self, vobj, mode=0):
        task = task_solver_z88._TaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(task)

        return True
