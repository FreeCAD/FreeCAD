# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 <shopinthewoods@gmail.com>
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


"""Rotary-axis wrap-strategy rewrite.

Generators for continuous 4-axis surfacing emit *unwound* rotary values
(monotonic across full revolutions). That is the right shape for
controllers that treat the rotary as a linear axis, but wrong for two
common controller families:

* Wrapped rotaries that take A modulo 360 — large unwound values may be
  rejected or normalized, and a 359°→1° step appears as a -358° move.
* Indexers with strict ±360° travel limits — anything beyond one
  revolution is invalid.

This module rewrites a flat list of ``Path.Command`` objects according
to the rotary axis's :class:`Machine.models.machine.WrapStrategy`,
producing output that the controller will interpret correctly while
preserving cut direction.

ADR-002 constrains internal path commands to a stateless,
machine-agnostic subset — no modal/coordinate-shift G-codes such as
``G92``. To honor that, the REZERO strategy does **not** inject extra
commands; it instead emits modulo-wrapped axis values plus an
``Annotations["rotary_rezero"]`` marker on each move that crosses a
revolution boundary. Carrying the rezero policy as an annotation keeps
the command stream stateless while letting a post-processor render the
boundary as ``G92 <axis>0`` (LinuxCNC), ``G10 L20`` (some controllers),
or simply ignore it (treating REZERO as MODULO) for controllers that
don't need an explicit reset. The same pattern is used by rigid-tap
output (``Annotations["rigid"]``).

The function is pure — no FreeCAD-document side effects, no operation
state. It works on whichever rotary letter the caller passes in.
"""

import Path

# String values mirror Machine.models.machine.WrapStrategy. We accept
# either the enum or its string form so this module stays free of an
# import cycle with the Machine model.
_UNWOUND = "unwound"
_MODULO = "modulo"
_REZERO = "rezero"


def _strategy_value(strategy):
    """Coerce a WrapStrategy enum or string to its string value."""
    if isinstance(strategy, str):
        return strategy
    return getattr(strategy, "value", _UNWOUND)


def apply_wrap_strategy(commands, axis_letter, strategy):
    """Rewrite rotary axis values according to a wrap strategy.

    Parameters
    ----------
    commands : list[Path.Command]
        Unwound input commands. Not mutated; a new list is returned.
    axis_letter : str
        Rotary axis word ("A", "B", or "C").
    strategy : WrapStrategy or str
        UNWOUND returns the input unchanged. MODULO emits values in
        [0, 360). REZERO emits values in [0, 360) and tags each move
        whose source value crossed a revolution boundary with
        ``Annotations["rotary_rezero"] = <axis_letter>``; the tag is
        the post-processor's signal to inject a controller-specific
        modal-A reset (e.g. ``G92 A0``) before that move.

    Returns
    -------
    list[Path.Command]
    """
    s = _strategy_value(strategy)

    if s == _UNWOUND or axis_letter not in ("A", "B", "C"):
        return list(commands)

    if s == _MODULO:
        return _apply_modulo(commands, axis_letter)

    if s == _REZERO:
        return _apply_rezero(commands, axis_letter)

    raise ValueError(f"unknown wrap strategy: {strategy!r}")


def _wrap_360(angle):
    """Wrap angle into [0, 360)."""
    a = angle % 360.0
    if a < 0.0:
        a += 360.0
    return a


def _apply_modulo(commands, axis_letter):
    """Replace each rotary value with its modulo-360 equivalent."""
    out = []
    for cmd in commands:
        params = dict(cmd.Parameters)
        if axis_letter in params:
            params[axis_letter] = _wrap_360(params[axis_letter])
        out.append(_clone(cmd, params))
    return out


def _apply_rezero(commands, axis_letter):
    """Emit wrapped values and annotate revolution-boundary moves.

    The rotary axis letter is rewritten to a value in [0, 360). When
    a source move's relative position crosses a 360° boundary versus
    the current frame, the frame is rebased to that boundary and the
    rewritten move is tagged with ``Annotations["rotary_rezero"]`` set
    to the axis letter. The post-processor reads that tag to render a
    controller-specific modal-A reset (e.g. ``G92 <axis>0``) before
    the tagged move.

    No ``G92`` or ``G92.1`` commands are emitted into the path — those
    would violate ADR-002 (no modal commands in the internal command
    stream). All boundary policy is carried as annotation metadata.
    """
    out = []
    # frame_offset is the unwound value the controller is to treat as
    # A=0 after the most recent (annotation-driven) modal reset.
    frame_offset = 0.0

    for cmd in commands:
        params = dict(cmd.Parameters)
        if axis_letter not in params:
            out.append(_clone(cmd, params))
            continue

        unwound = float(params[axis_letter])
        relative = unwound - frame_offset
        crossed = False

        # Rebase the frame to the boundary the source value crossed
        # before emitting. Direction is preserved by always rebasing in
        # the direction of travel.
        if relative >= 360.0:
            full_revs = int(relative // 360.0)
            frame_offset += full_revs * 360.0
            relative = unwound - frame_offset
            crossed = True
        elif relative < 0.0:
            full_revs = int((-relative) // 360.0) + 1
            frame_offset -= full_revs * 360.0
            relative = unwound - frame_offset
            crossed = True

        params[axis_letter] = relative
        new_cmd = _clone(cmd, params)
        if crossed:
            ann = dict(getattr(new_cmd, "Annotations", {}) or {})
            ann["rotary_rezero"] = axis_letter
            new_cmd.Annotations = ann
        out.append(new_cmd)

    return out


def _clone(cmd, params):
    """Make a fresh Path.Command preserving any source annotations."""
    new_cmd = Path.Command(cmd.Name, params)
    src_ann = getattr(cmd, "Annotations", None)
    if src_ann:
        new_cmd.Annotations = dict(src_ann)
    return new_cmd
