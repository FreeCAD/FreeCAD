# -*- coding: utf8 -*-
"""
GLTF Export Extension Module for FreeCAD
This module provides fixes a material naming issue with FreeCAD's default glTF exporter,
as well as exports some useful metadata by parsing VarSets
"""

import FreeCAD
import ImportGui
import json


def collect_all_material_names(objects):
    """Collect unique material names from object hierarchy in discovery order"""

    visited = set()

    def extract_materials(obj):
        obj_id = id(obj)
        if obj_id in visited:
            return
        visited.add(obj_id)

        try:
            for attr_name in dir(obj):
                if attr_name.startswith("__"):
                    continue

                try:
                    attr_value = getattr(obj, attr_name)
                    if callable(attr_value):
                        continue

                    if (
                        attr_name in ["Material", "ShapeMaterial"]
                        and attr_value
                        and hasattr(attr_value, "Name")
                    ):
                        material_name = str(attr_value.Name)
                        if material_name not in found_material_names:
                            found_material_names.append(material_name)

                    elif hasattr(attr_value, "__dict__") and not isinstance(
                        attr_value, (str, int, float, bool, list, tuple, dict)
                    ):
                        extract_materials(attr_value)

                except:
                    continue
        except:
            pass

    for obj in objects:
        extract_materials(obj)


def find_varsets_with_path(objects):
    """Find all VarSet objects and track their full path through the hierarchy"""
    varset_paths = []
    found_varsets = set()

    def build_path(obj, path="", visited=None):
        if visited is None:
            visited = set()

        obj_id = id(obj)
        if obj_id in visited:
            return

        visited.add(obj_id)

        current_path = f"{path}.{obj.Label}" if path else f"objects.{obj.Label}"

        if "VarSet" in obj.Label and hasattr(obj, "PropertiesList"):
            if obj_id not in found_varsets:
                varset_paths.append((obj, current_path))
                found_varsets.add(obj_id)

        try:
            if hasattr(obj, "Group") and obj.Group:
                for sub_obj in obj.Group:
                    build_path(sub_obj, current_path, visited.copy())

            if hasattr(obj, "OutList") and obj.OutList:
                for sub_obj in obj.OutList:
                    build_path(sub_obj, current_path, visited.copy())

            if hasattr(obj, "LinkedObject") and obj.LinkedObject:
                build_path(obj.LinkedObject, current_path, visited.copy())

            if hasattr(obj, "Objects") and obj.Objects:
                for sub_obj in obj.Objects:
                    build_path(sub_obj, current_path, visited.copy())
        except:
            pass

        visited.remove(obj_id)

    for obj in objects:
        build_path(obj)

    return varset_paths


def modify_gltf_data(gltf_data):
    """
    Replace generic material names and inject VarSet data into GLTF nodes
    """
    if not isinstance(gltf_data, dict):
        return gltf_data

    # Replace material names
    try:
        if (
            "materials" in gltf_data
            and isinstance(gltf_data["materials"], list)
            and "found_material_names" in globals()
            and found_material_names
        ):

            for i, material in enumerate(gltf_data["materials"]):
                if (
                    isinstance(material, dict)
                    and "name" in material
                    and i < len(found_material_names)
                    and found_material_names[i]
                ):
                    material["name"] = found_material_names[i]
    except:
        pass

    # Inject VarSet data into nodes
    try:
        if (
            "nodes" in gltf_data
            and isinstance(gltf_data["nodes"], list)
            and "collected_varset_data" in globals()
            and collected_varset_data
        ):

            for node in gltf_data["nodes"]:
                if (
                    isinstance(node, dict)
                    and "name" in node
                    and isinstance(node["name"], str)
                    and node["name"] in collected_varset_data
                ):

                    if "extras" not in node:
                        node["extras"] = {}
                    elif not isinstance(node["extras"], dict):
                        node["extras"] = {}

                    node["extras"].update(collected_varset_data[node["name"]])
    except:
        pass

    return gltf_data


def extract_json_from_glb(filename):
    """
    Extract JSON chunk from GLB file
    """
    import struct

    try:
        with open(filename, "rb") as f:
            # Read GLB header
            magic = f.read(4)
            if magic != b"glTF":
                return None

            version = struct.unpack("<I", f.read(4))[0]
            total_length = struct.unpack("<I", f.read(4))[0]

            # Read JSON chunk header
            json_chunk_length = struct.unpack("<I", f.read(4))[0]
            json_chunk_type = f.read(4)

            if json_chunk_type != b"JSON":
                return None

            json_data = f.read(json_chunk_length)
            return json.loads(json_data.decode("utf-8"))

    except:
        return None


