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
) -> Dict[str, Any]:
    """
    Extract properties matching expected_params from a FreeCAD object.

    Issues warnings for missing parameters but does not raise an error.

    Args:
        obj (FreeCAD.DocumentObject): The object to extract properties from.
        expected_params (List[str]): A list of property names to look for.

    Returns:
        Dict[str, Any]: A dictionary of found properties and their values.
    """
    params = {}
    for name in expected_params:
        if hasattr(obj, name):
            params[name] = getattr(obj, name)
        else:
            # Log a warning if a parameter expected by the shape class is missing
            FreeCAD.Console.PrintWarning(
                f"Parameter '{name}' not found on object '{obj.Label}' "
                f"({obj.Name}). Default value will be used by the shape class.\n"
            )
    return params


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
