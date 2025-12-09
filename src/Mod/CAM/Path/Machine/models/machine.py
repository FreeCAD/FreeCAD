# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Billy Huddleston <billy@ivdc.com>                  *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************
import json
import Path
import FreeCAD
import pathlib
from dataclasses import dataclass, field
from typing import Dict, Any, List, Optional, Tuple, Callable
from collections import namedtuple
from enum import Enum


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


# Reference axis vectors
RefAxes = namedtuple("RefAxes", ["x", "y", "z"])
refAxis = RefAxes(
    FreeCAD.Vector(1, 0, 0),  # x: linear direction
    FreeCAD.Vector(0, 1, 0),  # y: linear direction
    FreeCAD.Vector(0, 0, 1),  # z: linear direction
)

RefRotAxes = namedtuple("RefRotAxes", ["a", "b", "c"])
refRotAxis = RefRotAxes(
    FreeCAD.Vector(1, 0, 0),  # a: rotational direction
    FreeCAD.Vector(0, 1, 0),  # b: rotational direction
    FreeCAD.Vector(0, 0, 1),  # c: rotational direction
)


# ============================================================================
# Enums for Machine Configuration
# ============================================================================


class MachineUnits(Enum):
    """Machine unit system."""

    METRIC = "G21"
    IMPERIAL = "G20"


class MotionMode(Enum):
    """Motion mode for machine movements."""

    ABSOLUTE = "G90"
    RELATIVE = "G91"


class OutputUnits(Enum):
    """Output unit system for G-code generation."""

    METRIC = "metric"
    IMPERIAL = "imperial"


# ============================================================================
# Post-Processor Configuration Dataclasses
# ============================================================================


@dataclass
class OutputOptions:
    """Controls what gets included in the G-code output and its formatting."""

    # These options control conversion of Path Objects to actual gcode.

    output_units: OutputUnits = OutputUnits.METRIC  # G-code output units
    comments: bool = True
    blank_lines: bool = True
    header: bool = True
    line_numbers: bool = False
    path_labels: bool = False
    machine_name: bool = False
    doubles: bool = False

    # Line formatting options
    command_space: str = " "
    comment_symbol: str = "("
    line_increment: int = 10
    line_number_start: int = 100
    end_of_line_chars: str = "\n"

    # Numeric precision settings
    axis_precision: int = 3  # Decimal places for axis coordinates
    feed_precision: int = 3  # Decimal places for feed rates
    spindle_decimals: int = 0  # Decimal places for spindle speed


@dataclass
class GCodeBlocks:
    """
    G-code block templates for various lifecycle hooks.

    These templates are inserted at specific points during postprocessing
    to provide customization points for machine-specific behavior.
    """

    safetyblock: str = ""  # Safety commands (G40, G49, etc.) Reset machine to known safe condition

    # Legacy aliases (maintained for compatibility)
    preamble: str = ""  # Typically inserted at start of job

    # Job lifecycle
    pre_job: str = ""

    # Operation lifecycle
    pre_operation: str = ""
    post_operation: str = ""

    # Tool change lifecycle
    pre_tool_change: str = ""
    post_tool_change: str = ""
    tool_return: str = ""  # Return to tool change position  # do we need this?

    # Fixture/WCS change lifecycle
    pre_fixture_change: str = ""
    post_fixture_change: str = ""

    # Rotary axis lifecycle
    pre_rotary_move: str = ""
    post_rotary_move: str = ""

    # Spindle lifecycle
    # pre_spindle_change: str = ""   # Futures
    # post_spindle_change: str = ""   # Futures

    # Miscellaneous
    # finish_label: str = "Finish"   # do we need this?   Looks like bullshit

    post_job: str = ""
    postamble: str = ""  # Typically inserted at end of job


@dataclass
class ProcessingOptions:
    """Processing and transformation options."""

    # connversion and expansion of Path Objects.  Does not affect final gcode generation

    modal: bool = False  # Suppress repeated commands
    translate_drill_cycles: bool = False
    split_arcs: bool = False
    show_editor: bool = True
    list_tools_in_preamble: bool = False
    show_machine_units: bool = True
    show_operation_labels: bool = True
    tool_before_change: bool = False  # Output T before M6 (e.g., T1 M6 instead of M6 T1)

    # Lists of commands
    drill_cycles_to_translate: List[str] = field(
        default_factory=lambda: ["G73", "G81", "G82", "G83"]
    )
    suppress_commands: List[str] = field(default_factory=list)

    # Output processing options
    tool_change: bool = True  # Enable tool change commands
    adaptive: bool = False  # Enable adaptive toolpath optimization

    # Numeric settings
    chipbreaking_amount: float = 0.25  # mm
    spindle_wait: float = 0.0  # seconds
    return_to: Optional[Tuple[float, float, float]] = None  # (x, y, z) or None


# ============================================================================
# Machine Component Dataclasses
# ============================================================================


@dataclass
class LinearAxis:
    """Represents a single linear axis in a machine configuration"""

    name: str
    direction_vector: FreeCAD.Vector
    min_limit: float = 0
    max_limit: float = 1000
    max_velocity: float = 10000
    sequence: int = 0

    def __post_init__(self):
        """Normalize direction vector after initialization"""
        self.direction_vector = self.direction_vector.normalize()

    def is_valid_position(self, position):
        """Check if a position is within this axis's limits"""
        return self.min_limit <= position <= self.max_limit

    def to_dict(self):
        """Serialize to dictionary for JSON persistence"""
        return {
            "name": self.name,
            "direction_vector": [
                self.direction_vector.x,
                self.direction_vector.y,
                self.direction_vector.z,
            ],
            "min_limit": self.min_limit,
            "max_limit": self.max_limit,
            "max_velocity": self.max_velocity,
            "sequence": self.sequence,
        }

    @classmethod
    def from_dict(cls, data):
        """Deserialize from dictionary"""
        vec = FreeCAD.Vector(
            data["direction_vector"][0], data["direction_vector"][1], data["direction_vector"][2]
        )
        return cls(
            data["name"],
            vec,
            data.get("min_limit", 0),
            data.get("max_limit", 1000),
            data.get("max_velocity", 10000),
            data.get("sequence", 0),
        )


