# -*- coding: utf8 -*-
"""
Fast and robust GLTF/GLB exporter patch with material name preservation and VarSet metadata injection for FreeCAD
"""

import FreeCAD
import ImportGui
import json
import struct
import os


def traverse_objects_once(objects):
    """Single traversal to collect all objects, materials, and varsets"""

    all_objects = []
    materials = []
    varsets = {}

    stack = [(obj, obj.Label if hasattr(obj, "Label") else "Unknown") for obj in objects]
    visited = set()

    while stack:
        obj, path = stack.pop()
        obj_id = id(obj)

        if obj_id in visited:
            continue
        visited.add(obj_id)

        all_objects.append(obj)

        if hasattr(obj, "Material") and obj.Material and hasattr(obj.Material, "Name"):
            mat_name = obj.Material.Name
            if mat_name not in materials:
                materials.append(mat_name)
        elif (
            hasattr(obj, "ShapeMaterial")
            and obj.ShapeMaterial
            and hasattr(obj.ShapeMaterial, "Name")
        ):
            mat_name = obj.ShapeMaterial.Name
            if mat_name not in materials:
                materials.append(mat_name)

        if hasattr(obj, "Name") and "VarSet" in obj.Name and hasattr(obj, "PropertiesList"):
            parent_name = path.split(".")[-1] if "." in path else path
            if parent_name not in varsets:
                varsets[parent_name] = {}

            for prop in obj.PropertiesList:
                if hasattr(obj, prop):
                    val = getattr(obj, prop)
                    if val is not None:
                        varsets[parent_name][prop] = str(val)

        current_path = f"{path}.{obj.Label}" if hasattr(obj, "Label") else path

        if hasattr(obj, "Group") and obj.Group:
            stack.extend((child, current_path) for child in obj.Group)
        if hasattr(obj, "Objects") and obj.Objects:
            stack.extend((child, current_path) for child in obj.Objects)
        if hasattr(obj, "OutList") and obj.OutList:
            stack.extend((child, current_path) for child in obj.OutList)
        if hasattr(obj, "LinkedObject") and obj.LinkedObject:
            stack.append((obj.LinkedObject, current_path))

    return all_objects, materials, varsets


def modify_gltf_data(gltf_data, materials, varsets):
    """GLTF data modification"""

    if not isinstance(gltf_data, dict):
        return gltf_data

    # Replace material names
    if "materials" in gltf_data and materials:
        for i, material in enumerate(gltf_data["materials"]):
            if i < len(materials) and isinstance(material, dict) and "name" in material:
                material["name"] = materials[i]

    FreeCAD.Console.PrintMessage(f"{varsets}\n")

    # Inject VarSet data
    if "nodes" in gltf_data and varsets:
        for node in gltf_data["nodes"]:
            if isinstance(node, dict) and "name" in node and node["name"] in varsets:
                if "extras" not in node:
                    node["extras"] = {}
                node["extras"].update(varsets[node["name"]])

    return gltf_data


def process_glb_file(filename, gltf_data):
    """GLB file processing"""
    with open(filename, "rb") as f:
        data = f.read()

    json_str = json.dumps(gltf_data, separators=(",", ":")).encode("utf-8")
    json_padding = (4 - (len(json_str) % 4)) % 4
    json_str += b" " * json_padding

    original_json_length = struct.unpack("<I", data[12:16])[0]
    binary_start = (
        20 + original_json_length
    )  # 12 (GLB header) + 8 (JSON chunk header) + JSON length
    binary_data = data[binary_start:]

    total_length = 20 + len(json_str) + len(binary_data)

    with open(filename, "wb") as f:
        f.write(b"glTF")  # glTF header
        f.write(struct.pack("<I", 2))  # Version
        f.write(struct.pack("<I", total_length))  # Total length
        f.write(struct.pack("<I", len(json_str)))  # JSON chunk length
        f.write(b"JSON")  # JSON chunk type
        f.write(json_str)
        if binary_data:
            f.write(binary_data)


def export(objects, filename):
    """GLTF/GLB export with proper material names and metadata"""

    _, materials, varsets = traverse_objects_once(objects)

    export_objects = [obj for obj in objects if hasattr(obj, "Shape") and obj.Shape]

    ImportGui.export(export_objects, filename)

    if not os.path.exists(filename):
        return

    if filename.lower().endswith(".gltf"):
        with open(filename, "r", encoding="utf-8") as f:
            gltf_data = json.load(f)

        gltf_data = modify_gltf_data(gltf_data, materials, varsets)

        with open(filename, "w", encoding="utf-8") as f:
            json.dump(gltf_data, f, indent=2)

    elif filename.lower().endswith(".glb"):
        with open(filename, "rb") as f:
            # Read GLB header
            magic = f.read(4)
            if magic != b"glTF":
                return

            struct.unpack("<I", f.read(4))[0]
            struct.unpack("<I", f.read(4))[0]
            json_length = struct.unpack("<I", f.read(4))[0]
            json_type = f.read(4)

            if json_type != b"JSON":
                return

            json_data = f.read(json_length)
            gltf_data = json.loads(json_data.decode("utf-8"))

        gltf_data = modify_gltf_data(gltf_data, materials, varsets)
        process_glb_file(filename, gltf_data)
