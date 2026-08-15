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

"""Parametric Forms primitives."""

import math

import FreeCAD as App
import Part

from .box import FormFeatureProxy, ViewProviderFormBox
from .topology import (
    cylinder_control_cage,
    face_control_cage,
    quadball_control_cage,
    sphere_control_cage,
    torus_control_cage,
    tube_control_cage,
)


class FormCylinderProxy(FormFeatureProxy):
    Type = "Forms::Cylinder"
    ParameterNames = ("Radius", "Height", "SideSegments", "HeightSegments")

    def __init__(self, obj):
        self._add_common_properties(obj)
        obj.addProperty("App::PropertyLength", "Radius", "Cylinder", "Cage radius")
        obj.addProperty("App::PropertyLength", "Height", "Cylinder", "Cage height")
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "SideSegments",
            "Cylinder",
            "Segments along each quadrant",
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "HeightSegments",
            "Cylinder",
            "Segments along the cylinder axis",
        )
        obj.Radius = 10.0
        obj.Height = 20.0
        obj.SideSegments = (2, 1, 25, 1)
        obj.HeightSegments = (2, 1, 100, 1)
        self._finish_initialization(obj)

    def _topology(self, obj):
        return cylinder_control_cage(
            obj.Radius.Value, obj.Height.Value, obj.SideSegments, obj.HeightSegments
        )


class FormQuadballProxy(FormFeatureProxy):
    Type = "Forms::Quadball"
    ParameterNames = ("Radius", "Segments")

    def __init__(self, obj):
        self._add_common_properties(obj)
        obj.addProperty("App::PropertyLength", "Radius", "Quadball", "Cage radius")
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "Segments",
            "Quadball",
            "Segments along each cube-sphere direction",
        )
        obj.Radius = 10.0
        obj.Segments = (2, 1, 25, 1)
        self._finish_initialization(obj)

    def _topology(self, obj):
        return quadball_control_cage(obj.Radius.Value, obj.Segments)


class FormSphereProxy(FormFeatureProxy):
    Type = "Forms::Sphere"
    ParameterNames = ("Radius", "LongitudeSegments", "LatitudeSegments")

    def __init__(self, obj):
        self._add_common_properties(obj)
        obj.addProperty("App::PropertyLength", "Radius", "Sphere", "Cage radius")
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "LongitudeSegments",
            "Sphere",
            "Segments along each longitude quadrant",
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "LatitudeSegments",
            "Sphere",
            "Segments between the equator and each pole",
        )
        obj.Radius = 10.0
        obj.LongitudeSegments = (2, 1, 25, 1)
        obj.LatitudeSegments = (2, 1, 25, 1)
        self._finish_initialization(obj)

    def _topology(self, obj):
        return sphere_control_cage(
            obj.Radius.Value, obj.LongitudeSegments, obj.LatitudeSegments
        )

