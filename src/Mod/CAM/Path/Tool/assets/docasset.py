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

import os
import io
import pathlib
import tempfile
import zipfile
from typing import Dict, List, Any, Mapping, Optional, Tuple, Type
from abc import ABC
import FreeCAD
import Path
import Path.Base.Util as PathUtil
from ..docobject import DetachedDocumentObject
from .asset import Asset, AssetUri
from .serializer import AssetSerializer, DummyAssetSerializer


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


def _get_unset_value_for(attribute_type: str):
    if attribute_type == "App::PropertyLength":
        return FreeCAD.Units.Quantity(0)
    elif attribute_type == "App::PropertyString":
        return ""
    elif attribute_type == "App::PropertyInteger":
        return 0
    return None


def _get_object_properties(
    obj: "FreeCAD.DocumentObject",
    props: List[str] | None = None,
    group: Optional[str] = None,
) -> Dict[str, Any]:
    """
    Extract properties from a FreeCAD PropertyBag.

    Issues warnings for missing parameters but does not raise an error.

    Args:
        obj: The PropertyBag to extract properties from.
        expected_params (List[str]): A list of property names to look for.

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


def _update_object_properties(obj: "FreeCAD.DocumentObject", properties: Dict[str, Any]) -> None:
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


class DocumentAsset(DetachedDocumentObject, Asset, ABC):
    """
    Abstract base class for assets based on FreeCAD Documents
    (i.e. FCStd files).
    """

    _property_object_name: str  # the name of the object holding the properties
    _property_object_group: str  # the group that contains the properties

    def __init__(self, id: str):
        """
        Initialize the asset.

        Args:
            id (str): The unique identifier for the asset.
        """
        DetachedDocumentObject.__init__(self, label=id)
        self.id: str = id
        # Stores default property values loaded from the FCStd file
        self._defaults: Dict[str, Any] = {}
        # Keeps the loaded FreeCAD document content for this instance
        self._data: Optional[bytes] = None

    def __str__(self):
        return self.__class__.__name__

    def __repr__(self):
        props_str = ", ".join(f"{name}={getattr(self, name)}" for name in self.PropertiesList)
        return f"{str(self)}({props_str})"

    def get_id(self) -> str:
        return self.id

    @classmethod
    def _find_property_object(cls, doc: "FreeCAD.Document") -> Optional["FreeCAD.DocumentObject"]:
        """
        Find the PropertyBag object named "Attributes" in a document.

        Args:
            doc (FreeCAD.Document): The document to search within.

        Returns:
            Optional[FreeCAD.DocumentObject]: The found object or None.
        """
        for o in doc.Objects:
            if hasattr(o, "Label") and o.Label == cls._property_object_name:
                return o
        return None

    @classmethod
    def from_bytes(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
        serializer: Type[AssetSerializer],
    ) -> "DocumentAsset":
        """
        Create a DocumentAsset instance from the raw bytes of an FCStd file.

        Args:
            data (bytes): The raw bytes of the .FCStd file content.
            id (str): The unique identifier for the asset.
            dependencies (Optional[Mapping[AssetUri, Any]]): A mapping of
                resolved dependencies. If None, shallow load was attempted.

        Returns:
            DocumentAsset: An instance of the DocumentAsset.

        Raises:
            ValueError: If the data cannot be opened, or if no PropertyBag
                        is found.
            Exception: For other potential FreeCAD errors during loading.
        """
        assert serializer == DummyAssetSerializer, "DocumentAsset supports only native import"

        # Open the data temporarily to get the properties
        with DocFromBytes(data) as temp_doc:
            # Load properties from the temporary document
            props_obj = cls._find_property_object(temp_doc)
            if not props_obj:
                raise ValueError("No 'Attributes' PropertyBag object found in document bytes")
            loaded_props = _get_object_properties(props_obj, group=cls._property_object_group)

            # We log missing properties for now, and do not raise an error.
            # In the future, we may want to raise an error if critical properties
            # are missing.
            expected_props = cls.get_expected_properties()
            missing_props = [
                name
                for name in expected_props
                if name not in loaded_props or loaded_props[name] is None
            ]
            if missing_props:
                Path.Log.error(
                    f"Validation error: Object '{props_obj.Label}' in document {id} "
                    f"is missing properties for {cls.__name__}: {', '.join(missing_props)}."
                    f" In future releases, this file may not load!"
                )
                for prop in missing_props:
                    prop_type = cls.get_property_property_type(prop)
                    loaded_props[prop] = _get_unset_value_for(prop_type)

            # Instantiate the specific subclass with the provided ID
            instance = cls(id=id)
            instance._data = data
            instance._defaults = loaded_props.copy()

            # Add and initialize properties
            for name, (label, prop_type) in cls.schema().items():
                instance.addProperty(prop_type, name, group=cls._property_object_group, doc=label)
                if prop_type == "App::PropertyEnumeration" and name in loaded_props:
                    choices = props_obj.getEnumerationsOfProperty(name)
                    if choices:
                        setattr(instance, name, choices)  # Set enum choices

            # Initialize property values
            for name, value in loaded_props.items():
                setattr(instance, name, value)

            return instance

    def to_bytes(self, serializer: Type[AssetSerializer]) -> bytes:
        """
        Serializes a DocumentAsset object to bytes (e.g., an fcstd file).
        This is required by the Asset interface.
        """
        assert serializer == DummyAssetSerializer, "DocumentAsset supports only native export"

        with DocFromBytes(self._data) as tmp_doc:
            props = self._find_property_object(tmp_doc)
            if not props:
                raise ValueError(f"failed to find property object {self._property_object_name}")

            # Recompute the document with properties applied
            properties = {name: getattr(self, name) for name in self.PropertiesList}
            _update_object_properties(props, properties)
            tmp_doc.recompute()

            # Save the temporary document to a temporary file
            # We cannot use NamedTemporaryFile on Windows, because there
            # doc.saveAs() may not have permission to access the tempfile
            # while the NamedTemporaryFile is open.
            # So we use TemporaryDirectory instead, to ensure cleanup while
            # still having a the temporary file inside it.
            with tempfile.TemporaryDirectory() as thedir:
                temp_file_path = pathlib.Path(thedir, "temp.FCStd")
                tmp_doc.saveAs(str(temp_file_path))
                return temp_file_path.read_bytes()

    @classmethod
    def from_file(cls, filepath: pathlib.Path) -> "DocumentAsset":
        """
        Create a DocumentAsset instance from an FCStd file.

        Reads the file bytes and delegates to from_bytes().

        Args:
            filepath (pathlib.Path): Path to the .FCStd file.

        Returns:
            DocumentAsset: An instance of the appropriate DocumentAsset subclass.

        Raises:
            FileNotFoundError: If the file does not exist.
            ValueError: If the file cannot be opened, or if no PropertyBag
                        is found.
            Exception: For other potential FreeCAD errors during loading.
        """
        if not filepath.exists():
            raise FileNotFoundError(f"file not found: {filepath}")

        try:
            data = filepath.read_bytes()
            # Extract the ID from the filename (without extension)
            theid = filepath.stem
            # Pass an empty dictionary for dependencies when loading from a single file
            instance = cls.from_bytes(data, theid, {}, DummyAssetSerializer)
            return instance
        except (FileNotFoundError, ValueError) as e:
            raise e
        except Exception as e:
            raise RuntimeError(f"Failed to load document {filepath}: {e}")

    @classmethod
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
        """
        Subclasses MAY define the dictionary mapping parameter names to
        translations and FreeCAD property type strings (e.g.,
        'App::PropertyLength').

        The schema defines any parameters that MUST be in the file.
        Any attempt to load a file that does not match the schema
        will cause warning.
        """
        return {}

    @property
    def label(self) -> str:
        """Return a user friendly, translatable display name."""
        raise NotImplementedError

    def reset_properties(self):
        """Reset properties to their default values."""
        for name, value in self._defaults.items():
            setattr(self, name, value)

    def get_property_label(self, prop_name: str) -> str:
        """
        Get the user-facing label for a given property name.
        """
        str_prop_name = str(prop_name)
        entry = self.schema().get(prop_name)
        return entry[0] if entry else str_prop_name

    @classmethod
    def get_property_property_type(cls, prop_name: str) -> str:
        """
        Get the FreeCAD property type string for a given property name.
        """
        return cls.schema()[prop_name][1]

    @classmethod
    def get_expected_properties(cls) -> List[str]:
        """
        Get a list of property names expected by this DocumentAsset based on
        its schema.

        Returns:
            list[str]: List of property names.
        """
        return list(cls.schema().keys())

    def get_thumbnail(self) -> Optional[bytes]:
        """
        Retrieves the thumbnail data for the document in PNG format,
        as embedded in the file.
        """
        if not self._data:
            return None
        with zipfile.ZipFile(io.BytesIO(self._data)) as zf:
            try:
                with zf.open("thumbnails/Thumbnail.png", "r") as tn:
                    return tn.read()
            except KeyError:
                pass
        return None
