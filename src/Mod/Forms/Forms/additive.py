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

"""Part Design Forms primitives and associative boundary matching."""

from types import SimpleNamespace

import FreeCAD as App
import Part

from .box import FormBoxProxy, FormFeatureProxy, ViewProviderFormBox
from .brep import ConversionError
from .cage import ControlCage, update_object_shape
from .placement import global_placement
from .primitives import (
    FormCylinderProxy,
    FormFaceProxy,
    FormSphereProxy,
    FormQuadballProxy,
    FormTorusProxy,
    FormTubeProxy,
    ViewProviderFormCylinder,
    ViewProviderFormFace,
    ViewProviderFormSphere,
    ViewProviderFormQuadball,
    ViewProviderFormTorus,
    ViewProviderFormTube,
)
from .pipe import FormPipeProxy, ViewProviderFormPipe, fused_pipe_shape, update_pipe_shape

PRIMITIVES = {
    "Box": (FormBoxProxy, ViewProviderFormBox, "Form Box"),
    "Cylinder": (FormCylinderProxy, ViewProviderFormCylinder, "Form Cylinder"),
    "Sphere": (FormSphereProxy, ViewProviderFormSphere, "Form Sphere"),
    "Quadball": (FormQuadballProxy, ViewProviderFormQuadball, "Form Quadball"),
    "Pipe": (FormPipeProxy, ViewProviderFormPipe, "Form Pipe"),
    "Face": (FormFaceProxy, ViewProviderFormFace, "Form Face"),
    "Torus": (FormTorusProxy, ViewProviderFormTorus, "Form Torus"),
    "Tube": (FormTubeProxy, ViewProviderFormTube, "Form Tube"),
}

OPERATIONS = ("Additive", "Subtractive")


MATCH_CORNER_SHARPNESS = 10.0


def _primitive_proxy(obj):
    form_type = str(obj.FormType).removeprefix("Forms::")
    if form_type not in PRIMITIVES:
        raise ValueError(f"Unsupported Part Design Form primitive: {form_type}")
    return PRIMITIVES[form_type][0]


def _linked_support(reference):
    if not reference or reference[0] is None:
        return None, None, None
    source, names = reference
    if isinstance(names, str):
        names = (names,)
    names = tuple(str(name) for name in names)
    if len(names) == 1 and names[0].startswith("Face"):
        face = source.Shape.getElement(names[0])
        return face.OuterWire, face, "Face"
    if not names and source.Shape.ShapeType == "Wire":
        wire = source.Shape
    elif len(names) == 1 and names[0].startswith("Wire"):
        wire = source.Shape.getElement(names[0])
    elif names and all(name.startswith("Edge") for name in names):
        groups = Part.sortEdges([source.Shape.getElement(name) for name in names])
        if len(groups) != 1:
            return None, None, None
        wire = Part.Wire(groups[0])
    else:
        return None, None, None
    if wire.ShapeType != "Wire" or not wire.isClosed():
        return None, None, None
    try:
        plane_face = Part.Face(wire)
    except (Part.OCCError, RuntimeError):
        plane_face = None
    return wire, plane_face, "Wire"


def _linked_face(reference):
    _target, face, _kind = _linked_support(reference)
    return face


def _closest_point(target, point):
    probe = Part.Vertex(point.x, point.y, point.z)
    distance, closest, _info = probe.distToShape(target)
    if not closest:
        raise ConversionError("Could not project the Form boundary onto the support face")
    return App.Vector(closest[0][1]), float(distance)


def _face_match_parameters(face, points):
    raw = []
    for point in points:
        try:
            raw.append(face.Surface.parameter(point))
        except (Part.OCCError, RuntimeError, ValueError):
            projected, _distance = _closest_point(face, point)
            raw.append(face.Surface.parameter(projected))
    u_values = [value[0] for value in raw]
    v_values = [value[1] for value in raw]
    u_min, u_max = min(u_values), max(u_values)
    v_min, v_max = min(v_values), max(v_values)
    u_span = max(u_max - u_min, 1.0e-12)
    v_span = max(v_max - v_min, 1.0e-12)
    return [
        component
        for u_value, v_value in raw
        for component in (
            (u_value - u_min) / u_span,
            (v_value - v_min) / v_span,
        )
    ]


def _face_point(face, u_fraction, v_fraction):
    u_min, u_max, v_min, v_max = face.ParameterRange
    nominal = face.valueAt(
        u_min + max(0.0, min(float(u_fraction), 1.0)) * (u_max - u_min),
        v_min + max(0.0, min(float(v_fraction), 1.0)) * (v_max - v_min),
    )
    return _closest_point(face.OuterWire, nominal)[0]


