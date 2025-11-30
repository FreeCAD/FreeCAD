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

from typing import Dict, Any


def validate_machine_schema(data: Dict[str, Any]) -> bool:
    """Validate machine configuration data schema.

    Args:
        data: Dictionary containing machine configuration data

    Returns:
        True if data is valid, False otherwise
    """
    # Minimal validation of required fields
    if "machine" not in data:
        return False
    m = data["machine"]
    if "name" not in m or "type" not in m:
        return False
    return True


def load_machine_file(path: str) -> Dict[str, Any]:
    """Load machine configuration from a JSON file.

    Args:
        path: Path to the .fcm machine file to load

    Returns:
        Dictionary containing machine configuration data

    Raises:
        FileNotFoundError: If the file does not exist
        json.JSONDecodeError: If the file is not valid JSON
        Exception: For other I/O errors
    """
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        Path.Log.info(f"Loaded machine file: {path}")
        return data
    except FileNotFoundError:
        raise FileNotFoundError(f"Machine file not found: {path}")
    except json.JSONDecodeError as e:
        raise json.JSONDecodeError(f"Invalid JSON in machine file {path}: {e}")
    except Exception as e:
        raise Exception(f"Failed to load machine file {path}: {e}")


def save_machine_file(data: Dict[str, Any], path: str):
    """Save machine configuration to a JSON file.

    Args:
        data: Dictionary containing machine configuration data
        path: Path to save the .fcm machine file

    Raises:
        Exception: For I/O errors
    """
    try:
        with open(path, "w", encoding="utf-8") as f:
            json.dump(data, f, sort_keys=True, indent=4)
        Path.Log.info(f"Saved machine file: {path}")
    except Exception as e:
        raise Exception(f"Failed to save machine file {path}: {e}")


def create_default_machine_data() -> Dict[str, Any]:
    """Create default machine configuration data.

    Returns:
        Dictionary with default machine configuration
    """
    return {
        "machine": {
            "name": "New Machine",
            "manufacturer": "",
            "description": "",
            "units": "metric",
            "type": "custom",
        },
        "post": {
            "output_unit": "metric",
            "comments": True,
            "line_numbers": {"enabled": True},
            "tool_length_offset": True,
        },
        "version": 1,
    }
