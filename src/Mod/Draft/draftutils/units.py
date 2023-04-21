# ***************************************************************************
# *   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides utility functions to handle quantities and units."""
## @package units
# \ingroup draftutils
# \brief Provides utility functions to handle quantities and units.

## \addtogroup draftutils
# @{
import PySide.QtCore as QtCore

import FreeCAD as App


def get_default_unit(dim):
    """Return default Unit of Measure for a dimension.

    It is based on the user preferences.
    """
    if dim == 'Length':
        qty = App.Units.Quantity(1.0, App.Units.Length)
        uom = qty.getUserPreferred()[2]
    elif dim == 'Angle':
        qty = App.Units.Quantity(1.0, App.Units.Angle)
        uom = qty.getUserPreferred()[2]
    else:
        uom = "xx"
    return uom


getDefaultUnit = get_default_unit


def make_format_spec(decimals=4, dim='Length'):
    """Return a string format specifier with decimals for a dimension.

    It is based on the user preferences.
    """
    if dim == 'Length':
        fmt_spec = "%." + str(decimals) + "f " + get_default_unit('Length')
    elif dim == 'Angle':
        fmt_spec = "%." + str(decimals) + "f " + get_default_unit('Angle')
    else:
        fmt_spec = "%." + str(decimals) + "f " + "??"
    return fmt_spec


makeFormatSpec = make_format_spec


def display_external(internal_value,
                     decimals=None, dim='Length', showUnit=True, unit=None):
    """Return a converted value for display, according to the unit schema.

    Parameters
    ----------
    internal_value: float
        A value that will be transformed depending on the other parameters.

    decimals: float, optional
        It defaults ot `None`, in which case, the decimals are 2.

    dim: str, optional
        It defaults to `'Length'`. It can also be `'Angle'`.

    showUnit: bool, optional
        It defaults to `True`.
        If it is `False` it won't show the unit.

    unit: str, optional
        A unit string such as `'mm'`, `'cm'`, `'m'`, `'in'`, `'ft'`,
        in which to express the returned value.
    """
    if dim == 'Length':
        q = App.Units.Quantity(internal_value, App.Units.Length)
        if not unit:
            if decimals is None and showUnit:
                return q.UserString

            conversion = q.getUserPreferred()[1]
            uom = q.getUserPreferred()[2]
        elif unit.lower() == "arch":
            return App.Units.schemaTranslate(q,5)[0].replace("+"," ")
        else:
            try:
                uom = unit
                internal_value = q.getValueAs(unit)
                conversion = 1
            except Exception:
                conversion = q.getUserPreferred()[1]
                uom = q.getUserPreferred()[2]
    elif dim == 'Angle':
        q = App.Units.Quantity(internal_value, App.Units.Angle)
        if decimals is None:
            return q.UserString

        conversion = q.getUserPreferred()[1]
        uom = q.getUserPreferred()[2]
    else:
        conversion = 1.0
        if not decimals:
            decimals = 2
        uom = "??"

    if not showUnit:
        uom = ""

    decimals = abs(decimals)  # prevent negative values
    fmt = "{0:." + str(decimals) + "f} " + uom
    display = fmt.format(float(internal_value) / float(conversion))
    display = display.replace(".", QtCore.QLocale().decimalPoint())

    return display


displayExternal = display_external

## @}