def _wire_point(wire, fraction):
    edges = list(wire.OrderedEdges)
    total = sum(edge.Length for edge in edges)
    if total <= 1.0e-12:
        raise ValueError("Match support wire has zero length")
    distance = (float(fraction) % 1.0) * total
    for edge in edges:
        if distance <= edge.Length or edge is edges[-1]:
            parameter = edge.getParameterByLength(min(distance, edge.Length))
            return edge.valueAt(parameter)
        distance -= edge.Length
    return edges[-1].valueAt(edges[-1].LastParameter)


def _wire_fraction(wire, point):
    edges = list(wire.OrderedEdges)
    total = sum(edge.Length for edge in edges)
    if total <= 1.0e-12:
        raise ValueError("Match support wire has zero length")
    best = None
    elapsed = 0.0
    probe = Part.Vertex(point.x, point.y, point.z)
    for edge in edges:
        distance, closest, _info = probe.distToShape(edge)
        if not closest:
            elapsed += edge.Length
            continue
        target_parameter = edge.Curve.parameter(App.Vector(closest[0][1]))
        low, high = 0.0, edge.Length
        for _step in range(40):
            middle = (low + high) * 0.5
            parameter = edge.getParameterByLength(middle)
            if parameter < target_parameter:
                low = middle
            else:
                high = middle
        fraction = (elapsed + (low + high) * 0.5) / total
        candidate = (float(distance), fraction)
        if best is None or candidate[0] < best[0]:
            best = candidate
        elapsed += edge.Length
    if best is None:
        raise ConversionError("Could not parameterize the Match support wire")
    return best[1]


def _wire_match_parameters(wire, points):
    if not points:
        return []
    lengths = [0.0]
    for first, second in zip(points, points[1:] + points[:1]):
        lengths.append(lengths[-1] + second.sub(first).Length)
    perimeter = lengths[-1]
    if perimeter <= 1.0e-12:
        raise ValueError("Form opening has zero length")
    relative = [value / perimeter for value in lengths[:-1]]
    phase = _wire_fraction(wire, points[0])
    if len(points) > 1:
        forward = _wire_point(wire, phase + relative[1]).sub(points[1]).Length
        backward = _wire_point(wire, phase - relative[1]).sub(points[1]).Length
        direction = 1.0 if forward <= backward else -1.0
    else:
        direction = 1.0
    return [(phase + direction * value) % 1.0 for value in relative]


def _edge_direction_from_vertex(edge, point):
    """Return an edge direction pointing away from one of its end vertices."""
    length = float(edge.Length)
    if length <= 1.0e-12:
        return None
    start = edge.valueAt(edge.FirstParameter)
    end = edge.valueAt(edge.LastParameter)
    sample_length = min(length * 0.01, max(length * 1.0e-5, 1.0e-7))
    if point.sub(start).Length <= point.sub(end).Length:
        sample = edge.valueAt(edge.getParameterByLength(sample_length))
    else:
        sample = edge.valueAt(edge.getParameterByLength(length - sample_length))
    direction = sample - point
    if direction.Length <= 1.0e-12:
        return None
    return direction / direction.Length


def _wire_corner_points(wire):
    """Return topological wire vertices whose incident edges are not tangent."""
    scale = max(float(wire.BoundBox.DiagonalLength), 1.0)
    tolerance = max(1.0e-7, scale * 1.0e-7)
    corners = []
    edges = list(wire.OrderedEdges)
    for vertex in wire.OrderedVertexes:
        point = App.Vector(vertex.Point)
        incident = [
            edge
            for edge in edges
            if any(point.sub(candidate.Point).Length <= tolerance for candidate in edge.Vertexes)
        ]
        if len(incident) != 2:
            continue
        directions = [
            direction
            for direction in (
                _edge_direction_from_vertex(incident[0], point),
                _edge_direction_from_vertex(incident[1], point),
            )
            if direction is not None
        ]
        # At a smooth join the two directions point almost exactly opposite.
        # A small angular tolerance avoids creasing merely segmented curves.
        if len(directions) == 2 and directions[0].dot(directions[1]) > -0.999:
            corners.append(point)
    return corners, tolerance


def _apply_match_corner_sharpness(obj, cage, boundary, projected, support):
    """Keep genuine support corners exact in the subdivision limit boundary."""
    corners, tolerance = _wire_corner_points(support)
    matched = {
        index
        for index in boundary
        if any(projected[index].sub(corner).Length <= tolerance for corner in corners)
    }
    sharpness = list(cage.vertex_sharpness)
    previous = {int(index) for index in getattr(obj, "MatchCornerVertices", ())}
    for index in previous - matched:
        if 0 <= index < len(sharpness) and abs(sharpness[index] - MATCH_CORNER_SHARPNESS) <= 1.0e-9:
            sharpness[index] = 0.0
    for index in matched:
        sharpness[index] = max(sharpness[index], MATCH_CORNER_SHARPNESS)
    obj.VertexSharpness = sharpness
    obj.MatchCornerVertices = sorted(matched)
    return matched


