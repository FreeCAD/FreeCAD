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
FreeCAD Multi-Axis Machine Configuration Manager
Purpose: Load and manage machine definitions for multi-axis CAM operations
"""

__title__ = "CAM Machine Configuration"
__author__ = "FreeCAD Multi-Axis Development Team"
__url__ = "https://www.freecad.org"
__doc__ = "Machine configuration management for multi-axis CAM"

import json
import os
from typing import Dict, List, Optional, Tuple, Any
import FreeCAD
import Path

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class MachineConfig:
    """
    Machine configuration manager for multi-axis CAM operations.
    
    Loads machine definitions from JSON and provides access to machine
    parameters, limits, and capabilities.
    """
    
    def __init__(self, config_path: Optional[str] = None):
        """
        Initialize machine configuration manager.
        
        Args:
            config_path: Path to machines_full.json. If None, uses default location.
        """
        self.machines: Dict[str, Dict[str, Any]] = {}
        self.machine_categories: Dict[str, List[str]] = {}
        self.default_machine: str = "Langmuir_MR1"
        self.machine_type: str = "3axis"
        self.support_rtcp: bool = False
        
        if config_path is None:
            # Default to machines_full.json in same directory
            config_path = os.path.join(
                os.path.dirname(__file__),
                "machines_full.json"
            )
        
        self.config_path = config_path
        self.load_config(config_path)
    
    def load_config(self, config_path: str) -> bool:
        """
        Load machine configuration from JSON file.
        
        Args:
            config_path: Path to JSON configuration file
            
        Returns:
            True if successful, False otherwise
        """
        try:
            with open(config_path, 'r') as f:
                data = json.load(f)
            
            self.machines = data.get("machines", {})
            self.machine_categories = data.get("machine_categories", {})
            self.default_machine = data.get("default_machine", "Langmuir_MR1")
            
            Path.Log.info(f"Loaded {len(self.machines)} machine configurations")
            return True
            
        except FileNotFoundError:
            Path.Log.error(f"Machine configuration file not found: {config_path}")
            return False
        except json.JSONDecodeError as e:
            Path.Log.error(f"Invalid JSON in machine configuration: {e}")
            return False
        except Exception as e:
            Path.Log.error(f"Error loading machine configuration: {e}")
            return False
    
    def get_machine_names(self) -> List[str]:
        """Get list of all available machine names."""
        return list(self.machines.keys())
    
    def get_machine_config(self, machine_name: str) -> Optional[Dict[str, Any]]:
        """
        Get complete configuration for a machine.
        
        Args:
            machine_name: Name of the machine
            
        Returns:
            Machine configuration dictionary or None if not found
        """
        return self.machines.get(machine_name)
    
    def get_axes(self, machine_name: str) -> List[str]:
        """
        Get list of axes for a machine.
        
        Args:
            machine_name: Name of the machine
            
        Returns:
            List of axis names (e.g., ['X', 'Y', 'Z', 'A'])
        """
        config = self.get_machine_config(machine_name)
        if config:
            return config.get("axes", [])
        return []
    
    def get_limits(self, machine_name: str) -> Dict[str, Dict[str, float]]:
        """
        Get axis limits for a machine.
        
        Args:
            machine_name: Name of the machine
            
        Returns:
            Dictionary of axis limits
        """
        config = self.get_machine_config(machine_name)
        if config:
            return config.get("limits", {})
        return {}
    
    def get_feed_rate(self, machine_name: str) -> float:
        """
        Get default feed rate for a machine.
        
        Args:
            machine_name: Name of the machine
            
        Returns:
            Default feed rate in mm/min
        """
        config = self.get_machine_config(machine_name)
        if config:
            feed_config = config.get("feed_rate", {})
            return feed_config.get("default", 1000.0)
        return 1000.0
    
    def get_max_feed_rate(self, machine_name: str) -> float:
        """
        Get maximum feed rate for a machine.
        
        Args:
            machine_name: Name of the machine
            
        Returns:
            Maximum feed rate in mm/min
        """
        config = self.get_machine_config(machine_name)
        if config:
            feed_config = config.get("feed_rate", {})
            return feed_config.get("max", 3000.0)
        return 3000.0
    
    def get_spindle_speed(self, machine_name: str) -> int:
        """
        Get default spindle speed for a machine.
        
        Args:
            machine_name: Name of the machine
            
        Returns:
            Default spindle speed in RPM
        """
        config = self.get_machine_config(machine_name)
        if config:
            spindle_config = config.get("spindle", {})
            return spindle_config.get("default_rpm", 10000)
        return 10000
    
    def get_max_spindle_speed(self, machine_name: str) -> int:
        """
        Get maximum spindle speed for a machine.
        
        Args:
            machine_name: Name of the machine
            
        Returns:
            Maximum spindle speed in RPM
        """
        config = self.get_machine_config(machine_name)
        if config:
            spindle_config = config.get("spindle", {})
            return spindle_config.get("max_rpm", 20000)
        return 20000
    
    def has_rtcp_support(self, machine_name: str) -> bool:
        """
        Check if machine supports RTCP (Rotating Tool Center Point).
        
        Args:
            machine_name: Name of the machine
            
        Returns:
            True if RTCP is supported
        """
        config = self.get_machine_config(machine_name)
        if config:
            return config.get("rtcp_support", False)
        return False
    
    def get_post_processor(self, machine_name: str) -> str:
        """
        Get post-processor name for a machine.
        
        Args:
            machine_name: Name of the machine
            
        Returns:
            Post-processor identifier
        """
        config = self.get_machine_config(machine_name)
        if config:
            return config.get("post_processor", "generic_pp")
        return "generic_pp"
    
    def get_machine_type(self, machine_name: str) -> str:
        """
        Determine machine type based on axes.
        
        Args:
            machine_name: Name of the machine
            
        Returns:
            Machine type: '3axis', '4axis_A', '4axis_B', or '5axis'
        """
        axes = self.get_axes(machine_name)
        
        if len(axes) == 3 and 'X' in axes and 'Y' in axes and 'Z' in axes:
            self.machine_type = "3axis"
            self.support_rtcp = False
        elif len(axes) == 4:
            if 'A' in axes:
                self.machine_type = "4axis_A"
            elif 'B' in axes:
                self.machine_type = "4axis_B"
            else:
                self.machine_type = "4axis"
            self.support_rtcp = False
        elif len(axes) == 5:
            self.machine_type = "5axis"
            self.support_rtcp = self.has_rtcp_support(machine_name)
        else:
            self.machine_type = "unknown"
            self.support_rtcp = False
        
        # Special handling for Langmuir MR1
        if machine_name == "Langmuir_MR1":
            self.machine_type = "3axis"
            self.support_rtcp = False
        
        return self.machine_type
    
    def get_rotary_config(self, machine_name: str) -> Optional[Dict[str, Any]]:
        """
        Get rotary axis configuration for a machine.
        
        Args:
            machine_name: Name of the machine
            
        Returns:
            Rotary configuration dictionary or None
        """
        config = self.get_machine_config(machine_name)
        if config:
            return config.get("rotary_config")
        return None
    
    def validate_position(self, machine_name: str, 
                         position: Dict[str, float]) -> Tuple[bool, List[str]]:
        """
        Validate if a position is within machine limits.
        
        Args:
            machine_name: Name of the machine
            position: Dictionary of axis positions (e.g., {'X': 10.0, 'Y': 20.0})
            
        Returns:
            Tuple of (is_valid, list_of_violations)
        """
        limits = self.get_limits(machine_name)
        violations = []
        
        for axis, value in position.items():
            if axis in limits:
                axis_limits = limits[axis]
                min_val = axis_limits.get("min", float('-inf'))
                max_val = axis_limits.get("max", float('inf'))
                
                if value < min_val:
                    violations.append(
                        f"{axis} = {value:.2f} below minimum {min_val:.2f}"
                    )
                elif value > max_val:
                    violations.append(
                        f"{axis} = {value:.2f} above maximum {max_val:.2f}"
                    )
        
        return (len(violations) == 0, violations)
    
    def get_machines_by_category(self, category: str) -> List[str]:
        """
        Get list of machines in a category.
        
        Args:
            category: Category name (e.g., 'hobbyist', '3-axis', '5-axis')
            
        Returns:
            List of machine names in that category
        """
        return self.machine_categories.get(category, [])
    
    def get_machine_info(self, machine_name: str) -> str:
        """
        Get formatted information string about a machine.
        
        Args:
            machine_name: Name of the machine
            
        Returns:
            Formatted information string
        """
        config = self.get_machine_config(machine_name)
        if not config:
            return f"Machine '{machine_name}' not found"
        
        info = []
        info.append(f"Machine: {machine_name}")
        info.append(f"Description: {config.get('description', 'N/A')}")
        info.append(f"Manufacturer: {config.get('manufacturer', 'N/A')}")
        info.append(f"Model: {config.get('model', 'N/A')}")
        info.append(f"Category: {config.get('category', 'N/A')}")
        info.append(f"Axes: {', '.join(self.get_axes(machine_name))}")
        info.append(f"Machine Type: {self.get_machine_type(machine_name)}")
        info.append(f"RTCP Support: {self.has_rtcp_support(machine_name)}")
        info.append(f"Post-Processor: {self.get_post_processor(machine_name)}")
        
        return "\n".join(info)


# Global instance for easy access
_global_config: Optional[MachineConfig] = None


def get_machine_config() -> MachineConfig:
    """
    Get global machine configuration instance.
    
    Returns:
        Global MachineConfig instance
    """
    global _global_config
    if _global_config is None:
        _global_config = MachineConfig()
    return _global_config


def reload_machine_config(config_path: Optional[str] = None) -> MachineConfig:
    """
    Reload machine configuration from file.
    
    Args:
        config_path: Optional path to configuration file
        
    Returns:
        New MachineConfig instance
    """
    global _global_config
    _global_config = MachineConfig(config_path)
    return _global_config
