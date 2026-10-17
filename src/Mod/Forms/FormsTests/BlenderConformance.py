# SPDX-License-Identifier: LGPL-2.1-or-later
"""Run by Blender, not imported by the FreeCAD test runner."""

import json
import sys
from pathlib import Path

import bpy

source = Path(sys.argv[sys.argv.index("--") + 1])
cases = json.loads(source.read_text(encoding="utf-8"))
results = []
for case in cases:
    mesh = bpy.data.meshes.new("Cage")
    mesh.from_pydata(case["vertices"], [], case["faces"])
    mesh.update()
    obj = bpy.data.objects.new("Form", mesh)
    bpy.context.collection.objects.link(obj)
    edge_attribute = mesh.attributes.new("crease_edge", "FLOAT", "EDGE")
    values = {tuple(edge[:2]): edge[2] for edge in case["edges"]}
    for edge, item in zip(mesh.edges, edge_attribute.data):
        sharpness = values.get(tuple(sorted(edge.vertices)), 0)
        item.value = (min(sharpness, 10) / 10) ** 0.5
    vertex_attribute = mesh.attributes.new("crease_vert", "FLOAT", "POINT")
    for item, value in zip(vertex_attribute.data, case["corners"]):
        item.value = (min(value, 10) / 10) ** 0.5
    modifier = obj.modifiers.new("Subdivision", "SUBSURF")
    modifier.levels = 2
    modifier.use_limit_surface = False
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = obj.evaluated_get(depsgraph)
    output = evaluated.to_mesh()
    results.append([list(vertex.co) for vertex in output.vertices])
    evaluated.to_mesh_clear()
    bpy.data.objects.remove(obj, do_unlink=True)
    bpy.data.meshes.remove(mesh)
source.with_suffix(".out.json").write_text(json.dumps(results), encoding="utf-8")
