# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Billy Huddleston <billy@ivdc.com>                  *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************


import FreeCAD
import Path
from typing import Dict, Any, Optional, Union


class ParameterAccessor:
    """
    Unified accessor for dicts and FreeCAD objects for migration logic.
    """

    def __init__(self, target):
        self.target = target
        self.is_dict = isinstance(target, dict)

    def has(self, key):
        if self.is_dict:
            # For dicts, check in nested 'parameter' dict
            param = self.target.get("parameter", {})
            return key in param if isinstance(param, dict) else False
        else:
            return key in getattr(self.target, "PropertiesList", [])

    def get(self, key):
        if self.is_dict:
            # For dicts, get from nested 'parameter' dict
            param = self.target.get("parameter", {})
            return param.get(key) if isinstance(param, dict) else None
        else:
            return self.target.getPropertyByName(key)

    def set(self, key, value):
        if self.is_dict:
            # For dicts, set in nested 'parameter' dict
            if "parameter" not in self.target:
                self.target["parameter"] = {}
            self.target["parameter"][key] = value
        else:
            setattr(self.target, key, value)

    def add_property(self, prop_type, key, group, doc):
        if self.is_dict:
            # For dicts, just set the value
            pass  # No-op, handled by set()
        else:
            self.target.addProperty(prop_type, key, group, doc)

    def set_editor_mode(self, key, mode):
        if self.is_dict:
            pass  # No-op
        else:
            self.target.setEditorMode(key, mode)

    def label(self):
        if self.is_dict:
            return "toolbit"
        else:
            return getattr(self.target, "Label", "unknown toolbit")

    def get_shape_type(self):
        if self.is_dict:
            # For dicts, shape-type is at top level of attrs dict
            return self.target.get("shape-type")
        else:
            # For FreeCAD objects, use ShapeType attribute
            return getattr(self.target, "ShapeType", None)


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def migrate_parameters(accessor: ParameterAccessor) -> bool:
    """
    Migrates legacy parameters using a unified accessor.

    Currently handles:
    - TorusRadius → CornerRadius
    - FlatRadius/Diameter → CornerRadius

    Args:
        accessor: ParameterAccessor instance wrapping dict or FreeCAD object

    Returns:
        True if migration occurred, False otherwise
    """
    has_torus = accessor.has("TorusRadius")
    has_flat = accessor.has("FlatRadius")
    has_diam = accessor.has("Diameter")
    has_corner = accessor.has("CornerRadius")
    label = accessor.label()
    shape_type = accessor.get_shape_type()

    # Only run migration logic if shape type == 'Bullnose'
    if shape_type and str(shape_type).lower() == "bullnose":
        # Case 1: TorusRadius exists, copy to CornerRadius
        if has_torus and not has_corner:
            value = accessor.get("TorusRadius")
            accessor.add_property(
                "App::PropertyLength",
                "CornerRadius",
                "Shape",
                "Corner radius copied from TorusRadius",
            )
            accessor.set_editor_mode("CornerRadius", 0)
            accessor.set("CornerRadius", value)
            Path.Log.info(f"Copied TorusRadius to CornerRadius={value} for {label}")
            return True

        # Case 2: FlatRadius and Diameter exist, calculate CornerRadius
        if has_flat and has_diam and not has_corner:
            try:
                diam_raw = accessor.get("Diameter")
                flat_raw = accessor.get("FlatRadius")
                diameter = FreeCAD.Units.Quantity(diam_raw)
                flat_radius = FreeCAD.Units.Quantity(flat_raw)
                corner_radius = (float(diameter) / 2.0) - float(flat_radius)

                # Convert to correct unit
                if isinstance(diam_raw, str) and diam_raw.strip().endswith("in"):
                    cr_in = FreeCAD.Units.Quantity(f"{corner_radius} mm").getValueAs("in")
                    value = f"{float(cr_in):.4f} in"
                else:
                    if isinstance(diam_raw, str) and not diam_raw.strip().endswith("mm"):
                        cr_mm = FreeCAD.Units.Quantity(f"{corner_radius} in").getValueAs("mm")
                        value = f"{float(cr_mm):.4f} mm"
                    else:
                        value = f"{float(corner_radius):.4f} mm"

                accessor.add_property(
                    "App::PropertyLength",
                    "CornerRadius",
                    "Shape",
                    "Corner radius migrated from FlatRadius/Diameter",
                )
                accessor.set_editor_mode("CornerRadius", 0)
                accessor.set("CornerRadius", value)
                Path.Log.info(f"Migrated FlatRadius/Diameter to CornerRadius={value} for {label}")
                return True
            except Exception as e:
                Path.Log.error(f"Failed to migrate FlatRadius for toolbit {label}: {e}")
                return False

    return False