class FormFaceProxy(FormFeatureProxy):
    Type = "Forms::Face"
    ParameterNames = ("Length", "Width", "XSegments", "YSegments")

    def __init__(self, obj):
        self._add_common_properties(obj)
        obj.addProperty(
            "Part::PropertyPartShape",
            "ProfileShape",
            "Face",
            "Exact initial face or closed-wire profile",
        )
        obj.addProperty(
            "App::PropertyVectorList",
            "ProfileControlPoints",
            "Face",
            "Initial profile control points",
        )
        obj.addProperty(
            "App::PropertyStringList",
            "ProfileControlFaces",
            "Face",
            "Initial profile control topology",
        )
        for name in (
            "ProfileShape",
            "ProfileControlPoints",
            "ProfileControlFaces",
        ):
            obj.setEditorMode(name, 2)
        obj.addProperty("App::PropertyLength", "Length", "Face", "Length along X")
        obj.addProperty("App::PropertyLength", "Width", "Face", "Width along Y")
        obj.addProperty("App::PropertyIntegerConstraint", "XSegments", "Face", "Segments along X")
        obj.addProperty("App::PropertyIntegerConstraint", "YSegments", "Face", "Segments along Y")
        obj.Length = 20.0
        obj.Width = 20.0
        obj.XSegments = (2, 1, 100, 1)
        obj.YSegments = (2, 1, 100, 1)
        self._finish_initialization(obj)

    def _topology(self, obj):
        return face_control_cage(obj.Length.Value, obj.Width.Value, obj.XSegments, obj.YSegments)

    def execute(self, obj):
        profile = obj.ProfileShape
        profile_topology = list(obj.ProfileControlFaces)
        if (
            obj.CageMode == "Parametric"
            or profile.isNull()
            or list(obj.ControlFaces) != profile_topology
            or bool(str(getattr(obj, "TMeshData", "") or ""))
            or bool(getattr(obj, "LocalEdgeInserts", ()))
            or bool(getattr(obj, "LocalControlPoints", ()))
            or bool(getattr(obj, "DissolvedEdges", ()))
            or bool(getattr(obj, "EdgeSharpness", ()))
            or any(float(value) > 0.0 for value in getattr(obj, "VertexSharpness", ()))
        ):
            super().execute(obj)
            return
        if _same_control_points(obj.ControlPoints, obj.ProfileControlPoints):
            from .elementmap import map_form_shape

            if bool(getattr(self, "_show_edit_shape", False)):
                try:
                    shape = _segmented_profile_shape(
                        profile, obj.ControlPoints, obj.ControlFaces
                    )
                except ValueError:
                    vertices, faces, support = _validated_profile_control_cage(profile)
                    _write_profile_control_cage(obj, vertices, faces, support)
                    profile = support
                    shape = _segmented_profile_shape(
                        profile, obj.ControlPoints, obj.ControlFaces
                    )
            else:
                shape = profile.copy()
            obj.Shape = map_form_shape(obj, shape, source_shapes=(profile,))
            obj.MaximumDeviation = 0.0
            obj.ConversionLevel = 0
            obj.ConversionStatus = App.Qt.translate("Forms_Conversion", "Valid profile surface")
            return
        super().execute(obj)

    def show_edit_shape(self, obj, enabled):
        """Expose selectable control patches while the Form Face is edited."""
        self._show_edit_shape = bool(enabled)
        obj.touch()
        obj.Document.recompute()

    def onDocumentRestored(self, obj):
        self._show_edit_shape = False
        super().onDocumentRestored(obj)


class FormTorusProxy(FormFeatureProxy):
    Type = "Forms::Torus"
    ParameterNames = ("MajorRadius", "MinorRadius", "MajorSegments", "MinorSegments")

    def __init__(self, obj):
        self._add_common_properties(obj)
        obj.addProperty("App::PropertyLength", "MajorRadius", "Torus", "Centerline radius")
        obj.addProperty("App::PropertyLength", "MinorRadius", "Torus", "Section radius")
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "MajorSegments",
            "Torus",
            "Segments per major quadrant",
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "MinorSegments",
            "Torus",
            "Segments per section quadrant",
        )
        obj.MajorRadius = 15.0
        obj.MinorRadius = 5.0
        obj.MajorSegments = (2, 1, 25, 1)
        obj.MinorSegments = (2, 1, 25, 1)
        self._finish_initialization(obj)

    def _topology(self, obj):
        return torus_control_cage(
            obj.MajorRadius.Value,
            obj.MinorRadius.Value,
            obj.MajorSegments,
            obj.MinorSegments,
        )


class FormTubeProxy(FormFeatureProxy):
    Type = "Forms::Tube"
    ParameterNames = (
        "OuterRadius",
        "InnerRadius",
        "Height",
        "SideSegments",
        "HeightSegments",
    )

    def __init__(self, obj):
        self._add_common_properties(obj)
        obj.addProperty("App::PropertyLength", "OuterRadius", "Tube", "Outer radius")
        obj.addProperty("App::PropertyLength", "InnerRadius", "Tube", "Inner radius")
        obj.addProperty("App::PropertyLength", "Height", "Tube", "Tube height")
        obj.addProperty(
            "App::PropertyIntegerConstraint", "SideSegments", "Tube", "Segments per quadrant"
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint", "HeightSegments", "Tube", "Segments along the axis"
        )
        obj.OuterRadius = 10.0
        obj.InnerRadius = 6.0
        obj.Height = 20.0
        obj.SideSegments = (2, 1, 25, 1)
        obj.HeightSegments = (2, 1, 100, 1)
        self._finish_initialization(obj)

    def _topology(self, obj):
        return tube_control_cage(
            obj.OuterRadius.Value,
            obj.InnerRadius.Value,
            obj.Height.Value,
            obj.SideSegments,
            obj.HeightSegments,
        )


class ViewProviderFormCylinder(ViewProviderFormBox):
    IconName = "Forms_Cylinder.svg"


class ViewProviderFormSphere(ViewProviderFormBox):
    IconName = "Forms_Sphere.svg"


class ViewProviderFormQuadball(ViewProviderFormBox):
    IconName = "Forms_Sphere.svg"


