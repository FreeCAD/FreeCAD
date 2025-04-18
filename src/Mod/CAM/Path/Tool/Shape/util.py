# -*- coding: utf-8 -*-
# Utility functions for working with ToolBitShape definitions in FCStd files.

import os
import pathlib
import FreeCAD
from typing import Dict, List, Any, Optional, Tuple


def find_shape_object(doc: "FreeCAD.Document") -> Optional["FreeCAD.DocumentObject"]:
    """
    Find the primary object representing the shape in a document.

    Looks for PartDesign::Body, then Part::Feature. Falls back to the first
    object if no better candidate is found.

    Args:
        doc (FreeCAD.Document): The document to search within.

    Returns:
        Optional[FreeCAD.DocumentObject]: The found object or None.
    """
    obj = None
    # Prioritize Body
    for o in doc.Objects:
        if o.isDerivedFrom("PartDesign::Body"):
            return o
        # Keep track of the first Part::Feature found as a fallback
        if obj is None and o.isDerivedFrom("Part::Feature"):
            obj = o
    if obj:
        return obj
    # Fallback to the very first object if nothing else suitable found
    return doc.Objects[0] if doc.Objects else None


def get_object_properties(
    obj: "FreeCAD.DocumentObject", expected_params: List[str]
) -> Dict[str, Tuple[Any, Optional[str]]]:
    """
    Extract properties matching expected_params and their types from a FreeCAD object.

    Issues warnings for missing parameters but does not raise an error.

    Args:
        obj (FreeCAD.DocumentObject): The object to extract properties from.
        expected_params (List[str]): A list of property names to look for.

    Returns:
        Dict[str, Tuple[Any, Optional[str]]]: A dictionary mapping property names
                                              to a tuple of (value, property_type_string).
                                              property_type_string is None if not found.
    """
    params_with_types = {}
    for name in expected_params:
        if hasattr(obj, name):
            value = getattr(obj, name)
            # Attempt to get the property type string
            try:
                prop_type = obj.getTypeIdOfProperty(name)
            except Exception:
                # Handle cases where getTypeIdOfProperty might fail
                prop_type = None
                FreeCAD.Console.PrintWarning(
                    f"Could not get type for property '{name}' on object '{obj.Label}'"
                )

            params_with_types[name] = value, prop_type
        else:
            # Log a warning if a parameter expected by the shape class is missing
            FreeCAD.Console.PrintWarning(
                f"Parameter '{name}' not found on object '{obj.Label}' "
                f"({obj.Name}). Default value will be used by the shape class.\n"
            )
            params_with_types[name] = None, None  # missing value and type
    return params_with_types


def load_doc_and_get_properties(
    filepath: pathlib.Path, expected_params: List[str]
) -> Tuple["FreeCAD.Document", Dict[str, Any]]:
    """
    Helper to open an FCStd doc, find the shape object, and extract properties.

    Handles file opening, object finding, and property extraction. Closes the
    document before returning extracted properties but returns the doc object
    itself for potential further use before closing by the caller.

    Args:
        filepath (pathlib.Path): Path to the .FCStd file.
        expected_params (List[str]): Parameter names expected by the shape class.

    Returns:
        Tuple[FreeCAD.Document, Dict[str, Any]]: The opened document and a
                                                 dictionary of extracted parameters.

    Raises:
        FileNotFoundError: If the file does not exist.
        ValueError: If the document cannot be opened or no suitable object found.
    """
    if not os.path.exists(filepath):
        raise FileNotFoundError(f"File not found: {filepath}")

    # Important: Open hidden to avoid GUI interference if possible
    doc = FreeCAD.openDocument(str(filepath), Hidden=True)
    if not doc:
        # This case might occur if FreeCAD fails silently, though it usually raises
        raise ValueError(f"Failed to open document: {filepath}")

    shape_obj = find_shape_object(doc)
    if not shape_obj:
        # Close the doc before raising if we can't find the object
        FreeCAD.closeDocument(doc.Name)
        raise ValueError(f"Could not find suitable shape object in {filepath}")

    # Extract properties based on what the specific shape class expects
    params = get_object_properties(shape_obj, expected_params)

    # Return the doc and params; caller is responsible for closing the doc
    return doc, params


def update_shape_object_properties(
    obj: "FreeCAD.DocumentObject", parameters: Dict[str, Any]
) -> None:
    """
    Update properties of a FreeCAD object based on a dictionary of parameters.

    Args:
        obj (FreeCAD.DocumentObject): The object to update properties on.
        parameters (Dict[str, Any]): A dictionary of property names and values.
    """
    for name, value in parameters.items():
        if hasattr(obj, name):
            try:
                setattr(obj, name, value)
            except Exception as e:
                FreeCAD.Console.PrintWarning(
                    f"Failed to set property '{name}' on object '{obj.Label}'"
                    f" ({obj.Name}) with value '{value}': {e}\n"
                )
        else:
            FreeCAD.Console.PrintWarning(
                f"Property '{name}' not found on object '{obj.Label}'"
                f" ({obj.Name}). Skipping.\n"
            )


def _find_file_in_directory(directory: pathlib.Path, file_name: str) -> Optional[pathlib.Path]:
    """Recursively search for the file in a directory."""
    for root, _, files in os.walk(str(directory)):
        if file_name in files:
            return pathlib.Path(os.path.join(root, file_name))
    return None


def _get_shape_search_paths(relative_to_path: Optional[pathlib.Path] = None) -> List[pathlib.Path]:
    """Get the list of directories to search for shape files."""
    paths = []
    if relative_to_path:
        # Search relative to the provided path (e.g., the .fctb file's directory)
        root_path = relative_to_path.parent.parent
        paths.append(root_path / "Shape")

    # Add configured tool shape search paths
    try:
        import Path.Preferences
        # Assuming searchPathsTool returns a list of strings
        paths.extend([pathlib.Path(p) for p in Path.Preferences.searchPathsTool("Shape")])
    except ImportError:
        FreeCAD.Console.PrintWarning(
            "Could not import Path.Preferences. Cannot use configured search paths.\n"
        )
    return paths


def find_shape_file(name: str, relative_to_path: Optional[pathlib.Path] = None) -> Optional[pathlib.Path]:
    """
    Find a tool shape file by name in configured search paths.

    Args:
        name (str): The name of the shape file (e.g., "endmill.fcstd").
        relative_to_path (Optional[pathlib.Path]): Optional path to search relative to.

    Returns:
        Optional[pathlib.Path]: The found file path or None.
    """
    # Check if the name is an absolute path
    if os.path.exists(name):
        return pathlib.Path(name)

    search_paths = _get_shape_search_paths(relative_to_path)

    for search_path in search_paths:
        found_path = _find_file_in_directory(search_path, name)
        if found_path:
            return found_path

    return None
