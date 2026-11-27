# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""The fold state: a view of a part between flat and folded.

The fold state does not change the document. The kernel gives the solid for a part of each bend
angle (SheetDesign.foldedShape). This module gives the part of each bend for one state value,
with or without the bend sequence of the flat pattern.
"""

import SheetDesign
import SheetDesignExport


def fractions(flat, state, in_sequence=True):
    """The part of each bend that is done, for a state from 0 (flat) to 1 (folded).

    The list is in the sequence of the bend lines. With in_sequence, the scores fold one after
    the other, in the bend sequence of the flat pattern (BendOrder). All bends of one score
    fold together.
    """
    # The count comes from the model. The flat pattern of an older file can have no bend data
    # before its first recompute.
    count = SheetDesign.bendCount(flat.Source)
    state = min(1.0, max(0.0, state))
    if not in_sequence or count == 0:
        return [state] * count
    numbers = SheetDesignExport.bend_numbers(flat, count)
    scores = max(numbers)
    return [min(1.0, max(0.0, state * scores - (number - 1))) for number in numbers]


def shape(flat, state, in_sequence=True, fuse=False):
    """The solid of the source of a flat pattern in a fold state. Returns (shape, overlap)."""
    return SheetDesign.foldedShape(flat.Source, fractions(flat, state, in_sequence), fuse)


def worst_overlap(flat, steps=20, in_sequence=True):
    """Look at the fold in steps. Returns (largest overlap, state value where it is)."""
    worst, where = 0.0, 0.0
    for index in range(steps + 1):
        state = index / steps
        _shape, overlap = shape(flat, state, in_sequence, True)
        if overlap > worst:
            worst, where = overlap, state
    return worst, where
