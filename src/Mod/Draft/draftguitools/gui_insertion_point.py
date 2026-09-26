# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Furgo
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
"""Provides insertion-point cycling for interactive object placement commands.

During interactive placement, a command may offer several candidate anchor
points on the object's cross-section (for instance, its center, corners, and
edge midpoints) rather than always anchoring at the center. This module
provides the shared, object-agnostic mechanics for that: the candidate table,
the anchor offset calculation, the keyboard cycling behavior, and the
associated status bar hint. A consuming command supplies its own cross-section
dimensions, placement rotation, and (optionally) a shape origin offset; this
module has no knowledge of what kind of object is being placed.
"""

## @package gui_insertion_point
# \ingroup draftguitools
# \brief Provides insertion-point cycling for interactive object placement commands.

## \addtogroup draftguitools
# @{
import FreeCAD as App
import FreeCADGui as Gui

if App.GuiUp:
    from PySide import QtCore, QtGui
    import draftguitools.gui_tool_utils as gui_tool_utils
    from draftutils.translate import translate
else:

    def translate(context, text):
        return text


# Nine (u, v) unit offsets for a rectangular cross-section: center, four
# corners, four edge midpoints. A consumer scales these by half its own
# cross-section dimensions. Cycling order: center first, then clockwise
# from bottom-left.
POINT_CANDIDATES = [
    (0, 0),  # 0: Center
    (-1, -1),  # 1: Bottom-Left
    (-1, 0),  # 2: Left-Mid
    (-1, 1),  # 3: Top-Left
    (0, 1),  # 4: Top-Mid
    (1, 1),  # 5: Top-Right
    (1, 0),  # 6: Right-Mid
    (1, -1),  # 7: Bottom-Right
    (0, -1),  # 8: Bottom-Mid
]


def insertion_point_offset(
    cross_section_width,
    cross_section_height,
    insertion_index,
    rotation,
    width_axis=None,
    height_axis=None,
    shape_origin=None,
):
    """Return the insertion point as a world-space offset from the cursor.

    Computes one of nine candidate points on the cross-section (center + 4 corners +
    4 midpoints), selected by *insertion_index*, then transforms it to world space using
    *rotation*. Subtract the result from the cursor point to place the object with the
    chosen insertion point aligned to the cursor.

    *rotation* must be the same value the caller uses to set the object's actual
    placement, so the anchor can never end up somewhere other than where the object
    actually lands.

    Parameters
    ----------
    cross_section_width, cross_section_height : float
        The object's cross-section dimensions.
    insertion_index : int
        Index into the 9-point candidate table (wraps around).
    rotation : FreeCAD.Rotation
        The object's actual placement rotation.
    width_axis, height_axis : FreeCAD.Vector, optional
        The local axes the width and height candidates run along. Default to
        local X and Y. Pass different axes when the object's cross-section
        lies in a different local plane, such as local Y and Z for an object
        extruded along local X.
    shape_origin : FreeCAD.Vector, optional
        The shape's geometric origin in local coordinates, relative to the
        cross-section center. Defaults to no shift. Pass a non-zero value
        for shapes whose geometry is not built from the center, such as one
        built from a corner.
    """
    width_axis = App.Vector(1, 0, 0) if width_axis is None else width_axis
    height_axis = App.Vector(0, 1, 0) if height_axis is None else height_axis

    u, v = POINT_CANDIDATES[insertion_index % len(POINT_CANDIDATES)]
    half_width = cross_section_width / 2
    half_height = cross_section_height / 2

    insertion_point = (
        App.Vector(width_axis)
        .multiply(u * half_width)
        .add(App.Vector(height_axis).multiply(v * half_height))
    )

    if shape_origin:
        insertion_point = insertion_point - shape_origin

    return rotation.multVec(insertion_point)


def insertion_point_hint():
    """Return the generic 'next/previous insertion point' status bar hint.

    The wording never mentions the object being placed, so it is shared,
    unparameterized, and identical across every command that uses this module.
    """
    return Gui.InputHint(
        translate("draft", "%1 next insertion point / %2+%1 previous"),
        Gui.UserInput.KeyI,
        Gui.UserInput.KeyShift,
    )


def placement_hints(action_hint, cycler_active=True):
    """Compose the standard hint list for an interactive placement command.

    Parameters
    ----------
    action_hint : FreeCADGui.InputHint
        The consumer-specific hint for the current placement step, such as
        "insert column" or "pick first point of beam".
    cycler_active : bool, optional
        Whether an insertion point anchor currently exists to cycle through.
        Defaults to True. Pass False before enough points have been picked
        for an anchor to exist yet.
    """
    hints = [action_hint]
    if cycler_active:
        hints.append(insertion_point_hint())
    return hints + (
        gui_tool_utils._get_hint_xyz_constrain()
        + gui_tool_utils._get_hint_mod_constrain()
        + gui_tool_utils._get_hint_mod_snap()
    )


if App.GuiUp:

    class _CycleKeyFilter(QtCore.QObject):
        """Application-level Qt event filter that intercepts 'I' keypresses to
        cycle the insertion point anchor of an active InsertionPointCycler."""

        def __init__(self, cycler):
            super().__init__()
            self._cycler = cycler

        def eventFilter(self, watched, event):
            if event.type() == QtCore.QEvent.KeyPress:
                if event.text().upper() == "I":
                    shift = bool(event.modifiers() & QtCore.Qt.ShiftModifier)
                    self._cycler.cycle(shift)
                    return True
            return False


class InsertionPointCycler:
    """Owns the current insertion point index and the keyboard filter that
    cycles it, for one interactive placement command.

    Usage
    -----
    Construct one instance per placement command invocation. Call ``install``
    when the command becomes active and ``remove`` when it ends, on every
    exit path (including cancellation). ``index`` is read by the command to
    select the current candidate from ``POINT_CANDIDATES`` via
    ``insertion_point_offset``.

    Parameters
    ----------
    on_cycle : callable
        Called with no arguments whenever the index changes, so the caller
        can refresh its live preview. This module has no knowledge of what
        the caller is previewing.
    """

    def __init__(self, on_cycle):
        self.index = 0
        self._on_cycle = on_cycle
        self._filter = None

    def cycle(self, reverse=False):
        """Advance (or reverse) the insertion point index and notify the caller."""
        self.index += -1 if reverse else 1
        self._on_cycle()

    def install(self):
        """Install the application-level keyboard filter. Idempotent."""
        if self._filter is None:
            self._filter = _CycleKeyFilter(self)
            QtGui.QApplication.instance().installEventFilter(self._filter)

    def remove(self):
        """Remove the keyboard filter, if installed. Idempotent."""
        if self._filter is not None:
            QtGui.QApplication.instance().removeEventFilter(self._filter)
            self._filter = None


## @}
