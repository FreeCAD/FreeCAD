# -*- coding: utf-8 -*-
# Utility functions for working with ToolBitShape definitions in FCStd files.

import os
import pathlib
import FreeCAD
import Path
from typing import List, Tuple, Optional, Type
from .base import ToolBitShape


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


def get_builtin_shape_file_from_name(name):
    """
    Find the path to a built-in tool shape file using its alias.

    Iterates through FreeCAD.__ModDirs__ to find the CAM module directory
    by matching the last two path components ('Mod' and 'CAM') and
    constructs the path to the shape file.

    Args:
        name (str): The name or alias of the tool shape (e.g., "endmill").

    Returns:
        pathlib.Path: The full path to the shape file.

    Raises:
        FileNotFoundError: If the CAM module directory cannot be found
                           in FreeCAD.__ModDirs__ with the expected structure.
    """
    # Find class to resolve from alias to name.
    cls = get_shape_class_from_name(name)
    if not cls:
        return

    # Find the FreeCAD CAM module directory.
    cam_mod_dir = None
    for mod_dir_str in FreeCAD.__ModDirs__:
        mod_path = pathlib.Path(mod_dir_str)
        # Check if the last two components are 'Mod' and 'CAM'
        if (
            len(mod_path.parts) >= 2
            and mod_path.parts[-2] == "Mod"
            and mod_path.parts[-1] == "CAM"
        ):
            cam_mod_dir = mod_path
            break

    if cam_mod_dir is None:
        raise FileNotFoundError(
            "CAM module directory with expected structure not found in FreeCAD.__ModDirs__"
        )

    # The built-in shapes are stored in the CAM directory.
    return cam_mod_dir / "Tools" / "Shape" / (cls.name + ".fcstd")


def get_shape_class_from_name(name: str) -> Optional[Type[ToolBitShape]]:
    for cls in ToolBitShape.__subclasses__():
        if cls.name == name or name in cls.aliases:
            return cls
    return None


def get_shape_from_name(
    name: str,
    path: Optional[pathlib.Path] = None,
    params: Optional[dict] = None
) -> "ToolBitShape":
    # Find the shape class for the new shape
    shape_class = get_shape_class_from_name(name)
    if shape_class is None:
        err = f"Could not find shape class for '{name}'."
        Path.Log.error(err+"\n")
        raise AttributeError(err)

    # Find the shape file path for the new shape
    is_builtin = path is None
    if is_builtin:
        path = get_builtin_shape_file_from_name(shape_class.name)
        if not path:
            err = f"Could not find shape file for new shape '{shape_class.name}'."
            Path.Log.error(err+"\n")
            raise AttributeError(err)
        path = pathlib.Path(path)

    # Instantiate the new shape with the found file path and preserved parameters
    try:
        shape = shape_class(filepath=path, **(params or {}))
    except Exception as e:
        err = f"Could not create instance of shape '{name}' ({shape_class.name}): {e}"
        Path.Log.error(err+"\n")
        raise

    shape.is_builtin = is_builtin
    return shape

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


def get_shape_name_from_basename(filepath: str | pathlib.Path) -> Optional[str]:
    """
    Returns the name, from a name or alias of a built-in tool shape.

    Args:
        filepath: The path to the shape file.

    Returns:
        str: The name, None if none found
    """
    file_path_obj = pathlib.Path(filepath)
    file_base_name = file_path_obj.stem.lower()
    cls = get_shape_class_from_name(file_base_name)
    return cls.name if cls else None