def _apply_match_corner_creases(obj, cage, boundary, corners, tangent):
    """Separate tangent patches where adjacent support faces meet sharply."""
    boundary_set = set(boundary)
    desired = {
        edge
        for edge in cage.edge_counts()
        if tangent
        and ((edge[0] in boundary_set) != (edge[1] in boundary_set))
        and (edge[0] in corners or edge[1] in corners)
    }
    previous = set()
    for encoded in getattr(obj, "MatchCornerEdges", ()):
        try:
            first, second = str(encoded).split()
            previous.add(tuple(sorted((int(first), int(second)))))
        except ValueError:
            continue
    sharpness = dict(cage.edge_sharpness)
    for edge in previous - desired:
        if abs(sharpness.get(edge, 0.0) - MATCH_CORNER_SHARPNESS) <= 1.0e-9:
            sharpness.pop(edge, None)
    for edge in desired:
        sharpness[edge] = max(sharpness.get(edge, 0.0), MATCH_CORNER_SHARPNESS)
    obj.EdgeSharpness = [
        f"{edge[0]} {edge[1]} {value:.12g}"
        for edge, value in sorted(sharpness.items())
        if value > 0.0
    ]
    obj.MatchCornerEdges = [f"{edge[0]} {edge[1]}" for edge in sorted(desired)]


def _edge_contains_points(edge, points, tolerance):
    for point in points:
        distance, _closest, _info = Part.Vertex(point).distToShape(edge)
        if distance > tolerance:
            return False
    return True


def _neighboring_support_face(reference, selected_face, support_edge):
    source = reference[0] if reference else None
    shape = getattr(source, "Shape", None)
    if shape is None:
        return None
    matches = [
        face
        for face in shape.Faces
        if not face.isSame(selected_face) and any(edge.isSame(support_edge) for edge in face.Edges)
    ]
    return matches[0] if len(matches) == 1 else None


def _surface_tangent_plane(face, point):
    u_value, v_value = face.Surface.parameter(point)
    normal = face.normalAt(u_value, v_value)
    if normal.Length <= 1.0e-12:
        raise ConversionError("Could not determine a Match tangent plane")
    normal.normalize()
    return normal, normal.dot(point)


def _project_to_planes(point, planes):
    """Return the closest point satisfying one or two tangent-plane constraints."""
    unique = []
    for normal, offset in planes:
        if any(abs(normal.dot(existing[0])) > 1.0 - 1.0e-9 for existing in unique):
            continue
        unique.append((normal, offset))
    if not unique:
        return App.Vector(point)
    if len(unique) == 1:
        normal, offset = unique[0]
        return point - normal * (normal.dot(point) - offset)

    first, second = unique[:2]
    cosine = first[0].dot(second[0])
    determinant = 1.0 - cosine * cosine
    if determinant <= 1.0e-12:
        return point - first[0] * (first[0].dot(point) - first[1])
    first_error = first[0].dot(point) - first[1]
    second_error = second[0].dot(point) - second[1]
    first_weight = (first_error - cosine * second_error) / determinant
    second_weight = (second_error - cosine * first_error) / determinant
    return point - first[0] * first_weight - second[0] * second_weight


def _boundary_tangent_planes(reference, selected_face, support, boundary, projected):
    """Map each cage boundary vertex to the model faces across its seam edges."""
    scale = max(float(support.BoundBox.DiagonalLength), 1.0)
    tolerance = max(1.0e-7, scale * 1.0e-7)
    result = {index: [] for index in boundary}
    support_edges = list(support.OrderedEdges)
    for position, first in enumerate(boundary):
        second = boundary[(position + 1) % len(boundary)]
        edge = next(
            (
                candidate
                for candidate in support_edges
                if _edge_contains_points(
                    candidate, (projected[first], projected[second]), tolerance
                )
            ),
            None,
        )
        neighbor = (
            _neighboring_support_face(reference, selected_face, edge) if edge is not None else None
        )
        if neighbor is None:
            continue
        result[first].append(_surface_tangent_plane(neighbor, projected[first]))
        result[second].append(_surface_tangent_plane(neighbor, projected[second]))
    return result


