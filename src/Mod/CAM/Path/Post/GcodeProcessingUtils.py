# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 sliptonic <shopinthewoods@gmail.com>               *
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
"""
Various utilities for handling G-code.
These utilities do NOT operate on Path.Command objects. They
operate on strings of pre-processed G-code.
"""

from typing import List

# Annotation protecting functional rapid moves from filter_inefficient_moves.
# Producers working on Path.Command objects (e.g. DrillCycleExpander) set
# NO_COLLAPSE_ANNOTATION in the command's Annotations (by whole-dict
# reassignment; item assignment does not persist). The formatting layers
# (Processor._convert_move and UtilsParse) translate it into the
# NO_COLLAPSE_MARKER word on the G-code line, which filter_inefficient_moves
# honors and strips from the final output.
NO_COLLAPSE_ANNOTATION = "no_collapse"
NO_COLLAPSE_MARKER = "(no-collapse)"


class NumberGenerator:
    """
    Generate a sequence of line numbers with configurable formatting.

    Args:
        template: Format string for the line number (e.g., 'N{:04d}')
        start: Starting number for the sequence (default: 1)
        increment: Step size for the sequence (default: 1)
    """

    def __init__(self, template: str = "{}", start: int = 1, increment: int = 1):
        """Initialize the number generator with template, start, and increment values."""
        self._template = template
        self._start = start
        self._increment = increment
        self.reset()

    def get(self) -> str:
        """Get the next number in the sequence and format it according to the template."""
        current = self._current
        self._current += self._increment
        return self._template.format(current)

    def reset(self) -> None:
        """Reset the sequence to the starting number."""
        self._current = self._start


# Insert Line Numbers


def insert_line_numbers(gcode: List[str], start: int = 10, increment: int = 10) -> List[str]:
    """Insert line numbers (N-codes) into G-code lines.

    Args:
        gcode: List of G-code strings
        start: Starting line number (default: 10)
        increment: Line number increment (default: 10)

    Returns:
        List of G-code strings with line numbers inserted
    """
    result = []
    line_generator = NumberGenerator(template="N{}", start=start, increment=increment)

    for line in gcode:
        # Skip empty lines and comments
        stripped = line.strip()
        if not stripped or stripped.startswith("("):
            result.append(line)
            continue

        # Insert line number at the beginning
        line_number = line_generator.get()
        result.append(f"{line_number} {line}")

    return result


# Suppress redundant axes words


def suppress_redundant_axes_words(gcode: List[str]) -> List[str]:
    """Suppress redundant axis and feed rate words by tracking current machine state.

    Removes axis words where the value matches the current machine position,
    and F words where the feed rate matches the current feed rate.

    Args:
        gcode: List of G-code strings

    Returns:
        List of G-code strings with redundant words suppressed
    """
    result = []
    current_pos = {
        "X": None,
        "Y": None,
        "Z": None,
        "U": None,
        "V": None,
        "W": None,
        "A": None,
        "B": None,
        "C": None,
    }
    current_feed = None  # Track current feed rate

    for line in gcode:
        stripped = line.strip()

        # Keep comments and empty lines unchanged
        if not stripped or stripped.startswith("("):
            result.append(line)
            continue

        # Reset tracked state on tool change so post-change commands are not
        # suppressed as redundant (the new tool may need the same position/feed).
        if any(stripped.startswith(cmd) for cmd in ["M6", "M06"]):
            current_pos = {k: None for k in current_pos}
            current_feed = None
            result.append(line)
            continue

        # Check for drill cycle commands - these need ALL parameters, don't suppress
        # G80, G98, G99 have no parameters but should pass through
        is_parametric_drill_cycle = any(
            stripped.startswith(cmd)
            for cmd in ["G73", "G74", "G81", "G82", "G83", "G84", "G85", "G86", "G87", "G88", "G89"]
        )
        is_drill_mode_command = any(stripped.startswith(cmd) for cmd in ["G80", "G98", "G99"])

        if is_parametric_drill_cycle:
            # Parametric drill cycles need all parameters preserved
            result.append(line)
            continue
        elif is_drill_mode_command:
            # G80 (cancel), G98 (retract to initial), G99 (retract to R) have no parameters
            result.append(line)
            continue

        # Check for blockdelete slash
        has_blockdelete = line.lstrip().startswith("/")
        blockdelete_prefix = "/" if has_blockdelete else ""

        # Parse the line for axis and feed movements
        words = stripped.split()
        if has_blockdelete and words and words[0].startswith("/"):
            # Remove the slash from the first word if it's a blockdelete command
            words[0] = words[0][1:]
        new_pos = current_pos.copy()
        new_feed = current_feed
        filtered_words = []

        # First pass: collect all movements in this command
        for word in words:
            axis = word[0] if word else ""
            if axis in current_pos:
                try:
                    value = float(word[1:])
                    new_pos[axis] = value
                except (ValueError, IndexError):
                    # If we can't parse the value, skip updating position
                    pass
            elif axis == "F":
                try:
                    value = float(word[1:])
                    new_feed = value
                except (ValueError, IndexError):
                    # If we can't parse the value, skip updating feed rate
                    pass

        # Second pass: filter out redundant words
        for word in words:
            axis = word[0] if word else ""
            if axis in current_pos:
                try:
                    value = float(word[1:])
                    # Only include the axis if it differs from current position
                    if current_pos[axis] != value:
                        filtered_words.append(word)
                except (ValueError, IndexError):
                    # If we can't parse the value, keep the word
                    filtered_words.append(word)
            elif axis == "F":
                try:
                    value = float(word[1:])
                    # Only include F if it differs from current feed rate
                    if current_feed != value:
                        filtered_words.append(word)
                except (ValueError, IndexError):
                    # If we can't parse the value, keep the word
                    filtered_words.append(word)
            else:
                # Non-axis, non-feed words are always included
                filtered_words.append(word)

        # Update current state for next command
        current_pos = new_pos
        current_feed = new_feed

        # Join the filtered words back into a line with preserved blockdelete
        if filtered_words:
            result.append(f"{blockdelete_prefix}{' '.join(filtered_words)}")
        else:
            # If no words left, keep the original line (shouldn't happen for valid G-code)
            result.append(line)

    return result


