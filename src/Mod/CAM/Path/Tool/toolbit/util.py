# -*- coding: utf-8 -*-
import FreeCAD


def to_json(value):
    """Convert a value to JSON format."""
    if isinstance(value, FreeCAD.Units.Quantity):
        return str(value)
    return value


def format_value(value: FreeCAD.Units.Quantity | int | float | None):
    if value is None:
        return None
    elif isinstance(value, FreeCAD.Units.Quantity):
        return value.UserString
    return str(value)