def apply_match_constraints(obj):
    """Keep the stored cage boundary associated with its support face."""
    boundary = tuple(int(index) for index in getattr(obj, "MatchBoundary", ()))
    reference = getattr(obj, "MatchSupport", None)
    support, tangent_face, support_kind = _linked_support(reference)
    if not boundary:
        return
    if support is None:
        raise ValueError("The matched Form support is no longer valid")
    cage = ControlCage.from_object(obj)
    if any(index < 0 or index >= len(cage.vertices) for index in boundary):
        raise ValueError("The matched Form boundary is no longer valid")

    vertices = [App.Vector(*point) for point in cage.vertices]
    boundary_points = [vertices[index] for index in boundary]
    parameters = list(getattr(obj, "MatchParameters", ()))
    expected = len(boundary) * 2 if support_kind == "Face" else len(boundary)
    if len(parameters) != expected:
        parameters = (
            _face_match_parameters(tangent_face, boundary_points)
            if support_kind == "Face"
            else _wire_match_parameters(support, boundary_points)
        )
        obj.MatchParameters = parameters
    projected = {}
    for position, index in enumerate(boundary):
        if support_kind == "Face":
            projected[index] = _face_point(
                tangent_face,
                parameters[position * 2],
                parameters[position * 2 + 1],
            )
        else:
            projected[index] = _wire_point(support, parameters[position])
        vertices[index] = projected[index]

    match_corners = _apply_match_corner_sharpness(obj, cage, boundary, projected, support)
    _apply_match_corner_creases(
        obj,
        cage,
        boundary,
        match_corners,
        str(obj.MatchContinuity) == "Tangent" and str(obj.MatchTangentMode) == "AdjacentFaces",
    )

    if str(obj.MatchContinuity) == "Tangent":
        if tangent_face is None:
            raise ValueError("Tangent Match requires a face or planar closed wire")
        boundary_set = set(boundary)
        tangent_planes = (
            _boundary_tangent_planes(reference, tangent_face, support, boundary, projected)
            if support_kind == "Face" and str(obj.MatchTangentMode) == "AdjacentFaces"
            else {index: [] for index in boundary}
        )
        boundary_center = sum(projected.values(), App.Vector()) / len(projected)
        form_center = sum(vertices, App.Vector()) / len(vertices)
        boundary_spacing = sum(
            (projected[boundary[(index + 1) % len(boundary)]] - projected[vertex]).Length
            for index, vertex in enumerate(boundary)
        ) / len(boundary)
        boundary_edges = {
            tuple(sorted((boundary[index], boundary[(index + 1) % len(boundary)])))
            for index in range(len(boundary))
        }
        candidates = {}
        for edge in cage.edge_counts():
            if edge in boundary_edges:
                continue
            first, second = edge
            if (first in boundary_set) == (second in boundary_set):
                continue
            boundary_index = first if first in boundary_set else second
            interior_index = second if first in boundary_set else first
            point = projected[boundary_index]
            planes = tangent_planes.get(boundary_index) or [
                _surface_tangent_plane(tangent_face, point)
            ]
            target = _project_to_planes(vertices[interior_index], planes)
            if target.sub(point).Length <= 1.0e-9:
                inward = boundary_center - point
                handle = min(
                    vertices[interior_index].sub(point).Length,
                    boundary_spacing / 3.0,
                )
                if inward.Length > 1.0e-9:
                    inward = inward * (handle / inward.Length)
                    target = _project_to_planes(point + inward, planes)
                if target.sub(point).Length <= 1.0e-9:
                    selected_normal = _surface_tangent_plane(tangent_face, point)[0]
                    direction = (
                        1.0
                        if form_center.sub(boundary_center).dot(selected_normal) >= 0.0
                        else -1.0
                    )
                    target = _project_to_planes(
                        point + selected_normal * (direction * handle), planes
                    )
                if target.sub(point).Length <= 1.0e-9:
                    raise ConversionError("Could not determine a tangent Match direction")
            candidates.setdefault(interior_index, []).append(target)
        for index, points in candidates.items():
            vertices[index] = sum(points, App.Vector()) / len(points)

    obj.ControlPoints = vertices


def _matched_boundary(cage, boundary_edges):
    selected = {tuple(sorted((int(edge[0]), int(edge[1])))) for edge in boundary_edges}
    loops = cage.boundary_loops()
    if len(loops) != 1:
        raise ValueError("Match currently requires the Form to have exactly one opening")
    matches = []
    for loop in loops:
        edges = {
            tuple(sorted((loop[index], loop[(index + 1) % len(loop)])))
            for index in range(len(loop))
        }
        if selected.intersection(edges):
            matches.append(loop)
    if len(matches) != 1:
        raise ValueError("Select edges from exactly one Form opening")
    return matches[0]


def _validate_match_mode(continuity, tangent_mode, tangent_face):
    if continuity not in ("Connected", "Tangent"):
        raise ValueError("Match continuity must be Connected or Tangent")
    if continuity == "Tangent" and tangent_face is None:
        raise ValueError("Tangent Match requires a face or planar closed wire")
    if tangent_mode not in ("AdjacentFaces", "SelectedFace"):
        raise ValueError("Match tangent mode must use adjacent or selected faces")


