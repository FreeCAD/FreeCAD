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

"""Associative boundary constraints, independent of Part Design integration."""

from types import SimpleNamespace
import FreeCAD as App
import Part
from .brep import ConversionError
from .cage import ControlCage, update_object_shape
from .placement import global_placement
from .match_constraints import apply_match_constraints
from .match_support import (
    _support_shape,
    _linked_support,
    _linked_face,
    _closest_point,
    _face_match_parameters,
    _face_point,
    _wire_point,
    _wire_fraction,
    _wire_match_parameters,
    _edge_direction_from_vertex,
    _wire_corner_points,
    _apply_match_corner_sharpness,
    _apply_match_corner_creases,
    _edge_contains_points,
    _neighboring_support_face,
    _surface_tangent_plane,
    _project_to_planes,
    _boundary_tangent_planes,
    _matched_boundary,
    _validate_match_mode,
    _boundary_edges,
    _cap_matched_form,
)

MATCH_CORNER_SHARPNESS = 10.0


def _match_preview(
    obj,
    boundary_edges,
    support,
    continuity="Tangent",
    tangent_mode="AdjacentFaces",
):
    """Build a Match result without modifying the document object."""
    target, tangent_face, _support_kind = _linked_support(support, obj)
    if target is None:
        raise ValueError("Match support must be one face or one closed wire")
    cage = ControlCage.from_object(obj)
    boundary = _matched_boundary(cage, boundary_edges)
    _validate_match_mode(continuity, tangent_mode, tangent_face)
    preview = SimpleNamespace(
        FormType=str(obj.FormType),
        Placement=global_placement(obj),
        ControlPoints=[App.Vector(point) for point in obj.ControlPoints],
        ControlFaces=list(obj.ControlFaces),
        VertexSharpness=list(getattr(obj, "VertexSharpness", ())),
        EdgeSharpness=list(getattr(obj, "EdgeSharpness", ())),
        MatchBoundary=list(boundary),
        MatchSupport=support,
        MatchContinuity=continuity,
        MatchTangentMode=tangent_mode,
        MatchParameters=[],
        MatchCornerVertices=list(getattr(obj, "MatchCornerVertices", ())),
        MatchCornerEdges=list(getattr(obj, "MatchCornerEdges", ())),
        LocalEdgeInserts=list(getattr(obj, "LocalEdgeInserts", ())),
        LocalControlPoints=[App.Vector(point) for point in getattr(obj, "LocalControlPoints", ())],
        TMeshData=str(getattr(obj, "TMeshData", "") or ""),
        DissolvedEdges=list(getattr(obj, "DissolvedEdges", ())),
        BRepTolerance=obj.BRepTolerance,
        MaxRefinement=int(obj.MaxRefinement),
        Shape=Part.Shape(),
        MaximumDeviation=0.0,
        ConversionLevel=0,
        ConversionStatus="",
    )
    for name in ("MatchPositionTolerance", "MatchAngularTolerance", "MatchCurvatureTolerance"):
        setattr(preview, name, getattr(obj, name))
    update_object_shape(preview)
    if preview.Shape.isNull():
        raise ConversionError(preview.ConversionStatus or "Could not build Match preview")
    return preview


def preview_match_shape(obj, boundary_edges, support, continuity="Tangent",
                        tangent_mode="AdjacentFaces", with_diagnostics=False):
    preview = _match_preview(obj, boundary_edges, support, continuity, tangent_mode)
    if with_diagnostics:
        return preview.Shape, str(getattr(preview, "MatchStatus", ""))
    return preview.Shape


def match_boundary(
    obj,
    boundary_edges,
    support,
    continuity="Connected",
    tangent_mode=None,
):
    """Associatively align one complete cage opening with a face or closed wire."""
    if not str(getattr(obj, "FormType", "")).startswith("Forms::"):
        raise TypeError("Match requires a Forms object")
    from .feature import FormFeatureProxy
    FormFeatureProxy._ensure_match_properties(obj)
    target, tangent_face, _support_kind = _linked_support(support, obj)
    if target is None:
        raise ValueError("Match support must be one face or one closed wire")
    cage = ControlCage.from_object(obj)
    boundary = _matched_boundary(cage, boundary_edges)
    if tangent_mode is None:
        tangent_mode = str(obj.MatchTangentMode)
    _validate_match_mode(continuity, tangent_mode, tangent_face)

    if continuity == "Curvature":
        # Solve and validate on an unowned preview before changing the document.
        preview = _match_preview(obj, boundary_edges, support, continuity, tangent_mode)
        for name in ("ControlPoints", "VertexSharpness", "EdgeSharpness", "MatchBoundary",
                     "MatchSupport", "MatchContinuity", "MatchTangentMode", "MatchParameters",
                     "MatchCornerVertices", "MatchCornerEdges", "MatchPositionError",
                     "MatchAngularError", "MatchCurvatureError", "MatchStatus"):
            setattr(obj, name, getattr(preview, name))
        obj.CageMode = "Editable"
        obj.touch()
        return obj

    obj.CageMode = "Editable"
    obj.MatchBoundary = list(boundary)
    obj.MatchSupport = support
    obj.MatchContinuity = continuity
    obj.MatchTangentMode = tangent_mode
    obj.MatchParameters = []
    apply_match_constraints(obj)
    obj.touch()
    return obj


# Keep the historical scripting imports available after separating implementation modules.
__all__ = [
    "MATCH_CORNER_SHARPNESS",
    "_apply_match_corner_creases",
    "_apply_match_corner_sharpness",
    "_boundary_edges",
    "_boundary_tangent_planes",
    "_cap_matched_form",
    "_closest_point",
    "_edge_contains_points",
    "_edge_direction_from_vertex",
    "_face_match_parameters",
    "_face_point",
    "_linked_face",
    "_linked_support",
    "_matched_boundary",
    "_neighboring_support_face",
    "_project_to_planes",
    "_support_shape",
    "_surface_tangent_plane",
    "_validate_match_mode",
    "_wire_corner_points",
    "_wire_fraction",
    "_wire_match_parameters",
    "_wire_point",
    "apply_match_constraints",
    "match_boundary",
    "preview_match_shape",
]
