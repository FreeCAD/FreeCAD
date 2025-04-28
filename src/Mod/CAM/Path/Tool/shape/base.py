# -*- coding: utf-8 -*-
# This file defines the abstract base class for representing CAM tool bit shapes.

import abc
import pathlib
import FreeCAD
import Path
from typing import Dict, List, Any, Mapping, Optional, Tuple, Type
from .doc import (
    find_shape_object,
    get_doc_state,
    get_object_properties,
    restore_doc_state,
    update_shape_object_properties,
    ShapeDocFromBytes,
)
from .util import (
    get_abbreviations_from_svg,
    create_thumbnail,
    file_is_newer
)


class ToolBitShape(abc.ABC):
    """Abstract base class for tool bit shapes."""

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
    # "v-bit".
    aliases: Tuple[str, ...] = tuple()

    def __init__(self, **kwargs: Any):
        """
        Initialize the shape.

        Args:
            **kwargs: Keyword arguments for shape parameters (e.g., Diameter).
                      Values should be FreeCAD.Units.Quantity where applicable.
        """
        # _params will be populated with default values after loading
        self._params: Dict[str, Any] = {}

        # Stores default parameter values loaded from the FCStd file
        self._defaults: Dict[str, Any] = {}

        # Cache for the loaded FreeCAD document content for this instance
        self._cache: Optional[bytes] = None

        # Path to the file this shape was loaded from. Set by from_file()
        self.filepath: Optional[pathlib.Path] = None

        self.is_builtin: bool = True

        self.icon: Optional[bytes] = None # Shape SVG or PNG as a binary string

        self.icon_type: Optional[str] = None # 'png' or 'svg'

        # Assign parameters
        for param, value in kwargs.items():
            self.set_parameter(param, value)

    def __str__(self):
        params_str = ", ".join(f"{name}={val}" for name, val in self._params.items())
        return f"{self.name}({params_str})"

    def __repr__(self):
        return self.__str__()

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
            raise ValueError(
                f"No ToolBitShape subclass found matching Body label '{body_obj.Label}' in {doc}"
            )
        return shape_class

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
    def from_file(cls, filepath: pathlib.Path, **kwargs: Any) -> "ToolBitShape":
        """
        Create a ToolBitShape instance from an FCStd file.

        Identifies the correct subclass based on the Body label in the file,
        loads parameters, and caches the document content.

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

        temp_doc = None
        original_doc_state = get_doc_state()  # Save the current document state
        try:
            # Open the shape file temporarily to get the Body label and parameters
            temp_doc = FreeCAD.openDocument(str(filepath), hidden=True)
            if not temp_doc:
                raise ValueError(f"Failed to open shape document: {filepath}")

            # Load properties
            props_obj = cls._find_property_object(temp_doc)
            if not props_obj:
                raise ValueError(f"No 'Attributes' PropertyBag object found in {filepath}")

            # Determine the specific subclass of ToolBitShape
            shape_class = cls._get_shape_class_from_doc(temp_doc)

            # Get properties from the properties object
            expected_params = shape_class.get_expected_shape_parameters()
            loaded_params = get_object_properties(props_obj, expected_params)

            missing_params = [
                name
                for name in expected_params
                if name not in loaded_params or loaded_params[name] is None
            ]

            if missing_params:
                raise ValueError(
                    f"Validation error: Object '{props_obj.Label}' in {filepath} "
                    + f"is missing parameters for {shape_class.__name__}: {', '.join(missing_params)}"
                )

            # Instantiate the specific subclass
            instance = shape_class(**kwargs)
            instance.filepath = filepath
            instance._cache = filepath.read_bytes() # Cache the file content
            instance._defaults = loaded_params

            # Update instance parameters, prioritizing loaded defaults but not
            # overwriting parameters already set by kwargs during __init__
            instance._params = instance._defaults | instance._params

            instance.load_or_create_icon()
            return instance

        except (FileNotFoundError, ValueError) as e:
            raise e
        except Exception as e:
            raise RuntimeError(f"Failed to create shape from {filepath}: {e}")
        finally:
            # Ensure the temporary document is closed if it was opened
            if temp_doc:
                FreeCAD.closeDocument(temp_doc.Name)
            # Restore the original document state
            restore_doc_state(original_doc_state)

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
        label = self.schema()[param_name][0]
        # Return the found label or the param_name itself if not found
        return label if label is not None else str_param_name

    def get_parameter_property_type(self, param_name: str) -> str:
        """
        Get the FreeCAD property type string for a given parameter name.
        """
        return self.schema()[param_name][1]

    def get_parameters(self) -> Dict[str, Any]:
        """
        Get the dictionary of current parameters and their values.

        Returns:
            dict: A dictionary mapping parameter names to their values.
        """
        return self._params

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
        return self._params[name]

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
        if name not in self.schema().keys():
            Path.Log.debug(
                f"Shape '{self.name}' was given an invalid parameter '{name}'. Has {self._params}\n"
            )
            return

        self._params[name] = value

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

    @classmethod
    def validate(cls, filepath: pathlib.Path) -> Optional[str]:
        """
        Check if an FCStd file contains a suitable object with all the
        expected parameters for this shape class based on the class schema.

        Args:
            filepath (pathlib.Path): Path to the .FCStd file.

        Returns:
            bool: True if the file is valid for this shape type, False otherwise.
        """
        doc = None
        try:
            if not filepath.exists():
                err = f"Validation Error: File not found: {filepath}"
                FreeCAD.Console.PrintError(err + "\n")
                return err

            cls.from_file(filepath)
        except Exception as e:
            err = f"Validation Error for {filepath}: {e}"
            FreeCAD.Console.PrintError(err + "\n")
            return err
        finally:
            # Ensure the temporary document is closed
            if doc:
                FreeCAD.closeDocument(doc.Name)

    def make_body(self, doc: "FreeCAD.Document"):
        """
        Generates the body of the ToolBitShape and copies it to the provided
        document.
        """
        assert self._cache is not None
        with ShapeDocFromBytes(self._cache) as tmp_doc:
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

            # Copy the body to the given document without immediate compute.
            return doc.copyObject(shape, True)

    def get_icon(self):
        return self.icon_type, self.icon

    def get_icon_len(self):
        return len(self.icon) if self.icon else 0

    def add_icon_from_file(self, filename: pathlib.Path):
        with open(filename, 'rb') as fp:
            self.icon = fp.read()
            if filename.suffix == '.svg':
                self.abbr = get_abbreviations_from_svg(self.icon)
        self.icon_type = filename.suffix.lstrip('.')

    def get_abbr(self, param):
        normalized = param.label.lower().replace(' ', '_')
        return self.abbr.get(normalized)

    def create_icon(self):
        if not self.filepath:
            return
        filename = create_thumbnail(self.filepath)
        if filename: # success?
            self.add_icon_from_file(filename)
        return filename

    def load_or_create_icon(self):
        assert self.filepath is not None, "Need shape file to create icon for it"

        # Try SVG first.
        icon_file = self.filepath.with_suffix('.svg')
        if icon_file.is_file():
            return self.add_icon_from_file(icon_file)

        # Try PNG next. But make sure it's not out of date.
        icon_file = self.filepath.with_suffix('.png')
        if icon_file.is_file() and file_is_newer(self.filepath, icon_file):
            return self.add_icon_from_file(icon_file)

        # Next option: Try to re-generate the PNG.
        if self.create_icon():
            return

        # Last option: return the out-of date PNG.
        if icon_file.is_file():
            return self.add_icon_from_file(icon_file)

    def write_icon_to_file(self, filepath: Optional[pathlib.Path]=None):
        if self.icon is None:
            return
        assert self.icon_type is not None, "Bug: Shape has icon, but no icon type?"
        assert self.filepath is not None, "Bug: Shape has icon, but no file?"
        if filepath is None:
            filepath = self.filepath.with_suffix('.'+self.icon_type)
        with open(filepath, 'wb') as fp:
            fp.write(self.icon)
