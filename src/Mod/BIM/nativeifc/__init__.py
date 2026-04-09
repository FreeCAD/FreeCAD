"""Shared NativeIFC availability helpers."""

import importlib.util

import FreeCAD

translate = FreeCAD.Qt.translate

_has_ifcopenshell = None
_reported_missing_ifcopenshell = False


def invalidate_ifcopenshell_cache():
    """Clears the cached ifcopenshell availability state."""

    global _has_ifcopenshell, _reported_missing_ifcopenshell

    _has_ifcopenshell = None
    _reported_missing_ifcopenshell = False


def has_ifcopenshell(report=False):
    """Returns True when ifcopenshell is importable in this runtime."""

    global _has_ifcopenshell

    if _has_ifcopenshell is None:
        _has_ifcopenshell = importlib.util.find_spec("ifcopenshell") is not None

    if report and not _has_ifcopenshell:
        report_missing_ifcopenshell()

    return _has_ifcopenshell


def report_missing_ifcopenshell():
    """Reports the missing ifcopenshell dependency once per runtime."""

    global _reported_missing_ifcopenshell

    if _reported_missing_ifcopenshell:
        return

    FreeCAD.Console.PrintError(
        translate(
            "BIM",
            "IfcOpenShell was not found on this system. IFC support is disabled",
        )
        + "\n"
    )
    _reported_missing_ifcopenshell = True