@dataclass
class RotaryAxis:
    """Represents a single rotary axis in a machine configuration"""

    name: str
    rotation_vector: FreeCAD.Vector
    min_limit: float = -360
    max_limit: float = 360
    max_velocity: float = 36000
    sequence: int = 0
    prefer_positive: bool = True

    def __post_init__(self):
        """Normalize rotation vector after initialization"""
        if self.rotation_vector is None or self.rotation_vector.Length < 1e-6:
            # Default to Z-axis rotation if vector is null or zero-length
            self.rotation_vector = FreeCAD.Vector(0, 0, 1)
        else:
            self.rotation_vector = self.rotation_vector.normalize()

    def is_valid_angle(self, angle):
        """Check if an angle is within this axis's limits"""
        return self.min_limit <= angle <= self.max_limit

    def to_dict(self):
        """Serialize to dictionary for JSON persistence"""
        return {
            "name": self.name,
            "rotation_vector": [
                self.rotation_vector.x,
                self.rotation_vector.y,
                self.rotation_vector.z,
            ],
            "min_limit": self.min_limit,
            "max_limit": self.max_limit,
            "max_velocity": self.max_velocity,
            "sequence": self.sequence,
            "prefer_positive": self.prefer_positive,
        }

    @classmethod
    def from_dict(cls, data):
        """Deserialize from dictionary"""
        vec = FreeCAD.Vector(
            data["rotation_vector"][0], data["rotation_vector"][1], data["rotation_vector"][2]
        )
        return cls(
            data["name"],
            vec,
            data["min_limit"],
            data["max_limit"],
            data.get("max_velocity", 36000),
            data.get("sequence", 0),
            data.get("prefer_positive", True),
        )


@dataclass
class Spindle:
    """Represents a single spindle in a machine configuration"""

    name: str
    id: Optional[str] = None
    max_power_kw: float = 0
    max_rpm: float = 0
    min_rpm: float = 0
    tool_change: str = "manual"
    tool_axis: Optional[FreeCAD.Vector] = None

    def __post_init__(self):
        """Set default tool axis if not provided"""
        if self.tool_axis is None:
            self.tool_axis = FreeCAD.Vector(0, 0, -1)

    def to_dict(self):
        """Serialize to dictionary for JSON persistence"""
        data = {
            "name": self.name,
            "max_power_kw": self.max_power_kw,
            "max_rpm": self.max_rpm,
            "min_rpm": self.min_rpm,
            "tool_change": self.tool_change,
            "tool_axis": [self.tool_axis.x, self.tool_axis.y, self.tool_axis.z],
        }
        if self.id is not None:
            data["id"] = self.id
        return data

    @classmethod
    def from_dict(cls, data):
        """Deserialize from dictionary"""
        tool_axis_data = data.get("tool_axis", [0, 0, -1])
        tool_axis = FreeCAD.Vector(tool_axis_data[0], tool_axis_data[1], tool_axis_data[2])
        return cls(
            data["name"],
            data.get("id"),
            data.get("max_power_kw", 0),
            data.get("max_rpm", 0),
            data.get("min_rpm", 0),
            data.get("tool_change", "manual"),
            tool_axis,
        )


