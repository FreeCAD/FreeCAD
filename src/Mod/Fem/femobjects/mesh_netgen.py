# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD FEM mesh netgen document object"
__author__ = "Mario Passaglia"
__url__ = "https://www.freecad.org"

## @package mesh_netgen
#  \ingroup FEM
#  \brief mesh gmsh object

from FreeCAD import Base
from . import base_fempythonobject

_PropHelper = base_fempythonobject._PropHelper


class MeshNetgen(base_fempythonobject.BaseFemPythonObject):
    """
    A Fem::FemMeshShapeBaseObject python type, add Netgen specific properties
    """

    Type = "Fem::FemMeshNetgen"

    def __init__(self, obj):
        super().__init__(obj)

        for prop in self._get_properties():
            prop.add_to_object(obj)

    def _get_properties(self):
        prop = []

        prop.append(
            _PropHelper(
                type="App::PropertyString",
                name="Optimize3d",
                group="Mesh Parameters",
                doc="3d optimization strategy.\n"
                + "m: move nodes, M: move nodes, cheap functional\n"
                + "s: swap faces, c: combine elements, d: divide elements,\n"
                + "D: divide and join opposite edges, remove element,\n"
                + "p: plot, no pause, P: plot, Pause,\n"
                + "h: Histogramm, no pause, H: Histogramm, pause",
                value="cmdDmustm",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="OptimizationSteps3d",
                group="Mesh Parameters",
                doc="Number of 3d optimization steps",
                value=3,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyString",
                name="Optimize2d",
                group="Mesh Parameters",
                doc="2d optimization strategy.\n"
                + "s: swap opt 6 lines/node, S: swap optimal elements,\n"
                + "m: move nodes, p: plot, no pause\n"
                + "P: plot, pause, c: combine",
                value="smcmSmcmSmcm",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="OptimizationSteps2d",
                group="Mesh Parameters",
                doc="Number of 2d optimization steps",
                value=3,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloat",
                name="OptimizationErrorPower",
                group="Mesh Parameters",
                doc="Power of error to approximate max error optimization",
                value=2.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="BlockFill",
                group="Mesh Parameters",
                doc="Do block filling",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="FillDistance",
                group="Mesh Parameters",
                doc="Block filling up to distance",
                value=0.1,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloat",
                name="Safety",
                group="Mesh Parameters",
                doc="Radius of local environment (times h)",
                value=5.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloat",
                name="RelinnerSafety",
                group="Mesh Parameters",
                doc="Radius of active environment (times h)",
                value=3.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="LocalH",
                group="Mesh Parameters",
                doc="Use local h",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="UseLocalH",
                group="Mesh Parameters",
                doc="Use local H",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloat",
                name="GrowthRate",
                group="Mesh Parameters",
                doc="Grading for local h",
                value=0.3,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="Delaunay",
                group="Mesh Parameters",
                doc="Use Delaunay for 3d meshing",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="Delaunay2d",
                group="Mesh Parameters",
                doc="Use Delaunay for 2d meshing",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="MaxSize",
                group="Mesh Parameters",
                doc="Maximal mesh size",
                value="1000 mm",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyLength",
                name="MinSize",
                group="Mesh Parameters",
                doc="Minimal mesh size",
                value="0 mm",
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloat",
                name="CloseEdgeFactor",
                group="Mesh Parameters",
                doc="Factor to restrict meshing based on close edges",
                value=2.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="StartInSurface",
                group="Mesh Parameters",
                doc="Start surface meshing from everywhere in surface",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="CheckOverlap",
                group="Mesh Parameters",
                doc="Check overlapping surfaces",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="CheckOverlappingBoundary",
                group="Mesh Parameters",
                doc="Check overlapping surface mesh before volume meshing",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="CheckChartBoundary",
                group="Mesh Parameters",
                doc="Check chart boundary",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloat",
                name="CurvatureSafety",
                group="Mesh Parameters",
                doc="Safety factor for curvatures (elements per radius)",
                value=2.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloat",
                name="SegmentsPerEdge",
                group="Mesh Parameters",
                doc="Minimal number of segments per edge",
                value=2.0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloat",
                name="ElementSizeWeight",
                group="Mesh Parameters",
                doc="Weight of element size respect to  element shape",
                value=0.2,
            )
        )

        # Netgen meshing steps
        meshing_step = [
            "AnalizeGeometry",
            "MeshEdges",
            "MeshSurface",
            "OptimizeSurface",
            "MeshVolume",
            "OptimizeVolume",
        ]

        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="StartStep",
                group="Mesh Parameters",
                doc="First step",
                value=meshing_step,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="EndStep",
                group="Mesh Parameters",
                doc="Last step",
                value=meshing_step,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="GiveUpTolerance2d",
                group="Mesh Parameters",
                doc="Give up quality class, 2d meshing",
                value=200,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="GiveUpTolerance",
                group="Mesh Parameters",
                doc="Give up quality class, 3d meshing",
                value=10,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="GiveUpToleranceOpenQuads",
                group="Mesh Parameters",
                doc="Give up quality class, for closing open quads, greather than 100 for free pyramids",
                value=15,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="MaxOuterSteps",
                group="Mesh Parameters",
                doc="Maximal outer steps",
                value=10,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="StarShapeClass",
                group="Mesh Parameters",
                doc="Class starting star-shape filling",
                value=5,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="BaseElementNp",
                group="Mesh Parameters",
                doc="If non-zero, baseelement must have BaseElementlNp points",
                value=0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="Sloppy",
                group="Mesh Parameters",
                doc="Quality tolerances are handled less careful",
                value=10,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyFloat",
                name="BadElementLimit",
                group="Mesh Parameters",
                doc="Limit for max element angle (150-180)",
                value=175,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="CheckImpossible",
                group="Mesh Parameters",
                doc="",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="Only3dDomainNr",
                group="Mesh Parameters",
                doc="",
                value=0,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="SecondOrder",
                group="Mesh Parameters",
                doc="Second order element meshing",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="ElementOrder",
                group="Mesh Parameters",
                doc="High order element curvature",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="QuadDominated",
                group="Mesh Parameters",
                doc="Quad-dominated surface meshing",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="TryHexes",
                group="Mesh Parameters",
                doc="Try hexahedral elements",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="InvertTets",
                group="Mesh Parameters",
                doc="",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="InvertTrigs",
                group="Mesh Parameters",
                doc="",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="AutoZRefine",
                group="Mesh Parameters",
                doc="Automatic Z refine",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="ParallelMeshing",
                group="Mesh Parameters",
                doc="Use parallel meshing",
                value=True,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyInteger",
                name="Threads",
                group="Mesh Parameters",
                doc="Number of threads for parallel meshing",
                value=4,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyBool",
                name="HealShape",
                group="Mesh Parameters",
                doc="Heal shape before meshing",
                value=False,
            )
        )
        prop.append(
            _PropHelper(
                type="App::PropertyEnumeration",
                name="Fineness",
                group="Mesh Parameters",
                doc="Mesh granularity.\n"
                + "If differs from UserDefined, uses specific values\n"
                + "for CurvatureSafety, SegmentsPerEdge, GrowthRate,\n"
                + "CloseEdgeFactor and OptimizationSteps3d",
                value=["VeryCoarse", "Coarse", "Moderate", "Fine", "VeryFine", "UserDefined"],
            )
        )

        return prop

    def onChanged(self, obj, prop):
        if prop == "Fineness":
            if obj.Fineness != "UserDefined":
                p = self.get_predef_fineness_params(obj.Fineness)
                obj.CurvatureSafety = p["curvaturesafety"]
                obj.SegmentsPerEdge = p["segmentsperedge"]
                obj.GrowthRate = p["grading"]
                obj.CloseEdgeFactor = p["closeedgefac"]
                obj.OptimizationSteps3d = p["optsteps3d"]

                obj.setPropertyStatus("CurvatureSafety", "ReadOnly")
                obj.setPropertyStatus("SegmentsPerEdge", "ReadOnly")
                obj.setPropertyStatus("GrowthRate", "ReadOnly")
                obj.setPropertyStatus("CloseEdgeFactor", "ReadOnly")
                obj.setPropertyStatus("OptimizationSteps3d", "ReadOnly")
            else:
                obj.setPropertyStatus("CurvatureSafety", "-ReadOnly")
                obj.setPropertyStatus("SegmentsPerEdge", "-ReadOnly")
                obj.setPropertyStatus("GrowthRate", "-ReadOnly")
                obj.setPropertyStatus("CloseEdgeFactor", "-ReadOnly")
                obj.setPropertyStatus("OptimizationSteps3d", "-ReadOnly")

    def onDocumentRestored(self, obj):
        # update old project with new properties
        for prop in self._get_properties():
            try:
                obj.getPropertyByName(prop.name)
            except Base.PropertyError:
                prop.add_to_object(obj)

            # for StartStep and EndStep set enumeration index from old integer value
            if prop.name == "StartStep" or prop.name == "EndStep":
                prop.handle_change_type(
                    obj, "App::PropertyInteger", lambda x: 0 if x <= 1 else 5 if x >= 6 else x - 1
                )

    def get_predef_fineness_params(self, fineness):
        # set specific parameters by fineness
        params = {}
        if fineness == "VeryCoarse":
            params["curvaturesafety"] = 1
            params["segmentsperedge"] = 0.3
            params["grading"] = 0.7
            params["closeedgefac"] = 0.5
            params["optsteps3d"] = 5
        elif fineness == "Coarse":
            params["curvaturesafety"] = 1.5
            params["segmentsperedge"] = 0.5
            params["grading"] = 0.5
            params["closeedgefac"] = 1
            params["optsteps3d"] = 5
        elif fineness == "Moderate":
            params["curvaturesafety"] = 2
            params["segmentsperedge"] = 1
            params["grading"] = 0.3
            params["closeedgefac"] = 2
            params["optsteps3d"] = 5
        elif fineness == "Fine":
            params["curvaturesafety"] = 3
            params["segmentsperedge"] = 2
            params["grading"] = 0.2
            params["closeedgefac"] = 3.5
            params["optsteps3d"] = 5
        elif fineness == "VeryFine":
            params["curvaturesafety"] = 5
            params["segmentsperedge"] = 3
            params["grading"] = 0.1
            params["closeedgefac"] = 5
            params["optsteps3d"] = 5
        elif fineness == "UserDefined":
            params["curvaturesafety"] = 2
            params["segmentsperedge"] = 2
            params["grading"] = 0.3
            params["closeedgefac"] = 2
            params["optsteps3d"] = 3

        return params
