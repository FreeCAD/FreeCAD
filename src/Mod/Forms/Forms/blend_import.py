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

"""Import Blender subdivision control cages as editable Forms features."""

import json
import math
from pathlib import Path
import tempfile

import FreeCAD as App

from .form import create_form
from .blender_bridge import (
    BlenderBridgeError,
    find_blender_executable,
    run_blender_script,
)


class BlendImportError(RuntimeError):
    """A Blender document could not provide a compatible Forms cage."""


def _run_blender(filename, executable=None, timeout=180):
    exporter = Path(__file__).with_name("blender_extract.py")
    with tempfile.TemporaryDirectory(prefix="astocad-blend-") as temporary:
        output = Path(temporary) / "control-cages.json"
        try:
            run_blender_script(
                exporter,
                (output,),
                source_file=filename,
                executable=executable,
                timeout=timeout,
                operation="reading the Blender file",
            )
        except BlenderBridgeError as error:
            raise BlendImportError(str(error)) from error
        if not output.is_file():
            raise BlendImportError("Blender did not produce editable control-cage data")
        try:
            return json.loads(output.read_text(encoding="utf-8"))
        except (OSError, UnicodeError, json.JSONDecodeError) as error:
            raise BlendImportError("Blender produced invalid control-cage data") from error


def _validate_cage(data):
    try:
        vertices = [tuple(float(component) for component in point) for point in data["vertices"]]
        faces = [tuple(int(index) for index in face) for face in data["faces"]]
    except (KeyError, TypeError, ValueError) as error:
        raise BlendImportError("Blender produced malformed control-cage data") from error
    if not vertices or not faces:
        raise BlendImportError("The Blender mesh has no control cage")
    if any(len(point) != 3 for point in vertices):
        raise BlendImportError("The Blender control cage contains an invalid vertex")
    if any(not math.isfinite(component) for point in vertices for component in point):
        raise BlendImportError("The Blender control cage contains a non-finite vertex")
    if any(len(face) != 4 for face in faces):
        raise BlendImportError("Forms import requires an all-quad Blender control cage")
    edge_counts = {}
    used = set()
    for face in faces:
        if len(set(face)) != 4 or min(face) < 0 or max(face) >= len(vertices):
            raise BlendImportError("The Blender control cage contains an invalid face")
        used.update(face)
        for index, first in enumerate(face):
            edge = tuple(sorted((first, face[(index + 1) % 4])))
            edge_counts[edge] = edge_counts.get(edge, 0) + 1
    if len(used) != len(vertices):
        raise BlendImportError("The Blender control cage contains loose vertices")
    if any(count > 2 for count in edge_counts.values()):
        raise BlendImportError("The Blender control cage contains non-manifold edges")
    boundary = {}
    for (first, second), count in edge_counts.items():
        if count == 1:
            boundary.setdefault(first, set()).add(second)
            boundary.setdefault(second, set()).add(first)
    if any(len(neighbors) != 2 for neighbors in boundary.values()):
        raise BlendImportError("The Blender control cage has a branched or open boundary")
    return vertices, faces, edge_counts


def _origin_symmetry(vertices, faces, axis):
    diagonal = sum(
        (max(point[index] for point in vertices) - min(point[index] for point in vertices)) ** 2
        for index in range(3)
    ) ** 0.5
    tolerance = max(diagonal * 1.0e-6, 1.0e-6)
    buckets = {}
    for index, point in enumerate(vertices):
        key = tuple(math.floor(component / tolerance) for component in point)
        buckets.setdefault(key, []).append(index)
    mapped = {}
    for index, point in enumerate(vertices):
        target = list(point)
        target[axis] = -target[axis]
        key = tuple(math.floor(component / tolerance) for component in target)
        candidates = []
        for first_offset in (-1, 0, 1):
            for second_offset in (-1, 0, 1):
                for third_offset in (-1, 0, 1):
                    candidates.extend(
                        buckets.get(
                            (
                                key[0] + first_offset,
                                key[1] + second_offset,
                                key[2] + third_offset,
                            ),
                            (),
                        )
                    )
        if not candidates:
            return False
        nearest, squared_distance = min(
            (
                (
                    candidate,
                    sum(
                        (target[component] - vertices[candidate][component]) ** 2
                        for component in range(3)
                    ),
                )
                for candidate in candidates
            ),
            key=lambda item: item[1],
        )
        if squared_distance > tolerance * tolerance:
            return False
        mapped[index] = nearest
    face_sets = {frozenset(face) for face in faces}
    return all(frozenset(mapped[index] for index in face) in face_sets for face in faces)


