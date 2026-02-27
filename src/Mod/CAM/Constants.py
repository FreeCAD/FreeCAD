# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText:  2025 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

"""
CAM Module Constants

This module contains centralized constants used throughout the CAM workbench,
including G-code commands, M-codes, and other standardized values.
"""

# =============================================================================
# G-Code Motion Commands
# =============================================================================

# Rapid positioning
GCODE_MOVE_RAPID = ["G0", "G00"]

# Linear interpolation (feed moves)
GCODE_MOVE_STRAIGHT = ["G1", "G01"]

# Circular interpolation - clockwise
GCODE_MOVE_CW = ["G2", "G02"]

# Circular interpolation - counter-clockwise
GCODE_MOVE_CCW = ["G3", "G03"]

# Arc moves (combined CW and CCW)
GCODE_MOVE_ARC = GCODE_MOVE_CW + GCODE_MOVE_CCW

# Canned drilling cycles
GCODE_MOVE_DRILL = ["G73", "G81", "G82", "G83", "G85"]

# Cutting moves (feed moves and arcs)
GCODE_MOVE_MILL = GCODE_MOVE_STRAIGHT + GCODE_MOVE_ARC

# All cutting moves (feed moves, arcs, and drill cycles)
GCODE_MOVE = GCODE_MOVE_STRAIGHT + GCODE_MOVE_ARC + GCODE_MOVE_DRILL

# All move commands (cutting moves + rapid)
GCODE_MOVE_ALL = GCODE_MOVE + GCODE_MOVE_RAPID

# =============================================================================
# G-Code Modal Commands
# =============================================================================

# Units mode
GCODE_UNITS = ["G20", "G21"]
GCODE_UNITS_METRIC = ["G21"]
GCODE_UNITS_INCHES = ["G20"]

# Dwell
GCODE_DWELL = ["G4", "G04"]

GCODE_CUTTER_COMPENSATION = ["G40", "G41", "G42"]

# Canned cycle cancel
GCODE_CYCLE_CANCEL = ["G80"]

# Additional drilling cycles
GCODE_DRILL_EXTENDED = ["G74", "G84", "G88", "G89"]

# Probing
GCODE_PROBE = ["G38.2"]

# Distance modes
GCODE_ABSOLUTE = ["G90"]
GCODE_INCREMENTAL = ["G91"]
GCODE_DISTANCE_MODE = GCODE_ABSOLUTE + GCODE_INCREMENTAL

# Coordinate system offset
GCODE_OFFSET = ["G92"]

# Tool length offset
GCODE_TOOL_LENGTH_OFFSET = ["G43"]

# Feed rate modes
GCODE_FEED_INVERSE_TIME = ["G93"]
GCODE_FEED_UNITS_PER_MIN = ["G94"]
GCODE_FEED_UNITS_PER_REV = ["G95"]

# Spindle control modes
GCODE_SPINDLE_CSS = ["G96"]  # Constant surface speed
GCODE_SPINDLE_RPM = ["G97"]  # RPM mode

# Canned cycle return modes
GCODE_RETURN_INITIAL = ["G98"]  # Return to initial Z level
GCODE_RETURN_R = ["G99"]  # Return to R level
GCODE_RETURN_MODE = GCODE_RETURN_INITIAL + GCODE_RETURN_R

# =============================================================================
# Work Coordinate Systems (Fixtures)
# =============================================================================

GCODE_FIXTURES = [
    "G54",
    "G55",
    "G56",
    "G57",
    "G58",
    "G59",
    "G59.1",
    "G59.2",
    "G59.3",
    "G59.4",
    "G59.5",
    "G59.6",
    "G59.7",
    "G59.8",
    "G59.9",
]

# =============================================================================
# M-Codes (Machine Functions)
# =============================================================================

# Program control
MCODE_STOP = ["M0", "M00"]  # Program stop
MCODE_OPTIONAL_STOP = ["M1", "M01"]  # Optional program stop
MCODE_END = ["M2", "M02"]  # Program end
MCODE_END_RESET = ["M30"]  # Program end and reset

# Spindle control
MCODE_SPINDLE_CW = ["M3", "M03"]  # Spindle on clockwise
MCODE_SPINDLE_CCW = ["M4", "M04"]  # Spindle on counter-clockwise
MCODE_SPINDLE_ON = MCODE_SPINDLE_CW + MCODE_SPINDLE_CCW
MCODE_SPINDLE_OFF = ["M5", "M05"]  # Spindle off

# Tool change
MCODE_TOOL_CHANGE = ["M6", "M06"]

# Coolant control
MCODE_COOLANT_MIST = ["M7", "M07"]  # Mist coolant on
MCODE_COOLANT_FLOOD = ["M8", "M08"]  # Flood coolant on
MCODE_COOLANT_ON = MCODE_COOLANT_MIST + MCODE_COOLANT_FLOOD
MCODE_COOLANT_OFF = ["M9", "M09"]  # All coolant off

# =============================================================================
# Combined Lists for Post Processor Support
# =============================================================================

# All supported G-code commands for generic post processor
GCODE_SUPPORTED = (
    GCODE_MOVE_RAPID
    + GCODE_MOVE_STRAIGHT
    + GCODE_MOVE_ARC
    + GCODE_DWELL
    + GCODE_MOVE_DRILL
    + GCODE_DRILL_EXTENDED
    + GCODE_CYCLE_CANCEL
    + GCODE_PROBE
    + GCODE_ABSOLUTE
    + GCODE_INCREMENTAL
    + GCODE_OFFSET
    + GCODE_FEED_INVERSE_TIME
    + GCODE_FEED_UNITS_PER_MIN
    + GCODE_FEED_UNITS_PER_REV
    + GCODE_SPINDLE_CSS
    + GCODE_SPINDLE_RPM
    + GCODE_RETURN_INITIAL
    + GCODE_RETURN_R
    + GCODE_TOOL_LENGTH_OFFSET
)

# All supported M-codes for generic post processor
MCODE_SUPPORTED = (
    MCODE_STOP
    + MCODE_OPTIONAL_STOP
    + MCODE_SPINDLE_CW
    + MCODE_SPINDLE_CCW
    + MCODE_TOOL_CHANGE
    + MCODE_COOLANT_MIST
    + MCODE_COOLANT_FLOOD
    + MCODE_COOLANT_OFF
)

# All coolant M-codes
MCODE_COOLANT = MCODE_COOLANT_MIST + MCODE_COOLANT_FLOOD + MCODE_COOLANT_OFF

# =============================================================================
# Non-Conforming Commands (per ADR-002)
# =============================================================================

# These commands should not be used in operations.  They follow a valid format and
# will be accepted by the Command system.  They are valid for use in post processing.

GCODE_NON_CONFORMING = (
    GCODE_CUTTER_COMPENSATION
    + GCODE_DISTANCE_MODE
    + GCODE_UNITS
    + GCODE_RETURN_MODE
    + GCODE_CYCLE_CANCEL
)