# Filter inefficient moves


def filter_inefficient_moves(gcode: List[str]) -> List[str]:
    """Collapse runs of consecutive rapid (G0) moves that travel along a
    single axis, keeping only the final move of each run.

    A run is collapsed only when the dropped moves are provably redundant
    stutter: all motion is along one axis, in one direction, from a known
    starting position, and the surviving line states the endpoint. Rapids
    that change more than one axis are never collapsed, and neither is a
    run containing a direction reversal — an excursion such as the
    chip-clearing retract of an expanded peck-drilling cycle is intentional
    motion whose purpose is the excursion itself, not its endpoint.

    Producers can explicitly protect a rapid by annotating its line with the
    NO_COLLAPSE_MARKER word (the formatting layer adds it for Path.Command
    producers that set NO_COLLAPSE_ANNOTATION, e.g. the drill cycle
    expander). Annotated lines never collapse and the marker is stripped
    from the output.

    Absolute positioning (G90) is assumed until a G91 is seen; collapsing is
    suspended in incremental mode. Any command this function does not model
    (tool changes, coordinate system changes, canned cycles, block delete,
    unparsable words, ...) resets position tracking and passes through
    unchanged.

    Args:
        gcode: List of G-code strings

    Returns:
        List of G-code strings with inefficient moves filtered out
    """
    AXES = ("X", "Y", "Z", "A", "B", "C")
    MOTION_GS = {0.0, 1.0, 2.0, 3.0}
    # Words that neither move axes unpredictably nor change what tracked
    # coordinates mean. Anything else invalidates tracked state.
    TRACKED_LETTERS = {"G", "F", "S"} | set(AXES)

    result = []
    pos = {ax: None for ax in AXES}  # tracked absolute position, None = unknown
    absolute_mode = True
    modal_motion = None  # last motion G word seen (0.0-3.0)
    chain = []  # pending consecutive collapsible rapids
    chain_base = None  # tracked position before the first move of the chain

    def changed_axes(move_pos: dict, base: dict) -> set:
        """Axes whose value differs from base; unknown baselines count as changed."""
        changed = set()
        for ax in AXES:
            a, b = move_pos[ax], base[ax]
            if a is None and b is None:
                continue
            if a is None or b is None or a != b:
                changed.add(ax)
        return changed

    def is_monotonic(run: list, run_base: dict, ax: str) -> bool:
        """True when the run's motion along ax never reverses direction,
        starting from a known position."""
        values = [run_base[ax]] + [m["pos"][ax] for m in run]
        if any(v is None for v in values):
            return False
        deltas = [b - a for a, b in zip(values, values[1:])]
        return all(d >= 0 for d in deltas) or all(d <= 0 for d in deltas)

    def emit_run(run: list, run_changed: set, run_base: dict) -> list:
        """Emit a run of rapids, keeping only the last move when that is safe."""
        if len(run) > 1 and len(run_changed) <= 1:
            last = run[-1]
            # The surviving line must state the endpoint of the changing
            # axis, and a reversal (e.g. the chip-clearing retract of an
            # expanded peck cycle) must never be dropped.
            safe = run_changed <= last["axis_letters"] and all(
                is_monotonic(run, run_base, ax) for ax in run_changed
            )
            if safe:
                if last["has_motion_word"]:
                    return [last["line"]]
                # Modal rapid: carry the G0 word forward from a dropped line
                # so the surviving line cannot execute in a different mode.
                g_word = next((m["g_word"] for m in run if m["g_word"]), None)
                if g_word:
                    return [f"{g_word} {last['line'].strip()}"]
        return [m["line"] for m in run]

    def collapse_chain(moves: list, base: dict) -> list:
        """Split a chain into single-axis runs and emit each independently."""
        out = []
        run = []
        run_changed = set()
        run_base = base
        for move in moves:
            move_changed = changed_axes(move["pos"], run_base)
            if run and len(run_changed | move_changed) > 1:
                out.extend(emit_run(run, run_changed, run_base))
                run_base = run[-1]["pos"]
                run = []
                run_changed = set()
                move_changed = changed_axes(move["pos"], run_base)
            run.append(move)
            run_changed |= move_changed
        out.extend(emit_run(run, run_changed, run_base))
        return out

    def flush_chain():
        nonlocal chain, chain_base
        if chain:
            result.extend(collapse_chain(chain, chain_base))
            chain = []
            chain_base = None

    for line in gcode:
        # Honor and strip explicit no-collapse annotations from upstream
        # producers (e.g. drill-cycle retracts marked by DrillCycleExpander).
        blocked = NO_COLLAPSE_MARKER in line
        if blocked:
            line = line.replace(NO_COLLAPSE_MARKER, "").rstrip()

        stripped = line.strip()

        # Comments and empty lines pass through; they end any pending chain.
        if not stripped or stripped.startswith("("):
            flush_chain()
            result.append(line)
            continue

        # Block-deleted lines may or may not execute on the machine, so any
        # Block-deleted lines may or may not execute on the machine, so any
        # axis they mention becomes unknown.
        if stripped.startswith("/"):
            flush_chain()
            modal_motion = None
            for word in stripped[1:].split():
                letter = word[:1].upper()
                if letter in pos:
                    pos[letter] = None
            result.append(line)
            continue
        words = []
        parse_ok = True
        for raw in stripped.split():
            letter = raw[:1].upper()
            try:
                words.append((letter, float(raw[1:]), raw))
            except ValueError:
                parse_ok = False
                break

        g_values = {value for letter, value, raw in words if letter == "G"}
        letters = {letter for letter, value, raw in words}
        motion_words = g_values & MOTION_GS

        # Distance mode words can share a line with other commands, so update
        # the mode even when the line is otherwise passed through below.
        if 91.0 in g_values:
            absolute_mode = False
        elif 90.0 in g_values:
            absolute_mode = True

        if (
            not parse_ok
            or letters - TRACKED_LETTERS
            or g_values - MOTION_GS - {90.0, 91.0}
            or len(motion_words) > 1
        ):
            flush_chain()
            pos = {ax: None for ax in AXES}
            modal_motion = None
            result.append(line)
            continue

        if motion_words:
            modal_motion = next(iter(motion_words))

        pos_before = dict(pos)
        axis_letters = set()
        for letter, value, raw in words:
            if letter in pos:
                axis_letters.add(letter)
                pos[letter] = value if absolute_mode else None

        # A rapid is collapsible only when it is not annotated as functional
        # and carries nothing but motion: no F/S words, no distance mode
        # change riding along.
        collapsible = (
            not blocked
            and absolute_mode
            and modal_motion == 0.0
            and g_values <= {0.0}
            and letters <= {"G"} | set(AXES)
        )

        if collapsible:
            if not chain:
                chain_base = pos_before
            chain.append(
                {
                    "line": line,
                    "pos": dict(pos),
                    "axis_letters": axis_letters,
                    "g_word": next((raw for letter, _, raw in words if letter == "G"), None),
                    "has_motion_word": bool(motion_words),
                }
            )
        else:
            flush_chain()
            result.append(line)

    flush_chain()
    return result


