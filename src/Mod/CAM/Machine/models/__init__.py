# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Billy Huddleston <billy@ivdc.com>
# SPDX-FileNotice: Part of the FreeCAD project.

from .machine import (
    Machine,
    MachineFactory,
    LinearAxis,
    RotaryAxis,
    Toolhead,
    ToolheadType,
    MachineUnits,
    OutputUnits,
    OutputOptions,
    ProcessingOptions,
)

# Legacy compatibility aliases
Spindle = Toolhead
SpindleType = ToolheadType

__all__ = [
    "Machine",
    "MachineFactory",
    "LinearAxis",
    "RotaryAxis",
    "Toolhead",
    "ToolheadType",
    "Spindle",  # Legacy alias
    "SpindleType",  # Legacy alias
    "MachineUnits",
    "OutputUnits",
    "OutputOptions",
    "ProcessingOptions",
]
