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


class NumberGenerator:
    """
    Generate a sequence of line numbers with configurable formatting.
    
    Args:
        template: Format string for the line number (e.g., 'N{:04d}')
        start: Starting number for the sequence (default: 1)
        increment: Step size for the sequence (default: 1)
    """
    
    def __init__(self, template: str = '{}', start: int = 1, increment: int = 1):
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
    line_generator = NumberGenerator(template='N{}', start=start, increment=increment)
    
    for line in gcode:
        # Skip empty lines and comments
        stripped = line.strip()
        if not stripped or stripped.startswith('('):
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
    current_pos = {'X': None, 'Y': None, 'Z': None, 'U': None, 'V': None, 'W': None, 'A': None, 'B': None, 'C': None}
    current_feed = None  # Track current feed rate
    
    for line in gcode:
        stripped = line.strip()
        
        # Keep comments and empty lines unchanged
        if not stripped or stripped.startswith('('):
            result.append(line)
            continue
        
        # Check for drill cycle commands - these need ALL parameters, don't suppress
        # G80, G98, G99 have no parameters but should pass through
        is_parametric_drill_cycle = any(stripped.startswith(cmd) for cmd in ['G73', 'G74', 'G81', 'G82', 'G83', 'G84', 'G85', 'G86', 'G87', 'G88', 'G89'])
        is_drill_mode_command = any(stripped.startswith(cmd) for cmd in ['G80', 'G98', 'G99'])
        
        if is_parametric_drill_cycle:
            # Parametric drill cycles need all parameters preserved
            result.append(line)
            continue
        elif is_drill_mode_command:
            # G80 (cancel), G98 (retract to initial), G99 (retract to R) have no parameters
            result.append(line)
            continue
            
        # Check for blockdelete slash
        has_blockdelete = line.lstrip().startswith('/')
        blockdelete_prefix = '/' if has_blockdelete else ''
        
        # Parse the line for axis and feed movements
        words = stripped.split()
        if has_blockdelete and words and words[0].startswith('/'):
            # Remove the slash from the first word if it's a blockdelete command
            words[0] = words[0][1:]
        new_pos = current_pos.copy()
        new_feed = current_feed
        filtered_words = []
        
        # First pass: collect all movements in this command
        for word in words:
            axis = word[0] if word else ''
            if axis in current_pos:
                try:
                    value = float(word[1:])
                    new_pos[axis] = value
                except (ValueError, IndexError):
                    # If we can't parse the value, skip updating position
                    pass
            elif axis == 'F':
                try:
                    value = float(word[1:])
                    new_feed = value
                except (ValueError, IndexError):
                    # If we can't parse the value, skip updating feed rate
                    pass
        
        # Second pass: filter out redundant words
        for word in words:
            axis = word[0] if word else ''
            if axis in current_pos:
                try:
                    value = float(word[1:])
                    # Only include the axis if it differs from current position
                    if current_pos[axis] != value:
                        filtered_words.append(word)
                except (ValueError, IndexError):
                    # If we can't parse the value, keep the word
                    filtered_words.append(word)
            elif axis == 'F':
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
    """Filter out inefficient or redundant moves from G-code.
    
    Removes unnecessary rapid (G0) moves by collapsing chains that only move
    along single axes or within linear/rotary groups.
    
    Args:
        gcode: List of G-code strings
        
    Returns:
        List of G-code strings with inefficient moves filtered out
    """
    AXES = ("X", "Y", "Z", "A", "B", "C")
    
    SIDE_EFFECT_KEYS = {
        "tool", "tool_change", "spindle", "spindle_on", "spindle_off",
        "coolant", "dwell", "feed", "F", "M"
    }
    
    def parse_gcode_line(line: str) -> dict:
        """Parse a G-code line into command name and parameters."""
        stripped = line.strip()
        if not stripped or stripped.startswith('('):
            return {'name': 'COMMENT', 'params': {}, 'original': line}
            
        # Check for blockdelete
        has_blockdelete = stripped.startswith('/')
        if has_blockdelete:
            stripped = stripped[1:]
            
        words = stripped.split()
        if not words:
            return {'name': 'EMPTY', 'params': {}, 'original': line}
            
        cmd_name = words[0]
        params = {}
        
        for word in words[1:]:
            if len(word) > 1:
                key = word[0]
                try:
                    value = float(word[1:])
                    params[key] = value
                except (ValueError, IndexError):
                    params[word] = None  # Non-numeric parameter
                    
        return {
            'name': cmd_name,
            'params': params,
            'original': line,
            'blockdelete': has_blockdelete
        }
    
    def is_rapid(parsed_cmd: dict) -> bool:
        """Check if command is a rapid move (G0)."""
        return parsed_cmd['name'] in ('G0', 'G00')
    
    def has_side_effects(parsed_cmd: dict) -> bool:
        """Check if command has side effects that prevent optimization."""
        # Check for side effect parameter keys
        if any(k in parsed_cmd['params'] for k in SIDE_EFFECT_KEYS):
            return True
            
        # Check for M-codes and other side effect commands
        cmd = parsed_cmd['name']
        if cmd.startswith('M') or cmd in ('G28', 'G30', 'G53', 'G54', 'G55', 'G56', 'G57', 'G58', 'G59',
                                         'G92', 'G10', 'T',  # Tool change
                                         'G73', 'G74', 'G80', 'G81', 'G82', 'G83', 'G84', 'G85', 'G86', 'G87', 'G88', 'G89',  # Drill cycles
                                         'G98', 'G99'):  # Retract modes
            return True
            
        return False
    
    def full_position(parsed_cmd: dict, last_pos: dict) -> dict:
        """Compute full position from command and last position."""
        pos = {}
        for ax in AXES:
            if ax in parsed_cmd['params'] and parsed_cmd['params'][ax] is not None:
                pos[ax] = parsed_cmd['params'][ax]
            else:
                pos[ax] = last_pos.get(ax)
        return pos
    
    def collapse_rapid_chain(chain):
        """
        Collapse a chain of rapid moves.
        chain = list of dicts with 'parsed', 'pos', and 'original' keys.
        """
        if not chain:
            return []
            
        # Check which axes change across the chain
        first = chain[0]['pos']
        axes_changed = {ax for ax in AXES if any(c['pos'][ax] != first[ax] for c in chain)}
        
        # If only one axis changes → keep only the last command
        if len(axes_changed) == 1:
            return [chain[-1]['original']]
            
        # If changes are only within linear or rotary groups → keep only last
        lin = {"X", "Y", "Z"}
        rot = {"A", "B", "C"}
        
        if axes_changed <= lin or axes_changed <= rot:
            return [chain[-1]['original']]
            
        # Mixed changes → can't collapse, keep all
        return [c['original'] for c in chain]
    
    # Main optimization logic
    result = []
    rapid_chain = []
    last_full_pos = {ax: None for ax in AXES}
    
    def flush_chain():
        nonlocal rapid_chain
        if rapid_chain:
            result.extend(collapse_rapid_chain(rapid_chain))
            rapid_chain = []
    
    for line in gcode:
        parsed = parse_gcode_line(line)
        
        # Skip comments and empty lines
        if parsed['name'] in ('COMMENT', 'EMPTY'):
            flush_chain()  # Flush any pending rapid chain
            result.append(line)
            continue
            
        # Get full position for this command
        pos = full_position(parsed, last_full_pos)
        last_full_pos = pos
        
        # Check if this is a rapid move without side effects
        if is_rapid(parsed) and not has_side_effects(parsed):
            rapid_chain.append({
                'parsed': parsed,
                'pos': pos,
                'original': line
            })
        else:
            flush_chain()  # Flush any pending rapid chain before adding this command
            result.append(line)
    
    # Flush any remaining rapid chain
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

        # Extract the primary command (first word)
        words = stripped.split()
        if words:
            cmd = words[0]
            # Check for blockdelete
            if cmd.startswith("/"):
                cmd = cmd[1:]

            if cmd == last_cmd:
                # Same command - output only parameters (remove command word)
                params = ' '.join(words[1:])
                if params:  # Only if there are parameters
                    result.append(params)
            else:
                # Different command - output full line
                result.append(line)
                last_cmd = cmd
        else:
            result.append(line)

    return result
