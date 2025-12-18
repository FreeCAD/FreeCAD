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

__title__ = "FreeCAD FEM mesh advanced document object"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package mesh_advanced
#  \ingroup FEM
#  \brief  object defining mesh size by various advanced methods

from . import base_femmeshelement
from . import base_fempythonobject
_PropHelper = base_fempythonobject._PropHelper

class MeshAdvanced(base_femmeshelement.BaseFemMeshElement):
    """
    The FemMeshAdvanced object
    """

    Type = "Fem::MeshAdvanced"

    def _get_properties(self):

        props = [
            _PropHelper(
                type="App::PropertyLinkList",
                name="Refinements",
                group="Advanced",
                doc="Refinements usable in the math eval mesh refinements",
                value=None,
            ),
            _PropHelper(
                type="App::PropertyEnumeration",
                name="Type",
                group="Advanced",
                doc="Which advanced method is used to define the mesh size",
                value=["AttractorAnisoCurve", "MathEval", "MathEvalAniso", "Distance", "Result"],
            ),

            # AttractorAnisoCurve
            _PropHelper(
                type="App::PropertyLength",
                name="DistanceMax",
                group="AttractorAnisoCurve",
                doc="Maxmium distance, above this distance from the curves, prescribe the maximum mesh sizes",
                value="100mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="DistanceMin",
                group="AttractorAnisoCurve",
                doc="Minimum distance, below this distance from the curves, prescribe the minimum mesh sizes",
                value="20mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="SizeMaxNormal",
                group="AttractorAnisoCurve",
                doc="Maximum mesh size in the direction normal to the closest curve",
                value="10mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="SizeMaxTangent",
                group="AttractorAnisoCurve",
                doc="Maximum mesh size in the direction tangeant to the closest curve",
                value="20mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="SizeMinNormal",
                group="AttractorAnisoCurve",
                doc="Minimum mesh size in the direction normal to the closest curve",
                value="5mm",
            ),
            _PropHelper(
                type="App::PropertyLength",
                name="SizeMinTangent",
                group="AttractorAnisoCurve",
                doc="Minimum mesh size in the direction tangeant to the closest curve",
                value="10mm",
            ),
            _PropHelper(
                type="App::PropertyInteger",
                name="Sampling",
                group="Evaluation",
                doc="Number of sampling points on each curve or surface for AttractorAnisoCurve and Distance",
                value=20,
            ),

            # MathEval
            _PropHelper(
                type="App::PropertyString",
                name="Equation",
                group="MathEval",
                doc="The equation to calculate for the mesh size",
                value="",
            ),

            # MathEvalAniso
            _PropHelper(
                type="App::PropertyString",
                name="M11",
                group="MathEvalAniso",
                doc="The equation to calculate element 11 of the metric tensor",
                value="",
            ),
            _PropHelper(
                type="App::PropertyString",
                name="M12",
                group="MathEvalAniso",
                doc="The equation to calculate element 12 of the metric tensor",
                value="",
            ),
            _PropHelper(
                type="App::PropertyString",
                name="M13",
                group="MathEvalAniso",
                doc="The equation to calculate element 13 of the metric tensor",
                value="",
            ),
            _PropHelper(
                type="App::PropertyString",
                name="M22",
                group="MathEvalAniso",
                doc="The equation to calculate element 22 of the metric tensor",
                value="",
            ),
            _PropHelper(
                type="App::PropertyString",
                name="M23",
                group="MathEvalAniso",
                doc="The equation to calculate element 23 of the metric tensor",
                value="",
            ),
            _PropHelper(
                type="App::PropertyString",
                name="M33",
                group="MathEvalAniso",
                doc="The equation to calculate element 33 of the metric tensor",
                value="",
            ),

            # Result
            _PropHelper(
                type="App::PropertyLink",
                name="ResultObject",
                group="Result",
                doc="The result object which defines the mesh size",
                value=None,
            ),
            _PropHelper(
                type="App::PropertyString",
                name="ResultField",
                group="Result",
                doc="The data field of the result object used to define the mesh size",
                value="",
            ),
        ]

        return super()._get_properties() + props