def preview_match_shape(
    obj,
    boundary_edges,
    support,
    continuity="Tangent",
    tangent_mode="AdjacentFaces",
):
    """Build a Match result without modifying the document object."""
    target, tangent_face, _support_kind = _linked_support(support)
    if target is None:
        raise ValueError("Match support must be one face or one closed wire")
    cage = ControlCage.from_object(obj)
    boundary = _matched_boundary(cage, boundary_edges)
    _validate_match_mode(continuity, tangent_mode, tangent_face)
    preview = SimpleNamespace(
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
    apply_match_constraints(preview)
    update_object_shape(preview)
    if preview.Shape.isNull():
        raise ConversionError(preview.ConversionStatus or "Could not build Match preview")
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
    FormFeatureProxy._ensure_match_properties(obj)
    target, tangent_face, _support_kind = _linked_support(support)
    if target is None:
        raise ValueError("Match support must be one face or one closed wire")
    cage = ControlCage.from_object(obj)
    boundary = _matched_boundary(cage, boundary_edges)
    if tangent_mode is None:
        tangent_mode = str(obj.MatchTangentMode)
    _validate_match_mode(continuity, tangent_mode, tangent_face)

    obj.CageMode = "Editable"
    obj.MatchBoundary = list(boundary)
    obj.MatchSupport = support
    obj.MatchContinuity = continuity
    obj.MatchTangentMode = tangent_mode
    obj.MatchParameters = []
    apply_match_constraints(obj)
    obj.touch()
    return obj


def _boundary_edges(shape):
    result = []
    for edge in shape.Edges:
        uses = sum(any(candidate.isSame(edge) for candidate in face.Edges) for face in shape.Faces)
        if uses == 1:
            result.append(edge)
    return result


def _cap_matched_form(obj, shape):
    if shape.ShapeType == "Solid" or not getattr(obj, "MatchBoundary", ()):
        return shape
    support = _linked_face(obj.MatchSupport)
    edges = _boundary_edges(shape)
    if support is None or not edges:
        return shape
    filling = Part.BRepOffsetAPI.MakeFilling()
    filling.loadInitSurface(support)
    for edge in edges:
        filling.add(edge, 0)
    filling.build()
    if not filling.isDone():
        raise ConversionError("Could not cap the matched Form opening")
    cap = filling.shape()
    for candidate in (cap, cap.reversed()):
        sewed = Part.makeCompound(list(shape.Faces) + list(candidate.Faces))
        sewed.sewShape(max(float(obj.BRepTolerance.Value), 1.0e-7))
        if len(sewed.Shells) == 1 and sewed.Shells[0].isClosed():
            solid = Part.makeSolid(sewed.Shells[0])
            if not solid.isNull() and solid.isValid():
                return solid
    raise ConversionError("The matched Form opening did not produce a closed solid")


def _partdesign_operation(obj):
    if "Operation" in obj.PropertiesList:
        operation = str(obj.Operation)
        if operation in OPERATIONS:
            return operation
    if obj.isDerivedFrom("PartDesign::FeatureSubtractivePython"):
        return "Subtractive"
    return "Additive"


class PartDesignFormProxy(FormFeatureProxy):
    """Forms cage that adds material to or removes material from a Body."""

    def __init__(self, obj, primitive, base_feature=None, operation="Additive", path_object=None):
        if primitive not in PRIMITIVES:
            raise ValueError(f"Unsupported Part Design Form primitive: {primitive}")
        if operation not in OPERATIONS:
            raise ValueError(f"Unsupported Part Design Form operation: {operation}")
        primitive_proxy = (
            FormPipeProxy(obj, path_object)
            if primitive == "Pipe"
            else PRIMITIVES[primitive][0](obj)
        )
        obj.Proxy = self
        self._ensure_partdesign_properties(obj, operation)
        obj.BaseFeature = base_feature
        obj.PrimitiveKind = primitive
        self.onChanged(obj, "CageMode")
        del primitive_proxy

    @staticmethod
    def _ensure_partdesign_properties(obj, operation="Additive"):
        group = "Part Design Form"
        if "Operation" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration", "Operation", group, "Boolean operation"
            )
        obj.Operation = list(OPERATIONS)
        obj.Operation = operation
        if "PrimitiveKind" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString", "PrimitiveKind", group, "Primitive kind"
            )
        if "BaseFeature" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyLink",
                "BaseFeature",
                group,
                "Preceding Part Design feature",
            )
        if "FormPlacement" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyPlacement",
                "FormPlacement",
                group,
                "Initial placement of the Form cage",
            )
        if "FormShape" not in obj.PropertiesList:
            obj.addProperty(
                "Part::PropertyPartShape",
                "FormShape",
                group,
                "Uncombined editable Form geometry",
            )
        if "EditingForm" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyBool",
                "EditingForm",
                group,
                "Show the base and Form as separate shapes while editing",
            )
        FormFeatureProxy._ensure_match_properties(obj)
        if "CombinationStatus" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyString",
                "CombinationStatus",
                group,
                "Result of combining the Form with the preceding feature",
            )
        for name in (
            "Operation",
            "PrimitiveKind",
            "FormShape",
            "EditingForm",
            "MatchBoundary",
            "CombinationStatus",
        ):
            obj.setEditorMode(name, 2)

    def _topology(self, obj):
        proxy = _primitive_proxy(obj)
        vertices, faces = proxy._topology(self, obj)
        placement = obj.FormPlacement
        transformed = []
        for point in vertices:
            value = placement.multVec(App.Vector(*point))
            transformed.append((value.x, value.y, value.z))
        return transformed, faces

    def execute(self, obj):
        if obj.CageMode == "Parametric":
            vertices, faces = self._topology(obj)
            obj.ControlPoints = [App.Vector(*point) for point in vertices]
            obj.ControlFaces = [" ".join(str(index) for index in face) for face in faces]
            obj.VertexSharpness = [0.0] * len(vertices)
            obj.EdgeSharpness = []
            obj.LocalEdgeInserts = []
            obj.LocalControlPoints = []
            obj.TMeshData = ""
            obj.DissolvedEdges = []
        if str(obj.FormType) == "Forms::Pipe":
            update_pipe_shape(obj)
        else:
            update_object_shape(obj)
        form_shape = obj.Shape
        obj.FormShape = form_shape
        if "AddSubShape" in obj.PropertiesList:
            obj.AddSubShape = form_shape
        if obj.BaseFeature is not None:
            # Boolean history created below belongs to this Part Design
            # feature. Retag a private copy so generated/modified element
            # names record this object's ID without changing BaseFeature.
            base = obj.BaseFeature.Shape.copy()
            base.Tag = obj.ID
        else:
            base = Part.Shape()
        operation = _partdesign_operation(obj)
        if base.isNull():
            obj.CombinationStatus = App.Qt.translate(
                "Forms_PartDesign", "Form only; no preceding feature"
            )
            return
        if obj.EditingForm:
            # Keep the editable tool as the selectable shape. Its faces are
            # hidden by the view provider while Part Design renders AddSubShape
            # through its native preview framework.
            obj.Shape = form_shape
            obj.CombinationStatus = App.Qt.translate(
                "Forms_PartDesign", "Editing form preview"
            )
            return
        try:
            tool = (
                fused_pipe_shape(form_shape)
                if str(obj.FormType) == "Forms::Pipe"
                else _cap_matched_form(obj, form_shape)
            )
            if "AddSubShape" in obj.PropertiesList:
                # Downstream Part Design patterns consume this native tool shape.
                obj.AddSubShape = tool
            if tool.ShapeType != "Solid":
                obj.Shape = Part.makeCompound([base, form_shape])
                obj.CombinationStatus = App.Qt.translate(
                    "Forms_PartDesign", "Open form; match or thicken it before combining"
                )
                return
            result = base.fuse(tool) if operation == "Additive" else base.cut(tool)
            if result.isNull() or not result.isValid() or len(result.Solids) != 1:
                obj.Shape = Part.makeCompound([base, tool])
                obj.CombinationStatus = App.Qt.translate(
                    "Forms_PartDesign", "Form does not produce a valid single-solid result"
                )
                return
            obj.Shape = result
            if operation == "Additive":
                obj.CombinationStatus = App.Qt.translate(
                    "Forms_PartDesign", "Valid fused additive form"
                )
            else:
                obj.CombinationStatus = App.Qt.translate(
                    "Forms_PartDesign", "Valid cut subtractive form"
                )
        except (Part.OCCError, RuntimeError, ValueError) as error:
            obj.Shape = Part.makeCompound([base, form_shape])
            message = App.Qt.translate("Forms_PartDesign", "Not combined: %1")
            obj.CombinationStatus = message.replace("%1", str(error))

    def onChanged(self, obj, prop):
        proxy = _primitive_proxy(obj) if "FormType" in obj.PropertiesList else None
        if prop == "CageMode" and proxy is not None:
            read_only = 0 if obj.CageMode == "Parametric" else 1
            for name in proxy.ParameterNames:
                obj.setEditorMode(name, read_only)

    def onDocumentRestored(self, obj):
        self._ensure_conversion_properties(obj)
        self._ensure_symmetry_properties(obj)
        self._ensure_sharpness_properties(obj)
        self._ensure_local_edit_properties(obj)
        self._ensure_partdesign_properties(obj, _partdesign_operation(obj))
        # Edit mode itself is not restored by FreeCAD. Do not leave a recovered
        # document displaying the temporary, uncombined tool shape.
        obj.EditingForm = False
        obj.Proxy = self
        self.onChanged(obj, "CageMode")
        obj.touch()
        obj.recompute()