def deduplicate_repeated_commands(gcode: List[str]) -> List[str]:
    """Deduplicate consecutive repeated commands from G-code.

    Removes the command word from consecutive commands of the same type,
    keeping only the parameters. This is modal G-code behavior.

    Example:
        G1 X10 Y20
        G1 X30 Y40  -> X30 Y40 (G1 removed)
        G1 X50 Y60  -> X50 Y60 (G1 removed)
        G0 Z5       -> G0 Z5 (different command, kept)

    Args:
        gcode: List of G-code strings

    Returns:
        List of G-code strings with modal command words removed
    """
    result = []
    last_cmd = None

    for line in gcode:
        stripped = line.strip()

        # Keep comments and empty lines unchanged
        if not stripped or stripped.startswith("("):
            result.append(line)
            continue

        # Reset modal command tracking on tool change so the first command
        # after M6 is always output with its full command word.
        if any(stripped.startswith(cmd) for cmd in ["M6", "M06"]):
            last_cmd = None
            result.append(line)
            continue

        # Extract the primary command (first word)
        words = stripped.split()
        if words:
            cmd = words[0]
            # Check for blockdelete
            if cmd.startswith("/"):
                cmd = cmd[1:]

            if cmd == last_cmd:
                # Same command - output only parameters (remove command word)
                params = " ".join(words[1:])
                if params:  # Only if there are parameters
                    result.append(params)
            else:
                # Different command - output full line
                result.append(line)
                last_cmd = cmd
        else:
            result.append(line)

    return result
