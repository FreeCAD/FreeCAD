# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 The FreeCAD project association <fpa@freecad.org>  *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM temperature dependent material tables"
__author__ = "The FreeCAD project association"
__url__ = "https://www.freecad.org"

## @package materialtables
#  \ingroup FEM
#  \brief temperature dependent material tables, from Materials arrays to solver rows

import math
import re
import sys

from FreeCAD import Units

# FEM material property: (Materials 2DArray property, columns kept after the temperature)
TABLES = {
    "ElasticTable": ("ElasticProperties", ("YoungsModulus", "PoissonRatio")),
    "ConductivityTable": ("ThermalProperties", ("ThermalConductivity",)),
    "SpecificHeatTable": ("ThermalProperties", ("SpecificHeat",)),
    "ExpansionTable": ("ThermalProperties", ("ThermalExpansionCoefficient",)),
    "ElectricalConductivityTable": ("ElectricalProperties", ("ElectricalConductivity",)),
}

# Units the values of a row are checked against, "" for a ratio, and a row to show the form
ROW_FORMS = {
    "ElasticTable": (("MPa", ""), "293.15 K, 119000 MPa, 0.34"),
    "ConductivityTable": (("W/m/K",), "293.15 K, 401 W/m/K"),
    "SpecificHeatTable": (("J/kg/K",), "293.15 K, 385 J/kg/K"),
    "ExpansionTable": (("1/K",), "293.15 K, 1.65e-05 1/K"),
    "ElectricalConductivityTable": (("S/m",), "293.15 K, 5.96e+07 S/m"),
}

# A cell starts with a number, the parser reads a bare unit as 1 of it and an empty cell as DBL_MIN
NUMBER_FIRST = re.compile(r"\s*[-+]?\.?\d")


def is_blank(quantity):
    # An empty card cell is parsed as a dimensionless DBL_MIN, see "empty input" in Base/Quantity.y
    return quantity.Unit == Units.Unit() and quantity.Value == sys.float_info.min


def rows_from_material(material, table):
    """Rows of a FEM table read from a Materials array, a row with an empty cell is dropped

    Each row is one string "temperature, value, ..." written without locale, the form
    the FEM material map already uses for scalars.
    """
    array_name, columns = TABLES[table]
    if not material.hasPhysicalProperty(array_name):
        return []
    names = [column.Name for column in material.PropertyObjects[array_name].Columns]
    indices = [names.index("Temperature")] + [names.index(name) for name in columns]

    rows = []
    for cells in material.getPhysicalValue(array_name).Array:
        kept = [cells[i] for i in indices]
        if any(is_blank(cell) for cell in kept):
            continue
        rows.append(", ".join(str(cell) for cell in kept))
    return rows


def tables_from_material(material):
    return {table: rows_from_material(material, table) for table in TABLES}


def read_row(row, units):
    # None when a cell is no quantity, or the row lacks the cell count or the units of its table
    texts = row.split(",")
    if len(texts) != len(units) + 1 or not all(NUMBER_FIRST.match(text) for text in texts):
        return None
    try:
        cells = [Units.Quantity(text.strip()) for text in texts]
        # 1/0 or 1e400 parse to inf
        if not all(math.isfinite(cell.Value) for cell in cells):
            return None
        for cell, unit in zip(cells, ("K",) + units):
            if not unit:
                if cell.Unit != Units.Unit():
                    return None
            else:
                cell.getValueAs(unit)
        if cells[0].getValueAs("K").Value < 0:
            return None
    except ValueError:
        return None
    return cells


def sorted_rows(rows, table, owner):
    """Rows as (temperature in K, [quantities]), by rising temperature

    CalculiX does not sort a table: given in falling order it keeps the first value
    and does not interpolate, with no warning. The rows are typed by hand, a row that
    can't be read raises a ValueError naming it.
    """
    units, example = ROW_FORMS[table]
    parsed = []
    for row in rows:
        # The list editor leaves an empty item after a last Enter
        if not row.strip():
            continue
        cells = read_row(row, units)
        if cells is None:
            raise ValueError(
                f"{owner}, {table}: can't read the row '{row}'. Write it as '{example}', "
                "the temperature first."
            )
        parsed.append((cells[0].getValueAs("K").Value, cells[1:]))
    parsed.sort(key=lambda entry: entry[0])
    return parsed
