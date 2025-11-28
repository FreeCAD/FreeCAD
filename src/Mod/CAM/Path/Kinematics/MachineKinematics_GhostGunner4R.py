# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 FreeCAD Multi-Axis Development Team                *
# *                                                                         *
# *   Ghost Gunner R 4-Axis Kinematics Model                                *
# *   Supports: GGR, GG3/GG3S with rotary aftermarket kits                  *
# ***************************************************************************

"""
Ghost Gunner R 4-Axis Kinematics Model
Purpose: Cartesian + A-axis rotary kinematics for Ghost Gunner R
"""

__title__ = "CAM Ghost Gunner 4-Axis Kinematics"
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
    from .KinematicModelBase import KinematicModelBase


class MachineKinematics_GhostGunner4R(KinematicModelBase):
    """
    Kinematics model for Ghost Gunner R 4-axis machine.

    Supports:
    - Ghost Gunner R (GGR) - Factory 4-axis
    - Ghost Gunner 3/3S with aftermarket rotary kits

    Cartesian XYZ + A-axis horizontal rotary table.
    """

    def __init__(self, cfg):
        """
        Initialize Ghost Gunner R 4-axis kinematics.

        Args:
            cfg: Machine configuration dictionary
        """
        super().__init__(cfg)

        # Detect model
        model_name = cfg.get("name", "").lower()
        if "ghostgunnerr" in model_name or "ggr" in model_name:
            self.model = "GGR"
            self.is_factory_4axis = True
        else:
            self.model = "GG3_Rotary"
            self.is_factory_4axis = False

        self.controller = "GGC_Dreamer"

        # Motion constraints
        self.max_feed_rate = cfg.get("feed_rate", {}).get("max", 1200.0)
        self.default_feed_rate = cfg.get("feed_rate", {}).get("default", 800.0)
        self.rapid_feed_rate = cfg.get("feed_rate", {}).get("rapid", 2500.0)

        # Linear acceleration
        accel = cfg.get("acceleration", {})
        self.max_acceleration = accel.get("max", 600.0)
        self.default_acceleration = accel.get("default", 400.0)

        # Rotary acceleration
        rot_accel = cfg.get("rotary_acceleration", {})
        self.max_rotary_acceleration = rot_accel.get("max", 180.0)  # deg/s²
        self.default_rotary_acceleration = rot_accel.get("default", 90.0)

        # Rotary configuration
        rotary_cfg = cfg.get("rotary_config", {})
        self.rotary_axis = rotary_cfg.get("axis", "A")
        self.rotary_orientation = rotary_cfg.get("orientation", "horizontal")
        self.rotary_center_height = rotary_cfg.get("center_height", 50.0)
        self.rotary_max_diameter = rotary_cfg.get("max_diameter", 80.0)

        # Safe height
        self.safe_height = cfg.get("safe_height", 10.0)

    def forward(self, axes):
        """
        Forward kinematics for Ghost Gunner R 4-axis.

        Cartesian XYZ + A-axis rotation.

        Args:
            axes: Dictionary with X, Y, Z, A positions

        Returns:
            Tuple of (position, orientation)
        """
        x = axes.get("X", 0.0)
        y = axes.get("Y", 0.0)
        z = axes.get("Z", 0.0)
        a = axes.get("A", 0.0)

        # Tool position (not affected by A-axis rotation in this model)
        position = (x, y, z)

        # Tool orientation based on A-axis angle
        # A-axis rotates around Z, so tool can tilt in XY plane
        a_rad = math.radians(a)
        orientation = (
            math.sin(a_rad),  # X component
            0.0,  # Y component (horizontal axis)
            math.cos(a_rad),  # Z component
        )

        return (position, orientation)

    def inverse(self, tool_pos, tool_orient):
        """
        Inverse kinematics for Ghost Gunner R 4-axis.

        Computes X, Y, Z, A from tool position and orientation.

        Args:
            tool_pos: Tool tip position (x, y, z)
            tool_orient: Tool orientation vector (i, j, k)

        Returns:
            Dictionary of axis positions
        """
        # Extract position
        if isinstance(tool_pos, (list, tuple)):
            x, y, z = tool_pos[0], tool_pos[1], tool_pos[2]
        else:
            x, y, z = tool_pos.x, tool_pos.y, tool_pos.z

        # Extract orientation
        if isinstance(tool_orient, (list, tuple)):
            i, j, k = tool_orient[0], tool_orient[1], tool_orient[2]
        else:
            i, j, k = tool_orient.x, tool_orient.y, tool_orient.z

        # Normalize orientation
        length = math.sqrt(i * i + j * j + k * k)
        if length > 1e-6:
            i, j, k = i / length, j / length, k / length
        else:
            i, j, k = 0.0, 0.0, 1.0  # Default vertical

        # Compute A-axis angle from orientation
        # A-axis rotates around Z (horizontal rotation)
        # Tool orientation in XZ plane determines A angle
        a_angle = math.degrees(math.atan2(i, k))

        # Cartesian mapping
        axes = {"X": x, "Y": y, "Z": z, "A": a_angle}

        # Clamp to machine limits
        return self.clamp_axes(axes)

    def validate_motion(self, from_pos, to_pos, from_angle, to_angle, feed_rate):
        """
        Validate 4-axis motion between two positions.

        Args:
            from_pos: Starting position (x, y, z)
            to_pos: Ending position (x, y, z)
            from_angle: Starting A-axis angle (degrees)
            to_angle: Ending A-axis angle (degrees)
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

        # Check linear positions
        for pos, label in [(from_pos, "start"), (to_pos, "end")]:
            axes = {"X": pos[0], "Y": pos[1], "Z": pos[2]}
            is_valid, violations = self.check_limits(axes)
            if not is_valid:
                warnings.extend([f"{label}: {v}" for v in violations])

        # Check rotary angles
        for angle, label in [(from_angle, "start"), (to_angle, "end")]:
            axes = {"A": angle}
            is_valid, violations = self.check_limits(axes)
            if not is_valid:
                warnings.extend([f"{label} A-axis: {v}" for v in violations])

        # Calculate linear distance
        dx = to_pos[0] - from_pos[0]
        dy = to_pos[1] - from_pos[1]
        dz = to_pos[2] - from_pos[2]
        linear_distance = math.sqrt(dx * dx + dy * dy + dz * dz)

        # Calculate rotary distance
        rotary_distance = abs(to_angle - from_angle)

        # Warn if large rotary motion
        if rotary_distance > 90.0:
            warnings.append(f"Large rotary motion: {rotary_distance:.1f}° - verify clearance")

        # Check for potential collision with rotary table
        if self.rotary_max_diameter > 0:
            for pos in [from_pos, to_pos]:
                # Check if tool is too close to rotary center
                dist_from_center = math.sqrt(pos[0] ** 2 + pos[1] ** 2)
                if dist_from_center < (self.rotary_max_diameter / 2.0):
                    warnings.append(
                        f"Tool position may interfere with rotary table at ({pos[0]:.1f}, {pos[1]:.1f})"
                    )

        return (len(warnings) == 0, warnings)

    def get_safe_retract_position(self, current_pos, current_angle):
        """
        Get safe retract position for tool changes.

        Args:
            current_pos: Current tool position (x, y, z)
            current_angle: Current A-axis angle

        Returns:
            Tuple of (safe_position, safe_angle)
        """
        # Retract to safe height
        safe_pos = (current_pos[0], current_pos[1], self.safe_height)

        # Return A-axis to zero for tool changes
        safe_angle = 0.0

        return (safe_pos, safe_angle)

    def compute_cylindrical_toolpath(self, radius, height, turns, points_per_turn):
        """
        Generate cylindrical wrapping toolpath.

        Args:
            radius: Cylinder radius (mm)
            height: Total height (mm)
            turns: Number of complete rotations
            points_per_turn: Points per 360° rotation

        Returns:
            List of (position, angle) tuples
        """
        toolpath = []
        total_points = int(points_per_turn * turns)

        for i in range(total_points + 1):
            # Angle progression
            angle = (i / points_per_turn) * 360.0

            # Height progression
            z = (i / total_points) * height

            # Position (tool stays at radius distance)
            x = radius
            y = 0.0

            toolpath.append(((x, y, z), angle))

        return toolpath

    def get_controller_info(self):
        """
        Get controller-specific information.

        Returns:
            Dictionary with controller details
        """
        return {
            "model": self.model,
            "controller": self.controller,
            "is_factory_4axis": self.is_factory_4axis,
            "max_feed_rate": self.max_feed_rate,
            "max_acceleration": self.max_acceleration,
            "max_rotary_acceleration": self.max_rotary_acceleration,
            "rotary_axis": self.rotary_axis,
            "rotary_orientation": self.rotary_orientation,
            "rotary_center_height": self.rotary_center_height,
            "rotary_max_diameter": self.rotary_max_diameter,
            "safe_height": self.safe_height,
            "has_tool_changer": False,
            "has_coolant": False,
            "coordinate_system": "Cartesian + Rotary",
        }

    def get_machine_description(self):
        """
        Get human-readable machine description.

        Returns:
            Description string
        """
        desc = []
        desc.append(f"Ghost Gunner {self.model}")
        desc.append(f"Controller: {self.controller}")
        desc.append(f"Axes: X, Y, Z, A (Cartesian + Rotary)")
        desc.append(f"Max Feed: {self.max_feed_rate:.0f} mm/min")
        desc.append(f"Max Accel: {self.max_acceleration:.0f} mm/s²")
        desc.append(f"Max Rotary Accel: {self.max_rotary_acceleration:.0f} deg/s²")
        desc.append(f"Rotary: {self.rotary_orientation} A-axis")
        desc.append(f"Max Workpiece Diameter: {self.rotary_max_diameter:.0f} mm")
        desc.append(f"Safe Height: {self.safe_height:.1f} mm")

        return "\n".join(desc)


# Convenience function
def create_ghostgunner_r_kinematics(machine_config):
    """
    Factory function to create Ghost Gunner R kinematics model.

    Args:
        machine_config: Machine configuration dictionary

    Returns:
        MachineKinematics_GhostGunner4R instance
    """
    return MachineKinematics_GhostGunner4R(machine_config)
