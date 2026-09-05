# SPDX-License-Identifier: LGPL-2.1-or-later
"""Measured curvature matching of the subdivision surface, using cage controls."""

from functools import lru_cache
import math
import re

import FreeCAD as App

from .brep import _make_surface
from .errors import ConversionError
from .boundary import extended_cage
from .controlcage import ControlCage
from .limits import check_sampling
from .topology import catmull_clark_patch_grids


def _numpy():
    try:
        import numpy
    except ImportError as error:
        raise ConversionError("Curvature Match requires NumPy") from error
    return numpy


def surface_jet(surface, u, v):
    """Position and analytic first/second derivatives, in a common coordinate frame."""
    return [tuple(surface.value(u, v))] + [tuple(surface.getDN(u, v, a, b))
                                         for a, b in ((1, 0), (0, 1), (2, 0), (1, 1), (0, 2))]


def curvature_tensor(jet):
    """Return an oriented normal and the shape operator in ambient coordinates."""
    np = _numpy()
    jet = np.asarray(jet, dtype=float)
    if not np.isfinite(jet).all():
        raise ConversionError("Curvature Match encountered non-finite surface derivatives")
    _point, u, v, uu, uv, vv = jet
    normal = np.cross(u, v)
    area = np.linalg.norm(normal)
    if area <= 1e-12 * max(np.linalg.norm(u) * np.linalg.norm(v), 1e-30):
        raise ConversionError("Curvature Match encountered a singular surface tangent plane")
    normal /= area
    tangents = np.stack((u, v))
    metric = tangents @ tangents.T
    dual = np.linalg.solve(metric, tangents)
    second = np.array(((normal @ uu, normal @ uv), (normal @ uv, normal @ vv)))
    return normal, dual.T @ second @ dual


def _uv(side, t):
    return ((t, 0.0), (1.0, t), (1.0 - t, 1.0), (0.0, 1.0 - t))[side]


def _configuration(obj):
    cage = ControlCage.from_object(obj)
    if len(cage.connected_components()) != 1:
        raise ConversionError("Curvature Match requires one connected control cage")
    if (getattr(obj, "TMeshData", "") or getattr(obj, "LocalEdgeInserts", ())
            or getattr(obj, "DissolvedEdges", ()) or any(len(face) != 4 for face in cage.faces)
            or str(getattr(obj, "FormType", "")) == "Forms::Surface"):
        raise ConversionError("Curvature Match currently requires a regular quad control cage")
    level = int(obj.MaxRefinement)
    if level < 3:
        raise ConversionError("Curvature Match requires Max Refinement of at least 3")
    check_sampling(len(cage.faces) + len(cage.boundary_edges), level + 1)
    boundary = set(int(v) for v in obj.MatchBoundary)
    variables = set(boundary)
    # Two interior rings control the boundary's first and second derivatives.
    for _ in range(2):
        previous = set(variables)
        variables.update(v for edge in cage.edge_counts() if previous.intersection(edge) for v in edge)
    if len(variables) > 256 or len(boundary) > 64 or level > 5:
        raise ConversionError("Curvature Match exceeds the solver budget (256 nearby controls, 64 boundary vertices, refinement 5)")
    return cage, tuple(sorted(variables)), level


def _samples(obj, cage, level):
    from .match_support import (_support_shape, _linked_support, _face_point, _wire_point,
                           _edge_contains_points, _neighboring_support_face)
    np = _numpy()
    shape = _support_shape(obj.MatchSupport, obj)
    wire, selected_face, kind = _linked_support(obj.MatchSupport, obj, shape)
    if selected_face is None:
        raise ConversionError("Curvature Match needs a support face or a planar closed wire")
    boundary = tuple(obj.MatchBoundary)
    parameters = list(obj.MatchParameters)
    anchors = {vertex: (_face_point(selected_face, *parameters[2*i:2*i+2]) if kind == "Face"
                        else _wire_point(wire, parameters[i])) for i, vertex in enumerate(boundary)}
    boundary_edges = {tuple(sorted(edge)) for edge in cage.boundary_edges}
    scale = max(float(wire.BoundBox.DiagonalLength), 1e-6)
    result = []
    divisions = max(16, 4 * 2 ** (level - 1))
    for face_id, face in enumerate(cage.faces):
        for side, first in enumerate(face):
            second = face[(side + 1) % 4]
            if tuple(sorted((first, second))) not in boundary_edges:
                continue
            edge = next((edge for edge in wire.OrderedEdges
                         if _edge_contains_points(edge, (anchors[first], anchors[second]),
                                                 max(1e-7, scale * 1e-7))), None)
            if edge is None:
                raise ConversionError("A Match cage edge spans a support corner; add controls at the corner")
            support = selected_face
            if str(obj.MatchTangentMode) == "AdjacentFaces":
                support = _neighboring_support_face(obj.MatchSupport, selected_face, edge, obj, shape)
                if support is None:
                    raise ConversionError("Curvature to adjacent faces requires a support surface across every boundary edge")
            a = edge.Curve.parameter(anchors[first])
            b = edge.Curve.parameter(anchors[second])
            if edge.Curve.isPeriodic():
                period = edge.Curve.period()
                lo, hi = edge.FirstParameter, edge.LastParameter
                if hi - lo >= period - 1e-9:
                    b = a + (b - a + period / 2) % period - period / 2
                else:
                    # A trimmed periodic curve may use the long arc. Keep both
                    # anchors in its actual parameter interval, not its complement.
                    def in_interval(value):
                        value = lo + (value - lo) % period
                        if abs(value - lo - period) < 1e-9:
                            value = lo
                        if value > hi + 1e-9:
                            raise ConversionError("A Match parameter lies outside the support edge")
                        return min(hi, max(lo, value))
                    a, b = in_interval(a), in_interval(b)
            for index in range(divisions + 1):
                t = index / divisions
                point = edge.Curve.value(a + t * (b - a))
                u, v = support.Surface.parameter(point)
                normal, tensor = curvature_tensor(surface_jet(support.Surface, u, v))
                result.append((face_id, *_uv(side, t), np.array(tuple(point)), normal, tensor,
                               max(1.0 / scale, float(np.linalg.norm(tensor, 2))), index % 2 == 0))
    if not result:
        raise ConversionError("No surface boundary was found for Curvature Match")
    return result, scale


