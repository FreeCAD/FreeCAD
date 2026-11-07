# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 The FreeCAD project association <fpa@freecad.org>  *
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

__title__ = "FreeCAD FEM tables of loads given at points"
__author__ = "The FreeCAD project association"
__url__ = "https://www.freecad.org"

## @package loadtable
#  \ingroup FEM
#  \brief a load known at points (x, y, z, value), read from a sheet, a CSV file or rows

import csv
import re

import numpy as np

from FreeCAD import Units

HEADER_UNIT = re.compile(r"\[(.+)\]")


def _quantity(text, default_unit):
    # A cell keeps its own unit, a bare number takes the one of its column
    quantity = Units.Quantity(text)
    if quantity.Unit == Units.Unit() and default_unit:
        quantity = Units.Quantity(f"{quantity.Value} {default_unit}")
    return quantity


def _to_points(cells, units, value_unit):
    points = []
    for number, row in enumerate(cells, start=1):
        if len(row) < 4:
            raise ValueError(
                f"Load table row {number} has {len(row)} values, give x, y, z and value"
            )
        x, y, z, value = (_quantity(cell, unit) for cell, unit in zip(row, units))
        try:
            points.append(
                (
                    x.getValueAs("mm").Value,
                    y.getValueAs("mm").Value,
                    z.getValueAs("mm").Value,
                    value.getValueAs(value_unit).Value,
                )
            )
        except (ValueError, RuntimeError) as error:
            raise ValueError(
                f"Load table row {number}: {error}. Write the unit in each cell "
                f"or in the header, positions in a length and the value in {value_unit}"
            ) from None
    return np.array(points, dtype=float).reshape(-1, 4)


def _header_units(header):
    units = []
    for name in header[:4]:
        found = HEADER_UNIT.search(str(name))
        units.append(found.group(1).strip() if found else "")
    return units


def points_from_rows(rows, value_unit):
    """Rows "x, y, z, value", each cell with its unit"""
    cells = [[cell.strip() for cell in row.split(",")] for row in rows if row.strip()]
    return _to_points(cells, ["", "", "", ""], value_unit)


def points_from_csv(path, value_unit):
    """CSV file, first line "x [mm], y [mm], z [mm], flux [W/m^2]" then the values"""
    with open(path, newline="", encoding="utf-8") as handle:
        lines = [line for line in csv.reader(handle) if line and any(c.strip() for c in line)]
    units = _header_units(lines[0])
    return _to_points([[c.strip() for c in line[:4]] for line in lines[1:]], units, value_unit)


def points_from_sheet(sheet, value_unit):
    """Spreadsheet, columns A to D, row 1 the same header as a CSV file"""
    units = _header_units([sheet.getContents(f"{col}1") for col in "ABCD"])
    cells = []
    row = 2
    while sheet.getContents(f"A{row}"):
        cells.append([str(sheet.get(f"{col}{row}")) for col in "ABCD"])
        row += 1
    return _to_points(cells, units, value_unit)


def points_from_object(obj, value_unit):
    """The table a constraint points to, whatever its source"""
    if obj.TableSource == "Spreadsheet":
        return points_from_sheet(obj.TableSheet, value_unit)
    if obj.TableSource == "File":
        return points_from_csv(obj.TableFile, value_unit)
    return points_from_rows(obj.TableRows, value_unit)


def interpolate(points, queries, neighbours=4, power=2.0, budget=2e7):
    """Inverse distance weighting of points (x, y, z, value) at the query positions

    A query on a data point takes its value. Queries go by chunks sized so the
    distance arrays hold about budget numbers whatever the table length.
    """
    xyz, values = points[:, :3], points[:, 3]
    queries = np.asarray(queries, dtype=float).reshape(-1, 3)
    count = min(neighbours, len(values))
    chunk = max(1, int(budget // (3 * len(values))))
    result = np.empty(len(queries))
    for start in range(0, len(queries), chunk):
        block = queries[start : start + chunk]
        dist = np.linalg.norm(block[:, None, :] - xyz[None, :, :], axis=2)
        nearest = np.argpartition(dist, count - 1, axis=1)[:, :count]
        near_dist = np.take_along_axis(dist, nearest, axis=1)
        weights = 1.0 / np.maximum(near_dist, 1e-12) ** power
        result[start : start + len(block)] = (weights * values[nearest]).sum(axis=1) / weights.sum(
            axis=1
        )
    return result
