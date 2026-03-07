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
