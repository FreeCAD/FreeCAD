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

__title__ = "FreeCAD FEM solver calculix ccx tools document object"
__author__ = "Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package solver_ccxtools
#  \ingroup FEM
#  \brief solver calculix ccx tools object

import FreeCAD

from . import base_fempythonobject
from femsolver.calculix.solver import add_attributes
from femsolver.calculix.solver import on_restore_of_document


class SolverCcxTools(base_fempythonobject.BaseFemPythonObject):
    """The Fem::FemSolver's Proxy python type, add solver specific properties
    """

    Type = "Fem::SolverCcxTools"

    def __init__(self, obj):
        super(SolverCcxTools, self).__init__(obj)

        ccx_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx")

        # add attributes
        # implemented in framework calculix solver module
        add_attributes(obj, ccx_prefs)

        obj.addProperty(
            "App::PropertyPath",
            "WorkingDir",
            "Fem",
            "Working directory for calculations, will only be used it is left blank in preferences"
        )
        # the working directory is not set, the solver working directory is
        # only used if the preferences working directory is left blank

    def onDocumentRestored(self, obj):

        ccx_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem/Ccx")

        # implemented in framework calculix solver module
        on_restore_of_document(obj, ccx_prefs)
