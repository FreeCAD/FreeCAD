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

__title__ = "FreeCAD FEM transfinite surface document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package mesh_tfsurface
#  \ingroup FEM
#  \brief  object defining mesh size and strucutre by transfinite surface

from . import base_femmeshelement
from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class MeshTransfiniteSurface(base_femmeshelement.BaseFemMeshElement):
    """
    The FemMeshTransfiniteSurface object
    """

    Type = "Fem::MeshTransfiniteSurface"

    def _get_properties(self):

        props = [
            _PropHelper(
                type="App::PropertyEnumeration",
                name="TriangleOrientation",
                group="Transfinite",
                doc="Define how the triangles are oriented within the transfinite mesh (if not recombined)",
                value=["Left", "Right", "AlternateRight", "AlternateLeft"],
            ),
            _PropHelper(
                type="App::PropertyBool",
                name="Recombine",
                group="Transfinite",
                doc="Define if the triangles on the surface shall be recombined into quads",
                value=False,
            ),
            _PropHelper(
                type="App::PropertyBool",
                name="UseAutomation",
                group="Automation",
                doc="Enables the automatic application of transfinite curve definitions on all yet undefined edges",
                value=False,
            ),
            _PropHelper(
                type="App::PropertyInteger",
                name="Nodes",
                group="Automation",
                doc="The number of nodes distributed over each edge that is automatically converted to a transfinite curve",
                value=10,
            ),
            _PropHelper(
                type="App::PropertyFloat",
                name="Coefficient",
                group="Automation",
                doc="The coefficient used by distributon algorithms (except constant) for automatically converted transfinite curves",
                value=1.2,
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="Distribution",
                group="Automation",
                doc="The algorithm used to distribute the nodes on the edge for automatically converted transfinite curves",
                value=["Constant", "Bump", "Progression"],
            ),
            _PropHelper(
                type="App::PropertyBool",
                name="Invert",
                group="Automation",
                doc="Inverts the direction of of the non-constant distributions",
                value=False,
            ),
        ]

        return super()._get_properties() + props
