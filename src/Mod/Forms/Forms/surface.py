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

"""Editable Forms surface initialized from a Part Design face."""

import math

import FreeCAD as App
import Part

from .box import FormFeatureProxy, ViewProviderFormBox
from .cage import ControlCage


def _linked_face(obj):
    """Return the single face referenced by ``obj.SourceFace``."""
    reference = obj.SourceFace
    if not reference or reference[0] is None:
        raise ValueError("A source face is required")
    source, subelements = reference
    if isinstance(subelements, str):
        subelements = (subelements,)
    if len(subelements) != 1 or not str(subelements[0]).startswith("Face"):
        raise ValueError("SourceFace must reference exactly one face")
    face = source.Shape.getElement(str(subelements[0]))
    if face.isNull() or face.ShapeType != "Face":
        raise ValueError("The selected subelement is not a face")
    return face


def face_control_cage(face, u_segments=2, v_segments=2):
    """Sample a face's UV domain into a regular, editable quad cage."""
    u_segments = int(u_segments)
    v_segments = int(v_segments)
    if u_segments < 1 or v_segments < 1:
        raise ValueError("Surface segment counts must be at least one")

    u_min, u_max, v_min, v_max = face.ParameterRange
    if not all(math.isfinite(value) for value in (u_min, u_max, v_min, v_max)):
        raise ValueError("The selected face has an unbounded parameter range")
    if u_max <= u_min or v_max <= v_min:
        raise ValueError("The selected face has a degenerate parameter range")

    points = []
    for v_index in range(v_segments + 1):
        v_value = v_min + (v_max - v_min) * v_index / v_segments
        for u_index in range(u_segments + 1):
            u_value = u_min + (u_max - u_min) * u_index / u_segments
            point = face.valueAt(u_value, v_value)
            points.append((point.x, point.y, point.z))

    faces = []
    row = u_segments + 1
    reversed_face = str(face.Orientation) == "Reversed"
    for v_index in range(v_segments):
        for u_index in range(u_segments):
            lower_left = v_index * row + u_index
            quad = (
                lower_left,
                lower_left + 1,
                lower_left + row + 1,
                lower_left + row,
            )
            faces.append(tuple(reversed(quad)) if reversed_face else quad)
    return points, faces


def _boundary_indices(cage):
    return {index for edge in cage.boundary_edges for index in edge}


def _support_face(source_shape, source_face, boundary_edge):
    matches = [
        candidate
        for candidate in source_shape.Faces
        if not candidate.isSame(source_face)
        and any(edge.isSame(boundary_edge) for edge in candidate.Edges)
    ]
    return matches[0] if len(matches) == 1 else None


def _filled_face(source_shape, source_face, cage, tangent=False):
    """Create a face fixed to the source boundary and guided by cage interiors."""
    filling = Part.BRepOffsetAPI.MakeFilling()
    filling.loadInitSurface(source_face)
    for edge in source_face.Edges:
        support = _support_face(source_shape, source_face, edge) if tangent else None
        if support is None:
            filling.add(edge, 0)
        else:
            filling.add(edge, support, 1)

    boundary = _boundary_indices(cage)
    interior_points = [
        App.Vector(*point) for index, point in enumerate(cage.vertices) if index not in boundary
    ]
    for point in interior_points:
        filling.add(point)
    filling.build()
    if not filling.isDone():
        raise RuntimeError("OCCT could not fill the Form Surface boundary")

    result = filling.shape()
    if result.isNull() or len(result.Faces) != 1:
        raise RuntimeError("Form Surface filling did not produce one face")
    face = result.Faces[0]
    if str(face.Orientation) != str(source_face.Orientation):
        face = face.reversed()
    return face, interior_points


