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

UNITS_SCHEMA_MAP = {
    "metric": 6,  # 6 = Metric schema in FreeCAD
    "imperial": 3,  # 3 = Imperial schema in FreeCAD
}
AUTOSCALING_SCHEMA = 0  # the schema that drops to µm on its own

# Less than this rounds a 3.175mm endmill to 3.18mm, on disk and in the
# editor: forum.freecad.org/viewtopic.php?t=79854
METRIC_DECIMALS = 3
IMPERIAL_DECIMALS = 4


def tool_decimals(units=None) -> int:
    """Decimals for `units` ("Metric"/"Imperial"), or the active schema."""
    schema = UNITS_SCHEMA_MAP.get(units.lower()) if isinstance(units, str) else None
    if schema is None:
        schema = FreeCAD.Units.getSchema()
    return IMPERIAL_DECIMALS if schema == UNITS_SCHEMA_MAP["imperial"] else METRIC_DECIMALS


def at_tool_precision(value: FreeCAD.Units.Quantity) -> FreeCAD.Units.Quantity:
    """
    Stamp tool precision onto the value's format.

    Gui::QuantitySpinBox reads its precision from the Quantity it is given and
    rounds to match, so this has to arrive with the value, not be set after.
    """
    value = FreeCAD.Units.Quantity(value)
    fmt = value.Format
    fmt["Precision"] = tool_decimals()
    value.Format = fmt
    return value


def _in_microns(value) -> bool:
    """True when the value is small enough that FreeCAD would render it in µm."""
    return FreeCAD.Units.schemaTranslate(value, AUTOSCALING_SCHEMA)[2] == "\N{MICRO SIGN}m"


def quantity_to_str(value, units=None):
    """
    Format a Quantity for storage, in the units the bit is specified in.

    Not UserString: that follows the active schema rather than the bit's own,
    and rounds to the display preference, saving a 0.375" tap as 0.37".
    """
    schema = UNITS_SCHEMA_MAP.get(units.lower()) if isinstance(units, str) else None
    if schema is None:
        schema = FreeCAD.Units.getSchema()
    _, factor, unit = FreeCAD.Units.schemaTranslate(value, schema)
    decimals = tool_decimals(units)
    if decimals == METRIC_DECIMALS and _in_microns(value):
        # Below 0.1mm FreeCAD would switch to µm itself; keep that resolution.
        decimals += 1
    text = f"{value.Value / factor:.{decimals}f}"
    # FreeCAD writes degrees closed up against the symbol.
    return f"{text}{unit}" if unit == "\N{DEGREE SIGN}" else f"{text} {unit}"


def to_json(value, units=None):
    """Convert a value to JSON format."""
    if isinstance(value, FreeCAD.Units.Quantity):
        return quantity_to_str(value, units)
    return value


def units_from_json(params):
    """
    Infer Units (Metric/Imperial) from JSON parameter strings.

    For JSON files from disk, values are stored as strings like "3.175 in" or "6 mm".
    This function examines common dimensional parameters (Diameter, Length, CuttingEdgeHeight, etc.)
    to determine if the toolbit uses metric or imperial units.

    Args:
        params: Dictionary of parameters from JSON (before conversion to FreeCAD.Units.Quantity)

    Returns:
        str: "Metric" or "Imperial", or None if units cannot be determined
    """
    if not isinstance(params, dict):
        return None

    imperial_count = 0
    metric_count = 0

    for param_name in ("Diameter", "ShankDiameter", "Length", "CuttingEdgeLength"):
        value = params.get(param_name)
        if value is not None:
            # Check if it's a string with unit suffix
            if isinstance(value, str):
                value_lower = value.lower().strip()

                # Check for imperial units
                if any(unit in value_lower for unit in ["in", "inch", '"', "thou"]):
                    imperial_count += 1
                # Check for metric units
                elif any(unit in value_lower for unit in ["mm", "cm", "m "]):
                    metric_count += 1

        # Make a decision based on counts
        if imperial_count > metric_count:
            return "Imperial"
        elif metric_count > imperial_count:
            return "Metric"

    return "Metric"  # Default to Metric if uncertain


def format_value(
    value: FreeCAD.Units.Quantity | int | float | None,
    precision: int | None = None,
    units: str | None = None,
) -> str | None:
    """
    Format a numeric value as a string, optionally appending a unit and controlling precision.

    Lengths are formatted in the bit's own units at tool precision, so a list
    entry says the same thing as the editor field and the file.

    Args:
        value: The numeric value to format.
        units: (Optional) the bit's units, "Metric" or "Imperial".
        precision: (Optional) present to ask for a formatted value at all;
            the number of decimals follows the units.

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
            return quantity_to_str(value, units)
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
    if isinstance(schema, str) and schema.lower() in UNITS_SCHEMA_MAP:
        FreeCAD.Units.setSchema(UNITS_SCHEMA_MAP[schema.lower()])
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
