# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 FreeCAD Multi-Axis Development Team                *
# *                                                                         *
# *   Ghost Gunner 3-Axis Kinematics Model                                  *
# *   Supports: GG2, GG3, GG3-S, GGD                                        *
# ***************************************************************************

"""
Ghost Gunner 3-Axis Kinematics Model
Purpose: Pure Cartesian kinematics for Ghost Gunner 3-axis machines
"""

__title__ = "CAM Ghost Gunner 3-Axis Kinematics"
__author__ = "FreeCAD Multi-Axis Development Team"
__url__ = "https://www.freecad.org"

import math
import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from KinematicModelBase import KinematicModelBase
except ImportError:
    # Fallback for different import contexts
    from .KinematicModelBase import KinematicModelBase


class MachineKinematics_GhostGunner3(KinematicModelBase):
    """
    Kinematics model for Ghost Gunner 3-axis machines.

    Supports:
    - Ghost Gunner 2 (GG2) - GRBL Legacy
    - Ghost Gunner 3 (GG3) - GGC/Dreamer
    - Ghost Gunner 3S (GG3-S) - GGC/Dreamer
    - Ghost Gunner Defense (GGD) - GGC/Dreamer

    Pure Cartesian motion with no rotary axes.
    """

    def __init__(self, cfg):
        """
        Initialize Ghost Gunner 3-axis kinematics.

        Args:
            cfg: Machine configuration dictionary
        """
        super().__init__(cfg)

        # Detect Ghost Gunner model
        model_name = cfg.get("name", "").lower()
        if "ghostgunner2" in model_name or "gg2" in model_name:
            self.model = "GG2"
            self.controller = "GRBL_Legacy"
        elif "ghostgunnerdefense" in model_name or "ggd" in model_name:
            self.model = "GGD"
            self.controller = "GGC_Dreamer"
        elif "ghostgunner3s" in model_name or "gg3s" in model_name or "gg3-s" in model_name:
            self.model = "GG3S"
            self.controller = "GGC_Dreamer"
        elif "ghostgunner3" in model_name or "gg3" in model_name:
            self.model = "GG3"
            self.controller = "GGC_Dreamer"
        else:
            self.model = "GG3"  # Default
            self.controller = "GGC_Dreamer"

        # Motion constraints
        self.max_feed_rate = cfg.get("feed_rate", {}).get("max", 1200.0)
        self.default_feed_rate = cfg.get("feed_rate", {}).get("default", 800.0)
        self.rapid_feed_rate = cfg.get("feed_rate", {}).get("rapid", 2500.0)

        # Acceleration limits
        accel = cfg.get("acceleration", {})
        self.max_acceleration = accel.get("max", 600.0)
        self.default_acceleration = accel.get("default", 400.0)

        # Safe height for tool changes and rapids
        self.safe_height = cfg.get("safe_height", 10.0)

    def forward(self, axes):
        """
        Forward kinematics for Ghost Gunner 3-axis.

        Pure Cartesian: tool position = machine position

        Args:
            axes: Dictionary with X, Y, Z positions

        Returns:
            Tuple of (position, orientation)
        """
        x = axes.get("X", 0.0)
        y = axes.get("Y", 0.0)
        z = axes.get("Z", 0.0)

        # Tool position is directly machine position
        position = (x, y, z)

        # Tool orientation is always vertical (Z-up)
        orientation = (0.0, 0.0, 1.0)

        return (position, orientation)

    def inverse(self, tool_pos, tool_orient):
        """
        Inverse kinematics for Ghost Gunner 3-axis.

        Pure Cartesian: machine position = tool position
        Tool orientation is ignored (always vertical).

        Args:
            tool_pos: Tool tip position (x, y, z)
            tool_orient: Tool orientation (ignored for 3-axis)

        Returns:
            Dictionary of axis positions
        """
        # Extract position
        if isinstance(tool_pos, (list, tuple)):
            x, y, z = tool_pos[0], tool_pos[1], tool_pos[2]
        else:
            # Assume it's a Vector-like object
            x, y, z = tool_pos.x, tool_pos.y, tool_pos.z

        # Pure Cartesian mapping
        axes = {"X": x, "Y": y, "Z": z}

        # Clamp to machine limits
        return self.clamp_axes(axes)

    def validate_motion(self, from_pos, to_pos, feed_rate):
        """
        Validate motion between two positions.

        Args:
            from_pos: Starting position (x, y, z)
            to_pos: Ending position (x, y, z)
            feed_rate: Requested feed rate (mm/min)

        Returns:
            Tuple of (is_valid, warnings)
        """
        warnings = []

        # Check feed rate
        if feed_rate > self.max_feed_rate:
            warnings.append(
                f"Feed rate {feed_rate:.1f} exceeds maximum {self.max_feed_rate:.1f} mm/min"
            )

        # Check positions are within limits
        for pos, label in [(from_pos, "start"), (to_pos, "end")]:
            axes = {"X": pos[0], "Y": pos[1], "Z": pos[2]}
            is_valid, violations = self.check_limits(axes)
            if not is_valid:
                warnings.extend([f"{label}: {v}" for v in violations])

        # Calculate motion distance
        dx = to_pos[0] - from_pos[0]
        dy = to_pos[1] - from_pos[1]
        dz = to_pos[2] - from_pos[2]
        distance = math.sqrt(dx * dx + dy * dy + dz * dz)

        # Estimate motion time
        if feed_rate > 0:
            motion_time = (distance / feed_rate) * 60.0  # seconds

            # Warn if motion is very slow
            if motion_time > 60.0 and distance > 10.0:
                warnings.append(
                    f"Motion will take {motion_time:.1f}s - consider increasing feed rate"
                )

        return (len(warnings) == 0, warnings)

    def get_safe_retract_position(self, current_pos):
        """
        Get safe retract position for tool changes.

        Args:
            current_pos: Current tool position (x, y, z)

        Returns:
            Safe position (x, y, z_safe)
        """
        return (current_pos[0], current_pos[1], self.safe_height)

    def get_controller_info(self):
        """
        Get controller-specific information.

        Returns:
            Dictionary with controller details
        """
        return {
            "model": self.model,
            "controller": self.controller,
            "max_feed_rate": self.max_feed_rate,
            "max_acceleration": self.max_acceleration,
            "safe_height": self.safe_height,
            "has_tool_changer": False,
            "has_coolant": False,
            "coordinate_system": "Cartesian",
        }

    def estimate_machining_time(self, toolpath_length, avg_feed_rate):
        """
        Estimate total machining time.

        Args:
            toolpath_length: Total path length (mm)
            avg_feed_rate: Average feed rate (mm/min)

        Returns:
            Estimated time in seconds
        """
        if avg_feed_rate <= 0:
            return 0.0

        # Base cutting time
        cutting_time = (toolpath_length / avg_feed_rate) * 60.0

        # Add acceleration/deceleration overhead (estimate 10%)
        overhead = cutting_time * 0.10

        return cutting_time + overhead

    def get_machine_description(self):
        """
        Get human-readable machine description.

        Returns:
            Description string
        """
        desc = []
        desc.append(f"Ghost Gunner {self.model}")
        desc.append(f"Controller: {self.controller}")
        desc.append(f"Axes: X, Y, Z (Cartesian)")
        desc.append(f"Max Feed: {self.max_feed_rate:.0f} mm/min")
        desc.append(f"Max Accel: {self.max_acceleration:.0f} mm/s²")
        desc.append(f"Safe Height: {self.safe_height:.1f} mm")

        return "\n".join(desc)


# Convenience function for creating Ghost Gunner kinematics
def create_ghostgunner_kinematics(machine_config):
    """
    Factory function to create appropriate Ghost Gunner kinematics model.

    Args:
        machine_config: Machine configuration dictionary

    Returns:
        MachineKinematics_GhostGunner3 instance
    """
    return MachineKinematics_GhostGunner3(machine_config)
