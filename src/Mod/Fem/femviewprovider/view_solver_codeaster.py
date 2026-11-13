# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Tim Swait <t.swait@sheffield.ac.uk>                *
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

__title__ = "FreeCAD FEM solver Code Aster view provider"
__author__ = "Tim Swait"
__url__ = "https://www.freecad.org"

## @package view_codeaster
#  \ingroup FEM
#  \brief solver Code Aster view provider

import FreeCADGui

from femtaskpanels import task_solver_codeaster
from femviewprovider import view_base_femobject


class VPSolverCodeAster(view_base_femobject.VPBaseFemObject):

    def __init__(self, vobj):
        super().__init__(vobj)

    def getIcon(self):
        return ":/icons/FEM_SolverCodeAster.svg"

    def setEdit(self, vobj, mode=0):
        task = task_solver_codeaster._TaskPanel(vobj.Object)
        FreeCADGui.Control.showDialog(task)

        return True
