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
#  \brief  object for manipulating other refinements in various ways

from . import base_femmeshelement
from . import base_fempythonobject
_PropHelper = base_fempythonobject._PropHelper

class MeshManipulate(base_femmeshelement.BaseFemMeshElement):
    """
    The FemMeshManipulate object
    """

    Type = "Fem::MeshManipulate"

    def _get_properties(self):

        props = [
            _PropHelper(
                type="App::PropertyLink",
                name="Refinement",
                group="Manipulate",
                doc="Which refinement is manipulated",
                value=None,
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="Type",
                group="Manipulate",
                doc="Which manipulation is applied to the refinement",
                value=["Restrict", "Threshold", "Mean", "Gradient", "Curvature", "Laplacian"],
            ),

            # restrict
            _PropHelper(
                type="App::PropertyBool",
                name="IncludeBoundary",
                group="Restrict",
                doc="Include the boundary of the selected entities",
                value=True,
            ),

            # threshold
            _PropHelper(
                type="App::PropertyLength",
                name="InputMinimum",
                group="Threshold",
                doc="Refinement input value up to which the mesh size will be SizeMinimum",
                value="10mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="InputMaximum",
                group="Threshold",
                doc="Refinement input value at which the mesh size will be SizeMaximum",
                value="100mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="SizeMinimum",
                group="Threshold",
                doc="Mesh size when refinement value < ValueMinimum",
                value="3mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="SizeMaximum",
                group="Threshold",
                doc="Mesh size when refinement value = ValueMaximum",
                value="20mm",
            ),
            _PropHelper(
                type="App::PropertyBool",
                name="LinearInterpolation",
                group="Threshold",
                doc="Defines if interpolation of mesh size between SizeMinimum and SizeMaximum should be a linear or a sigmoid function",
                value=True,
            ),
            _PropHelper(
                type="App::PropertyBool",
                name="StopAtInputMax",
                group="Threshold",
                doc="Defines if the SizeMaximum value shall be defined for input values larger then InputMaximum",
                value=True,
            ),

            # Gradient
            _PropHelper(
                type="App::PropertyEnumeration",
                name="Kind",
                group="Gradient",
                doc="Component of the gradient to evaluate",
                value= ["X", "Y", "Z", "Mean"],
            ),

            # mean  + Curvature + Laplacian
            _PropHelper(
                type="App::PropertyLength",
                name="Delta",
                group="Evaluation",
                doc="Distance over which the field shall be computed",
                value="10mm",
            ),

        ]

        # super() props needed as restrict requires selected elements
        return super()._get_properties() + props