class AdditiveFormProxy(PartDesignFormProxy):
    """Backward-compatible additive Part Design Form proxy."""

    def __init__(self, obj, primitive, base_feature=None, path_object=None):
        super().__init__(obj, primitive, base_feature, "Additive", path_object)


class SubtractiveFormProxy(PartDesignFormProxy):
    """Subtractive Part Design Form proxy."""

    def __init__(self, obj, primitive, base_feature=None, path_object=None):
        super().__init__(obj, primitive, base_feature, "Subtractive", path_object)


class ViewProviderPartDesignForm(ViewProviderFormBox):
    def __init__(self, view_object):
        super().__init__(view_object)

    def getIcon(self):
        view_object = getattr(self, "ViewObject", None)
        obj = getattr(view_object, "Object", None)
        primitive = str(getattr(obj, "PrimitiveKind", ""))
        if primitive not in PRIMITIVES:
            primitive = "Box"
        operation = _partdesign_operation(obj) if obj is not None else "Additive"
        return f":/icons/PartDesign_{operation}Form{primitive}.svg"

    def claimChildren(self):
        obj = getattr(getattr(self, "ViewObject", None), "Object", None)
        path = getattr(obj, "PathObject", None) if obj is not None else None
        if path is None:
            return []
        return [path]

    def _show_only_edited_body_feature(self, view_object):
        """Show the editable tool with the native Part Design preview color."""
        obj = view_object.Object
        body = obj.getParentGeoFeatureGroup()
        if body is None or not body.isDerivedFrom("PartDesign::Body"):
            return super()._show_only_edited_body_feature(view_object)

        self._visibility_before_edit = [
            (feature, bool(feature.ViewObject.Visibility))
            for feature in body.Group
            if hasattr(feature, "ViewObject")
        ]
        self._transparency_before_edit = int(view_object.Transparency)
        self._shape_color_before_edit = tuple(view_object.ShapeColor)[:3]
        preview_color = tuple(getattr(view_object, "PreviewColor", view_object.ShapeColor))
        view_object.ShapeColor = preview_color[:3]
        # The standard Part Design preview is intentionally unpickable and very
        # transparent. Forms instead displays the real tool shape so its faces,
        # edges, and vertices remain available to the editor.
        view_object.Transparency = 0

        if obj.BaseFeature is not None:
            view_object.showPreviousFeature(True)
            # Part Design permits an editing feature to remain visible beside
            # its predecessor. Use the normal live view provider so topology
            # and placement changes are reflected immediately.
            view_object.Visibility = True
        else:
            view_object.Visibility = True

    def _restore_body_feature_visibility(self):
        view_object = getattr(self, "ViewObject", None)
        if view_object is not None:
            if hasattr(self, "_transparency_before_edit"):
                view_object.Transparency = self._transparency_before_edit
                del self._transparency_before_edit
            if hasattr(self, "_shape_color_before_edit"):
                view_object.ShapeColor = self._shape_color_before_edit
                del self._shape_color_before_edit
        super()._restore_body_feature_visibility()

    def setEdit(self, view_object, mode):
        if mode != 0:
            return False
        obj = view_object.Object
        obj.EditingForm = True
        obj.Document.recompute()
        try:
            return super().setEdit(view_object, mode)
        except Exception:
            obj.EditingForm = False
            obj.Document.recompute()
            raise

    def unsetEdit(self, view_object, mode):
        result = super().unsetEdit(view_object, mode)
        if mode == 0:
            obj = view_object.Object
            obj.EditingForm = False
            obj.Document.recompute()
        return result