def _jets(vertices, faces, edges, corners, level, locations):
    vertices, extended_faces, corners = extended_cage(vertices, faces, corners)
    grids = catmull_clark_patch_grids(vertices, extended_faces, level, dict(edges), corners)
    surfaces = {index: _make_surface(grids[index]) for index in {p[0] for p in locations}}
    return _numpy().array([surface_jet(surfaces[index], u, v) for index, u, v in locations])


@lru_cache(maxsize=2)
def _weights(count, faces, edges, corners, level, variables, locations):
    np = _numpy()
    weights = np.zeros((len(locations), 6, len(variables)))
    # Subdivision and spline interpolation are linear for fixed crease weights.
    # Pack three scalar basis vectors into each geometry evaluation.
    for offset in range(0, len(variables), 3):
        points = np.zeros((count, 3))
        for axis, vertex in enumerate(variables[offset:offset+3]):
            points[vertex, axis] = 1.0
        jets = _jets(points, faces, edges, corners, level, locations)
        for axis in range(min(3, len(variables) - offset)):
            weights[:, :, offset + axis] = jets[:, :, axis]
    weights.setflags(write=False)
    return weights


def _tolerances(obj):
    position = float(getattr(obj, "MatchPositionTolerance", 0.001))
    angle = math.radians(float(getattr(obj, "MatchAngularTolerance", 0.1)))
    curvature = float(getattr(obj, "MatchCurvatureTolerance", 0.01))
    if not all(math.isfinite(v) and v > 0 for v in (position, angle, curvature)):
        raise ConversionError("Match tolerances must be finite and positive")
    return position, angle, curvature


def _errors(jets, samples):
    np = _numpy()
    worst = [0.0, 0.0, 0.0]
    for jet, sample in zip(jets, samples):
        _face, _u, _v, point, normal, tensor, curvature_scale, _fit = sample
        actual_normal, actual_tensor = curvature_tensor(jet)
        cosine = float(actual_normal @ normal)
        if cosine < 0:
            actual_tensor = -actual_tensor
        errors = (np.linalg.norm(jet[0] - point), math.acos(min(1.0, abs(cosine))),
                  np.linalg.norm(actual_tensor - tensor, 2) / curvature_scale)
        worst = [max(a, float(b)) for a, b in zip(worst, errors)]
    return tuple(worst)


def _store_errors(obj, errors):
    obj.MatchPositionError, angle, obj.MatchCurvatureError = errors
    obj.MatchAngularError = math.degrees(angle)
    obj.MatchStatus = (f"G2 checked: gap {errors[0]:.4g} mm, "
                       f"angle {math.degrees(angle):.4g} degrees, "
                       f"curvature error {100 * errors[2]:.4g}%")


def _failure(errors):
    return (f"G2 tolerance was not reached: gap {errors[0]:.4g} mm, "
            f"angle {math.degrees(errors[1]):.4g} degrees, curvature error {100*errors[2]:.4g}%. "
            "Add control rows near the opening or adjust the Match tolerances.")