class ViewProviderFormFace(ViewProviderFormBox):
    IconName = "Forms_Face.svg"

    @staticmethod
    def _show_edit_shape(view_object, enabled):
        proxy = getattr(view_object.Object, "Proxy", None)
        if proxy is not None and hasattr(proxy, "show_edit_shape"):
            proxy.show_edit_shape(view_object.Object, enabled)

    def setEdit(self, view_object, mode):
        if mode != 0:
            return False
        self._show_edit_shape(view_object, True)
        try:
            return super().setEdit(view_object, mode)
        except Exception:
            self._show_edit_shape(view_object, False)
            raise

    def unsetEdit(self, view_object, mode):
        try:
            return super().unsetEdit(view_object, mode)
        finally:
            if mode == 0 and view_object.Object.Document is not None:
                self._show_edit_shape(view_object, False)


class ViewProviderFormTorus(ViewProviderFormBox):
    IconName = "Forms_Torus.svg"


class ViewProviderFormTube(ViewProviderFormBox):
    IconName = "Forms_Tube.svg"


def _profile_face(profile):
    if profile is None or profile.isNull():
        raise ValueError("A face or closed wire is required")
    if profile.ShapeType == "Face":
        face = profile
    elif profile.ShapeType == "Wire":
        if not profile.isClosed() or not profile.Edges:
            raise ValueError("The selected wire is not closed")
        face = Part.Face(profile)
    else:
        raise ValueError("Form Face requires a face or closed wire")
    if not face.Wires or any(not wire.isClosed() for wire in face.Wires):
        raise ValueError("The selected profile contains an open wire")
    return face


def _edge_sample_counts(edges, minimum=8):
    """Distribute an even boundary count while retaining curves and junctions."""
    # A curved edge needs an interior sample. Representing it by only its end
    # chord can erase a concavity from the polygonal meshing domain; projecting
    # that chord midpoint back later may then cross neighboring control seams.
    counts = [1 if isinstance(edge.Curve, Part.Line) else 2 for edge in edges]
    target = max(int(minimum), sum(counts))
    if target % 2:
        target += 1
    while sum(counts) < target:
        index = max(range(len(edges)), key=lambda item: edges[item].Length / counts[item])
        counts[index] += 1
    return counts


def _ordered_profile_samples(wire, minimum=8):
    edges = list(wire.OrderedEdges)
    counts = _edge_sample_counts(edges, minimum)
    sampled = [
        [App.Vector(point) for point in edge.discretize(Number=count + 1)]
        for edge, count in zip(edges, counts)
    ]
    if len(sampled) > 1:
        next_points = sampled[1]
        if min(sampled[0][0].sub(point).Length for point in (next_points[0], next_points[-1])) < min(
            sampled[0][-1].sub(point).Length for point in (next_points[0], next_points[-1])
        ):
            sampled[0].reverse()

    result = []
    scale = max(float(wire.BoundBox.DiagonalLength), 1.0)
    tolerance = max(1.0e-7, scale * 1.0e-9)
    for points in sampled:
        if result and result[-1].sub(points[-1]).Length < result[-1].sub(points[0]).Length:
            points.reverse()
        if result:
            if result[-1].sub(points[0]).Length > tolerance:
                raise ValueError("The selected wire contains disconnected edges")
            points = points[1:]
        result.extend(points)
    if result and result[-1].sub(result[0]).Length <= tolerance:
        result.pop()
    if len(result) < 4 or len(result) % 2:
        raise ValueError("A Form Face boundary requires an even number of controls")
    return result


def _profile_mesh_face(profile, boundary_segments):
    polygon_wires = []
    for wire in profile.Wires:
        points = _ordered_profile_samples(wire, boundary_segments)
        polygon_wires.append(Part.makePolygon(points + [points[0]]))
    meshed = Part.makeFace(polygon_wires, "Part::FaceMakerCheese")
    if meshed.isNull() or len(meshed.Faces) != 1:
        raise ValueError("Could not create a mesh domain from the selected profile")
    return meshed.Faces[0]


def _profile_source_edge(support, first, second, tolerance):
    """Return the one source edge containing both boundary controls."""
    candidates = []
    for source_edge in support.Edges:
        first_distance = Part.Vertex(first).distToShape(source_edge)[0]
        second_distance = Part.Vertex(second).distToShape(source_edge)[0]
        candidates.append((max(first_distance, second_distance), source_edge))
    distance, source_edge = min(candidates, key=lambda candidate: candidate[0])
    if distance > tolerance:
        raise ValueError("A profile cage boundary does not lie on one source edge")
    return source_edge


