# -*- coding: utf-8 -*-
# This file defines the abstract base class for representing CAM tool bit shapes.

import abc
import pathlib
import FreeCAD
from typing import Dict, List, Any, Optional, Tuple
from .doc import (
    find_shape_object,
    find_property_object,
    get_doc_state,
    get_object_properties,
    restore_doc_state,
    update_shape_object_properties,
    ShapeDocFromBytes,
)


class ToolBitShape(abc.ABC):
    """Abstract base class for tool bit shapes."""

    # Define name and a tuple of aliases. The name is used as a base for the
    # default filename. E.g. if the name is "endmill", then by default the
    # file is "endmill.fcstd"
    name: str
    aliases: Tuple[str, ...] = tuple()

    # Subclasses must define this dictionary mapping parameter names to
    # FreeCAD property type strings (e.g., 'App::PropertyLength').
    _schema: Dict[str, str] = {}

    def __init__(self, filepath: pathlib.Path, **kwargs: Any):
        """
        Initialize the shape.

        Args:
            filepath: Path to an FCStd file to load the shape
            **kwargs: Keyword arguments for shape parameters (e.g., Diameter).
                      Values should be FreeCAD.Units.Quantity where applicable.
        """
        # Subclasses must define this dictionary mapping internal param names
        # to user-facing labels, for translatability.
        self._labels: Dict[str, str] = {}

        # _params will be populated with default values after loading
        self._params: Dict[str, Any] = {}

        # Stores default parameter values loaded from the FCStd file
        self._defaults: Dict[str, Any] = {}

        # Cache for the loaded FreeCAD document content for this instance
        self._cache: Optional[bytes] = None

        # Path to the file this shape was loaded from
        self.filepath: Optional[pathlib.Path] = None

        # Assign parameters and load the file
        self.load_file(filepath)
        for param, value in kwargs.items():
            self.set_parameter(param, value)

    def __str__(self):
        params_str = ", ".join(
            f"{name}={val}" for name, val in self._params.items()
        )
        return f"{self.name}({params_str})"

    def __repr__(self):
        return self.__str__()

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
        Uses the _labels dictionary defined in the subclass.
        """
        str_param_name = str(param_name)
        label = self._labels.get(str_param_name)
        # Return the found label or the param_name itself if not found
        return label if label is not None else str_param_name

    def get_parameter_property_type(self, param_name: str) -> Optional[str]:
        """
        Get the FreeCAD property type string for a given parameter name.
        Retrieves the stored property type from the class-level _schema.
        """
        return self._schema.get(param_name)

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
        if name not in self._params:
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
        if name not in self._params.keys():
            FreeCAD.Console.PrintError(
                f"Shape '{self.name}' was given an invalid parameter '{name}'. Has {self._params}"
            )

        self._params[name] = value

    def set_parameters(self, **kwargs):
        """
        Set multiple parameters using keyword arguments.

        Args:
            **kwargs: Keyword arguments where keys are parameter names.
        """
        for name, value in kwargs.items():
            try:
                # When setting parameters from kwargs, we don't have type info
                # from the file. We rely on set_parameter to maintain the type
                # from the default parameters or the loaded parameters.
                self.set_parameter(name, value)
            except KeyError:
                FreeCAD.Console.PrintWarning(
                    f"Ignoring unknown parameter '{name}' for shape '{self.name}'.\n"
                )

    @classmethod
    def get_expected_parameter_names(cls) -> List[str]:
        """
        Get a list of parameter names expected by this shape class based on
        its schema.

        Returns:
            list[str]: List of parameter names.
        """
        return list(cls._schema.keys())

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
                FreeCAD.Console.PrintError(err+"\n")
                return err

            doc = FreeCAD.openDocument(str(filepath), hidden=True)
            if not doc:
                err = f"Validation Error: Failed to open document: {filepath}"
                FreeCAD.Console.PrintError(err+"\n")
                return err

            property_obj = find_property_object(doc)
            if not property_obj:
                err = f"Validation Error: No suitable property_obj object found in {filepath}"
                FreeCAD.Console.PrintError(err+"\n")
                return err

            expected_params = cls.get_expected_parameter_names()
            loaded_params = get_object_properties(property_obj, expected_params)

            missing_params = [
                name for name in expected_params
                if name not in loaded_params or loaded_params[name] is None
            ]

            if missing_params:
                err = f"Validation Warning: Object '{property_obj.Label}' in {filepath} " \
                    + f"is missing parameters for {cls.__name__}: {', '.join(missing_params)}"
                FreeCAD.Console.PrintError(err+"\n")
                return err

        except Exception as e:
            err = f"Validation Error for {filepath}: {e}"
            FreeCAD.Console.PrintError(err+"\n")
            return err
        finally:
            # Ensure the temporary document is closed
            if doc:
                FreeCAD.closeDocument(doc.Name)

    def load_file(self, filepath: pathlib.Path) -> "ToolBitShape":
        """
        Load a shape's parameters from properties of an object in an FCStd file
        and cache the document content and default parameters within the instance.
        Does not create a body in the active document.

        Args:
            filepath (pathlib.Path): Path to the .FCStd file.

        Raises:
            FileNotFoundError: If the file does not exist.
            ValueError: If the file cannot be opened, a suitable object is not
                        found, or essential data is missing.
            Exception: For other potential FreeCAD errors during loading.
        """
        temp_doc = None
        original_doc_state = get_doc_state() # Save the current document state
        try:
            # Open the shape file in a temporary hidden document to extract parameters
            temp_doc = FreeCAD.openDocument(str(filepath), hidden=True)
            if not temp_doc:
                raise ValueError(f"Failed to open shape document: {filepath}")

            # Find the object holding the parameters (usually "Attributes" PropertyBag)
            props_obj = find_property_object(temp_doc)
            if not props_obj:
                raise ValueError(
                    f"No 'Attributes' PropertyBag object found in {filepath}"
                )

            # Get properties based on the class schema from the properties object
            expected_params = self.get_expected_parameter_names()
            loaded_params = get_object_properties(props_obj, expected_params)

            # Store the default parameters
            self._defaults = loaded_params
            # Update instance parameters, prioritizing loaded defaults but not
            # overwriting parameters already set by kwargs during __init__
            self._params = self._defaults | self._params

            # Close the temporary document as we don't need it open anymore
            FreeCAD.closeDocument(temp_doc.Name)
            temp_doc = None # Ensure temp_doc is None in finally block

            # Read the raw file content for caching
            with open(filepath, 'rb') as f:
                self._cache = f.read()

            # Store the path
            self.filepath = filepath

            return self

        except (FileNotFoundError, ValueError) as e:
            # Re-raise known exceptions for clarity
            raise e
        except Exception as e:
            # Catch other potential FreeCAD or OS errors
            raise RuntimeError(f"Failed to load shape from {filepath}: {e}")
        finally:
             # Ensure the temporary document is closed if it was opened
             if temp_doc:
                  FreeCAD.closeDocument(temp_doc.Name)
             # Restore the original document state
             restore_doc_state(original_doc_state)

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
                    "No suitable shape object found in document. "
                    "Cannot create solid shape.\n"
                )
                return None

            props = find_property_object(tmp_doc)
            if not props:
                FreeCAD.Console.PrintWarning(
                    "No suitable shape object found in document. "
                    "Cannot create solid shape.\n"
                )
                return None

            update_shape_object_properties(props, self.get_parameters())

            # Recompute the document to apply property changes
            tmp_doc.recompute()

            # Copy the body to the given document.
            doc.openTransaction("Create ToolBit Shape")
            body = doc.copyObject(shape, True)
            #copied_obj = tmp_doc.copyObject(shape, False)
            #doc.addObject(copied_obj.Name)
            doc.commitTransaction()
            return body
