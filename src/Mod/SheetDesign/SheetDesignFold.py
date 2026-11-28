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


class Motion:
    """The motion of a part with coupled folds, from the fold solver of the kernel."""

    def __init__(self, path, order, changed):
        self.states = path["States"]
        self.complete = path["Complete"]
        self.mismatch = path["Mismatch"]
        self.notes = list(path["Notes"])
        #: The number of each score in the sequence that the solver used.
        self.order = order
        #: The scores that the solver moved to an earlier place, by their number of the user.
        self.changed = changed

    def angles(self, state):
        """The angle of each bend for a state from 0 to 1."""
        position = min(1.0, max(0.0, state)) * (len(self.states) - 1)
        low = int(position)
        high = min(low + 1, len(self.states) - 1)
        part = position - low
        return [a + (b - a) * part for a, b in zip(self.states[low], self.states[high])]


_MOTIONS = {}


def motion(flat):
    """The motion of a part with coupled folds. None if the part has no loop of panels.

    The fold sequence of the flat pattern can be impossible for a part with coupled folds: a
    score cannot fold after a different score closed its loop. The function then moves the
    first score that cannot move to the start of the sequence and tries again.
    """
    source = flat.Source
    info = SheetDesign.modelInfo(source)
    if not any(bend["Closing"] for bend in info["Bends"]):
        return None
    groups, numbers = SheetDesignExport.scores_of(flat, len(info["Bends"]))
    key = (source.Document.Name, source.Name, tuple(numbers), tuple(round(b["Angle"], 9) for b in info["Bends"]))
    if key in _MOTIONS:
        return _MOTIONS[key]
    order = list(numbers)
    changed = []
    result = None
    for _attempt in range(len(groups) + 1):
        path = SheetDesign.foldPath(source, groups, order, 12)
        result = Motion(path, list(order), list(changed))
        if path["Complete"]:
            break
        blocked = [note for note in path["Notes"] if "cannot move" in note]
        if not blocked:
            break
        number = int(blocked[0].split(":")[0].split()[-1])
        index = order.index(number)
        user_number = numbers[index]
        if user_number in changed:
            break  # The score was moved before: no sequence makes this part fold.
        changed.append(user_number)
        # The score goes to the start. The scores before it move one place.
        order = [value + 1 if value < number else value for value in order]
        order[index] = 1
    _MOTIONS.clear()
    _MOTIONS[key] = result
    return result


def fractions(flat, state, in_sequence=True):
    """The part of each bend that is done, for a state from 0 (flat) to 1 (folded).

    The list is in the sequence of the bend lines. With in_sequence, the scores fold one after
    the other, in the bend sequence of the flat pattern (BendOrder). All bends of one score
    fold together.

    A part with coupled folds has loops of panels. Its bends cannot fold alone, thus the fold
    solver gives the angles, with or without in_sequence.
    """
    # The count comes from the model. The flat pattern of an older file can have no bend data
    # before its first recompute.
    count = SheetDesign.bendCount(flat.Source)
    state = min(1.0, max(0.0, state))
    coupled = motion(flat) if count else None
    if coupled is not None and coupled.complete:
        final = [bend["Angle"] for bend in SheetDesign.modelInfo(flat.Source)["Bends"]]
        return [
            min(1.0, max(0.0, angle / end)) if end > 1e-9 else 1.0
            for angle, end in zip(coupled.angles(state), final)
        ]
    if not in_sequence or count == 0:
        return [state] * count
    numbers = SheetDesignExport.bend_numbers(flat, count)
    scores = max(numbers)
    return [min(1.0, max(0.0, state * scores - (number - 1))) for number in numbers]


def message(flat):
    """A text about the fold of a part with coupled folds. Empty for other parts."""
    coupled = motion(flat)
    if coupled is None:
        return ""
    if not coupled.complete:
        return "This part has coupled folds, and the solver found no fold sequence. The view between the ends is not exact."
    if coupled.changed:
        scores = ", ".join(str(number) for number in coupled.changed)
        return f"Coupled folds: the sequence is not possible as it is. Folded first: score {scores}."
    return "Coupled folds: the solver gives the angles."


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
