# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 FreeCAD Multi-Axis Development Team                *
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
Kinematic Model Base Class
Purpose: Abstract base class for machine kinematics
"""

__title__ = "CAM Kinematic Model Base"
__author__ = "FreeCAD Multi-Axis Development Team"
__url__ = "https://www.freecad.org"
__doc__ = "Base class for machine kinematic models"

import math
from typing import Tuple, Dict, List, Optional

# Try to import FreeCAD, but allow standalone usage
try:
    import FreeCAD
    from FreeCAD import Vector

    FREECAD_AVAILABLE = True
except ImportError:
    FREECAD_AVAILABLE = False

    # Fallback Vector class for standalone usage
    class Vector:
        def __init__(self, x=0, y=0, z=0):
            self.x = x
            self.y = y
            self.z = z

        @property
        def Length(self):
            return math.sqrt(self.x**2 + self.y**2 + self.z**2)


class KinematicModelBase:
    """
    Abstract base class for machine kinematic models.

    Provides common functionality for forward/inverse kinematics,
    coordinate transformations, and machine envelope checking.
    """

    def __init__(self, cfg):
        """
        Initialize kinematic model.

        Args:
            cfg: Machine configuration dictionary
        """
        self.cfg = cfg
        self.machine_config = cfg
        self.name = cfg.get("name", "generic")
        self.axes = cfg.get("axes", [])
        self.limits = cfg.get("limits", {})
        self.rtcp_support = cfg.get("rtcp_support", False)

    def forward(self, axes):
        """
        Compute forward kinematics: joint angles -> tool position and orientation.

        Args:
            axes: Dictionary of joint angles (e.g., {'X': 10, 'Y': 20, 'Z': 30, 'A': 45})

        Returns:
            Tool center transform (implementation specific)
        """
        raise NotImplementedError

    def inverse(self, tool_pos, tool_orient):
        """
        Compute inverse kinematics: tool position and orientation -> joint angles.

        Args:
            tool_pos: Tool tip position (tuple or Vector)
            tool_orient: Tool axis direction (tuple or Vector)

        Returns:
            Dictionary of joint angles or None if unreachable
        """
        raise NotImplementedError

    def forward_kinematics(self, joint_angles):
        """Alias for forward() for compatibility"""
        return self.forward(joint_angles)

    def inverse_kinematics(self, tool_position, tool_direction):
        """Alias for inverse() for compatibility"""
        return self.inverse(tool_position, tool_direction)

    def clamp_axes(self, axes):
        """
        Clamp axes to limits if present.

        Args:
            axes: Dictionary of axis values

        Returns:
            Dictionary of clamped axis values
        """
        limits = self.cfg.get("limits", {})
        clamped = {}
        for k, v in axes.items():
            if k in limits:
                mn = limits[k]["min"]
                mx = limits[k]["max"]
                clamped[k] = max(mn, min(mx, v))
            else:
                clamped[k] = v
        return clamped

    def check_limits(self, joint_angles):
        """
        Check if joint angles are within machine limits.

        Args:
            joint_angles: Dictionary of joint angles

        Returns:
            Tuple of (is_valid, list_of_violations)
        """
        violations = []

        for axis, value in joint_angles.items():
            if axis in self.limits:
                axis_limits = self.limits[axis]
                min_val = axis_limits.get("min", float("-inf"))
                max_val = axis_limits.get("max", float("inf"))

                if value < min_val:
                    violations.append(f"{axis} = {value:.2f} below minimum {min_val:.2f}")
                elif value > max_val:
                    violations.append(f"{axis} = {value:.2f} above maximum {max_val:.2f}")

        return (len(violations) == 0, violations)

    def check_envelope(self, tool_position: Vector) -> bool:
        """
        Check if tool position is within machine work envelope.

        Args:
            tool_position: Tool tip position

        Returns:
            True if within envelope
        """
        # Check linear axes
        for axis_name, coord in [
            ("X", tool_position.x),
            ("Y", tool_position.y),
            ("Z", tool_position.z),
        ]:
            if axis_name in self.limits:
                limits = self.limits[axis_name]
                if coord < limits.get("min", float("-inf")) or coord > limits.get(
                    "max", float("inf")
                ):
                    return False

        return True

    @staticmethod
    def normalize_vector(v: Vector) -> Vector:
        """
        Normalize a vector to unit length.

        Args:
            v: Input vector

        Returns:
            Normalized vector
        """
        length = v.Length
        if length < 1e-10:
            return Vector(0, 0, 1)  # Default to Z-up
        return Vector(v.x / length, v.y / length, v.z / length)

    @staticmethod
    def rotation_matrix_x(angle_deg: float) -> List[List[float]]:
        """
        Create rotation matrix around X axis.

        Args:
            angle_deg: Rotation angle in degrees

        Returns:
            3x3 rotation matrix
        """
        angle_rad = math.radians(angle_deg)
        c = math.cos(angle_rad)
        s = math.sin(angle_rad)

        return [[1, 0, 0], [0, c, -s], [0, s, c]]

    @staticmethod
    def rotation_matrix_y(angle_deg: float) -> List[List[float]]:
        """
        Create rotation matrix around Y axis.

        Args:
            angle_deg: Rotation angle in degrees

        Returns:
            3x3 rotation matrix
        """
        angle_rad = math.radians(angle_deg)
        c = math.cos(angle_rad)
        s = math.sin(angle_rad)

        return [[c, 0, s], [0, 1, 0], [-s, 0, c]]

    @staticmethod
    def rotation_matrix_z(angle_deg: float) -> List[List[float]]:
        """
        Create rotation matrix around Z axis.

        Args:
            angle_deg: Rotation angle in degrees

        Returns:
            3x3 rotation matrix
        """
        angle_rad = math.radians(angle_deg)
        c = math.cos(angle_rad)
        s = math.sin(angle_rad)

        return [[c, -s, 0], [s, c, 0], [0, 0, 1]]

    @staticmethod
    def apply_rotation_matrix(matrix: List[List[float]], v: Vector) -> Vector:
        """
        Apply rotation matrix to a vector.

        Args:
            matrix: 3x3 rotation matrix
            v: Input vector

        Returns:
            Rotated vector
        """
        x = matrix[0][0] * v.x + matrix[0][1] * v.y + matrix[0][2] * v.z
        y = matrix[1][0] * v.x + matrix[1][1] * v.y + matrix[1][2] * v.z
        z = matrix[2][0] * v.x + matrix[2][1] * v.y + matrix[2][2] * v.z

        return Vector(x, y, z)

    @staticmethod
    def multiply_matrices(m1: List[List[float]], m2: List[List[float]]) -> List[List[float]]:
        """
        Multiply two 3x3 matrices.

        Args:
            m1: First matrix
            m2: Second matrix

        Returns:
            Product matrix
        """
        result = [[0, 0, 0], [0, 0, 0], [0, 0, 0]]

        for i in range(3):
            for j in range(3):
                for k in range(3):
                    result[i][j] += m1[i][k] * m2[k][j]

        return result

    def vector_to_angles(self, direction: Vector) -> Tuple[float, float]:
        """
        Convert direction vector to spherical angles.

        Args:
            direction: Direction vector (normalized)

        Returns:
            Tuple of (tilt_angle, rotation_angle) in degrees
        """
        # Normalize
        d = self.normalize_vector(direction)

        # Tilt angle from vertical (Z-axis)
        tilt_angle = math.degrees(math.acos(max(-1.0, min(1.0, d.z))))

        # Rotation angle in XY plane
        rotation_angle = math.degrees(math.atan2(d.y, d.x))

        return (tilt_angle, rotation_angle)

    def angles_to_vector(self, tilt_angle: float, rotation_angle: float) -> Vector:
        """
        Convert spherical angles to direction vector.

        Args:
            tilt_angle: Tilt from vertical in degrees
            rotation_angle: Rotation in XY plane in degrees

        Returns:
            Direction vector
        """
        tilt_rad = math.radians(tilt_angle)
        rot_rad = math.radians(rotation_angle)

        x = math.sin(tilt_rad) * math.cos(rot_rad)
        y = math.sin(tilt_rad) * math.sin(rot_rad)
        z = math.cos(tilt_rad)

        return Vector(x, y, z)

    def get_machine_description(self) -> str:
        """
        Get human-readable machine description.

        Returns:
            Description string
        """
        desc = []
        desc.append(f"Machine: {self.machine_config.get('description', 'Unknown')}")
        desc.append(f"Axes: {', '.join(self.axes)}")
        desc.append(f"RTCP Support: {self.rtcp_support}")

        return "\n".join(desc)
