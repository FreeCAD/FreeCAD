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

__title__ = "FreeCAD FEM distance document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package mesh_distance
#  \ingroup FEM
#  \brief  object defining mesh size by distance to object and size transitions

from . import base_femmeshelement
from . import base_fempythonobject
_PropHelper = base_fempythonobject._PropHelper

class MeshDistance(base_femmeshelement.BaseFemMeshElement):
    """
    The FemMeshDistance object
    """

    Type = "Fem::MeshDistance"

    def _get_properties(self):

        props = [
            _PropHelper(
                type="App::PropertyLength",
                name="DistanceMinimum",
                group="DistanceSizeField",
                doc="Distance up to which the mesh size will be SizeMinimum",
                value="10mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="DistanceMaximum",
                group="DistanceSizeField",
                doc="Distance at which the mesh size will be SizeMaximum",
                value="100mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="SizeMinimum",
                group="DistanceSizeField",
                doc="Mesh size when distance < DistanceMinimum",
                value="3mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="SizeMaximum",
                group="DistanceSizeField",
                doc="Mesh size when distance = DistanceMaximum",
                value="20mm",
            ),
            _PropHelper(
                type="App::PropertyBool",
                name="LinearInterpolation",
                group="DistanceSizeField",
                doc="Defines if interpolation of mesh size between SizeMinimum and SizeMaximum should be a linear or a sigmoid function",
                value=True,
            ),
            _PropHelper(
                type="App::PropertyIntegerConstraint",
                name="Sampling",
                group="DistanceSizeField",
                doc="Sampling points used to discretize curves and surfaces. For surface it is the sampling size per direction.",
                value=(20, 0, 1000, 1),
            ),
        ]

        return super()._get_properties() + props