def write_json_to_glb(filename, gltf_data):
    """
    Write modified JSON back to GLB file
    """
    import struct

    try:
        with open(filename, "rb") as f:
            original_data = f.read()

        # Extract binary data (everything after JSON chunk)
        json_str = json.dumps(gltf_data, separators=(",", ":")).encode("utf-8")

        json_padding = (4 - (len(json_str) % 4)) % 4
        json_str += b" " * json_padding

        header_size = 12  # GLB header
        json_chunk_header_size = 8  # JSON chunk header

        original_json_length = struct.unpack("<I", original_data[12:16])[0]

        binary_start = header_size + json_chunk_header_size + original_json_length

        binary_data = original_data[binary_start:]

        new_total_length = header_size + json_chunk_header_size + len(json_str) + len(binary_data)

        with open(filename, "wb") as f:
            f.write(b"glTF")
            f.write(struct.pack("<I", 2))
            f.write(struct.pack("<I", new_total_length))

            f.write(struct.pack("<I", len(json_str)))
            f.write(b"JSON")

            f.write(json_str)

            if binary_data:
                f.write(binary_data)

    except:
        pass


def get_all_objects_recursive(objects):
    """
    Recursively traverse all objects including sub-groups to find all objects.

    Args:
        objects: List of FreeCAD objects to traverse

    Returns:
        List of all objects found (including nested ones)
    """
    all_objects = []

    def traverse_object(obj):
        all_objects.append(obj)

        if hasattr(obj, "Group") and obj.Group:
            for sub_obj in obj.Group:
                traverse_object(sub_obj)

        if hasattr(obj, "OutList") and obj.OutList:
            for sub_obj in obj.OutList:
                traverse_object(sub_obj)

        if hasattr(obj, "LinkedObject") and obj.LinkedObject:
            traverse_object(obj.LinkedObject)

        if hasattr(obj, "Objects") and obj.Objects:
            for sub_obj in obj.Objects:
                traverse_object(sub_obj)

    for obj in objects:
        traverse_object(obj)

    # Remove duplicates while preserving order
    seen = set()
    unique_objects = []
    for obj in all_objects:
        if id(obj) not in seen:
            seen.add(id(obj))
            unique_objects.append(obj)

    return unique_objects


def export(objects, filename):
    """
    GLTF/GLB export extension function with material name preservation.
    """
    processed_objects = preprocess_objects(objects)

    try:
        ImportGui.export(processed_objects, filename)
        postprocess_export(filename)
    except Exception as e:
        raise


def preprocess_objects(objects):
    """
    Preprocess objects and collect material names for GLTF export.
    """
    global found_material_names
    found_material_names = []

    all_objects = get_all_objects_recursive(objects)
    collect_all_material_names(all_objects)

    # Collect VarSet data for GLTF injection
    global collected_varset_data
    collected_varset_data = {}

    varset_paths = find_varsets_with_path(objects)
    for varset_obj, full_path in varset_paths:
        try:
            path_parts = full_path.split(".")
            if len(path_parts) >= 3:
                target_node = path_parts[-2]

                if hasattr(varset_obj, "PropertiesList") and varset_obj.PropertiesList:
                    varset_properties = {}
                    for prop_name in varset_obj.PropertiesList:
                        try:
                            prop_value = getattr(varset_obj, prop_name)
                            if prop_value is not None:
                                varset_properties[prop_name] = str(prop_value)
                        except:
                            continue

                    if varset_properties and target_node:
                        if target_node not in collected_varset_data:
                            collected_varset_data[target_node] = {}
                        collected_varset_data[target_node].update(varset_properties)

        except:
            continue

    return [obj for obj in objects if hasattr(obj, "Shape") and obj.Shape]


def postprocess_export(filename):
    """
    Post-process the exported GLTF/GLB file and rename materials.
    """
    import os
    import json

    if not os.path.exists(filename):
        return

    if filename.lower().endswith(".gltf"):
        try:
            with open(filename, "r", encoding="utf-8") as f:
                gltf_data = json.load(f)

            gltf_data = modify_gltf_data(gltf_data)

            with open(filename, "w", encoding="utf-8") as f:
                json.dump(gltf_data, f, indent=2)

        except:
            pass

    elif filename.lower().endswith(".glb"):
        try:
            gltf_data = extract_json_from_glb(filename)
            if gltf_data:
                gltf_data = modify_gltf_data(gltf_data)
                write_json_to_glb(filename, gltf_data)

        except:
            pass


def insert(filename, docName, merge=False, useLinkGroup=True):
    """
    Import function - delegates to ImportGui.insert
    This maintains compatibility with the original ImportGui interface
    """
    return ImportGui.insert(filename, docName, merge, useLinkGroup)


found_material_names = []
collected_varset_data = {}