@dataclass
class Machine:
    def __init__(
        self, name: str = "Default Machine", configuration_units: str = "metric", **kwargs
    ):
        # Set default values for all fields
        self.name = name
        self.manufacturer = kwargs.get("manufacturer", "")
        self.description = kwargs.get("description", "")
        self.linear_axes = {}
        self.rotary_axes = {}
        self.spindles = []
        self.reference_system = kwargs.get(
            "reference_system",
            {
                "X": FreeCAD.Vector(1, 0, 0),
                "Y": FreeCAD.Vector(0, 1, 0),
                "Z": FreeCAD.Vector(0, 0, 1),
            },
        )
        self.tool_axis = kwargs.get("tool_axis", FreeCAD.Vector(0, 0, -1))
        self.primary_rotary_axis = kwargs.get("primary_rotary_axis")
        self.secondary_rotary_axis = kwargs.get("secondary_rotary_axis")
        self.compound_moves = kwargs.get("compound_moves", True)
        self.prefer_positive_rotation = kwargs.get("prefer_positive_rotation", True)
        self._configuration_units = configuration_units
        self.version = kwargs.get("version", 1)
        self.output = kwargs.get("output", OutputOptions())
        # Handle legacy precision settings if provided
        precision = kwargs.get("precision")
        if precision is not None:
            self.output.axis_precision = precision.axis_precision
            self.output.feed_precision = precision.feed_precision
            self.output.spindle_decimals = precision.spindle_decimals
        self.blocks = kwargs.get("blocks", GCodeBlocks())
        self.processing = kwargs.get("processing", ProcessingOptions())
        self.postprocessor_file_name = kwargs.get("postprocessor_file_name", "")
        self.postprocessor_args = kwargs.get("postprocessor_args", "")
        self.motion_mode = kwargs.get("motion_mode", MotionMode.ABSOLUTE)
        self.parameter_functions = kwargs.get("parameter_functions", {})
        self.parameter_order = kwargs.get(
            "parameter_order",
            [
                "D",
                "H",
                "L",
                "X",
                "Y",
                "Z",
                "A",
                "B",
                "C",
                "U",
                "V",
                "W",
                "I",
                "J",
                "K",
                "R",
                "P",
                "E",
                "Q",
                "F",
                "S",
                "T",
            ],
        )

        # Initialize computed fields
        self.freecad_version = ".".join(FreeCAD.Version()[0:3])

        # Handle machine_type if provided (for backward compatibility)
        if "machine_type" in kwargs and kwargs["machine_type"]:
            self._initialize_from_machine_type(kwargs["machine_type"])

    """
    Unified machine configuration combining physical machine definition
    with post-processor settings.

    This is the single source of truth for all machine-related configuration,
    including physical capabilities (axes, spindles) and G-code generation
    preferences (output options, formatting, processing).
    """

    # ========================================================================
    # PHYSICAL MACHINE DEFINITION
    # ========================================================================

    # Basic identification
    name: str = "Default Machine"
    manufacturer: str = ""
    description: str = ""

    # Machine components
    linear_axes: Dict[str, LinearAxis] = field(default_factory=dict)
    rotary_axes: Dict[str, RotaryAxis] = field(default_factory=dict)
    spindles: List[Spindle] = field(default_factory=list)

    # Coordinate system
    reference_system: Dict[str, FreeCAD.Vector] = field(
        default_factory=lambda: {
            "X": FreeCAD.Vector(1, 0, 0),
            "Y": FreeCAD.Vector(0, 1, 0),
            "Z": FreeCAD.Vector(0, 0, 1),
        }
    )
    tool_axis: FreeCAD.Vector = field(default_factory=lambda: FreeCAD.Vector(0, 0, -1))

    # Rotary axis configuration
    primary_rotary_axis: Optional[str] = None
    secondary_rotary_axis: Optional[str] = None
    compound_moves: bool = True
    prefer_positive_rotation: bool = True

    # Units and versioning
    _configuration_units: str = field(
        default="metric", init=False
    )  # Internal storage for configuration_units
    version: int = 1
    freecad_version: str = field(init=False)

    # ========================================================================
    # POST-PROCESSOR CONFIGURATION
    # ========================================================================

    # Output options
    output: OutputOptions = field(default_factory=OutputOptions)
    blocks: GCodeBlocks = field(default_factory=GCodeBlocks)
    processing: ProcessingOptions = field(default_factory=ProcessingOptions)

    # Post-processor selection
    postprocessor_file_name: str = ""
    postprocessor_args: str = ""

    # Motion mode
    motion_mode: MotionMode = MotionMode.ABSOLUTE

    # Dynamic state (for runtime)
    parameter_functions: Dict[str, Callable] = field(default_factory=dict)
    parameter_order: List[str] = field(
        default_factory=lambda: [
            "D",
            "H",
            "L",
            "X",
            "Y",
            "Z",
            "A",
            "B",
            "C",
            "U",
            "V",
            "W",
            "I",
            "J",
            "K",
            "R",
            "P",
            "E",
            "Q",
            "F",
            "S",
            "T",
        ]
    )

    def __post_init__(self):
        """Initialize computed fields"""
        self.freecad_version = ".".join(FreeCAD.Version()[0:3])
        # Initialize configuration_units if not set
        if not hasattr(self, "_configuration_units"):
            self._configuration_units = "metric"

    # ========================================================================
    # PROPERTIES - Bridge between physical machine and post-processor
    # ========================================================================

    @property
    def configuration_units(self) -> str:
        """Get machine configuration units ("metric" or "imperial")"""
        if not hasattr(self, "_configuration_units"):
            self._configuration_units = "metric"
        return self._configuration_units

    @configuration_units.setter
    def configuration_units(self, value: str) -> None:
        """Set machine configuration units ("metric" or "imperial")"""
        if not value:  # Skip empty strings
            return
        if value not in ["metric", "imperial"]:
            raise ValueError("configuration_units must be 'metric' or 'imperial'")
        self._configuration_units = value

    @property
    def machine_units(self) -> MachineUnits:
        """Get machine configuration units as enum"""
        return (
            MachineUnits.METRIC if self.configuration_units == "metric" else MachineUnits.IMPERIAL
        )

    @property
    def output_machine_units(self) -> MachineUnits:
        """Get output units as enum for G-code generation"""
        return (
            MachineUnits.METRIC
            if self.output.output_units == OutputUnits.METRIC
            else MachineUnits.IMPERIAL
        )

    @property
    def gcode_units(self) -> MachineUnits:
        """Get G-code output units as enum for post-processor"""
        return (
            MachineUnits.METRIC
            if self.output.output_units == OutputUnits.METRIC
            else MachineUnits.IMPERIAL
        )

    @property
    def unit_format(self) -> str:
        """Get machine configuration unit format string (mm or in)"""
        return "mm" if self.configuration_units == "metric" else "in"

    @property
    def output_unit_format(self) -> str:
        """Get G-code output unit format string (mm or in)"""
        return "mm" if self.output.output_units == OutputUnits.METRIC else "in"

    @property
    def unit_speed_format(self) -> str:
        """Get machine configuration unit speed format string (mm/min or in/min)"""
        return "mm/min" if self.configuration_units == "metric" else "in/min"

    @property
    def output_unit_speed_format(self) -> str:
        """Get G-code output unit speed format string (mm/min or in/min)"""
        return "mm/min" if self.output.output_units == OutputUnits.METRIC else "in/min"

    @property
    def machine_type(self) -> str:
        """
        Determine machine type based on available axes.
        Returns one of: 'xyz', 'xyza', 'xyzb', 'xyzac', 'xyzbc', or 'custom'
        """
        if not all(axis in self.linear_axes for axis in ["X", "Y", "Z"]):
            return "custom"

        rot_axes = set(self.rotary_axes.keys())

        # Check for 5-axis configurations
        if {"A", "C"}.issubset(rot_axes):
            return "xyzac"
        if {"B", "C"}.issubset(rot_axes):
            return "xyzbc"

        # Check for 4-axis configurations
        if "A" in rot_axes:
            return "xyza"
        if "B" in rot_axes:
            return "xyzb"

        # 3-axis configuration
        return "xyz"

    @property
    def has_rotary_axes(self) -> bool:
        """Check if machine has any rotary axes"""
        return len(self.rotary_axes) > 0

    @property
    def is_5axis(self) -> bool:
        """Check if machine is 5-axis (2 rotary axes)"""
        return len(self.rotary_axes) >= 2

    @property
    def is_4axis(self) -> bool:
        """Check if machine is 4-axis (1 rotary axis)"""
        return len(self.rotary_axes) == 1

    @property
    def motion_commands(self) -> List[str]:
        """Get list of motion commands that change position"""
        import Path.Geom as PathGeom

        return PathGeom.CmdMoveAll

    @property
    def rapid_moves(self) -> List[str]:
        """Get list of rapid move commands"""
        import Path.Geom as PathGeom

        return PathGeom.CmdMoveRapid

    # ========================================================================
    # BUILDER METHODS - Fluent interface for machine construction
    # ========================================================================

    def add_linear_axis(
        self, name, direction_vector, min_limit=0, max_limit=1000, max_velocity=10000
    ):
        """Add a linear axis to the configuration"""
        self.linear_axes[name] = LinearAxis(
            name, direction_vector, min_limit, max_limit, max_velocity
        )
        return self

    def add_rotary_axis(
        self, name, rotation_vector, min_limit=-360, max_limit=360, max_velocity=36000
    ):
        """Add a rotary axis to the configuration"""
        self.rotary_axes[name] = RotaryAxis(
            name, rotation_vector, min_limit, max_limit, max_velocity
        )
        return self

    def add_spindle(
        self,
        name,
        id=None,
        max_power_kw=0,
        max_rpm=0,
        min_rpm=0,
        tool_change="manual",
        tool_axis=None,
    ):
        """Add a spindle to the configuration"""
        if tool_axis is None:
            tool_axis = FreeCAD.Vector(0, 0, -1)
        self.spindles.append(
            Spindle(name, id, max_power_kw, max_rpm, min_rpm, tool_change, tool_axis)
        )
        return self

    def save(self, filepath):
        """Save this configuration to a file

        Args:
            filepath: Path to save the configuration file

        Returns:
            Path object of saved file
        """
        filepath = pathlib.Path(filepath)
        data = self.to_dict()

        try:
            with open(filepath, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=4, ensure_ascii=False)
            Path.Log.debug(f"Saved machine configuration to {filepath}")
            return filepath
        except Exception as e:
            Path.Log.error(f"Failed to save configuration: {e}")
            raise Exception(f"Failed to save machine file {filepath}: {e}")

    def set_alignment_axes(self, primary, secondary=None):
        """Set the primary and secondary rotary axes for alignment strategy

        For 4-axis machines, secondary can be None (single rotary axis)
        For 5-axis machines, both primary and secondary are required
        """
        if primary not in self.rotary_axes:
            raise ValueError(f"Primary axis {primary} not found in configuration")

        if secondary is not None and secondary not in self.rotary_axes:
            raise ValueError(f"Secondary axis {secondary} not found in configuration")

        self.primary_rotary_axis = primary
        self.secondary_rotary_axis = secondary
        return self

    def get_axis_by_name(self, name):
        """Get a rotary axis by name"""
        return self.rotary_axes.get(name)

    def get_spindle_by_index(self, index):
        """Get a spindle by its index in the list"""
        if 0 <= index < len(self.spindles):
            return self.spindles[index]
        raise ValueError(f"Spindle index {index} out of range")

    def get_spindle_by_name(self, name):
        """Get a spindle by name (case-insensitive)"""
        name_lower = name.lower()
        for spindle in self.spindles:
            if spindle.name.lower() == name_lower:
                return spindle
        raise ValueError(f"Spindle with name '{name}' not found")

    def get_spindle_by_id(self, id):
        """Get a spindle by ID (if present)"""
        if id is None:
            raise ValueError("ID cannot be None")
        for spindle in self.spindles:
            if spindle.id == id:
                return spindle
        raise ValueError(f"Spindle with ID '{id}' not found")

    @classmethod
    def create_AC_table_config(cls, a_limits=(-120, 120), c_limits=(-360, 360)):
        """Create standard A/C table configuration"""
        config = cls("AC Table Configuration")
        config.add_linear_axis("X", FreeCAD.Vector(1, 0, 0))
        config.add_linear_axis("Y", FreeCAD.Vector(0, 1, 0))
        config.add_linear_axis("Z", FreeCAD.Vector(0, 0, 1))
        config.add_rotary_axis("A", FreeCAD.Vector(1, 0, 0), a_limits[0], a_limits[1])
        config.add_rotary_axis("C", FreeCAD.Vector(0, 0, 1), c_limits[0], c_limits[1])
        config.set_alignment_axes("C", "A")
        return config

    @classmethod
    def create_BC_head_config(cls, b_limits=(-120, 120), c_limits=(-360, 360)):
        """Create standard B/C head configuration"""
        config = cls("BC Head Configuration")
        config.add_linear_axis("X", FreeCAD.Vector(1, 0, 0))
        config.add_linear_axis("Y", FreeCAD.Vector(0, 1, 0))
        config.add_linear_axis("Z", FreeCAD.Vector(0, 0, 1))
        config.add_rotary_axis("B", FreeCAD.Vector(0, 1, 0), b_limits[0], b_limits[1])
        config.add_rotary_axis("C", FreeCAD.Vector(0, 0, 1), c_limits[0], c_limits[1])
        config.set_alignment_axes("C", "B")
        config.compound_moves = True  # Ensure compound moves are enabled for test compatibility
        return config

    @classmethod
    def create_AB_table_config(cls, a_limits=(-120, 120), b_limits=(-120, 120)):
        """Create standard A/B table configuration"""
        config = cls("AB Table Configuration")
        # AB configuration will be detected as 'custom' by the machine_type property
        config.add_linear_axis("X", FreeCAD.Vector(1, 0, 0))
        config.add_linear_axis("Y", FreeCAD.Vector(0, 1, 0))
        config.add_linear_axis("Z", FreeCAD.Vector(0, 0, 1))
        config.add_rotary_axis("A", FreeCAD.Vector(1, 0, 0), a_limits[0], a_limits[1])
        config.add_rotary_axis("B", FreeCAD.Vector(0, 1, 0), b_limits[0], b_limits[1])
        config.set_alignment_axes("A", "B")
        return config

    @classmethod
    def create_4axis_A_config(cls, a_limits=(-120, 120)):
        """Create standard 4-axis XYZA configuration (rotary table around X)"""
        config = cls("4-Axis XYZA Configuration")
        config.add_linear_axis("X", FreeCAD.Vector(1, 0, 0))
        config.add_linear_axis("Y", FreeCAD.Vector(0, 1, 0))
        config.add_linear_axis("Z", FreeCAD.Vector(0, 0, 1))
        config.add_rotary_axis("A", FreeCAD.Vector(1, 0, 0), a_limits[0], a_limits[1])
        config.set_alignment_axes("A", None)
        config.description = "4-axis machine with A-axis rotary table (rotation around X-axis)"
        return config

    @classmethod
    def create_4axis_B_config(cls, b_limits=(-120, 120)):
        """Create standard 4-axis XYZB configuration (rotary table around Y)"""
        config = cls("4-Axis XYZB Configuration")
        config.add_linear_axis("X", FreeCAD.Vector(1, 0, 0))
        config.add_linear_axis("Y", FreeCAD.Vector(0, 1, 0))
        config.add_linear_axis("Z", FreeCAD.Vector(0, 0, 1))
        config.add_rotary_axis("B", FreeCAD.Vector(0, 1, 0), b_limits[0], b_limits[1])
        config.set_alignment_axes("B", None)
        config.description = "4-axis machine with B-axis rotary table (rotation around Y-axis)"
        return config

    @classmethod
    def create_3axis_config(cls):
        """Create standard 3-axis XYZ configuration (no rotary axes)"""
        config = cls("3-Axis XYZ Configuration")
        config.add_linear_axis("X", FreeCAD.Vector(1, 0, 0))
        config.add_linear_axis("Y", FreeCAD.Vector(0, 1, 0))
        config.add_linear_axis("Z", FreeCAD.Vector(0, 0, 1))
        config.description = "Standard 3-axis machine with no rotary axes"
        # No rotary axes to add, no alignment axes to set
        return config

    def to_dict(self):
        """Serialize configuration to dictionary for JSON persistence"""
        # Build flattened axes structure
        axes = {}

        # Add linear axes from LinearAxis objects
        for axis_name, axis_obj in self.linear_axes.items():
            dir_vec = axis_obj.direction_vector
            joint = [[dir_vec.x, dir_vec.y, dir_vec.z], [0, 0, 0]]

            axes[axis_name] = {
                "type": "linear",
                "min": axis_obj.min_limit,
                "max": axis_obj.max_limit,
                "max_velocity": axis_obj.max_velocity,
                "joint": joint,
                "sequence": axis_obj.sequence,
            }

        # Add rotary axes
        for axis_name, axis_obj in self.rotary_axes.items():
            rot_vec = axis_obj.rotation_vector
            joint = [[0, 0, 0], [rot_vec.x, rot_vec.y, rot_vec.z]]
            axes[axis_name] = {
                "type": "angular",
                "min": axis_obj.min_limit,
                "max": axis_obj.max_limit,
                "max_velocity": axis_obj.max_velocity,
                "joint": joint,
                "sequence": axis_obj.sequence,
                "prefer_positive": axis_obj.prefer_positive,
            }

        data = {
            "freecad_version": self.freecad_version,
            "machine": {
                "name": self.name,
                "manufacturer": self.manufacturer,
                "description": self.description,
                # "type": self.machine_type,
                "units": self.configuration_units,
                "axes": axes,
                "spindles": [spindle.to_dict() for spindle in self.spindles],
            },
            "version": self.version,
        }

        # Add post-processor configuration
        data["postprocessor"] = {
            "file_name": self.postprocessor_file_name,
            "args": self.postprocessor_args,
            "motion_mode": self.motion_mode.value,
        }

        # Output options
        data["output"] = {
            "comments": self.output.comments,
            "blank_lines": self.output.blank_lines,
            "header": self.output.header,
            "line_numbers": self.output.line_numbers,
            "path_labels": self.output.path_labels,
            "machine_name": self.output.machine_name,
            "doubles": self.output.doubles,
            "output_units": self.output.output_units.value,
            "axis_precision": self.output.axis_precision,
            "feed_precision": self.output.feed_precision,
            "spindle_decimals": self.output.spindle_decimals,
            "command_space": self.output.command_space,
            "comment_symbol": self.output.comment_symbol,
            "line_increment": self.output.line_increment,
            "line_number_start": self.output.line_number_start,
            "end_of_line_chars": self.output.end_of_line_chars,
        }

        # G-code blocks (only non-empty ones)
        blocks = {}
        if self.blocks.pre_job:
            blocks["pre_job"] = self.blocks.pre_job
        if self.blocks.post_job:
            blocks["post_job"] = self.blocks.post_job
        if self.blocks.preamble:
            blocks["preamble"] = self.blocks.preamble
        if self.blocks.postamble:
            blocks["postamble"] = self.blocks.postamble
        if self.blocks.safetyblock:
            blocks["safetyblock"] = self.blocks.safetyblock
        if self.blocks.pre_operation:
            blocks["pre_operation"] = self.blocks.pre_operation
        if self.blocks.post_operation:
            blocks["post_operation"] = self.blocks.post_operation
        if self.blocks.pre_tool_change:
            blocks["pre_tool_change"] = self.blocks.pre_tool_change
        if self.blocks.post_tool_change:
            blocks["post_tool_change"] = self.blocks.post_tool_change
        if self.blocks.tool_return:
            blocks["tool_return"] = self.blocks.tool_return
        if self.blocks.pre_fixture_change:
            blocks["pre_fixture_change"] = self.blocks.pre_fixture_change
        if self.blocks.post_fixture_change:
            blocks["post_fixture_change"] = self.blocks.post_fixture_change
        if self.blocks.pre_rotary_move:
            blocks["pre_rotary_move"] = self.blocks.pre_rotary_move
        if self.blocks.post_rotary_move:
            blocks["post_rotary_move"] = self.blocks.post_rotary_move

        if blocks:
            data["blocks"] = blocks

        # Processing options
        data["processing"] = {
            "modal": self.processing.modal,
            "translate_drill_cycles": self.processing.translate_drill_cycles,
            "split_arcs": self.processing.split_arcs,
            "show_editor": self.processing.show_editor,
            "list_tools_in_preamble": self.processing.list_tools_in_preamble,
            "show_machine_units": self.processing.show_machine_units,
            "show_operation_labels": self.processing.show_operation_labels,
            "tool_before_change": self.processing.tool_before_change,
            "drill_cycles_to_translate": self.processing.drill_cycles_to_translate,
            "suppress_commands": self.processing.suppress_commands,
            "tool_change": self.processing.tool_change,
            "adaptive": self.processing.adaptive,
            "chipbreaking_amount": self.processing.chipbreaking_amount,
            "spindle_wait": self.processing.spindle_wait,
        }
        if self.processing.return_to:
            data["processing"]["return_to"] = list(self.processing.return_to)

        return data

    def _initialize_3axis_config(self) -> None:
        """Initialize as a standard 3-axis XYZ configuration (no rotary axes)"""
        self.name = self.name or "3-Axis XYZ Configuration"
        self.linear_axes = {
            "X": LinearAxis("X", FreeCAD.Vector(1, 0, 0)),
            "Y": LinearAxis("Y", FreeCAD.Vector(0, 1, 0)),
            "Z": LinearAxis("Z", FreeCAD.Vector(0, 0, 1)),
        }

    @classmethod
    def create_3axis_config(cls) -> "Machine":
        """Create standard 3-axis XYZ configuration (no rotary axes)"""
        config = cls("3-Axis XYZ Configuration")
        config._initialize_3axis_config()
        return config

    def _initialize_3axis_config(self) -> None:
        """Initialize as a standard 3-axis XYZ configuration (no rotary axes)"""
        self.linear_axes = {
            "X": LinearAxis("X", FreeCAD.Vector(1, 0, 0)),
            "Y": LinearAxis("Y", FreeCAD.Vector(0, 1, 0)),
            "Z": LinearAxis("Z", FreeCAD.Vector(0, 0, 1)),
        }
        self.rotary_axes = {}
        self.primary_rotary_axis = None
        self.secondary_rotary_axis = None
        self.compound_moves = True

    def _initialize_4axis_A_config(self, a_limits=(-120, 120)) -> None:
        """Initialize as a 4-axis XYZA configuration (rotary table around X)"""
        self._initialize_3axis_config()
        self.rotary_axes["A"] = RotaryAxis(
            "A", FreeCAD.Vector(1, 0, 0), min_limit=a_limits[0], max_limit=a_limits[1]
        )
        self.primary_rotary_axis = "A"

    def _initialize_4axis_B_config(self, b_limits=(-120, 120)) -> None:
        """Initialize as a 4-axis XYZB configuration (rotary table around Y)"""
        self._initialize_3axis_config()
        self.rotary_axes["B"] = RotaryAxis(
            "B", FreeCAD.Vector(0, 1, 0), min_limit=b_limits[0], max_limit=b_limits[1]
        )
        self.primary_rotary_axis = "B"

    def _initialize_AC_table_config(self, a_limits=(-120, 120), c_limits=(-360, 360)) -> None:
        """Initialize as a 5-axis AC table configuration"""
        self._initialize_4axis_A_config(a_limits)
        self.rotary_axes["C"] = RotaryAxis(
            "C", FreeCAD.Vector(0, 0, 1), min_limit=c_limits[0], max_limit=c_limits[1]
        )
        self.secondary_rotary_axis = "C"

    def _initialize_BC_head_config(self, b_limits=(-120, 120), c_limits=(-360, 360)) -> None:
        """Initialize as a 5-axis BC head configuration"""
        self._initialize_4axis_B_config(b_limits)
        self.rotary_axes["C"] = RotaryAxis(
            "C", FreeCAD.Vector(0, 0, 1), min_limit=c_limits[0], max_limit=c_limits[1]
        )
        self.secondary_rotary_axis = "C"

    def _initialize_from_machine_type(self, machine_type: str) -> None:
        """Initialize machine configuration based on machine type"""
        if machine_type == "xyz":
            self._initialize_3axis_config()
        elif machine_type == "xyza":
            self._initialize_4axis_A_config()
        elif machine_type == "xyzb":
            self._initialize_4axis_B_config()
        elif machine_type == "xyzac":
            self._initialize_AC_table_config()
        elif machine_type == "xyzbc":
            self._initialize_BC_head_config()

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "Machine":
        """Deserialize configuration from dictionary (supports both old and new formats)"""
        machine_data = data.get("machine", data)  # Support both old and new formats

        # Extract basic configuration
        config = cls(
            name=machine_data.get("name", "Loaded Machine"),
            configuration_units=machine_data.get("units", "metric"),
            manufacturer=machine_data.get("manufacturer", ""),
            description=machine_data.get("description", ""),
            # machine_type=machine_data.get("type"),
            **{
                k: v
                for k, v in machine_data.items()
                if k not in ["name", "units", "manufacturer", "description", "type"]
            },
        )

        # Parse axes from new flattened structure
        axes = machine_data.get("axes", {})
        config.linear_axes = {}
        config.rotary_axes = {}

        # Determine primary/secondary rotary axes
        rotary_axis_names = [
            name for name, axis_data in axes.items() if axis_data.get("type") == "angular"
        ]
        rotary_axis_names.sort()  # Sort to get consistent ordering

        if len(rotary_axis_names) > 0:
            config.primary_rotary_axis = rotary_axis_names[0]
        if len(rotary_axis_names) > 1:
            config.secondary_rotary_axis = rotary_axis_names[1]

        # Parse linear and rotary axes
        for axis_name, axis_data in axes.items():
            axis_type = axis_data.get("type", "linear")

            if axis_type == "linear":
                # Extract direction vector from joint
                joint = axis_data.get("joint", [[1, 0, 0], [0, 0, 0]])
                direction_vec = FreeCAD.Vector(joint[0][0], joint[0][1], joint[0][2])

                min_limit = axis_data.get("min", 0)
                max_limit = axis_data.get("max", 1000)
                max_velocity = axis_data.get("max_velocity", 10000)

                config.linear_axes[axis_name] = LinearAxis(
                    name=axis_name,
                    direction_vector=direction_vec,
                    min_limit=min_limit,
                    max_limit=max_limit,
                    max_velocity=max_velocity,
                )
            elif axis_type == "angular":
                joint = axis_data.get("joint", [[0, 0, 1], [0, 0, 0]])
                rotation_vec = FreeCAD.Vector(joint[0][0], joint[0][1], joint[0][2])

                min_limit = axis_data.get("min", -360)
                max_limit = axis_data.get("max", 360)
                max_velocity = axis_data.get("max_velocity", 36000)
                prefer_positive = axis_data.get("prefer_positive", True)

                config.rotary_axes[axis_name] = RotaryAxis(
                    name=axis_name,
                    rotation_vector=rotation_vec,
                    min_limit=min_limit,
                    max_limit=max_limit,
                    max_velocity=max_velocity,
                    prefer_positive=prefer_positive,
                )

        # Parse spindles if present
        spindles = machine_data.get("spindles", [])
        config.spindles = [Spindle.from_dict(s) for s in spindles]

        # Parse post-processor settings if present
        post_data = data.get("postprocessor", {})
        if post_data:
            config.postprocessor_file_name = post_data.get("file_name", "")
            config.postprocessor_args = post_data.get("args", "")
            config.motion_mode = MotionMode(post_data.get("motion_mode", "G90"))

        # Load output options
        output_data = data.get("output", {})
        if output_data:
            config.output.comments = output_data.get("comments", True)
            config.output.blank_lines = output_data.get("blank_lines", True)
            config.output.header = output_data.get("header", True)
            config.output.line_numbers = output_data.get("line_numbers", False)
            config.output.path_labels = output_data.get("path_labels", False)
            config.output.machine_name = output_data.get("machine_name", False)
            config.output.doubles = output_data.get("doubles", False)

            # Handle output_units conversion from string to enum
            output_units_str = output_data.get("output_units", "metric")
            config.output.output_units = (
                OutputUnits.METRIC if output_units_str == "metric" else OutputUnits.IMPERIAL
            )

            # These fields are now in ProcessingOptions
            if "tool_change" in output_data:
                config.processing.tool_change = output_data["tool_change"]
            if "adaptive" in output_data:
                config.processing.adaptive = output_data["adaptive"]

            # Set precision values from output_data
            if "axis_precision" in output_data:
                config.output.axis_precision = output_data["axis_precision"]
            if "feed_precision" in output_data:
                config.output.feed_precision = output_data["feed_precision"]
            if "spindle_decimals" in output_data:
                config.output.spindle_decimals = output_data["spindle_decimals"]

            # Load formatting fields from output_data
            config.output.command_space = output_data.get("command_space", " ")
            config.output.comment_symbol = output_data.get("comment_symbol", "(")
            config.output.line_increment = output_data.get("line_increment", 10)
            config.output.line_number_start = output_data.get("line_number_start", 100)
            config.output.end_of_line_chars = output_data.get("end_of_line_chars", "\n")

        # Load processing options
        processing_data = data.get("processing", {})
        if processing_data:
            config.processing.modal = processing_data.get("modal", False)
            config.processing.translate_drill_cycles = processing_data.get(
                "translate_drill_cycles", False
            )
            config.processing.split_arcs = processing_data.get("split_arcs", False)
            config.processing.show_editor = processing_data.get("show_editor", True)
            config.processing.list_tools_in_preamble = processing_data.get(
                "list_tools_in_preamble", False
            )
            config.processing.show_machine_units = processing_data.get("show_machine_units", True)
            config.processing.show_operation_labels = processing_data.get(
                "show_operation_labels", True
            )
            config.processing.tool_before_change = processing_data.get("tool_before_change", False)
            config.processing.drill_cycles_to_translate = processing_data.get(
                "drill_cycles_to_translate", ["G73", "G81", "G82", "G83"]
            )
            config.processing.suppress_commands = processing_data.get("suppress_commands", [])
            config.processing.tool_change = processing_data.get("tool_change", True)
            config.processing.adaptive = processing_data.get("adaptive", False)
            config.processing.chipbreaking_amount = processing_data.get("chipbreaking_amount", 0.25)
            config.processing.spindle_wait = processing_data.get("spindle_wait", 0.0)
            return_to = processing_data.get("return_to", None)
            config.processing.return_to = tuple(return_to) if return_to is not None else None

        # Load G-code blocks
        blocks_data = data.get("blocks", {})
        if blocks_data:
            for block_name in [
                "pre_job",
                "post_job",
                "preamble",
                "postamble",
                "safetyblock",
                "pre_operation",
                "post_operation",
                "pre_tool_change",
                "post_tool_change",
                "tool_return",
                "pre_fixture_change",
                "post_fixture_change",
                "pre_rotary_move",
                "post_rotary_move",
                "pre_spindle_change",
                "post_spindle_change",
                "finish_label",
            ]:
                if block_name in blocks_data:
                    setattr(config.blocks, block_name, blocks_data[block_name])

        return config