def _profile_edge_parameters(source_edge, first, second):
    """Return the short directed parameter interval between two edge points."""
    first_parameter = float(source_edge.Curve.parameter(first))
    second_parameter = float(source_edge.Curve.parameter(second))
    if source_edge.Curve.isPeriodic():
        period = float(source_edge.Curve.period())
        if abs(second_parameter - first_parameter) > period * 0.5:
            if first_parameter < second_parameter:
                first_parameter += period
            else:
                second_parameter += period
    return first_parameter, second_parameter


def profile_control_cage(profile, boundary_segments=8):
    """Create a hole-aware all-quad cage following a face or closed wire."""
    support = _profile_face(profile)
    mesh_face = _profile_mesh_face(support, boundary_segments)
    mesh_vertices, triangles = mesh_face.tessellate(
        max(float(mesh_face.BoundBox.DiagonalLength) * 0.1, 1.0e-3)
    )
    if not triangles:
        raise ValueError("The selected profile could not be tessellated")

    vertices = [tuple(float(component) for component in point) for point in mesh_vertices]
    edge_counts = {}
    for triangle in triangles:
        for index in range(3):
            edge = tuple(sorted((triangle[index], triangle[(index + 1) % 3])))
            edge_counts[edge] = edge_counts.get(edge, 0) + 1

    tolerance = max(float(support.BoundBox.DiagonalLength) * 1.0e-7, 1.0e-7)
    midpoints = {}
    for edge in sorted(edge_counts):
        first, second = (App.Vector(*vertices[index]) for index in edge)
        point = (first + second) * 0.5
        if edge_counts[edge] == 1:
            source_edge = _profile_source_edge(support, first, second, tolerance)
            parameters = _profile_edge_parameters(source_edge, first, second)
            try:
                point = App.Vector(source_edge.Curve.value(sum(parameters) * 0.5))
            except (Part.OCCError, RuntimeError):
                raise ValueError("Could not project a profile boundary control")
            if not all(math.isfinite(component) for component in point):
                raise ValueError("Could not project a profile boundary control")
        midpoints[edge] = len(vertices)
        vertices.append((point.x, point.y, point.z))

    faces = []
    for triangle in triangles:
        center = sum((App.Vector(*vertices[index]) for index in triangle), App.Vector()) / 3.0
        center_index = len(vertices)
        vertices.append((center.x, center.y, center.z))
        for index, vertex in enumerate(triangle):
            previous = triangle[(index - 1) % 3]
            following = triangle[(index + 1) % 3]
            faces.append(
                (
                    vertex,
                    midpoints[tuple(sorted((vertex, following)))],
                    center_index,
                    midpoints[tuple(sorted((previous, vertex)))],
                )
            )
    return vertices, faces, support


def _segmented_profile_shape(profile, control_points, encoded_faces):
    """Build the cage patches with exact profile curves on their free boundary."""
    support = _profile_face(profile).copy()
    vertices = [App.Vector(point) for point in control_points]
    faces = [tuple(int(index) for index in str(face).split()) for face in encoded_faces]
    edge_counts = {}
    for face in faces:
        for position, first in enumerate(face):
            edge = tuple(sorted((first, face[(position + 1) % len(face)])))
            edge_counts[edge] = edge_counts.get(edge, 0) + 1
    boundary_edges = {edge for edge, count in edge_counts.items() if count == 1}
    tolerance = max(float(support.BoundBox.DiagonalLength) * 1.0e-7, 1.0e-7)

    def exact_boundary_edge(first, second):
        source_edge = _profile_source_edge(support, first, second, tolerance)

        first_parameter, second_parameter = _profile_edge_parameters(
            source_edge, first, second
        )
        lower = min(first_parameter, second_parameter)
        upper = max(first_parameter, second_parameter)
        result = source_edge.Curve.toShape(lower, upper)
        if result.Vertexes[0].Point.sub(first).Length > result.Vertexes[-1].Point.sub(first).Length:
            result.reverse()
        return result

    patches = []
    for face in faces:
        patch_edges = []
        for position, first_index in enumerate(face):
            second_index = face[(position + 1) % len(face)]
            first = vertices[first_index]
            second = vertices[second_index]
            key = tuple(sorted((first_index, second_index)))
            patch_edges.append(
                exact_boundary_edge(first, second)
                if key in boundary_edges
                else Part.makeLine(first, second)
            )
        candidate_wires = []
        try:
            candidate_wires.append(Part.Wire(patch_edges))
        except (Part.OCCError, RuntimeError):
            pass
        try:
            groups = Part.sortEdges(patch_edges)
            if len(groups) == 1 and len(groups[0]) == len(patch_edges):
                sorted_wire = Part.Wire(groups[0])
                if not any(sorted_wire.isSame(wire) for wire in candidate_wires):
                    candidate_wires.append(sorted_wire)
        except (Part.OCCError, RuntimeError):
            pass

        patch = None
        for wire in candidate_wires:
            constructors = (
                lambda wire=wire: Part.Face(support.Surface, wire),
                lambda wire=wire: Part.Face(wire),
                lambda wire=wire: Part.makeFilledFace(list(wire.Edges)),
            )
            for constructor in constructors:
                try:
                    candidate = constructor()
                except (Part.OCCError, RuntimeError, ValueError):
                    continue
                if not candidate.isNull() and candidate.isValid():
                    patch = candidate
                    break
            if patch is not None:
                break
        if patch is None:
            raise ValueError("Could not build an exact profile control patch")
        patches.append(patch)

    area_tolerance = max(float(support.Area) * 1.0e-8, 1.0e-8)
    if abs(sum(patch.Area for patch in patches) - support.Area) > area_tolerance:
        raise ValueError("The exact profile control patches do not cover the profile")
    shell = Part.makeShell(patches)
    if shell.isNull() or not shell.isValid():
        raise ValueError("The exact profile patches do not form a valid shell")
    return shell


