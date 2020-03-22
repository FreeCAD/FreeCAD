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

__title__ = "FreeCAD FEM solver calculix document object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package FemSolverCalculix
#  \ingroup FEM
#  \brief FreeCAD FEM _FemSolverCalculix

import FreeCAD

from . import FemConstraint
from femsolver.calculix.solver import add_attributes


class _FemSolverCalculix(FemConstraint.Proxy):
    """The Fem::FemSolver's Proxy python type, add solver specific properties
    """

    Type = "Fem::FemSolverCalculixCcxTools"

    def __init__(self, obj):
        super(_FemSolverCalculix, self).__init__(obj)

        ccx_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx")

        obj.addProperty(
            "App::PropertyPath",
            "WorkingDir",
            "Fem",
            "Working directory for calculations, will only be used it is left blank in preferences"
        )
        # the working directory is not set, the solver working directory is
        # only used if the preferences working directory is left blank

        # add attributes from framework calculix solver
        add_attributes(obj, ccx_prefs)
