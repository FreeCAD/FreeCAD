# -*- coding: utf-8 -*-
# This file defines the abstract base class for representing CAM tool bit shapes.

import abc
import os
import pathlib
import FreeCAD
from typing import Dict, List, Any, Optional, Type, Tuple
from .util import (
    find_shape_object,
    get_object_properties,
    load_doc_and_get_properties,
)


class ToolBitShape(abc.ABC):
    """Abstract base class for tool bit shapes."""

    # Subclasses must define this dictionary mapping internal param names
    # to user-facing labels.
    _LABELS: Dict[str, str] = {}

    def __init__(self, **kwargs):
        """
        Initialize the shape.

        Args:
            **kwargs: Keyword arguments for shape parameters (e.g., Diameter=...).
                      Values should be FreeCAD.Units.Quantity where applicable.
        """
        self._params: Dict[str, FreeCAD.Units.Quantity] = {}
        self.set_default_parameters()  # Initialize with defaults first
        self.set_parameters(**kwargs)  # Override with provided values

    @property
    def name(self) -> str:
        """Return the class name, representing the shape type."""
        return self.__class__.__name__

    @abc.abstractmethod
    def set_default_parameters(self):
        """
        Initialize the expected parameters for this shape type with defaults.
        Subclasses must implement this to define their specific parameters.
        Store parameters in self._params as {name: default_value}.
        Use FreeCAD.Units.Quantity for dimensional values.
        """
        pass

    def get_parameter_label(self, param_name: str) -> str:
        """
        Get the user-facing label for a given parameter name.
        Uses the _LABELS dictionary defined in the subclass.
        """
        # Ensure param_name is treated as str for lookup and default
        str_param_name = str(param_name)
        label = self._LABELS.get(str_param_name)
        # Return the found label or the param_name itself if not found
        return label if label is not None else str_param_name

    def get_parameters(self) -> Dict[str, Any]:
        """
        Get the dictionary of current parameters and their values.

        Returns:
            dict: A dictionary mapping parameter names to their values.
        """
        return self._params.copy()

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
        if name not in self._params:
            # Allow setting if it's a known label's internal name? No, strict.
            raise KeyError(f"Shape '{self.name}' has no parameter '{name}'")
        # TODO: Add type/unit validation if necessary, comparing to default type
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
                FreeCAD.Console.PrintWarning(
                    f"Ignoring unknown parameter '{name}' for shape '{self.name}'.\n"
                )

    @classmethod
    def get_expected_parameter_names(cls) -> List[str]:
        """
        Get a list of parameter names expected by this shape class based on
        its default parameters.

        Returns:
            list[str]: List of parameter names.
        """
        # Instantiate temporarily to access set_default_parameters logic
        temp_instance = cls.__new__(cls)
        temp_instance._params = {}
        temp_instance.set_default_parameters()
        return list(temp_instance._params.keys())

    @classmethod
    def validate(cls, filepath: pathlib.Path) -> bool:
        """
        Check if an FCStd file contains a suitable object with all the
        expected parameters for this shape class.

        Args:
            filepath (str): Path to the .FCStd file.

        Returns:
            bool: True if the file is valid for this shape type, False otherwise.
        """
        doc = None
        try:
            if not os.path.exists(filepath):
                FreeCAD.Console.PrintError(f"Validation Error: File not found: {filepath}\n")
                return False

            doc = FreeCAD.openDocument(filepath, Hidden=True)
            if not doc:
                FreeCAD.Console.PrintError(f"Validation Error: Failed to open document: {filepath}\n")
                return False

            shape_obj = find_shape_object(doc)
            if not shape_obj:
                FreeCAD.Console.PrintError(
                    f"Validation Error: No suitable shape object found in {filepath}\n"
                )
                return False

            expected_params = cls.get_expected_parameter_names()
            missing_params = []
            for name in expected_params:
                if not hasattr(shape_obj, name):
                    missing_params.append(name)

            if missing_params:
                FreeCAD.Console.PrintWarning(
                    f"Validation Warning: Object '{shape_obj.Label}' in {filepath} "
                    f"is missing parameters for {cls.__name__}: {', '.join(missing_params)}\n"
                )
                # Decide if missing params constitute failure. For now, let's say yes.
                return False

            # Optional: Add type checking here if needed
            # for name in expected_params:
            #     # Compare type of getattr(shape_obj, name) with default type
            #     pass

            return True

        except Exception as e:
            FreeCAD.Console.PrintError(f"Validation Error for {filepath}: {e}\n")
            return False
        finally:
            if doc:
                FreeCAD.closeDocument(doc.Name)

    @classmethod
    def from_file(cls: Type["ToolBitShape"],
                  filepath: pathlib.Path) -> "ToolBitShape":
        """
        Load a shape's parameters from properties of an object in an FCStd file.

        Args:
            filepath (pathlib.Path): Path to the .FCStd file.

        Returns:
            ToolBitShape: An instance of the shape subclass with loaded parameters.

        Raises:
            FileNotFoundError: If the file does not exist.
            ValueError: If the file cannot be opened, a suitable object is not
                        found, or essential data is missing.
            Exception: For other potential FreeCAD errors during loading.
        """
        doc = None
        try:
            expected_params = cls.get_expected_parameter_names()
            doc, params = load_doc_and_get_properties(filepath, expected_params)

            # Instantiate the class with the extracted parameters
            instance = cls(**params)
            return instance

        # Keep specific exceptions if needed, otherwise catch broader Exception
        except (FileNotFoundError, ValueError) as e:
            # Re-raise known exceptions for clarity
            raise e
        except Exception as e:
            # Catch other potential FreeCAD or OS errors
            raise RuntimeError(f"Failed to load shape from {filepath}: {e}")
        finally:
            # Ensure the document is closed even if errors occurred
            if doc:
                FreeCAD.closeDocument(doc.Name)

    def __str__(self):
        params_str = ", ".join(
            f"{name}={val}" for name, val in self._params.items()
        )
        return f"{self.name}({params_str})"

    def __repr__(self):
        return self.__str__()
