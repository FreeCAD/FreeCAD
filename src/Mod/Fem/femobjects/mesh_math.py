# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net               *
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

__title__ = "FreeCAD FEM mesh math document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package mesh_math
#  \ingroup FEM
#  \brief  object defining mesh size by evaluating mathematical formula

from . import base_femmeshelement
from . import base_fempythonobject
_PropHelper = base_fempythonobject._PropHelper

class MeshMath(base_femmeshelement.BaseFemMeshElement):
    """
    The FemMeshMath object
    """

    Type = "Fem::MeshMath"

    def _get_properties(self):

        props = [
            _PropHelper(
                type="App::PropertyLinkList",
                name="Refinements",
                group="Math",
                doc="Refinements usable in the mathematical equation",
                value=None,
            ),
            _PropHelper(
                type="App::PropertyString",
                name="Equation",
                group="Math",
                doc="The equation to calculate for the mesh size",
                value="",
            ),
        ]

        # do not user super() properties, as we do not need references
        return props

