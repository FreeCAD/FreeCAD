# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""Shared document-feature lifecycle and cage persistence."""

import FreeCAD as App
from .cage import update_object_shape


def reset_cage(obj, vertices, faces):
    """Replace parametric topology and reset all index-dependent properties."""
    obj.ControlPoints = [App.Vector(*point) for point in vertices]
    obj.ControlFaces = [" ".join(str(index) for index in face) for face in faces]
    obj.VertexSharpness = [0.0] * len(vertices)
    obj.EdgeSharpness = []
    obj.LocalEdgeInserts = []
    obj.LocalControlPoints = []
    obj.TMeshData = ""
    obj.DissolvedEdges = []


class FormFeatureProxy:
    """Shared persistence and recompute lifecycle for every Forms primitive."""

    Type = ""
    ParameterNames = ()

    @classmethod
    def _add_common_properties(cls, obj):
        obj.addProperty("App::PropertyString", "FormType", "Forms", "Forms object type")
        obj.addProperty(
            "App::PropertyInteger", "TopologyVersion", "Forms", "Control-cage data version"
        )
        obj.addProperty(
            "App::PropertyVectorList", "ControlPoints", "Control Cage", "Control-cage vertices"
        )
        obj.addProperty(
            "App::PropertyStringList", "ControlFaces", "Control Cage", "Quad vertex indices"
        )
        obj.addProperty(
            "App::PropertyEnumeration",
            "CageMode",
            "Control Cage",
            "Whether the cage follows primitive parameters or is edited directly",
        )
        cls._ensure_symmetry_properties(obj)
        cls._ensure_sharpness_properties(obj)
        cls._ensure_local_edit_properties(obj)
        cls._ensure_conversion_properties(obj)
        cls._ensure_match_properties(obj)

        obj.FormType = cls.Type
        obj.TopologyVersion = 1
        obj.CageMode = ["Parametric", "Editable"]
        obj.CageMode = "Parametric"
        obj.setEditorMode("FormType", 1)
        obj.setEditorMode("TopologyVersion", 1)
        obj.setEditorMode("ControlPoints", 1)
        obj.setEditorMode("ControlFaces", 1)

    def _finish_initialization(self, obj):
        obj.Proxy = self
        self.onChanged(obj, "CageMode")

    def _topology(self, _obj):
        raise NotImplementedError

    @staticmethod
    def _ensure_match_properties(obj):
        if "MatchSupport" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLinkSub",
                "MatchSupport",
                "Match",
                "Associative face or closed-wire support",
            )
        if "MatchBoundary" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyIntegerList",
                "MatchBoundary",
                "Match",
                "Ordered control vertices around the matched opening",
            )
        if "MatchContinuity" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "MatchContinuity",
                "Match",
                "Continuity across the matched transition",
            )
            obj.MatchContinuity = ["Connected", "Tangent"]
            obj.MatchContinuity = "Tangent"
        if "MatchTangentMode" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "MatchTangentMode",
                "Match",
                "Faces that define tangent continuity around the matched opening",
            )
            obj.MatchTangentMode = ["AdjacentFaces", "SelectedFace"]
            obj.MatchTangentMode = "AdjacentFaces"
        if "MatchParameters" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyFloatList",
                "MatchParameters",
                "Match",
                "Normalized support coordinates for the matched boundary",
            )
        if "MatchCornerVertices" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyIntegerList",
                "MatchCornerVertices",
                "Match",
                "Matched control vertices held at corners of the support wire",
            )
        if "MatchCornerEdges" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "MatchCornerEdges",
                "Match",
                "Control edges creased from corners of a tangent matched opening",
            )
        obj.setEditorMode("MatchBoundary", 2)
        obj.setEditorMode("MatchParameters", 2)
        obj.setEditorMode("MatchCornerVertices", 2)
        obj.setEditorMode("MatchCornerEdges", 2)

    @staticmethod
    def _ensure_local_edit_properties(obj):
        if "DissolvedEdges" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "DissolvedEdges",
                "Topology",
                "Internal control edges hidden by logical face merging",
            )
        obj.setEditorMode("DissolvedEdges", 1)
        if "TMeshData" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString",
                "TMeshData",
                "Topology",
                "Versioned hierarchical T-mesh topology",
            )
        obj.setEditorMode("TMeshData", 1)
        if "LocalEdgeInserts" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "LocalEdgeInserts",
                "Topology",
                "Exact local BRep edge insertions",
            )
        obj.setEditorMode("LocalEdgeInserts", 1)
        if "LocalControlPoints" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyVectorList",
                "LocalControlPoints",
                "Topology",
                "Persistent controls introduced by local topology edits",
            )
        obj.setEditorMode("LocalControlPoints", 1)

    @staticmethod
    def _ensure_sharpness_properties(obj):
        if "VertexSharpness" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyFloatList",
                "VertexSharpness",
                "Subdivision",
                "Semi-sharp corner value for each control vertex",
            )
        if "EdgeSharpness" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "EdgeSharpness",
                "Subdivision",
                "Semi-sharp control edges encoded as start end value",
            )
        obj.setEditorMode("VertexSharpness", 1)
        obj.setEditorMode("EdgeSharpness", 1)

    @staticmethod
    def _ensure_conversion_properties(obj):
        if "BRepTolerance" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLength",
                "BRepTolerance",
                "BRep Conversion",
                "Maximum accepted distance from sampled Catmull-Clark limit points",
            )
            obj.BRepTolerance = 0.05
        if "MaxRefinement" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyIntegerConstraint",
                "MaxRefinement",
                "BRep Conversion",
                "Maximum patch sampling refinement",
            )
            obj.MaxRefinement = (3, 2, 4, 1)
        if "MaximumDeviation" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLength",
                "MaximumDeviation",
                "BRep Conversion",
                "Measured distance from validation samples to the generated surfaces",
            )
        if "ConversionLevel" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyInteger",
                "ConversionLevel",
                "BRep Conversion",
                "Patch sampling level used for the current BRep",
            )
        if "ConversionStatus" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString",
                "ConversionStatus",
                "BRep Conversion",
                "Result of the most recent cage-to-BRep conversion",
            )
        for name in ("MaximumDeviation", "ConversionLevel", "ConversionStatus"):
            obj.setEditorMode(name, 1)

    @staticmethod
    def _ensure_symmetry_properties(obj):
        if "Symmetric" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyBool",
                "Symmetric",
                "Symmetry",
                "Keep the control cage symmetric",
            )
            obj.Symmetric = False
        if "SymmetryPlane" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "SymmetryPlane",
                "Symmetry",
                "Local plane used to mirror control points",
            )
            obj.SymmetryPlane = ["XY", "XZ", "YZ"]
            obj.SymmetryPlane = "YZ"

    def execute(self, obj):
        if obj.CageMode == "Parametric":
            vertices, faces = self._topology(obj)
            reset_cage(obj, vertices, faces)
        update_object_shape(obj)

    def onChanged(self, obj, prop):
        if prop == "CageMode":
            read_only = 0 if obj.CageMode == "Parametric" else 1
            for name in self.ParameterNames:
                obj.setEditorMode(name, read_only)

    def onDocumentRestored(self, obj):
        self._ensure_conversion_properties(obj)
        self._ensure_symmetry_properties(obj)
        self._ensure_sharpness_properties(obj)
        self._ensure_local_edit_properties(obj)
        self._ensure_match_properties(obj)
        obj.Proxy = self
        self.onChanged(obj, "CageMode")


