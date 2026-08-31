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

"""Blender-side writer for AstoCAD Forms and tessellated shapes."""

import json
import math
import sys
import traceback

import bpy


def _crease(sharpness):
    """Convert OpenSubdiv semi-sharpness to Blender's normalized crease."""
    sharpness = max(0.0, float(sharpness))
    return min(1.0, 1.0 - math.pow(2.0, -sharpness)) if sharpness else 0.0


def _material(name, color):
    material = bpy.data.materials.new(f"{name} Material")
    rgba = tuple(float(value) for value in color)
    material.diffuse_color = rgba
    return material


def _add_creases(mesh, data):
    edge_values = {
        tuple(sorted((int(first), int(second)))): _crease(value)
        for first, second, value in data.get("edge_sharpness", ())
    }
    if edge_values:
        attribute = mesh.attributes.new("crease_edge", "FLOAT", "EDGE")
        for edge, item in zip(mesh.edges, attribute.data):
            item.value = edge_values.get(tuple(sorted(edge.vertices)), 0.0)
    vertex_values = [_crease(value) for value in data.get("vertex_sharpness", ())]
    if any(vertex_values):
        attribute = mesh.attributes.new("crease_vert", "FLOAT", "POINT")
        for value, item in zip(vertex_values, attribute.data):
            item.value = value


def _mark_sharp_edges(mesh, angle_degrees=30.0):
    """Split smooth shading across triangle edges sharper than *angle_degrees*."""
    edge_normals = {}
    for polygon in mesh.polygons:
        vertices = list(polygon.vertices)
        for index, first in enumerate(vertices):
            edge = tuple(sorted((first, vertices[(index + 1) % len(vertices)])))
            edge_normals.setdefault(edge, []).append(polygon.normal.copy())
    cosine_limit = math.cos(math.radians(float(angle_degrees)))
    for edge in mesh.edges:
        normals = edge_normals.get(tuple(sorted(edge.vertices)), ())
        edge.use_edge_sharp = (
            len(normals) != 2 or normals[0].dot(normals[1]) < cosine_limit
        )


def _create_object(data, collection):
    name = str(data.get("name") or "FreeCAD Object")
    mesh = bpy.data.meshes.new(f"{name} Mesh")
    mesh.from_pydata(data["vertices"], [], data["faces"])
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    material = _material(name, data.get("color", (0.8, 0.8, 0.8, 1.0)))
    mesh.materials.append(material)
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    if data.get("kind") == "SUBDIVISION":
        _add_creases(mesh, data)
        modifier = obj.modifiers.new("Subdivision Surface", "SUBSURF")
        modifier.subdivision_type = "CATMULL_CLARK"
        modifier.levels = 2
        modifier.render_levels = 2
        obj["FreeCAD_Form"] = True
    else:
        _mark_sharp_edges(mesh)
        obj["FreeCAD_TessellatedShape"] = True
    return obj


def create(input_path, output_path):
    with open(input_path, "r", encoding="utf-8") as stream:
        payload = json.load(stream)
    if payload.get("format") != "AstoCAD Blender export" or payload.get("version") != 1:
        raise ValueError("unsupported AstoCAD Blender export payload")

    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 0.001
    collection = bpy.data.collections.new("FreeCAD Export")
    scene.collection.children.link(collection)
    for data in payload.get("objects", ()):
        _create_object(data, collection)
    if not collection.objects:
        raise ValueError("the export payload contains no objects")
    bpy.ops.wm.save_as_mainfile(filepath=output_path, check_existing=False)


def main():
    try:
        separator = sys.argv.index("--")
        create(sys.argv[separator + 1], sys.argv[separator + 2])
    except Exception:
        traceback.print_exc()
        raise


if __name__ == "__main__":
    main()