def solve_curvature_match(obj):
    """Move nearby controls only; reject unachievable matches before publishing them."""
    np = _numpy()
    cage, variables, level = _configuration(obj)
    tolerances = _tolerances(obj)
    samples, scale = _samples(obj, cage, level)
    locations = tuple(tuple(sample[:3]) for sample in samples)
    faces, edges, corners = tuple(cage.faces), tuple(sorted(cage.edge_sharpness.items())), tuple(cage.vertex_sharpness)
    original = _jets(cage.vertices, faces, edges, corners, level, locations)
    errors = _errors(original, samples)
    if all(a <= b for a, b in zip(errors, tolerances)):
        _store_errors(obj, errors)
        return
    weights = _weights(len(cage.vertices), faces, edges, corners, level, variables, locations)
    delta = np.zeros((len(variables), 3))

    def system(jets, jacobian=True):
        rows, values = [], []
        for w, jet, sample in zip(weights, jets, samples):
            _face, _u, _v, target, normal, tensor, curvature_scale, fit = sample
            if not fit:
                continue
            point, u, v, uu, uv, vv = jet
            lengths = [max(np.linalg.norm(d), scale * 1e-8) for d in (u, v)]
            for axis in range(3):
                values.append((point[axis] - target[axis]) / tolerances[0])
                if jacobian:
                    row = np.zeros((len(variables), 3))
                    row[:, axis] = w[0] / tolerances[0]
                    rows.append(row.ravel() * scale)
            for index, tangent in enumerate((u, v)):
                divisor = lengths[index] * tolerances[1]
                values.append(normal @ tangent / divisor)
                if jacobian:
                    gradient = normal - (normal @ tangent) * tangent / lengths[index]**2
                    rows.append((w[index+1, :, None] * gradient).ravel() * scale / divisor)
            for index, (a, b, derivative) in enumerate(((0, 0, uu), (0, 1, uv), (1, 1, vv))):
                first, second = (u, v)[a], (u, v)[b]
                divisor = lengths[a] * lengths[b] * curvature_scale * tolerances[2]
                numerator = normal @ derivative - first @ tensor @ second
                values.append(numerator / divisor)
                if jacobian:
                    row = (w[index+3, :, None] * normal
                           - w[a+1, :, None] * (tensor @ second)
                           - w[b+1, :, None] * (tensor @ first))
                    row -= numerator * (w[a+1, :, None] * first / lengths[a]**2
                                        + w[b+1, :, None] * second / lengths[b]**2)
                    rows.append(row.ravel() * scale / divisor)
        return np.array(values), np.array(rows) if jacobian else None

    for _iteration in range(18):
        jets = original + np.einsum("sdn,nc->sdc", weights, delta * scale)
        residual, matrix = system(jets)
        penalty = 0.01
        matrix = np.vstack((matrix, penalty * np.eye(delta.size)))
        rhs = -np.concatenate((residual, penalty * delta.ravel()))
        step = np.linalg.lstsq(matrix, rhs, rcond=1e-10)[0].reshape(delta.shape)
        maximum = np.max(np.linalg.norm(step, axis=1))
        if maximum > 0.25:
            step *= 0.25 / maximum
        cost = float(residual @ residual)
        for factor in (1.0, 0.5, 0.25, 0.125, 0.0625, 0.03125):
            candidate = delta + factor * step
            test_jets = original + np.einsum("sdn,nc->sdc", weights, candidate * scale)
            test_residual, _ = system(test_jets, False)
            if float(test_residual @ test_residual) < cost:
                delta = candidate
                break
        else:
            break
        errors = _errors(test_jets, samples)
        if all(a <= b for a, b in zip(errors, tolerances)):
            points = list(cage.vertices)
            for vertex, change in zip(variables, delta * scale):
                points[vertex] = tuple(np.array(points[vertex]) + change)
            obj.ControlPoints = [App.Vector(*point) for point in points]
            _store_errors(obj, errors)
            return
    raise ConversionError(_failure(errors))


def validate_curvature_shape(obj, shape):
    """Check the sewn CAD faces on samples between the solver's constraint points."""
    cage, _variables, level = _configuration(obj)
    samples, _scale = _samples(obj, cage, level)
    faces = {}
    for name, element in shape.ElementMap.items():
        match = re.match(r"FormsFace(\d+);", str(name))
        if match:
            faces[int(match.group(1))] = shape.getElement(str(element))
    if any(sample[0] not in faces for sample in samples):
        raise ConversionError("Could not identify the fitted Match boundary patches")
    jets = [surface_jet(faces[index].Surface, u, v) for index, u, v, *_rest in samples]
    errors = _errors(jets, samples)
    if any(a > b for a, b in zip(errors, _tolerances(obj))):
        raise ConversionError(_failure(errors))
    _store_errors(obj, errors)


# Keep the historical scripting imports available after separating implementation modules.
__all__ = [
    "curvature_tensor",
    "extended_cage",
    "solve_curvature_match",
    "surface_jet",
    "validate_curvature_shape",
]
