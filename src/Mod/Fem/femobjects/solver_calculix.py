# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2017 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM solver CalculiX document object"
__author__ = "Bernd Hahnebach, Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package solver_calculix
#  \ingroup FEM
#  \brief solver CalculiX object

from . import base_fempythonobject
from femsolver.calculix.solver import _BaseSolverCalculix


class SolverCalculiX(base_fempythonobject.BaseFemPythonObject, _BaseSolverCalculix):

    Type = "Fem::SolverCalculiX"

    def __init__(self, obj):
        super().__init__(obj)
        self.add_attributes(obj)

    def onDocumentRestored(self, obj):
        self.on_restore_of_document(obj)
