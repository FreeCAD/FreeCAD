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

"""Export editable Forms cages and general shapes through headless Blender."""

import json
import math
import os
from pathlib import Path
import shutil
import tempfile

import FreeCAD as App

from .blender_bridge import BlenderBridgeError, run_blender_script
from .cage import ControlCage
from .placement import global_placement


class BlendExportError(RuntimeError):
    """Selected FreeCAD objects could not be written as a Blender document."""


def _object_color(obj):
    if not App.GuiUp:
        return [0.8, 0.8, 0.8, 1.0]
    try:
        color = obj.ViewObject.ShapeColor
        transparency = float(obj.ViewObject.Transparency) / 100.0
        return [float(color[0]), float(color[1]), float(color[2]), 1.0 - transparency]
    except (AttributeError, IndexError, TypeError, ValueError):
        return [0.8, 0.8, 0.8, 1.0]


def _is_body(obj):
    try:
        return obj.isDerivedFrom("PartDesign::Body")
    except (AttributeError, RuntimeError):
        return False


def _can_export_cage(obj):
    if _is_body(obj) or not str(getattr(obj, "FormType", "")).startswith("Forms::"):
        return False
    if any(
        getattr(obj, name, None)
        for name in ("TMeshData", "LocalEdgeInserts", "DissolvedEdges")
    ):
        return False
    try:
        return all(len(face) == 4 for face in ControlCage.from_object(obj).faces)
    except (AttributeError, TypeError, ValueError):
        return False


def _form_payload(obj):
    cage = ControlCage.from_object(obj)
    placement = global_placement(obj)
    vertices = []
    for point in cage.vertices:
        transformed = placement.multVec(App.Vector(*point))
        vertices.append([float(transformed.x), float(transformed.y), float(transformed.z)])
    return {
        "kind": "SUBDIVISION",
        "name": str(obj.Label or obj.Name),
        "vertices": vertices,
        "faces": [list(face) for face in cage.faces],
        "edge_sharpness": [
            [edge[0], edge[1], float(value)]
            for edge, value in sorted(cage.edge_sharpness.items())
        ],
        "vertex_sharpness": [float(value) for value in cage.vertex_sharpness],
        "color": _object_color(obj),
    }


def _shape_payload(obj, deflection):
    shape = getattr(obj, "Shape", None)
    if shape is None or shape.isNull():
        raise BlendExportError(f"{obj.Label}: the selected object has no shape")
    try:
        shape = shape.copy(False, True)
    except TypeError:
        shape = shape.copy()
    shape.Placement = global_placement(obj)
    try:
        vertices, triangles = shape.tessellate(deflection)
    except (RuntimeError, ValueError) as error:
        raise BlendExportError(f"{obj.Label}: its shape could not be tessellated") from error
    if not vertices or not triangles:
        raise BlendExportError(f"{obj.Label}: tessellation produced no faces")
    return {
        "kind": "MESH",
        "name": str(obj.Label or obj.Name),
        "vertices": [
            [float(point.x), float(point.y), float(point.z)] for point in vertices
        ],
        "faces": [[int(index) for index in face] for face in triangles],
        "color": _object_color(obj),
    }


def build_payload(objects, deflection=None):
    """Build the Blender-neutral payload for selected document objects."""
    if deflection is None:
        deflection = App.ParamGet(
            "User parameter:BaseApp/Preferences/Mod/Forms"
        ).GetFloat("BlenderLinearDeflection", 0.1)
    try:
        deflection = float(deflection)
    except (TypeError, ValueError) as error:
        raise BlendExportError("The Blender tessellation deflection is invalid") from error
    if not math.isfinite(deflection) or deflection <= 0.0:
        raise BlendExportError("The Blender tessellation deflection must be positive")

    exported = []
    rejected = []
    for obj in objects:
        try:
            exported.append(
                _form_payload(obj)
                if _can_export_cage(obj)
                else _shape_payload(obj, deflection)
            )
        except (BlendExportError, AttributeError, RuntimeError, TypeError, ValueError) as error:
            rejected.append(str(error))
    if not exported:
        details = "\n".join(rejected) or "No objects were selected"
        raise BlendExportError(f"No selected object can be exported:\n{details}")
    return {
        "format": "AstoCAD Blender export",
        "version": 1,
        "objects": exported,
        "rejected": rejected,
    }


def _run_blender(payload, filename, executable=None, timeout=180):
    exporter = Path(__file__).with_name("blender_create.py")
    destination = Path(filename)
    if not destination.parent.is_dir():
        raise BlendExportError("The destination directory does not exist")
    with tempfile.TemporaryDirectory(prefix="astocad-blend-") as temporary:
        temporary = Path(temporary)
        input_file = temporary / "freecad-objects.json"
        output_file = temporary / "export.blend"
        input_file.write_text(json.dumps(payload, separators=(",", ":")), encoding="utf-8")
        try:
            run_blender_script(
                exporter,
                (input_file, output_file),
                executable=executable,
                timeout=timeout,
                operation="writing the Blender file",
            )
        except BlenderBridgeError as error:
            raise BlendExportError(str(error)) from error
        if not output_file.is_file():
            raise BlendExportError("Blender did not produce a .blend file")
        staging = None
        try:
            descriptor, staging_name = tempfile.mkstemp(
                prefix=f".{destination.name}.", suffix=".tmp", dir=destination.parent
            )
            os.close(descriptor)
            staging = Path(staging_name)
            shutil.copyfile(output_file, staging)
            os.replace(staging, destination)
            staging = None
        except OSError as error:
            raise BlendExportError(f"The Blender file could not be saved: {error}") from error
        finally:
            if staging is not None:
                try:
                    staging.unlink()
                except OSError:
                    pass


def export_file(objects, filename, executable=None):
    """Export *objects* to *filename* and return any skipped-object messages."""
    payload = build_payload(objects)
    _run_blender(payload, filename, executable)
    rejected = payload.get("rejected", [])
    for reason in rejected:
        App.Console.PrintWarning(f"Blender export skipped {reason}\n")
    return rejected


__all__ = ["BlendExportError", "build_payload", "export_file"]