def _create_feature(document, source_file, blender_version, data):
    vertices, faces, edges = _validate_cage(data)
    vertex_values = data.get("vertex_sharpness", ())
    try:
        vertex_values = [max(0.0, float(value)) for value in vertex_values]
    except (TypeError, ValueError) as error:
        raise BlendImportError("The Blender control cage has invalid vertex creases") from error
    if len(vertex_values) not in (0, len(vertices)):
        raise BlendImportError("The Blender control cage has incomplete vertex creases")
    if any(not math.isfinite(value) for value in vertex_values):
        raise BlendImportError("The Blender control cage has non-finite vertex creases")

    encoded_edges = []
    for value in data.get("edge_sharpness", ()):
        try:
            first, second, sharpness = int(value[0]), int(value[1]), float(value[2])
        except (IndexError, TypeError, ValueError) as error:
            raise BlendImportError("The Blender control cage has invalid edge creases") from error
        edge = tuple(sorted((first, second)))
        if edge not in edges or sharpness < 0.0 or not math.isfinite(sharpness):
            raise BlendImportError("A Blender edge crease does not reference a control edge")
        if sharpness > 0.0:
            encoded_edges.append(f"{edge[0]} {edge[1]} {sharpness:.12g}")

    name = str(data.get("name") or "Form")
    obj = create_form(document, name)
    try:
        obj.addProperty(
            "App::PropertyFile", "SourceFile", "Import", "Source Blender document"
        )
        obj.addProperty(
            "App::PropertyString", "SourceObject", "Import", "Source Blender object"
        )
        obj.addProperty(
            "App::PropertyString", "BlenderVersion", "Import", "Blender version used to import"
        )
        for property_name in ("SourceFile", "SourceObject", "BlenderVersion"):
            obj.setEditorMode(property_name, 1)
        obj.Label = name
        obj.SourceFile = str(source_file)
        obj.SourceObject = name
        obj.BlenderVersion = str(blender_version)
        obj.ControlPoints = [App.Vector(*point) for point in vertices]
        obj.ControlFaces = [" ".join(str(index) for index in face) for face in faces]
        obj.VertexSharpness = vertex_values or [0.0] * len(vertices)
        obj.EdgeSharpness = encoded_edges

        for axis, plane in ((0, "YZ"), (1, "XZ"), (2, "XY")):
            if _origin_symmetry(vertices, faces, axis):
                obj.SymmetryPlane = plane
                obj.Symmetric = True
                break
        document.recompute()
        if obj.Shape.isNull():
            status = str(getattr(obj, "ConversionStatus", ""))
            raise BlendImportError(
                "The Blender control cage could not be converted to a Form"
                + (f": {status}" if status else "")
            )
        if App.GuiUp:
            obj.ViewObject.ShowControlCage = True
        return obj
    except Exception:
        if obj.Document is document:
            document.removeObject(obj.Name)
        raise


def import_file(filename, document, executable=None):
    """Import every compatible mesh in *filename* into *document*."""
    path = Path(filename)
    if not path.is_file():
        raise BlendImportError(f"Blender file does not exist: {filename}")
    payload = _run_blender(path, executable=executable)
    if payload.get("format") != "AstoCAD Forms control cage" or payload.get("version") != 1:
        raise BlendImportError("Blender produced an unsupported control-cage format")
    objects = payload.get("objects") or []
    if not objects:
        rejected = payload.get("rejected") or []
        details = "\n".join(f"• {reason}" for reason in rejected)
        message = (
            "This Blender file does not contain an editable Forms control cage. "
            "A compatible object must have a manifold, all-quad mesh control cage."
        )
        if details:
            message += f"\n\n{details}"
        raise BlendImportError(message)

    created = []
    try:
        for data in objects:
            created.append(
                _create_feature(
                    document,
                    path,
                    payload.get("blender_version", ""),
                    data,
                )
            )
    except Exception:
        for obj in reversed(created):
            if obj.Document is document:
                document.removeObject(obj.Name)
        raise
    return created, list(payload.get("rejected") or ())
