"""Shared NativeIFC availability helpers."""

import importlib.util

import FreeCAD

translate = FreeCAD.Qt.translate

_ifcopenshell_state = {"available": None, "reported_missing": False}


def invalidate_ifcopenshell_cache():
    """Clears the cached ifcopenshell availability state."""

    _ifcopenshell_state["available"] = None
    _ifcopenshell_state["reported_missing"] = False


def has_ifcopenshell(report=False):
    """Returns True when ifcopenshell is importable in this runtime."""

    if _ifcopenshell_state["available"] is None:
        _ifcopenshell_state["available"] = importlib.util.find_spec("ifcopenshell") is not None

    if report and not _ifcopenshell_state["available"]:
        report_missing_ifcopenshell()

    return _ifcopenshell_state["available"]


def report_missing_ifcopenshell():
    """Reports the missing ifcopenshell dependency once per runtime."""

    if _ifcopenshell_state["reported_missing"]:
        return

    FreeCAD.Console.PrintError(
        translate(
            "BIM",
            "IfcOpenShell was not found on this system. IFC support is disabled",
        )
        + "\n"
    )
    _ifcopenshell_state["reported_missing"] = True