def _split_solid_face(shape, face, cage, u_segments, v_segments):
    """Partition one boundary face without reopening the already-sewn solid."""
    u_min, u_max, v_min, v_max = face.ParameterRange
    surface = face.Surface
    u_segments = int(u_segments)
    v_segments = int(v_segments)
    split_pairs = []
    for division in range(1, u_segments):
        parameter = u_min + (u_max - u_min) * division / u_segments
        split_pairs.append((surface.uIso(parameter).toShape(v_min, v_max), face))
    for division in range(1, v_segments):
        parameter = v_min + (v_max - v_min) * division / v_segments
        split_pairs.append((surface.vIso(parameter).toShape(u_min, u_max), face))
    if not split_pairs:
        return shape, _replacement_face_map(shape, [(face, cage.faces[0])])

    # Fragment the face in-place after the replacement has already been sewn.
    # Splitting the free face first and then sewing every fragment causes OCCT
    # to lose the closed shell as soon as the filling is no longer planar.
    from BOPTools import SplitAPI

    split_shape = SplitAPI.slice(
        shape,
        [split_edge for split_edge, _target in split_pairs],
        "Standard",
    )
    if len(split_shape.Solids) != 1:
        raise RuntimeError("Form Surface partition did not preserve one solid")
    result = split_shape if split_shape.ShapeType == "Solid" else split_shape.Solids[0]
    if result.isNull() or not result.isValid():
        raise RuntimeError("OCCT rejected the partitioned Form Surface solid")

    expected = u_segments * v_segments
    expected_faces = len(shape.Faces) - 1 + expected
    if len(result.Faces) != expected_faces:
        raise RuntimeError(
            "Form Surface partition produced "
            f"{len(result.Faces)} solid faces instead of {expected_faces}"
        )

    logical_patches = []
    for v_index in range(v_segments):
        v0 = v_min + (v_max - v_min) * v_index / v_segments
        v1 = v_min + (v_max - v_min) * (v_index + 1) / v_segments
        for u_index in range(u_segments):
            u0 = u_min + (u_max - u_min) * u_index / u_segments
            u1 = u_min + (u_max - u_min) * (u_index + 1) / u_segments
            cell = v_index * u_segments + u_index
            logical_patches.append((surface.toShape(u0, u1, v0, v1), cage.faces[cell]))
    return result, _replacement_face_map(result, logical_patches)


def _replace_solid_face(source_shape, source_face, replacements, tolerance):
    """Sew replacement patches to every untouched face of a single solid."""
    if len(source_shape.Solids) != 1:
        raise ValueError("Form Surface requires a Body containing one solid")
    untouched = [face for face in source_shape.Faces if not face.isSame(source_face)]
    if len(untouched) + 1 != len(source_shape.Faces):
        raise ValueError("Could not identify the selected source face")

    sewed = Part.makeCompound(untouched + list(replacements))
    sewed.sewShape(max(float(tolerance), 1.0e-7))
    if len(sewed.Shells) != 1 or not sewed.Shells[0].isClosed():
        raise RuntimeError("The deformed Form Surface did not remain attached to the solid")
    solid = Part.makeSolid(sewed.Shells[0])
    if solid.isNull() or not solid.isValid() or len(solid.Solids) != 1:
        raise RuntimeError("OCCT rejected the solid containing the Form Surface")
    return solid


def _replacement_face_map(shape, patches):
    """Resolve sewn face indices and retain their logical control corners."""
    available = set(range(len(shape.Faces)))
    result = []
    for patch, controls in patches:
        same = [index for index in available if shape.Faces[index].isSame(patch)]
        if same:
            index = same[0]
        else:
            probe = Part.Vertex(
                patch.CenterOfMass.x,
                patch.CenterOfMass.y,
                patch.CenterOfMass.z,
            )
            index = min(
                available,
                key=lambda candidate: shape.Faces[candidate].distToShape(probe)[0],
            )
        available.remove(index)
        result.append((index + 1, tuple(controls)))
    return result


