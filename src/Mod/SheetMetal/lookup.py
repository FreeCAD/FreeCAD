########################################################################
#
#  lookup.py
#
#  Copyright 2019 Cerem Cem Aslan
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
########################################################################

import collections


def get_val_from_range(lookup, input, interpolate=False):
    """
    lookup: dictionary
    input: float

    For working principle, see below tests
    """
    lookup_sorted = collections.OrderedDict(
        sorted(lookup.items(), key=lambda t: float(t[0]))
    )
    val = None
    prev_val = None
    prev_key = None
    input = float(input)
    for _range in lookup_sorted:
        val = float(lookup_sorted[_range])
        if input > float(_range):
            prev_val = val
            prev_key = float(_range)
            continue

        if interpolate:
            # Do the interpolation here.
            if prev_key is not None:
                key = float(_range)
                # print("interpolate for input: ", input, ": ", prev_key,
                #       "to ", key, "->", prev_val, val)
                input_offset_percentage = (input - prev_key) / (key - prev_key)
                val_diff = val - prev_val
                val_offset = val_diff * input_offset_percentage
                interpolated_val = prev_val + val_offset
                val = int((interpolated_val * 100) + 0.5) / 100.0
                # print("...interpolated to: ", val, interpolated_val)
        break
    return val


mytable = {1: 0.25, 1.1: 0.28, 3: 0.33, 5: 0.42, 7: 0.5}

# Interpolation disabled.
assert get_val_from_range(mytable, 0.1) == 0.25
assert get_val_from_range(mytable, 0.99) == 0.25
assert get_val_from_range(mytable, 1) == 0.25
assert get_val_from_range(mytable, 1.01) == 0.28
assert get_val_from_range(mytable, 1.09) == 0.28
assert get_val_from_range(mytable, 1.2) == 0.33
assert get_val_from_range(mytable, 2.5) == 0.33
assert get_val_from_range(mytable, 4) == 0.42
assert get_val_from_range(mytable, 40) == 0.5
assert get_val_from_range(mytable, 1000) == 0.5

# Interpolation enabled.
assert get_val_from_range(mytable, 0.1, True) == 0.25
assert get_val_from_range(mytable, 0.99, True) == 0.25
assert get_val_from_range(mytable, 1, True) == 0.25
assert get_val_from_range(mytable, 1.01, True) == 0.25
assert get_val_from_range(mytable, 1.09, True) == 0.28
assert get_val_from_range(mytable, 2.05, True) == 0.31
assert get_val_from_range(mytable, 2.5, True) == 0.32
assert get_val_from_range(mytable, 4, True) == 0.38
assert get_val_from_range(mytable, 6, True) == 0.46
assert get_val_from_range(mytable, 40, True) == 0.5
assert get_val_from_range(mytable, 1000, True) == 0.5
