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

MATCH_CORNER_SHARPNESS = 10.0


def _support_shape(reference, obj=None):
    source = reference[0] if reference else None
    if source is None:
        return None
    # A shallow wrapper retains topology identity for adjacent-face lookup
    # while allowing placement changes without modifying the document property.
    shape = Part.Shape(source.Shape)
    if obj is not None:
        target = global_placement(obj)
        shape.Placement = (target.inverse() * global_placement(source)
                           * source.Placement.inverse() * shape.Placement)
    return shape


def _linked_support(reference, obj=None, shape=None):
    if not reference or reference[0] is None:
        return None, None, None
    source, names = reference
    shape = shape if shape is not None else _support_shape(reference, obj)
    if isinstance(names, str):
        names = (names,)
    names = tuple(str(name) for name in names)
    if len(names) == 1 and names[0].startswith("Face"):
        face = shape.getElement(names[0])
        return face.OuterWire, face, "Face"
    if not names and shape.ShapeType == "Wire":
        wire = shape.Wires[0]
    elif len(names) == 1 and names[0].startswith("Wire"):
        wire = shape.getElement(names[0])
    elif names and all(name.startswith("Edge") for name in names):
        groups = Part.sortEdges([shape.getElement(name) for name in names])
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


def _linked_face(reference, obj=None):
    _target, face, _kind = _linked_support(reference, obj)
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


def _neighboring_support_face(reference, selected_face, support_edge, obj=None, shape=None):
    source = reference[0] if reference else None
    if shape is None:
        shape = _support_shape(reference, obj) if source is not None else None
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


def _boundary_tangent_planes(reference, selected_face, support, boundary, projected, obj=None, shape=None):
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
            _neighboring_support_face(reference, selected_face, edge, obj, shape) if edge is not None else None
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
    support_shape = _support_shape(reference, obj)
    support, tangent_face, support_kind = _linked_support(reference, obj, support_shape)
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
            _boundary_tangent_planes(reference, tangent_face, support, boundary, projected, obj, support_shape)
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
    target, tangent_face, _support_kind = _linked_support(support, obj)
    if target is None:
        raise ValueError("Match support must be one face or one closed wire")
    cage = ControlCage.from_object(obj)
    boundary = _matched_boundary(cage, boundary_edges)
    _validate_match_mode(continuity, tangent_mode, tangent_face)
    preview = SimpleNamespace(
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
    support = _linked_face(obj.MatchSupport, obj)
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