class FormSurfaceProxy(FormFeatureProxy):
    """Part Design feature whose initial cage follows a linked face."""

    Type = "Forms::Surface"
    ParameterNames = ("USegments", "VSegments")

    def __init__(self, obj, source=None, subelement=""):
        self._add_common_properties(obj)
        obj.addProperty(
            "App::PropertyLinkSub",
            "SourceFace",
            "Form Surface",
            "Part Design face replaced by the boundary-constrained Form Surface",
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "USegments",
            "Form Surface",
            "Control faces along the source surface U direction",
        )
        obj.addProperty(
            "App::PropertyIntegerConstraint",
            "VSegments",
            "Form Surface",
            "Control faces along the source surface V direction",
        )
        obj.addProperty(
            "App::PropertyIntegerList",
            "FormSurfaceFaces",
            "Form Surface",
            "Generated solid faces controlled by the Form cage",
        )
        obj.setEditorMode("FormSurfaceFaces", 2)
        obj.addProperty(
            "App::PropertyStringList",
            "FormSurfaceFaceMap",
            "Form Surface",
            "Generated solid face and logical cage-corner mapping",
        )
        obj.setEditorMode("FormSurfaceFaceMap", 2)
        obj.addProperty(
            "App::PropertyEnumeration",
            "Continuity",
            "Form Surface",
            "Continuity where the Form Surface meets neighboring solid faces",
        )
        obj.Continuity = ["Connected", "Tangent"]
        obj.Continuity = "Connected"
        obj.USegments = (2, 1, 100, 1)
        obj.VSegments = (2, 1, 100, 1)
        if source is not None and subelement:
            obj.SourceFace = (source, [str(subelement)])
        self._finish_initialization(obj)

    def _topology(self, obj):
        return face_control_cage(_linked_face(obj), obj.USegments, obj.VSegments)

    def execute(self, obj):
        source_shape = None
        try:
            source_face = _linked_face(obj)
            source_shape = obj.SourceFace[0].Shape
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

            cage = ControlCage.from_object(obj)
            replacement, _interior_points = _filled_face(
                source_shape,
                source_face,
                cage,
                str(obj.Continuity) == "Tangent",
            )
            base_shape = _replace_solid_face(
                source_shape,
                source_face,
                [replacement],
                obj.BRepTolerance.Value,
            )
            base_mapping = _replacement_face_map(
                base_shape,
                [(replacement, tuple(range(len(cage.vertices))))],
            )
            base_index = base_mapping[0][0]
            try:
                shape, mapping = _split_solid_face(
                    base_shape,
                    base_shape.Faces[base_index - 1],
                    cage,
                    obj.USegments,
                    obj.VSegments,
                )
                status = App.Qt.translate(
                    "Forms_Conversion", "Valid solid with subdivided form surface"
                )
            except (Part.OCCError, ValueError, RuntimeError) as split_error:
                shape = base_shape
                mapping = base_mapping
                status = App.Qt.translate(
                    "Forms_Conversion", "Valid form surface (unsplit fallback: %1)"
                ).replace("%1", str(split_error))
            obj.Shape = shape
            obj.FormSurfaceFaces = [index for index, _controls in mapping]
            obj.FormSurfaceFaceMap = [
                " ".join(str(value) for value in (index,) + controls) for index, controls in mapping
            ]
            from .elementmap import map_form_shape

            shape = map_form_shape(
                obj,
                shape,
                face_controls=mapping,
                source_shapes=(source_shape,),
            )
            obj.Shape = shape
            obj.MaximumDeviation = 0.0
            obj.ConversionLevel = 0
            obj.ConversionStatus = status
        except (Part.OCCError, ValueError, RuntimeError) as error:
            # Never make the preceding Part Design result disappear when an
            # intermediate deformation cannot be sewn into a valid solid.
            obj.Shape = source_shape if source_shape is not None else Part.Shape()
            obj.FormSurfaceFaces = []
            obj.FormSurfaceFaceMap = []
            obj.MaximumDeviation = 0.0
            obj.ConversionLevel = 0
            obj.ConversionStatus = App.Qt.translate("Forms_Conversion", "Failed: %1").replace(
                "%1", str(error)
            )

    def onDocumentRestored(self, obj):
        super().onDocumentRestored(obj)
        if "FormSurfaceFaces" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyIntegerList",
                "FormSurfaceFaces",
                "Form Surface",
                "Generated solid faces controlled by the Form cage",
            )
        obj.setEditorMode("FormSurfaceFaces", 2)
        if "FormSurfaceFaceMap" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyStringList",
                "FormSurfaceFaceMap",
                "Form Surface",
                "Generated solid face and logical cage-corner mapping",
            )
        obj.setEditorMode("FormSurfaceFaceMap", 2)
        if "Continuity" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "Continuity",
                "Form Surface",
                "Continuity where the Form Surface meets neighboring solid faces",
            )
            obj.Continuity = ["Connected", "Tangent"]
            obj.Continuity = "Connected"


class ViewProviderFormSurface(ViewProviderFormBox):
    IconName = "Forms_Face.svg"


def create_surface(body, source, subelement, name="FormSurface"):
    """Create a Body-contained Form Surface initialized from ``subelement``."""
    if body is None or not body.isDerivedFrom("PartDesign::Body"):
        raise TypeError("A Part Design Body is required")
    if source is None or source.Document != body.Document:
        raise ValueError("The source face must belong to the Body document")
    if source.getParentGeoFeatureGroup() != body:
        raise ValueError("The source face must belong to the active Body")

    obj = body.newObject("PartDesign::FeaturePython", name)
    obj.Label = App.Qt.translate("Forms_Create", "Form Surface")
    FormSurfaceProxy(obj, source, subelement)
    if App.GuiUp:
        ViewProviderFormSurface(obj.ViewObject)
    obj.recompute()
    return obj


__all__ = ["FormSurfaceProxy", "ViewProviderFormSurface", "create_surface", "face_control_cage"]