class ViewProviderAdditiveForm(ViewProviderPartDesignForm):
    """Backward-compatible additive Form view provider."""


class ViewProviderSubtractiveForm(ViewProviderPartDesignForm):
    """Subtractive Form view provider."""


def create_partdesign_form(
    body,
    base_feature,
    primitive="Box",
    name=None,
    placement=None,
    operation="Additive",
    path_object=None,
):
    """Create an additive or subtractive Forms primitive inside *body*."""
    if primitive not in PRIMITIVES:
        raise ValueError(f"Unsupported Part Design Form primitive: {primitive}")
    if operation not in OPERATIONS:
        raise ValueError(f"Unsupported Part Design Form operation: {operation}")
    name = name or f"{operation}Form{primitive}"
    feature_type = f"PartDesign::Feature{operation}Python"
    obj = body.newObject(feature_type, name)
    label_template = App.Qt.translate("Forms_Create", "%1 Form %2")
    operation_label = (
        App.Qt.translate("Forms_Create", "Additive")
        if operation == "Additive"
        else App.Qt.translate("Forms_Create", "Subtractive")
    )
    primitive_labels = {
        "Box": App.Qt.translate("Forms_Create", "Box"),
        "Cylinder": App.Qt.translate("Forms_Create", "Cylinder"),
        "Sphere": App.Qt.translate("Forms_Create", "Sphere"),
        "Quadball": App.Qt.translate("Forms_Create", "Quadball"),
        "Pipe": App.Qt.translate("Forms_Create", "Pipe"),
        "Face": App.Qt.translate("Forms_Create", "Face"),
        "Torus": App.Qt.translate("Forms_Create", "Torus"),
        "Tube": App.Qt.translate("Forms_Create", "Tube"),
    }
    primitive_label = primitive_labels[primitive]
    obj.Label = label_template.replace("%1", operation_label).replace(
        "%2", primitive_label
    )
    proxy_type = AdditiveFormProxy if operation == "Additive" else SubtractiveFormProxy
    proxy_type(obj, primitive, base_feature, path_object)
    if placement is not None:
        obj.FormPlacement = placement
    if App.GuiUp:
        view_provider_type = (
            ViewProviderAdditiveForm
            if operation == "Additive"
            else ViewProviderSubtractiveForm
        )
        view_provider_type(obj.ViewObject)
    obj.recompute()
    return obj


