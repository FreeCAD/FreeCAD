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

"""Blender-side exporter for editable Forms control cages.

This file is executed by Blender's Python interpreter.  It intentionally has
no FreeCAD imports so Blender installations can run it unchanged.
"""

import json
import math
import sys
import traceback

import bpy


FORMAT_VERSION = 1


def _sharpness(crease):
    """Convert Blender's normalized crease to OpenSubdiv semi-sharpness."""
    crease = max(0.0, float(crease))
    if crease >= 1.0 - 1.0e-6:
        return 10.0
    return min(crease, 1.0) ** 2 * 10.0


def _attribute_values(mesh, name, count):
    attribute = mesh.attributes.get(name)
    if attribute is None or len(attribute.data) != count:
        return [0.0] * count
    return [float(item.value) for item in attribute.data]


def _enabled_modifiers(obj):
    return [modifier for modifier in obj.modifiers if modifier.show_viewport]


def _remove_work_object(work, work_mesh):
    try:
        bpy.data.objects.remove(work, do_unlink=True)
    finally:
        if work_mesh is not None and work_mesh.users == 0:
            bpy.data.meshes.remove(work_mesh)


def _control_mesh(obj, depsgraph):
    modifiers = _enabled_modifiers(obj)
    subdivision = [
        index for index, modifier in enumerate(modifiers) if modifier.type == "SUBSURF"
    ]
    if len(subdivision) > 1:
        raise ValueError(
            "more than one enabled Subdivision Surface modifier is not supported"
        )

    subdivision_index = subdivision[0] if subdivision else None
    if subdivision_index is not None:
        modifier = modifiers[subdivision_index]
        if modifier.subdivision_type != "CATMULL_CLARK":
            raise ValueError("the Subdivision Surface modifier is not Catmull-Clark")
        after = modifiers[subdivision_index + 1 :]
        if after:
            names = ", ".join(item.name for item in after)
            raise ValueError(f"enabled modifiers after Subdivision Surface are unsupported: {names}")
    elif any(modifier.type != "MIRROR" for modifier in modifiers):
        names = ", ".join(
            modifier.name for modifier in modifiers if modifier.type != "MIRROR"
        )
        raise ValueError(
            "a mesh without Catmull-Clark subdivision may only use Mirror modifiers"
            + (f": {names}" if names else "")
        )

    work = obj.copy()
    work_mesh = None
    try:
        work_mesh = obj.data.copy()
        work.data = work_mesh
        bpy.context.scene.collection.objects.link(work)
        keep = subdivision_index if subdivision_index is not None else len(modifiers)
        # The copied modifier objects have different Python identities, so retain
        # the same enabled modifier positions and remove everything else.
        enabled_position = 0
        for modifier in list(work.modifiers):
            if not modifier.show_viewport:
                work.modifiers.remove(modifier)
                continue
            if enabled_position >= keep:
                work.modifiers.remove(modifier)
            enabled_position += 1
        depsgraph.update()
        evaluated = work.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
        if mesh is None:
            raise ValueError("Blender could not evaluate the control mesh")
        return work, evaluated, mesh
    except Exception:
        _remove_work_object(work, work_mesh)
        raise


def _validate_faces(name, vertex_count, faces):
    if not faces:
        raise ValueError("the mesh has no faces")
    if any(len(face) != 4 for face in faces):
        counts = sorted({len(face) for face in faces})
        raise ValueError(f"the control cage must contain only quad faces (found {counts})")
    edge_counts = {}
    boundary_neighbors = {}
    used_vertices = set()
    for face in faces:
        if len(set(face)) != 4 or min(face) < 0 or max(face) >= vertex_count:
            raise ValueError("the control cage contains an invalid face")
        used_vertices.update(face)
        for index, first in enumerate(face):
            second = face[(index + 1) % 4]
            edge = tuple(sorted((first, second)))
            edge_counts[edge] = edge_counts.get(edge, 0) + 1
    if len(used_vertices) != vertex_count:
        raise ValueError("the control cage contains loose vertices")
    if any(count > 2 for count in edge_counts.values()):
        raise ValueError("the control cage contains non-manifold edges")
    for (first, second), count in edge_counts.items():
        if count == 1:
            boundary_neighbors.setdefault(first, set()).add(second)
            boundary_neighbors.setdefault(second, set()).add(first)
    if any(len(neighbors) != 2 for neighbors in boundary_neighbors.values()):
        raise ValueError("the control cage has a branched or open boundary")
    return name


def _extract_object(obj, depsgraph, millimeters_per_unit):
    work, evaluated, mesh = _control_mesh(obj, depsgraph)
    try:
        matrix = obj.matrix_world
        vertices = [
            [
                float(component) * millimeters_per_unit
                for component in (matrix @ vertex.co)
            ]
            for vertex in mesh.vertices
        ]
        reverse = matrix.to_3x3().determinant() < 0.0
        faces = [
            list(reversed(poly.vertices)) if reverse else list(poly.vertices)
            for poly in mesh.polygons
        ]
        _validate_faces(obj.name, len(vertices), faces)

        edge_creases = _attribute_values(mesh, "crease_edge", len(mesh.edges))
        edge_sharpness = []
        for edge, crease in zip(mesh.edges, edge_creases):
            sharpness = _sharpness(crease)
            if sharpness > 0.0:
                edge_sharpness.append(
                    [int(edge.vertices[0]), int(edge.vertices[1]), sharpness]
                )
        vertex_creases = _attribute_values(mesh, "crease_vert", len(mesh.vertices))
        vertex_sharpness = [_sharpness(value) for value in vertex_creases]
        return {
            "name": obj.name,
            "vertices": vertices,
            "faces": faces,
            "edge_sharpness": edge_sharpness,
            "vertex_sharpness": vertex_sharpness,
        }
    finally:
        evaluated.to_mesh_clear()
        work_mesh = work.data
        _remove_work_object(work, work_mesh)


def export(output_path):
    scene = bpy.context.scene
    scale_length = float(scene.unit_settings.scale_length or 1.0)
    millimeters_per_unit = scale_length * 1000.0
    depsgraph = bpy.context.evaluated_depsgraph_get()
    objects = []
    rejected = []
    # Evaluation links and removes a temporary object, so iterate a stable
    # snapshot instead of mutating Blender's RNA collection during traversal.
    for obj in list(scene.objects):
        if obj.type != "MESH":
            continue
        try:
            objects.append(_extract_object(obj, depsgraph, millimeters_per_unit))
        except (AttributeError, RuntimeError, TypeError, ValueError) as error:
            rejected.append(f"{obj.name}: {error}")
    payload = {
        "format": "AstoCAD Forms control cage",
        "version": FORMAT_VERSION,
        "blender_version": bpy.app.version_string,
        "objects": objects,
        "rejected": rejected,
    }
    with open(output_path, "w", encoding="utf-8") as stream:
        json.dump(payload, stream, separators=(",", ":"))


def main():
    try:
        separator = sys.argv.index("--")
        output_path = sys.argv[separator + 1]
        export(output_path)
    except Exception:
        traceback.print_exc()
        raise


if __name__ == "__main__":
    main()
