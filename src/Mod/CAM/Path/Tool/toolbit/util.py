# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
import FreeCAD


def to_json(value):
    """Convert a value to JSON format."""
    if isinstance(value, FreeCAD.Units.Quantity):
        return value.UserString
    return value


def format_value(
    value: FreeCAD.Units.Quantity | int | float | None,
    precision: int | None = None,
    units: str | None = None,
) -> str | None:
    """
    Format a numeric value as a string, optionally appending a unit and controlling precision.

    This function uses the ToolBitSchema (via setToolBitSchema) to ensure that units are formatted according to the correct schema (Metric or Imperial) when a FreeCAD.Units.Quantity is provided. The schema is temporarily set for formatting and then restored.

    Args:
        value: The numeric value to format.
        unit: (Optional) The unit string to append (e.g., 'mm', 'in').
        precision: (Optional) Number of decimal places (default: 3).

    Returns:
        str: The formatted value as a string, with unit if provided.
    """
    if value is None:
        return None
    elif isinstance(value, FreeCAD.Units.Quantity):
        if precision is not None:
            user_val, _, user_unit = value.getUserPreferred()
            if user_unit in ("deg", "°", "degree", "degrees"):
                # Remove the last character (degree symbol) and convert to float
                try:
                    deg_val = float(str(user_val)[:-1])
                except Exception:
                    return value.getUserPreferred()[0]
                formatted_value = f"{deg_val:.1f}".rstrip("0").rstrip(".")
                return f"{formatted_value}°"
            # Format the value with the specified number of precision and strip trailing zeros
            setToolBitSchema(units)
            _value = value.getUserPreferred()[0]
            setToolBitSchema()
            return _value
        return value.UserString
    return str(value)


def is_imperial_pitch(pitch_mm, tol=1e-6):
    """
    Classify a pitch in mm as imperial vs metric.
    Rule:
        - If pitch_mm is ~2 decimal places clean -> metric,
        unless it corresponds to an exact whole-number TPI.
        - Otherwise, treat as imperial.
    """
    import math

    try:
        mm = float(pitch_mm)
    except Exception:
        return False
    if mm <= 0:
        return False

    # Check if it's "two-decimal clean"
    two_dec_clean = abs(mm - round(mm, 2)) <= tol

    # Compute TPI
    tpi = 25.4 / mm
    whole_tpi = round(tpi)
    is_whole_tpi = math.isclose(tpi, whole_tpi, abs_tol=1e-6)

    if two_dec_clean and not is_whole_tpi:
        return False  # metric
    return True  # imperial


def setToolBitSchema(schema=None):
    """
    Set the FreeCAD units schema. If passed 'Metric' or 'Imperial', set accordingly (case-insensitive).
    Otherwise, if a document is open, set to its schema. If no document, fallback to user preference or provided schema.
    """
    units_schema_map = {
        "metric": 6,  # 6 = Metric schema in FreeCAD
        "imperial": 3,  # 3 = Imperial schema in FreeCAD
    }
    if isinstance(schema, str) and schema.lower() in units_schema_map:
        FreeCAD.Units.setSchema(units_schema_map[schema.lower()])
        return
    if FreeCAD.ActiveDocument is not None:
        try:
            doc_schema = FreeCAD.ActiveDocument.getSchema()
            FreeCAD.Units.setSchema(doc_schema)
            return
        except Exception:
            pass
    # Fallback to user preference or provided schema
    if schema is None:
        schema = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units").GetInt(
            "UserSchema", 6
        )
    FreeCAD.Units.setSchema(schema)