class MachineFactory:
    """Factory class for creating, loading, and saving machine configurations"""

    # Default configuration directory
    _config_dir = None

    @classmethod
    def set_config_directory(cls, directory):
        """Set the directory for storing machine configuration files"""
        cls._config_dir = pathlib.Path(directory)
        cls._config_dir.mkdir(parents=True, exist_ok=True)

    @classmethod
    def get_config_directory(cls):
        """Get the configuration directory, creating default if not set"""
        if cls._config_dir is None:
            # Use FreeCAD user data directory + CAM/Machines
            try:
                cls._config_dir = Path.Preferences.getAssetPath() / "Machines"
                cls._config_dir.mkdir(parents=True, exist_ok=True)
            except Exception as e:
                Path.Log.warning(f"Could not create default config directory: {e}")
                cls._config_dir = pathlib.Path.cwd() / "Machines"
                cls._config_dir.mkdir(parents=True, exist_ok=True)
        return cls._config_dir

    @classmethod
    def save_configuration(cls, config, filename=None):
        """
        Save a machine configuration to a JSON file

        Args:
            config: Machine object to save
            filename: Optional filename (without path). If None, uses sanitized config name

        Returns:
            Path to the saved file
        """
        if filename is None:
            # Sanitize the config name for use as filename
            filename = config.name.replace(" ", "_").replace("/", "_") + ".fcm"

        config_dir = cls.get_config_directory()
        filepath = config_dir / filename

        try:
            data = config.to_dict()
            with open(filepath, "w", encoding="utf-8") as f:
                json.dump(data, f, sort_keys=True, indent=4)
            Path.Log.debug(f"Saved machine file: {filepath}")
            return filepath
        except Exception as e:
            Path.Log.error(f"Failed to save configuration: {e}")
            raise Exception(f"Failed to save machine file {filepath}: {e}")

    @classmethod
    def load_configuration(cls, filename):
        """
        Load a machine configuration from a JSON file

        Args:
            filename: Filename (with or without path). If no path, searches config directory

        Returns:
            Dictionary containing machine configuration data (new format) or
            Machine object if loading old format

        Raises:
            FileNotFoundError: If the file does not exist
            json.JSONDecodeError: If the file is not valid JSON
            Exception: For other I/O errors
        """
        filepath = pathlib.Path(filename)

        # If no directory specified, look in config directory
        if not filepath.parent or filepath.parent == pathlib.Path("."):
            filepath = cls.get_config_directory() / filename

        try:
            with open(filepath, "r", encoding="utf-8") as f:
                data = json.load(f)
            Path.Log.debug(f"Loaded machine file: {filepath}")
            machine = Machine.from_dict(data)
            Path.Log.debug(f"Loaded machine configuration from {filepath}")
            return machine

        except FileNotFoundError:
            raise FileNotFoundError(f"Machine file not found: {filepath}")
        except json.JSONDecodeError as e:
            raise ValueError(f"Invalid JSON in machine file {filepath}: {e}")
        except Exception as e:
            raise Exception(f"Failed to load machine file {filepath}: {e}")

    @classmethod
    def create_default_machine_data(cls):
        """
        Create a default machine configuration dictionary for the editor.

        Returns:
            Dictionary with default machine configuration structure
        """
        machine = Machine(name="New Machine")
        return machine.to_dict()

    @classmethod
    def list_configuration_files(cls) -> list[tuple[str, pathlib.Path]]:
        """Get list of available machine files from the asset directory.

        Scans the Machine subdirectory of the asset path for .fcm files
        and returns tuples of (display_name, file_path).

        Returns:
            list: List of (name, path) tuples for discovered machine files
        """
        machines = [("<any>", None)]
        try:
            asset_base = cls.get_config_directory()
            if asset_base.exists():
                for p in sorted(asset_base.glob("*.fcm")):
                    name = cls.get_machine_display_name(p.name)
                    machines.append((name, p.name))
        except Exception:
            pass
        return machines

    @classmethod
    def list_configurations(cls) -> list[str]:
        """Get list of available machines from the asset directory.

        Scans the Machine subdirectory of the asset path for .fcm files
        and extracts machine names. Returns ["<any>"] plus discovered machine names.

        Returns:
            list: List of machine names starting with "<any>"
        """
        machines = cls.list_configuration_files()
        return [name for name, path in machines]

    @classmethod
    def delete_configuration(cls, filename):
        """
        Delete a machine configuration file

        Args:
            filename: Name of the configuration file to delete

        Returns:
            True if deleted successfully, False otherwise
        """
        filepath = cls.get_config_directory() / filename
        try:
            if filepath.exists():
                filepath.unlink()
                Path.Log.debug(f"Deleted machine: {filepath}")
                return True
            else:
                Path.Log.warning(f"Machine file not found: {filepath}")
                return False
        except Exception as e:
            Path.Log.error(f"Failed to delete machine: {e}")
            return False

    @classmethod
    def create_standard_configs(cls):
        """
        Create and save all standard machine configurations

        Returns:
            Dictionary mapping config names to file paths
        """
        configs = {
            "XYZ": Machine.create_3axis_config(),
            "XYZAC": Machine.create_AC_table_config(),
            "XYZBC": Machine.create_BC_head_config(),
            "XYZA": Machine.create_4axis_A_config(),
            "XYZB": Machine.create_4axis_B_config(),
        }

        saved_paths = {}
        for name, config in configs.items():
            try:
                filepath = cls.save_configuration(config, f"{name}.fcm")
                saved_paths[name] = filepath
            except Exception as e:
                Path.Log.error(f"Failed to save {name}: {e}")

        return saved_paths

    @classmethod
    def get_builtin_config(cls, config_type):
        """
        Get a built-in machine configuration without loading from disk

        Args:
            config_type: One of "XYZ", "XYZAC", "XYZBC", "XYZA", "XYZB"

        Returns:
            Machine object
        """
        config_map = {
            "XYZ": Machine.create_3axis_config,
            "XYZAC": Machine.create_AC_table_config,
            "XYZBC": Machine.create_BC_head_config,
            "XYZA": Machine.create_4axis_A_config,
            "XYZB": Machine.create_4axis_B_config,
        }

        if config_type not in config_map:
            raise ValueError(
                f"Unknown config type: {config_type}. Available: {list(config_map.keys())}"
            )

        return config_map[config_type]()

    @classmethod
    def get_machine(cls, machine_name):
        """
        Get a machine configuration by name from the assets folder

        Args:
            machine_name: Name of the machine to load (without .fcm extension)

        Returns:
            Machine object

        Raises:
            FileNotFoundError: If no machine with that name is found
            ValueError: If the loaded data is not a valid machine configuration
        """
        # Get list of available machine files
        machine_files = cls.list_configuration_files()

        # Find the file matching the machine name (case-insensitive)
        target_path = None
        machine_name_lower = machine_name.lower()
        for name, path in machine_files:
            if name.lower() == machine_name_lower and path is not None:
                target_path = path
                break

        if target_path is None:
            available = [name for name, path in machine_files if path is not None]
            raise FileNotFoundError(
                f"Machine '{machine_name}' not found. Available machines: {available}"
            )

        # Load the configuration using the path from list_configuration_files()
        data = cls.load_configuration(target_path)

        # If load_configuration returned a dict (new format), convert to Machine
        if isinstance(data, dict):
            return Machine.from_dict(data)
        else:
            # Already a Machine object (old format)
            return data

    @classmethod
    def get_machine_display_name(cls, filename):
        """
        Get the display name for a machine from its filename in the config directory.

        Args:
            filename: Name of the machine file (without path)

        Returns:
            str: Display name (machine name from JSON or filename stem)
        """
        filepath = cls.get_config_directory() / filename
        try:
            with open(filepath, "r", encoding="utf-8") as f:
                data = json.load(f)
                return data.get("machine", {}).get("name", filepath.stem)
        except Exception:
            return filepath.stem
