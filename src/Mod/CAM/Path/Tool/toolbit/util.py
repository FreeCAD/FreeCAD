import os
from typing import Optional
import Path


def _findToolFile(name, containerFile, typ) -> Optional[str]:
    Path.Log.track(name)
    if os.path.exists(name):  # absolute reference
        return name

    if containerFile:
        rootPath = os.path.dirname(os.path.dirname(containerFile))
        paths = [os.path.join(rootPath, typ)]
    else:
        paths = []
    paths.extend(Path.Preferences.searchPathsTool(typ))

    def _findFile(path, name):
        Path.Log.track(path, name)
        fullPath = os.path.join(path, name)
        if os.path.exists(fullPath):
            return (True, fullPath)
        for root, ds, fs in os.walk(path):
            for d in ds:
                found, fullPath = _findFile(d, name)
                if found:
                    return (True, fullPath)
        return (False, None)

    for p in paths:
        found, path = _findFile(p, name)
        if found:
            return path
    return None


def findToolShape(name, path=None) -> Optional[str]:
    """search for name, if relative path look in path"""
    Path.Log.track(name, path)
    return _findToolFile(name, path, "Shape")


def findToolBit(name, path=None):
    """search for name, if relative path look in path"""
    Path.Log.track(name, path)
    if name.endswith(".fctb"):
        return _findToolFile(name, path, "Bit")
    return _findToolFile("{}.fctb".format(name), path, "Bit")


# Only used in ToolBit unit test module: TestPathToolBit.py
def findToolLibrary(name, path=None):
    """search for name, if relative path look in path"""
    Path.Log.track(name, path)
    if name.endswith(".fctl"):
        return _findToolFile(name, path, "Library")
    return _findToolFile("{}.fctl".format(name), path, "Library")
