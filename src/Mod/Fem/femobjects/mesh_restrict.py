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

__title__ = "FreeCAD FEM restrict document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package mesh_distance
#  \ingroup FEM
#  \brief  object defining mesh size by restricting other refinements to certain elements

from . import base_femmeshelement
from . import base_fempythonobject
_PropHelper = base_fempythonobject._PropHelper

class MeshRestrict(base_femmeshelement.BaseFemMeshElement):
    """
    The FemMeshRestrict object
    """

    Type = "Fem::MeshRestrict"

    def _get_properties(self):

        props = [
            _PropHelper(
                type="App::PropertyLink",
                name="Refinement",
                group="Restrict",
                doc="Which refinement is restricted in its application",
                value=None,
            ),
            _PropHelper(
                type="App::PropertyBool",
                name="IncludeBoundary",
                group="Restrict",
                doc="Include the boundary of the selected entities",
                value=True,
            ),
            #_PropHelper(
            #    type="App::PropertyBool",
            #    name="IncludeEmbedded",
            #    group="Restrict",
            #    doc="Include elements that are embedded in the selected entities",
            #    value=True,
            #),
        ]

        return super()._get_properties() + props

