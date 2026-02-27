# SPDX-License-Identifier: LGPL-2.1-or-later

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

import pathlib
import FreeCAD
import Path
import os
from typing import Dict, List, Any, Mapping, Optional, Tuple, Type, cast
import zipfile
import xml.etree.ElementTree as ET
import io
import tempfile
from ...assets import Asset, AssetUri, AssetSerializer, DummyAssetSerializer
from ...camassets import cam_assets
from ..doc import (
    find_shape_object,
    get_object_properties,
    get_unset_value_for,
    update_shape_object_properties,
    ShapeDocFromBytes,
)
from .icon import ToolBitShapeIcon


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.ERROR, Path.Log.thisModule())


class ToolBitShape(Asset):
    """Abstract base class for tool bit shapes."""

    asset_type: str = "toolbitshape"

    # The name is used...
    #   1. as a base for the default filename. E.g. if the name is
    #      "Endmill", then by default the file is "endmill.fcstd".
    #   2. to identify the shape class from a shape.fcstd file.
    #      Upon loading a shape, the name of the body in the shape
    #      file is read. It MUST match one of the names.
    name: str

    # Aliases exist for backward compatibility. If an existing .fctb file
    # references a shape such as "v-bit.fctb", and that shape file cannot
    # be found, then we can attempt to find a shape class from the string
    # "v-bit", "vbit", etc.
    aliases: Tuple[str, ...] = tuple()

    def __init__(self, id: str, **kwargs: Any):
        """
        Initialize the shape.

        Args:
            id (str): The unique identifier for the shape.
            **kwargs: Keyword arguments for shape parameters (e.g., Diameter).
                      Values should be FreeCAD.Units.Quantity where applicable.
        """
        # _params will be populated with default values after loading
        self._params: Dict[str, Any] = {}

        # Stores default parameter values loaded from the FCStd file
        self._defaults: Dict[str, Any] = {}

        # Stores the FreeCAD property types for each parameter
        self._param_types: Dict[str, str] = {}

        # Keeps the loaded FreeCAD document content for this instance
        self._data: Optional[bytes] = None

        self.id: str = id

        self.icon: Optional[ToolBitShapeIcon] = None

        # Assign parameters
        for param, value in kwargs.items():
            self.set_parameter(param, value)

    def __str__(self):
        params_str = ", ".join(f"{name}={val}" for name, val in self._params.items())
        return f"{self.name}({params_str})"

    def __repr__(self):
        return self.__str__()

    def get_id(self) -> str:
        """
        Get the ID of the shape.

        Returns:
            str: The ID of the shape.
        """
        return self.id

    @property
    def is_builtin(self) -> bool:
        """
        Check if this shape is a built-in shape.

        Returns:
            bool: True if the shape is built-in, False otherwise.
        """
        return self.name.lower() == self.id

    @classmethod
    def _get_shape_class_from_doc(cls, doc: "FreeCAD.Document") -> Type["ToolBitShape"]:
        # Find the Body object to identify the shape type
        body_obj = find_shape_object(doc)
        if not body_obj:
            raise ValueError(f"No 'PartDesign::Body' object found in {doc}")

        # Find the correct subclass based on the body label
        shape_classes = {c.name: c for c in ToolBitShape.__subclasses__()}
        shape_class = shape_classes.get(body_obj.Label)
        if not shape_class:
            custom = ToolBitShape.get_subclass_by_name("Custom")
            assert custom is not None, "BUG: Custom tool class not found"
            return custom
        return shape_class

    @classmethod
    def get_shape_class_from_id(
        cls,
        shape_id: str,
        shape_type: Optional[str] = None,
        default: Optional[Type["ToolBitShape"]] = None,
    ) -> Optional[Type["ToolBitShape"]]:
        """
        Extracts the shape class from the given ID and shape_type, retrieving it
        from the asset manager if necessary.
        """
        # Best method: if the shape-type is specified, use that.
        if shape_type:
            return ToolBitShape.get_subclass_by_name(shape_type)

        # If no shape type is specified, try to find the shape class from the ID.
        shape_class = ToolBitShape.get_subclass_by_name(shape_id)
        if shape_class:
            return shape_class

        # If that also fails, try to load the shape to get the class.
        Path.Log.debug(
            f'Failed to infer shape type from "{shape_id}", trying to load'
            f' the shape "{shape_id}" to determine the class. This may'
            " negatively impact performance."
        )
        shape_asset_uri = ToolBitShape.resolve_name(shape_id)
        try:
            data = cam_assets.get_raw(shape_asset_uri)
        except FileNotFoundError:
            pass  # rely on fallback below
        else:
            try:
                shape_class = ToolBitShape.get_shape_class_from_bytes(data)
            except ValueError:
                pass
            else:
                return shape_class

        # Otherwise use the default, if we have one.
        shape_types = [c.name for c in ToolBitShape.__subclasses__()]
        if default is not None:
            Path.Log.debug(
                f'Failed to infer shape type from {shape_id}, using "{default.name}".'
                f" To fix, name the body in the shape file to one of: {shape_types}"
            )
            return default

        # Default to Custom if nothing else works
        return ToolBitShape.get_subclass_by_name("Custom")

    @classmethod
    def get_shape_class_from_bytes(cls, data: bytes) -> Type["ToolBitShape"]:
        """
        Identifies the ToolBitShape subclass from the raw bytes of an FCStd file
        by parsing the XML content to find the Body label.

        Args:
            data (bytes): The raw bytes of the .FCStd file.

        Returns:
            Type[ToolBitShape]: The appropriate ToolBitShape subclass.

        Raises:
            ValueError: If the data is not a valid FCStd file, Document.xml is
                        missing, no Body object is found, or the Body label
                        does not match a known shape name.
        """
        try:
            # FCStd files are zip archives
            with zipfile.ZipFile(io.BytesIO(data)) as zf:
                # Read Document.xml from the archive
                with zf.open("Document.xml") as doc_xml_file:
                    tree = ET.parse(doc_xml_file)
                    root = tree.getroot()

            # Extract name of the main Body from XML tree using xpath.
            # The body should be a PartDesign::Body, and its label is
            # stored in an Property element with a matching name.
            body_label = None
            xpath = './/Object[@name="Body"]//Property[@name="Label"]/String'
            body_label_elem = root.find(xpath)
            if body_label_elem is not None:
                body_label = body_label_elem.get("value")

            if not body_label:
                raise ValueError(
                    "No 'Label' property found for 'PartDesign::Body' object using XPath"
                )

            # Find the correct subclass based on the body label
            shape_class = cls.get_subclass_by_name(body_label)
            if shape_class:
                return shape_class

            # All else fails, treat the shape as a custom shape.
            custom = ToolBitShape.get_subclass_by_name("Custom")
            assert custom is not None, "BUG: Custom tool class not found"
            return custom

        except zipfile.BadZipFile:
            raise ValueError("Invalid FCStd file data (not a valid zip archive)")
        except KeyError:
            raise ValueError("Invalid FCStd file data (Document.xml not found)")
        except ET.ParseError:
            raise ValueError("Error parsing Document.xml")
        except Exception as e:
            # Catch any other unexpected errors during parsing
            raise ValueError(f"Error processing FCStd data: {e}")

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
            # Check if the object has a Label property and if its value is "Attributes"
            # This seems to be the convention in the shape files.
            if hasattr(o, "Label") and o.Label == "Attributes":
                # We assume this object holds the parameters.
                # Further type checking (e.g., for App::FeaturePython or PropertyBag)
                # could be added if needed, but Label check might be sufficient.
                return o
        return None

    @classmethod
    def extract_dependencies(cls, data: bytes, serializer: Type[AssetSerializer]) -> List[AssetUri]:
        """
        Extracts URIs of dependencies from the raw bytes of an FCStd file.
        For ToolBitShape, this is the associated ToolBitShapeIcon, identified
        by the same ID as the shape asset.
        """
        Path.Log.debug(f"ToolBitShape.extract_dependencies called for {cls.__name__}")
        assert (
            serializer == DummyAssetSerializer
        ), f"ToolBitShape supports only native import, not {serializer}"

        # A ToolBitShape asset depends on a ToolBitShapeIcon asset with the same ID.
        # We need to extract the shape ID from the FCStd data.
        try:
            # Open the shape data temporarily to get the Body label, which can
            # be used to derive the ID if needed, or assume the ID is available
            # in the data somehow (e.g., in a property).
            # For now, let's assume the ID is implicitly the asset name derived
            # from the Body label.
            shape_class = cls.get_shape_class_from_bytes(data)
            shape_id = shape_class.name.lower()  # Assuming ID is lowercase name

            # Construct the URI for the corresponding icon asset
            svg_uri = AssetUri.build(
                asset_type="toolbitshapesvg",
                asset_id=shape_id + ".svg",
            )
            png_uri = AssetUri.build(
                asset_type="toolbitshapepng",
                asset_id=shape_id + ".png",
            )
            return [svg_uri, png_uri]

        except Exception as e:
            # If we can't extract the shape ID or something goes wrong,
            # assume no dependencies for now.
            Path.Log.error(f"Failed to extract dependencies from shape data: {e}")
            return []

    @classmethod
    def from_bytes(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
        serializer: Type[AssetSerializer],
    ) -> "ToolBitShape":
        """
        Create a ToolBitShape instance from the raw bytes of an FCStd file.

        Identifies the correct subclass based on the Body label in the file,
        loads parameters, and caches the document content.

        Args:
            data (bytes): The raw bytes of the .FCStd file.
            id (str): The unique identifier for the shape.
            dependencies (Optional[Mapping[AssetUri, Any]]): A mapping of
                  resolved dependencies. If None, shallow load was attempted.

        Returns:
            ToolBitShape: An instance of the appropriate ToolBitShape subclass.

        Raises:
            ValueError: If the data cannot be opened, no Body or PropertyBag
                        is found, or the Body label does not match a known
                        shape name.
            Exception: For other potential FreeCAD errors during loading.
        """
        assert serializer == DummyAssetSerializer, "ToolBitShape supports only native import"
        Path.Log.debug(f"{id}: ToolBitShape.from_bytes called with {len(data)} bytes")

        # Open the shape data temporarily to get the Body label and parameters
        with ShapeDocFromBytes(data) as temp_doc:
            if not temp_doc:
                # This case might be covered by ShapeDocFromBytes exceptions,
                # but keeping for clarity.
                raise ValueError("Failed to open shape document from bytes")

            # Determine the specific subclass of ToolBitShape.
            try:
                shape_class = ToolBitShape.get_shape_class_from_bytes(data)
            except Exception as e:
                Path.Log.debug(f"{id}: Failed to determine shape class from bytes: {e}")
                shape_class = ToolBitShape.get_shape_class_from_id("Custom")
            if shape_class is None:
                # This should ideally not happen due to get_shape_class_from_bytes fallback
                # but added for linter satisfaction.
                raise ValueError("Shape class could not be determined.")

            # Load properties from the temporary document
            props_obj = ToolBitShape._find_property_object(temp_doc)
            if not props_obj:
                raise ValueError("No 'Attributes' PropertyBag object found in document bytes")

            # loaded_raw_params will now be Dict[str, Tuple[Any, str]]
            loaded_raw_params = get_object_properties(props_obj, exclude_groups=["", "Base"])

            # Separate values and types, and populate _param_types
            loaded_params = {}
            loaded_param_types = {}
            for name, (value, type_id) in loaded_raw_params.items():
                loaded_params[name] = value
                loaded_param_types[name] = type_id

            # For now, we log missing parameters, but do not raise an error.
            # This allows for more flexible shape files that may not have all
            # parameters set, while still warning the user.
            # In the future, we may want to raise an error if critical parameters
            # are missing.
            expected_params = shape_class.get_expected_shape_parameters()
            missing_params = [
                name
                for name in expected_params
                if name not in loaded_params or loaded_params[name] is None
            ]
            if missing_params:
                Path.Log.error(
                    f"Validation error: Object '{props_obj.Label}' in document {id} "
                    f"is missing parameters for {shape_class.__name__}: {', '.join(missing_params)}."
                    f" In future releases, these shapes will not load!"
                )
                for param in missing_params:
                    param_type = shape_class.get_schema_property_type(param)
                    loaded_params[param] = get_unset_value_for(param_type)
                    loaded_param_types[param] = param_type  # Store the type for missing params

            # Instantiate the specific subclass with the provided ID
            instance = shape_class(id=id)
            instance._data = data  # Keep the byte content
            instance._defaults = loaded_params
            instance._param_types = loaded_param_types
            Path.Log.debug(f"Params: {instance._params} {instance._defaults}")
            instance._params = instance._defaults | instance._params

            if dependencies:  # dependencies is None = shallow load
                # Assign resolved dependencies (like the icon) to the instance
                # The icon has the same ID as the shape, with .png or .svg appended.
                icon_uri = AssetUri.build(
                    asset_type="toolbitshapesvg",
                    asset_id=id + ".svg",
                )
                instance.icon = cast(ToolBitShapeIcon, dependencies.get(icon_uri))
                if not instance.icon:
                    icon_uri = AssetUri.build(
                        asset_type="toolbitshapepng",
                        asset_id=id + ".png",
                    )
                    instance.icon = cast(ToolBitShapeIcon, dependencies.get(icon_uri))

            return instance

    def to_bytes(self, serializer: Type[AssetSerializer]) -> bytes:
        """
        Serializes a ToolBitShape object to bytes (e.g., an fcstd file).
        This is required by the Asset interface.
        """
        assert serializer == DummyAssetSerializer, "ToolBitShape supports only native export"
        doc = None
        try:
            # Create a new temporary document
            doc = FreeCAD.newDocument("TemporaryShapeDoc", hidden=True)

            # Add the shape's body to the temporary document
            self.make_body(doc)

            # Recompute the document to ensure the body is created
            doc.recompute()

            # Save the temporary document to a temporary file
            # We cannot use NamedTemporaryFile on Windows, because there
            # doc.saveAs() may not have permission to access the tempfile
            # while the NamedTemporaryFile is open.
            # So we use TemporaryDirectory instead, to ensure cleanup while
            # still having a the temporary file inside it.
            with tempfile.TemporaryDirectory() as thedir:
                temp_file_path = pathlib.Path(thedir, "temp.FCStd")
                doc.saveAs(str(temp_file_path))
                return temp_file_path.read_bytes()

        finally:
            # Clean up the temporary document
            if doc:
                FreeCAD.closeDocument(doc.Name)

    @classmethod
    def from_file(cls, filepath: pathlib.Path, **kwargs: Any) -> "ToolBitShape":
        """
        Create a ToolBitShape instance from an FCStd file.

        Reads the file bytes and delegates to from_bytes().

        Args:
            filepath (pathlib.Path): Path to the .FCStd file.
            **kwargs: Keyword arguments for shape parameters to override defaults.

        Returns:
            ToolBitShape: An instance of the appropriate ToolBitShape subclass.

        Raises:
            FileNotFoundError: If the file does not exist.
            ValueError: If the file cannot be opened, no Body or PropertyBag
                        is found, or the Body label does not match a known
                        shape name.
            Exception: For other potential FreeCAD errors during loading.
        """
        if not filepath.exists():
            raise FileNotFoundError(f"Shape file not found: {filepath}")
        Path.Log.debug(f"{id}: ToolBitShape.from_file called with {filepath}")

        try:
            data = filepath.read_bytes()
            # Extract the ID from the filename (without extension)
            shape_id = filepath.stem
            # Pass an empty dictionary for dependencies when loading from a single file
            # TODO: pass ToolBitShapeIcon as a dependency
            instance = cls.from_bytes(data, shape_id, {}, DummyAssetSerializer)
            # Apply kwargs parameters after loading from bytes
            if kwargs:
                instance.set_parameters(**kwargs)
            return instance
        except (FileNotFoundError, ValueError) as e:
            raise e
        except Exception as e:
            raise RuntimeError(f"Failed to create shape from {filepath}: {e}")

    @classmethod
    def get_subclass_by_name(
        cls, name: str, default: Optional[Type["ToolBitShape"]] = None
    ) -> Optional[Type["ToolBitShape"]]:
        """
        Retrieves a ToolBitShape class by its name or alias.
        """
        name = name.lower()
        for thecls in cls.__subclasses__():
            if (
                thecls.name.lower() == name
                or thecls.__name__.lower() == name
                or name in thecls.aliases
            ):
                return thecls
        return default

    @classmethod
    def guess_subclass_from_name(
        cls, name: str, default: Optional[Type["ToolBitShape"]] = None
    ) -> Optional[Type["ToolBitShape"]]:
        """
        Retrieves a ToolBitShape class by its name or alias.
        """
        name = name.lower()
        for thecls in cls.__subclasses__():
            if thecls.name.lower() in name or thecls.__name__.lower() in name:
                return thecls
            for alias in thecls.aliases:
                if alias.lower() in name:
                    return thecls
        return default

    @classmethod
    def resolve_name(cls, identifier: str) -> AssetUri:
        """
        Resolves an identifier (name, filename, or URI) to a Uri object.
        """
        # 1. If the input is a url string, return the AssetUri for it.
        if AssetUri.is_uri(identifier):
            return AssetUri(identifier)

        # 2. If the input is a filename (with extension), assume the asset
        #    name is the base name.
        asset_name = identifier
        if pathlib.Path(identifier).suffix.lower() == ".fcstd":
            asset_name = os.path.splitext(os.path.basename(identifier))[0]

        # 3. Construct the Uri using AssetUri.build() and return it
        return AssetUri.build(
            asset_type="toolbitshape",
            asset_id=asset_name,
        )

    @classmethod
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
        """
        Subclasses must define the dictionary mapping parameter names to
        translations and FreeCAD property type strings (e.g.,
        'App::PropertyLength').

        The schema defines any parameters that MUST be in the shape file.
        Any attempt to load a shape file that does not match the schema
        will cause an error.
        """
        raise NotImplementedError

    @property
    def label(self) -> str:
        """Return a user friendly, translatable display name."""
        raise NotImplementedError

    def reset_parameters(self):
        """Reset parameters to their default values."""
        self._params.update(self._defaults)

    def get_parameter_label(self, param_name: str) -> str:
        """
        Get the user-facing label for a given parameter name.
        """
        str_param_name = str(param_name)
        entry = self.schema().get(param_name)
        return entry[0] if entry else str_param_name

    @classmethod
    def get_schema_property_type(cls, param_name: str) -> str:
        """
        Get the FreeCAD property type string for a given parameter name.
        """
        return cls.schema()[param_name][1]

    def get_parameter_property_type(
        self, param_name: str, default: str = "App::PropertyString"
    ) -> str:
        """
        Get the FreeCAD property type string for a given parameter name.
        """
        try:
            return self.get_schema_property_type(param_name)
        except KeyError:
            try:
                return self._param_types[param_name]
            except KeyError:
                return default

    def _normalize_value(self, name: str, value: Any) -> Any:
        """
        Normalize the value for a parameter based on its expected type.
        This is a placeholder for any type-specific normalization logic.

        Args:
            name (str): The name of the parameter.
            value: The value to normalize.

        Returns:
            The normalized value, potentially converted to a FreeCAD.Units.Quantity.
        """
        prop_type = self.get_parameter_property_type(name)
        if prop_type in ("App::PropertyDistance", "App::PropertyLength", "App::PropertyAngle"):
            return FreeCAD.Units.Quantity(value)
        elif prop_type == "App::PropertyInteger":
            return int(value)
        elif prop_type == "App::PropertyFloat":
            return float(value)
        elif prop_type == "App::PropertyBool":
            if value in ("True", "true", "1"):
                return True
            elif value in ("False", "false", "0"):
                return False
            return bool(value)
        return str(value)

    def get_parameters(self) -> Dict[str, Any]:
        """
        Get the dictionary of current parameters and their values.

        Returns:
            dict: A dictionary mapping parameter names to their values.
        """
        return {name: self._normalize_value(name, value) for name, value in self._params.items()}

    def get_parameter(self, name: str) -> Any:
        """
        Get the value of a specific parameter.

        Args:
            name (str): The name of the parameter.

        Returns:
            The value of the parameter (often a FreeCAD.Units.Quantity).

        Raises:
            KeyError: If the parameter name is not valid for this shape.
        """
        if name not in self.schema():
            raise KeyError(f"Shape '{self.name}' has no parameter '{name}'")
        return self._normalize_value(name, self._params[name])

    def set_parameter(self, name: str, value: Any):
        """
        Set the value of a specific parameter.

        Args:
            name (str): The name of the parameter.
            value: The new value for the parameter. Should be compatible
                   with the expected type (e.g., FreeCAD.Units.Quantity).

        Raises:
            KeyError: If the parameter name is not valid for this shape.
        """
        self._params[name] = self._normalize_value(name, value)

    def set_parameters(self, **kwargs):
        """
        Set multiple parameters using keyword arguments.

        Args:
            **kwargs: Keyword arguments where keys are parameter names.
        """
        for name, value in kwargs.items():
            try:
                self.set_parameter(name, value)
            except KeyError:
                Path.Log.debug(f"Ignoring unknown parameter '{name}' for shape '{self.name}'.\n")

    @classmethod
    def get_expected_shape_parameters(cls) -> List[str]:
        """
        Get a list of parameter names expected by this shape class based on
        its schema.

        Returns:
            list[str]: List of parameter names.
        """
        return list(cls.schema().keys())

    def make_body(self, doc: "FreeCAD.Document"):
        """
        Generates the body of the ToolBitShape and copies it to the provided
        document.
        """
        assert self._data is not None
        with ShapeDocFromBytes(self._data) as tmp_doc:
            shape = find_shape_object(tmp_doc)
            if not shape:
                FreeCAD.Console.PrintWarning(
                    "No suitable shape object found in document. " "Cannot create solid shape.\n"
                )
                return None

            props = self._find_property_object(tmp_doc)
            if not props:
                FreeCAD.Console.PrintWarning(
                    "No suitable shape object found in document. " "Cannot create solid shape.\n"
                )
                return None

            update_shape_object_properties(props, self.get_parameters())

            # Recompute the document to apply property changes
            tmp_doc.recompute()

            # Temporarily disable duplicate labels to let FreeCAD automatically
            # make labels unique during the copy operation

            param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Document")
            original_setting = param.GetBool("DuplicateLabels", False)

            try:
                # Disable duplicate labels temporarily
                param.SetBool("DuplicateLabels", False)

                # Copy the body - FreeCAD will now automatically make labels unique
                copied_shape = doc.copyObject(shape, True)

                return copied_shape
            finally:
                # Restore the original setting
                param.SetBool("DuplicateLabels", original_setting)

        """
        Retrieves the thumbnail data for the tool bit shape in PNG format.
        """

    def get_parameter_type(self, name: str) -> str:
        """
        Get the FreeCAD property type string for a given parameter name,
        as loaded from the FCStd file.
        """
        return self._param_types.get(name, "App::PropertyString")

    def get_icon(self) -> Optional[ToolBitShapeIcon]:
        """
        Get the associated ToolBitShapeIcon instance. Tries to load one from
        the asset manager if none was assigned.

        Returns:
            Optional[ToolBitShapeIcon]: The icon instance, or None if none found.
        """
        if self.icon:
            return self.icon

        # Try to get a matching SVG from the asset manager.
        self.icon = cast(
            ToolBitShapeIcon, cam_assets.get_or_none(f"toolbitshapesvg://{self.id}.svg")
        )
        if self.icon:
            return self.icon

        # Try to get a matching PNG from the asset manager.
        self.icon = cast(
            ToolBitShapeIcon, cam_assets.get_or_none(f"toolbitshapepng://{self.id}.png")
        )
        if self.icon:
            return self.icon
        return None

    def get_thumbnail(self) -> Optional[bytes]:
        """
        Retrieves the thumbnail data for the tool bit shape in PNG format,
        as embedded in the shape file.
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
