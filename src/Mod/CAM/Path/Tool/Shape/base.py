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
    find_shape_file,
    update_shape_object_properties,
)


class ToolBitShape(abc.ABC):
    """Abstract base class for tool bit shapes."""

    # Subclasses must define this dictionary mapping internal param names
    # to user-facing labels, for translatability.
    _LABELS: Dict[str, str] = {}

    # Define a tuple of aliases. The first alias is used as a base for the
    # default filename. E.g. if the alias is "endmill", then by default the
    # file is "endmill.fcstd"
    aliases: Tuple[str, ...]

    def __init__(self, **kwargs):
        """
        Initialize the shape.

        Args:
            **kwargs: Keyword arguments for shape parameters (e.g., Diameter=...).
                      Values should be FreeCAD.Units.Quantity where applicable.
        """
        # Use the first alias for the filename
        default_filename = self.aliases[0] + '.fcstd'
        self.filepath = find_shape_file(default_filename)

        # Store parameters as {name: (value, property_type_string)}
        self._params: Dict[str, Tuple[Any, Optional[str]]] = {}
        self.set_default_parameters()  # Initialize with defaults first
        self.set_parameters(**kwargs)  # Override with provided values

    @property
    def name(self) -> str:
        """Return the class name, representing the shape type."""
        return self.__class__.__name__

    @property
    def label(self) -> str:
        """Return a user friendly, translatable display name."""
        raise NotImplementedError

    @abc.abstractmethod
    def set_default_parameters(self):
        """
        Initialize the expected parameters for this shape type with defaults.
        Subclasses must implement this to define their specific parameters.
        Store parameters in self._params as {name: (default_value, property_type_string)}.
        Use FreeCAD.Units.Quantity for dimensional values and the corresponding
        FreeCAD property type string (e.g., 'App::PropertyLength').
        """
        pass

    def get_parameter_label(self, param_name: str) -> str:
        """
        Get the user-facing label for a given parameter name.
        Uses the _LABELS dictionary defined in the subclass.
        """
        str_param_name = str(param_name)
        label = self._LABELS.get(str_param_name)
        # Return the found label or the param_name itself if not found
        return label if label is not None else str_param_name

    def get_parameter_property_type(self, param_name: str) -> Optional[str]:
        """
        Get the FreeCAD property type string for a given parameter name.
        Retrieves the stored property type from the _params dictionary.
        """
        if param_name not in self._params:
            return None

        # Return the stored property type string
        return self._params[param_name][1]

    def get_parameters(self) -> Dict[str, Any]:
        """
        Get the dictionary of current parameters and their values.

        Returns:
            dict: A dictionary mapping parameter names to their values.
        """
        # Return only the values, not the types
        return {name: value for name, (value, _) in self._params.items()}

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
        # Return only the value
        return self._params[name][0]

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

        # Get the original type information
        original_value, prop_type = self._params[name]

        # If the original value was a Quantity and the new value is a string,
        # attempt to convert the string to a Quantity.
        if isinstance(original_value, FreeCAD.Units.Quantity) and isinstance(value, str):
            try:
                value = FreeCAD.Units.Quantity(value)
            except Exception as e:
                FreeCAD.Console.PrintWarning(
                    f"Could not convert string value '{value}' to Quantity for "
                    f"parameter '{name}' in shape '{self.name}': {e}. "
                    "Using raw string value.\n"
                )
                # Continue with the raw string value if conversion fails

        # Check if the type of the new value matches the type of the original value
        # This provides basic type consistency, especially for Quantity types.
        # We compare against the type of the original value, not the stored type string.
        # Only warn if the original value was not None and the types don't match
        if original_value is not None and not isinstance(value, type(original_value)):
            FreeCAD.Console.PrintWarning(
                f"Setting parameter '{name}' for shape '{self.name}' with "
                f"incompatible type. Expected {type(original_value)}, "
                f"got {type(value)}.\n"
            )

        # Update the value, keeping the original property type
        self._params[name] = value, prop_type

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
        its default parameters.

        Returns:
            list[str]: List of parameter names.
        """
        # Instantiate temporarily to access set_default_parameters logic
        temp_instance = cls.__new__(cls)
        # Initialize the _params dictionary for the temporary instance
        temp_instance._params = {}
        temp_instance.set_default_parameters()
        return list(temp_instance._params.keys())

    @classmethod
    def validate(cls, filepath: pathlib.Path) -> bool:
        """
        Check if an FCStd file contains a suitable object with all the
        expected parameters for this shape class.

        Args:
            filepath (pathlib.Path): Path to the .FCStd file.

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
            # Use the updated get_object_properties that returns values and types
            loaded_params_with_types = get_object_properties(shape_obj, expected_params)

            missing_params = []
            type_mismatches = []
            # Instantiate temporarily to get default parameters with types
            temp_instance = cls.__new__(cls)
            temp_instance._params = {} # Initialize _params for the temp instance
            temp_instance.set_default_parameters()
            default_params_with_types = temp_instance._params


            for name in expected_params:
                loaded_tuple = loaded_params_with_types.get(name, (None, None))
                loaded_value, loaded_prop_type = loaded_tuple

                default_tuple = default_params_with_types.get(name, (None, None))
                default_value, expected_prop_type = default_tuple


                if loaded_value is None and loaded_prop_type is None:
                     missing_params.append(name)
                else:
                    # Compare loaded type with expected default type
                    if expected_prop_type is not None and loaded_prop_type != expected_prop_type:
                         type_mismatches.append(
                             f"{name} (Expected Type: {expected_prop_type}, Got Type: {loaded_prop_type})"
                         )
                    # Also check value type consistency if default value exists
                    elif default_value is not None and not isinstance(loaded_value, type(default_value)):
                         type_mismatches.append(
                             f"{name} (Expected Value Type: {type(default_value)}, Got Value Type: {type(loaded_value)})"
                         )


            if missing_params or type_mismatches:
                if missing_params:
                    FreeCAD.Console.PrintWarning(
                        f"Validation Warning: Object '{shape_obj.Label}' in {filepath} "
                        f"is missing parameters for {cls.__name__}: {', '.join(missing_params)}\n"
                    )
                if type_mismatches:
                     FreeCAD.Console.PrintWarning(
                        f"Validation Warning: Object '{shape_obj.Label}' in "
                        f"{filepath} has type mismatches for {cls.__name__}: "
                        f"{', '.join(type_mismatches)}\n"
                    )
                return False

            return True

        except Exception as e:
            FreeCAD.Console.PrintError(f"Validation Error for {filepath}: {e}\n")
            return False
        finally:
            if doc:
                FreeCAD.closeDocument(doc.Name)

    @classmethod
    def _check_parameter_types(cls, shape_obj: Any, expected_params: List[str]) -> List[str]:
        """
        Helper method to check if parameters on a shape object have the expected types.
        This method might become less necessary with the new loading logic,
        but keeping it for now.
        """
        temp_instance = cls.__new__(cls)
        temp_instance._params = {} # Initialize _params for the temp instance
        temp_instance.set_default_parameters()
        default_params_with_types = temp_instance._params

        type_mismatches = []
        for name in expected_params:
            # Only check type if the attribute exists on the object
            if hasattr(shape_obj, name):
                obj_value = getattr(shape_obj, name)
                # Get the expected value type from the default parameters
                default_tuple = default_params_with_types.get(name, (None, None))
                expected_value = default_tuple[0]

                if expected_value is not None and not isinstance(obj_value, type(expected_value)):
                    type_mismatches.append(
                        f"{name} (Expected Value Type: {type(expected_value)}, Got Value Type: {type(obj_value)})"
                    )
                # Also check the property type string if available
                try:
                    obj_prop_type = shape_obj.getTypeIdOfProperty(name)
                    expected_prop_type = default_tuple[1]
                    if expected_prop_type is not None and obj_prop_type != expected_prop_type:
                         type_mismatches.append(
                             f"{name} (Expected Property Type: {expected_prop_type}, Got Property Type: {obj_prop_type})"
                         )
                except Exception:
                    # Ignore if we can't get the property type from the object
                    pass

        return type_mismatches

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
            # load_doc_and_get_properties now returns tuples of (value, type)
            doc, loaded_params_with_types = load_doc_and_get_properties(filepath, expected_params)

            # Create a new instance
            instance = cls.__new__(cls)
            # Initialize the _params dictionary with default values and types
            instance._params = {} # Initialize _params for the new instance
            instance.set_default_parameters()

            # Update parameters with loaded values and types
            for name, (value, prop_type) in loaded_params_with_types.items():
                 if name in instance._params:
                      # Update the value and property type from the loaded data
                      instance._params[name] = value, prop_type
                 else:
                      # This case should ideally not happen if expected_params is correct,
                      # but handle it just in case. Add as a new parameter with loaded type.
                      FreeCAD.Console.PrintWarning(
                          f"Loaded parameter '{name}' not found in default parameters for shape '{cls.__name__}'. Adding it.\n"
                      )
                      instance._params[name] = value, prop_type

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

    def create_feature(self) -> Optional[FreeCAD.GeoFeature]:
        """
        Create and return a FreeCAD object representing the visual shape of the tool bit.
        This method loads the corresponding .FCStd file and updates its properties.
        """
        if not self.filepath:
            FreeCAD.Console.PrintError(
                f"ToolBitShape subclass '{self.name}' has no valid filepath.\n"
            )
            return None

        doc = None
        original_active_doc = FreeCAD.ActiveDocument # Save the current active document
        try:
            # Open the shape file hidden
            doc = FreeCAD.openDocument(str(self.filepath), hidden=True)
            if not doc:
                FreeCAD.Console.PrintError(
                    f"Failed to open shape document: {self.filepath}\n"
                )
                return None

            # Find the main shape object in the document
            shape_obj = find_shape_object(doc)
            if not shape_obj:
                FreeCAD.Console.PrintError(
                    f"No suitable shape object found in {self.filepath}\n"
                )
                return None

            # Update the shape object's properties with the parameter values
            update_shape_object_properties(shape_obj, self.get_parameters())
            # Recompute the document to update the shape based on new parameters
            doc.recompute()

            # Return the updated shape object
            # We need to copy it to the active document later in ToolBit
            return shape_obj

        except Exception as e:
            FreeCAD.Console.PrintError(
                f"Error creating FreeCAD shape for '{self.name}' from"
                f" {self.filepath}: {e}\n"
            )
            return None
        finally:
            # Close the shape document
            if doc:
                FreeCAD.closeDocument(doc.Name)
            # Restore the original active document
            if original_active_doc:
                FreeCAD.setActiveDocument(original_active_doc.Name)


    def __str__(self):
        params_str = ", ".join(
            f"{name}={val}" for name, val in self._params.items()
        )
        return f"{self.name}({params_str})"

    def __repr__(self):
        return self.__str__()
