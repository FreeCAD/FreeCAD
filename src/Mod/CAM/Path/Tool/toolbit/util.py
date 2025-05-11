# -*- coding: utf-8 -*-
import FreeCAD


def to_json(value):
    """Convert a value to JSON format."""
    if isinstance(value, FreeCAD.Units.Quantity):
        return str(value)
    return value
