# -*- coding: utf-8 -*-
# Utility functions for working with ToolBitShape definitions in FCStd files.

import FreeCAD
import Path.Base.Util as PathUtil
from typing import Dict, List, Any, Optional
import tempfile
import os


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


def find_property_object(doc: "FreeCAD.Document") -> Optional["FreeCAD.DocumentObject"]:
    """
    Find the PropertyBag object named "Attributes" in a document.

    Args:
        doc (FreeCAD.Document): The document to search within.

    Returns:
        Optional[FreeCAD.DocumentObject]: The found object or None.
    """
    # Removed debug logging
    for o in doc.Objects:
        # Check if the object has a Label property and if its value is "Attributes"
        # This seems to be the convention in the shape files.
        if hasattr(o, "Label") and o.Label == "Attributes":
            # We assume this object holds the parameters.
            # Further type checking (e.g., for App::FeaturePython or PropertyBag)
            # could be added if needed, but Label check might be sufficient.
            return o
    return None


def get_object_properties(
    obj: "FreeCAD.DocumentObject", expected_params: List[str]
) -> Dict[str, Any]:
    """
    Extract properties matching expected_params from a FreeCAD PropertyBag.

    Issues warnings for missing parameters but does not raise an error.

    Args:
        obj: The PropertyBag to extract properties from.
        expected_params (List[str]): A list of property names to look for.

    Returns:
        Dict[str, Any]: A dictionary mapping property names to their values.
                        Values are FreeCAD native types.
    """
    properties = {}
    for name in expected_params:
        if hasattr(obj, name):
            properties[name] = getattr(obj, name)
        else:
            # Log a warning if a parameter expected by the shape class is missing
            FreeCAD.Console.PrintWarning(
                f"Parameter '{name}' not found on object '{obj.Label}' "
                f"({obj.Name}). Default value will be used by the shape class.\n"
            )
            properties[name] = None  # Indicate missing value
    return properties


def update_shape_object_properties(
    obj: "FreeCAD.DocumentObject", parameters: Dict[str, Any]
) -> None:
    """
    Update properties of a FreeCAD PropertyBag based on a dictionary of parameters.

    Args:
        obj (FreeCAD.DocumentObject): The PropertyBag to update properties on.
        parameters (Dict[str, Any]): A dictionary of property names and values.
    """
    for name, value in parameters.items():
        if hasattr(obj, name):
            try:
                PathUtil.setProperty(obj, name, value)
            except Exception as e:
                FreeCAD.Console.PrintWarning(
                    f"Failed to set property '{name}' on object '{obj.Label}'"
                    f" ({obj.Name}) with value '{value}': {e}\n"
                )
        else:
            FreeCAD.Console.PrintWarning(
                f"Property '{name}' not found on object '{obj.Label}'" f" ({obj.Name}). Skipping.\n"
            )


def get_doc_state() -> Any:
    """
    Used to make a "snapshot" of the current state of FreeCAD, to allow
    for restoring the ActiveDocument and selection state later.
    """
    doc_name = FreeCAD.ActiveDocument.Name if FreeCAD.ActiveDocument else None
    if FreeCAD.GuiUp:
        import FreeCADGui

        selection = FreeCADGui.Selection.getSelection()
    else:
        selection = []
    return doc_name, selection


def restore_doc_state(state):
    doc_name, selection = state
    if doc_name:
        FreeCAD.setActiveDocument(doc_name)
    if FreeCAD.GuiUp:
        import FreeCADGui

        for sel in selection:
            FreeCADGui.Selection.addSelection(doc_name, sel.Name)


class ShapeDocFromBytes:
    """
    Context manager to create and manage a temporary FreeCAD document,
    loading content from a byte string.
    """

    def __init__(self, content: bytes):
        self._content = content
        self._doc = None
        self._temp_file = None
        self._old_state = None

    def __enter__(self) -> "FreeCAD.Document":
        """Creates a new temporary FreeCAD document or loads cache if provided."""
        # Create a temporary file and write the cache content to it
        with tempfile.NamedTemporaryFile(suffix=".FCStd", delete=False) as tmp_file:
            tmp_file.write(self._content)
            self._temp_file = tmp_file.name

        # When we open a new document, FreeCAD loses the state, of the active
        # document (i.e. current selection), even if the newly opened document
        # is a hidden one.
        # So we need to restore the active document state at the end.
        self._old_state = get_doc_state()

        # Open the document from the temporary file
        # Use a specific name to avoid clashes if multiple docs are open
        # Open the document from the temporary file
        self._doc = FreeCAD.openDocument(self._temp_file, hidden=True)
        if not self._doc:
            raise RuntimeError(f"Failed to open document from {self._temp_file}")
        return self._doc

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        """Closes the temporary FreeCAD document and cleans up the temp file."""
        if self._doc:
            # Note that .closeDocument() is extremely slow; it takes
            # almost 400ms per document - much longer than opening!
            FreeCAD.closeDocument(self._doc.Name)
            self._doc = None

        # Restore the original active document
        restore_doc_state(self._old_state)

        # Clean up the temporary file if it was created
        if self._temp_file and os.path.exists(self._temp_file):
            try:
                os.remove(self._temp_file)
            except Exception as e:
                FreeCAD.Console.PrintWarning(
                    f"Failed to remove temporary file {self._temp_file}: {e}\n"
                )
