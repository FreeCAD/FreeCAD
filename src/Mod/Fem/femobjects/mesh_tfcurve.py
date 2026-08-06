# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Stefan Tröger <stefantroeger@gmx.net>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

__title__ = "FreeCAD FEM transfinite curve document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package mesh_tfcurve
#  \ingroup FEM
#  \brief  object for structured meshing of curves

from . import base_femmeshelement
from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class MeshTransfiniteCurve(base_femmeshelement.BaseFemMeshElement):
    """
    The FemMeshTransfiniteCurve object
    """

    Type = "Fem::MeshTransfiniteCurve"

    def _get_properties(self):

        props = [
            _PropHelper(
                type="App::PropertyInteger",
                name="Nodes",
                group="Transfinite",
                doc="The number of nodes distributed over each edge",
                value=10,
            ),
            _PropHelper(
                type="App::PropertyFloat",
                name="Coefficient",
                group="Transfinite",
                doc="The coefficient used by distributon algorithms (except constant)",
                value=1.2,
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="Distribution",
                group="Transfinite",
                doc="The algorithm used to distribute the nodes on the edge",
                value=["Constant", "Bump", "Progression"],
            ),
            _PropHelper(
                type="App::PropertyBool",
                name="Invert",
                group="Transfinite",
                doc="Inverts the direction of of the non-constant distributions",
                value=False,
            ),
        ]

        return super()._get_properties() + props
