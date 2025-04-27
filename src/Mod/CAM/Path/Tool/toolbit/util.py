import pathlib
from typing import Optional
import Path


def get_toolbit_filepath_from_name(
    name, path: Optional[pathlib.Path] = None
) -> pathlib.Path:
    """
    returns the full filepath for the tool with the given name.
    path is used for tests only, to override the preferences path-
    """
    Path.Log.track(name, path)
    if not name.endswith(".fctb"):
        name += ".fctb"
    if path:
        return path / name
    return Path.Preferences.getToolBitPath() / name


def get_tool_library_from_name(
    name, path: Optional[pathlib.Path] = None
) -> pathlib.Path:
    """search for name, if relative path look in path"""
    Path.Log.track(name, path)
    if not name.endswith(".fctl"):
        name += ".fctl"
    if path:
        return path / name
    return Path.Preferences.getLibraryPath() / name
