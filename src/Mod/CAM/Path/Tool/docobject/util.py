# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Path
import Path.Base.Util as PathUtil
from typing import Dict, List, Any, Optional
import tempfile
import os


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


def get_unset_value_for(attribute_type: str):
    if attribute_type == "App::PropertyLength":
        return FreeCAD.Units.Quantity(0)
    elif attribute_type == "App::PropertyString":
        return ""
    elif attribute_type == "App::PropertyInteger":
        return 0
    return None


def get_object_properties(
    obj: "FreeCAD.DocumentObject",
    props: List[str] | None = None,
    group: Optional[str] = None,
) -> Dict[str, Any]:
    """
    Extract properties from a FreeCAD PropertyBag.

    Issues warnings for missing parameters but does not raise an error.

    Args:
        obj: The PropertyBag to extract properties from.
        props (List[str]): A list of property names to look for.

    Returns:
        Dict[str, Any]: A dictionary mapping property names to their values.
                        Values are FreeCAD native types.
    """
    properties = {}
    for name in props or obj.PropertiesList:
        if group and not obj.getGroupOfProperty(name) == group:
            continue
        if hasattr(obj, name):
            properties[name] = getattr(obj, name)
        else:
            properties[name] = None  # Indicate missing value
    return properties


def update_object_properties(obj: "FreeCAD.DocumentObject", properties: Dict[str, Any]) -> None:
    """
    Update properties of a FreeCAD PropertyBag based on a dictionary of properties.

    Args:
        obj (FreeCAD.DocumentObject): The PropertyBag to update properties on.
        properties (Dict[str, Any]): A dictionary of property names and values.
    """
    for name, value in properties.items():
        if hasattr(obj, name):
            try:
                PathUtil.setProperty(obj, name, value)
            except Exception as e:
                Path.Log.warning(
                    f"Failed to set property '{name}' on object '{obj.Label}'"
                    f" ({obj.Name}) with value '{value}': {e}"
                )
        else:
            Path.Log.warning(
                f"Property '{name}' not found on object '{obj.Label}' ({obj.Name}). Skipping."
            )


class DocFromBytes:
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
                Path.Log.warning(f"Failed to remove temporary file {self._temp_file}: {e}")
