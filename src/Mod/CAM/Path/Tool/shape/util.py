# -*- coding: utf-8 -*-
# Utility functions for working with ToolBitShape definitions in FCStd files.

import pathlib
import FreeCAD
from typing import Optional, Type
from .base import ToolBitShape


def get_mod_dir() -> pathlib.Path:
    """
    Iterates through FreeCAD.__ModDirs__ to find the CAM module directory
    by matching the last two path components ('Mod' and 'CAM').

    Returns:
        pathlib.Path: The full path to the shape directory.

    Raises:
        FileNotFoundError: If the CAM module directory cannot be found
                           in FreeCAD.__ModDirs__ with the expected structure.
    """
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

    return cam_mod_dir


def get_builtin_shape_dir() -> pathlib.Path:
    """
    Find the path to a built-in shape dir.
    """
    return get_mod_dir() / "Tools" / "Shape"


def get_shape_class_from_name(name: str) -> Optional[Type[ToolBitShape]]:
    for cls in ToolBitShape.__subclasses__():
        if cls.name == name or name in cls.aliases:
            return cls
    return None


def get_shape_filename_from_alias(alias: str) -> Optional[Type[ToolBitShape]]:
    for cls in ToolBitShape.__subclasses__():
        if cls.name == alias or alias in cls.aliases:
            return cls.name.lower() + '.fcstd'
    return None
