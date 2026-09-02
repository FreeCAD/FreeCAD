"""Shared NativeIFC availability helpers."""

import FreeCAD

from . import backend

translate = FreeCAD.Qt.translate

_ifcopenshell_state = {"reported_missing": False}


def invalidate_ifcopenshell_cache():
    """Clears the cached ifcopenshell availability state."""

    backend.invalidate()
    _ifcopenshell_state["reported_missing"] = False


def has_ifcopenshell(report=False):
    """Returns True when ifcopenshell is importable in this runtime."""

    available = backend.get_status().available

    if report and not available:
        report_missing_ifcopenshell()

    return available


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