def _same_control_points(current, initial, tolerance=1.0e-9):
    return len(current) == len(initial) and all(
        App.Vector(first).sub(second).Length <= tolerance
        for first, second in zip(current, initial)
    )


def _validated_profile_control_cage(profile):
    """Return the sparsest profile cage whose exact edit patches are valid."""
    last_error = None
    for boundary_segments in (8, 16, 32, 64):
        vertices, faces, support = profile_control_cage(profile, boundary_segments)
        encoded_faces = [" ".join(str(index) for index in face) for face in faces]
        try:
            _segmented_profile_shape(support, vertices, encoded_faces)
            return vertices, faces, support
        except ValueError as error:
            last_error = error
    raise ValueError("Could not segment the selected profile exactly") from last_error


def _write_profile_control_cage(obj, vertices, faces, support):
    """Store a validated profile cage without leaving stale local topology."""
    obj.CageMode = "Editable"
    obj.ControlPoints = [App.Vector(*point) for point in vertices]
    obj.ControlFaces = [" ".join(str(index) for index in face) for face in faces]
    obj.VertexSharpness = [0.0] * len(vertices)
    obj.EdgeSharpness = []
    obj.LocalEdgeInserts = []
    obj.LocalControlPoints = []
    obj.TMeshData = ""
    obj.DissolvedEdges = []
    obj.ProfileShape = support
    obj.ProfileControlPoints = list(obj.ControlPoints)
    obj.ProfileControlFaces = list(obj.ControlFaces)


def _create(document, name, label, proxy_class, view_provider_class):
    document = document or App.ActiveDocument
    if document is None:
        raise RuntimeError("A document is required to create a Forms primitive")
    obj = document.addObject("Part::FeaturePython", name)
    obj.Label = label
    proxy_class(obj)
    if App.GuiUp:
        view_provider_class(obj.ViewObject)
    obj.recompute()
    return obj


def create_cylinder(document=None, name="FormCylinder"):
    return _create(
        document,
        name,
        App.Qt.translate("Forms_Create", "Form Cylinder"),
        FormCylinderProxy,
        ViewProviderFormCylinder,
    )


def create_sphere(document=None, name="FormSphere"):
    return _create(
        document,
        name,
        App.Qt.translate("Forms_Create", "Form Sphere"),
        FormSphereProxy,
        ViewProviderFormSphere,
    )


def create_quadball(document=None, name="FormQuadball"):
    return _create(
        document,
        name,
        App.Qt.translate("Forms_Create", "Form Quadball"),
        FormQuadballProxy,
        ViewProviderFormQuadball,
    )


def create_face(document=None, name="FormFace", profile=None):
    obj = _create(
        document,
        name,
        App.Qt.translate("Forms_Create", "Form Face"),
        FormFaceProxy,
        ViewProviderFormFace,
    )
    if profile is not None:
        vertices, faces, support = _validated_profile_control_cage(profile)
        _write_profile_control_cage(obj, vertices, faces, support)
        obj.touch()
        obj.recompute()
    return obj


def create_torus(document=None, name="FormTorus"):
    return _create(
        document,
        name,
        App.Qt.translate("Forms_Create", "Form Torus"),
        FormTorusProxy,
        ViewProviderFormTorus,
    )


def create_tube(document=None, name="FormTube"):
    return _create(
        document,
        name,
        App.Qt.translate("Forms_Create", "Form Tube"),
        FormTubeProxy,
        ViewProviderFormTube,
    )