def create_additive_form(
    body, base_feature, primitive="Box", name=None, placement=None, path_object=None
):
    """Create an additive Forms primitive inside *body*."""
    return create_partdesign_form(
        body, base_feature, primitive, name, placement, "Additive", path_object
    )


def create_subtractive_form(
    body, base_feature, primitive="Box", name=None, placement=None, path_object=None
):
    """Create a subtractive Forms primitive inside *body*."""
    return create_partdesign_form(
        body, base_feature, primitive, name, placement, "Subtractive", path_object
    )


def move_form_to_body(source, body):
    """Replace a standalone Forms primitive with an additive Body feature."""
    if source is None or body is None:
        raise ValueError("A Form and destination Body are required")
    if source.Document is not body.Document:
        raise ValueError("The Form and destination Body must be in the same document")
    parent = source.getParentGeoFeatureGroup()
    if parent is not None and parent.isDerivedFrom("PartDesign::Body"):
        raise ValueError("Only a standalone Form can be moved into a Body")

    primitive = str(getattr(source, "FormType", "")).removeprefix("Forms::")
    if primitive not in PRIMITIVES:
        raise ValueError(f"Unsupported additive Form primitive: {primitive}")

    document = source.Document
    base_feature = body.Tip
    source_name = source.Name
    source_label = source.Label
    source_mode = str(source.CageMode)
    relative_placement = global_placement(body).inverse() * global_placement(source)
    parameter_names = PRIMITIVES[primitive][0].ParameterNames

    copied = {}
    for name in (
        *parameter_names,
        "BRepTolerance",
        "MaxRefinement",
        "Symmetric",
        "SymmetryPlane",
        "VertexSharpness",
        "EdgeSharpness",
        "LocalEdgeInserts",
        "TMeshData",
        "DissolvedEdges",
        "ControlFaces",
        "MatchSupport",
        "MatchBoundary",
        "MatchContinuity",
        "MatchTangentMode",
        "MatchParameters",
        "MatchCornerVertices",
        "MatchCornerEdges",
        "SegmentDiameters",
        "SegmentSamples",
    ):
        if name in source.PropertiesList:
            copied[name] = getattr(source, name)
    source_points = [App.Vector(point) for point in source.ControlPoints]
    local_points = [App.Vector(point) for point in getattr(source, "LocalControlPoints", ())]
    view_values = {}
    if App.GuiUp:
        for name in ("ShapeColor", "LineColor", "PointColor", "Transparency"):
            if name in source.ViewObject.PropertiesList:
                view_values[name] = getattr(source.ViewObject, name)

    result = create_additive_form(
        body,
        base_feature,
        primitive,
        name=f"AdditiveForm{primitive}",
        placement=relative_placement,
        path_object=getattr(source, "PathObject", None),
    )
    for name, value in copied.items():
        if name in result.PropertiesList:
            setattr(result, name, value)

    if source_mode == "Editable":
        # Editable cages already contain their final primitive geometry. Bake
        # the standalone object's global placement into Body-local controls.
        result.CageMode = "Editable"
        result.FormPlacement = App.Placement()
        result.ControlPoints = [relative_placement.multVec(point) for point in source_points]
        result.LocalControlPoints = [relative_placement.multVec(point) for point in local_points]
    else:
        result.CageMode = "Parametric"
        result.FormPlacement = relative_placement

    if App.GuiUp:
        for name, value in view_values.items():
            if name in result.ViewObject.PropertiesList:
                setattr(result.ViewObject, name, value)
        import FreeCADGui as Gui

        Gui.Selection.clearSelection()
    document.removeObject(source_name)
    result.Label = source_label
    body.Tip = result
    document.recompute()
    if App.GuiUp:
        Gui.Selection.addSelection(result)
    return result
