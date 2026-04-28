# SPDX-License-Identifier: LGPL-2.1-or-later

# This package aggregates tool bit shape classes.

import os
import json
import Path
from Path.Preferences import getAssetPath, getBuiltinAssetPath

# Import the base class and all concrete shape classes
from .models.base import ToolBitShape
from .models.ballend import ToolBitShapeBallend
from .models.bullnose import ToolBitShapeBullnose
from .models.chamfer import ToolBitShapeChamfer
from .models.custom import ToolBitShapeCustom
from .models.dovetail import ToolBitShapeDovetail
from .models.drill import ToolBitShapeDrill
from .models.endmill import ToolBitShapeEndmill
from .models.radius import ToolBitShapeRadius
from .models.probe import ToolBitShapeProbe
from .models.reamer import ToolBitShapeReamer
from .models.slittingsaw import ToolBitShapeSlittingSaw
from .models.tap import ToolBitShapeTap
from .models.taperedballnose import ToolBitShapeTaperedBallNose
from .models.threadmill import ToolBitShapeThreadMill
from .models.vbit import ToolBitShapeVBit
from .models.icon import (
    ToolBitShapeIcon,
    ToolBitShapePngIcon,
    ToolBitShapeSvgIcon,
)

# Paths for alias files
_INTERNAL_ALIASES_FILE = os.path.join(getBuiltinAssetPath(), "Shape", "shape_aliases.json")
_USER_ALIASES_FILE = os.path.join(getAssetPath(), "Tools", "Shape", "shape_aliases.json")


def _load_shape_aliases():
    """Load shape aliases from JSON files (internal default merged with user overrides, avoiding duplicates)."""
    aliases = {}

    # Always load internal file first
    try:
        if os.path.isfile(_INTERNAL_ALIASES_FILE):
            with open(_INTERNAL_ALIASES_FILE, "r") as f:
                aliases = json.load(f)
            Path.Log.debug(f"Loaded default shape aliases from {_INTERNAL_ALIASES_FILE}")
        else:
            Path.Log.warning(
                f"Internal shape aliases file not found: {_INTERNAL_ALIASES_FILE}"
            )  # Should not happen in installed system
    except Exception as e:
        Path.Log.error(f"Failed to load internal shape aliases: {e}")

    # Merge user file if it exists
    try:
        if os.path.isfile(_USER_ALIASES_FILE):
            with open(_USER_ALIASES_FILE, "r") as f:
                user_aliases = json.load(f)

            # Merge user aliases into the loaded aliases
            for shape_name, data in user_aliases.items():
                if shape_name not in aliases:
                    aliases[shape_name] = {"aliases": [], "subtypes": []}

                if "aliases" in data:
                    # Use set to avoid duplicates
                    existing_aliases = set(aliases[shape_name]["aliases"])
                    for alias in data["aliases"]:
                        if alias.startswith("!"):
                            # Remove alias (strip the ! prefix)
                            existing_aliases.discard(alias[1:])
                        else:
                            # Add alias
                            existing_aliases.add(alias)
                    aliases[shape_name]["aliases"] = sorted(existing_aliases)
                if "subtypes" in data:
                    # Use set to avoid duplicates
                    existing_subtypes = set(aliases[shape_name]["subtypes"])
                    for subtype in data["subtypes"]:
                        if subtype.startswith("!"):
                            # Remove subtype (strip the ! prefix)
                            existing_subtypes.discard(subtype[1:])
                        else:
                            # Add subtype
                            existing_subtypes.add(subtype)
                    aliases[shape_name]["subtypes"] = sorted(existing_subtypes)

            Path.Log.info(f"Merged user shape aliases from {_USER_ALIASES_FILE}")
    except Exception as e:
        Path.Log.error(f"Failed to load/merge user shape aliases: {e}")

    return aliases


def _apply_aliases_to_shape_classes():
    """Apply loaded aliases and subtypes to all registered shape classes."""
    config = _load_shape_aliases()

    # Map class names to actual classes
    shape_classes = {cls.name: cls for cls in ToolBitShape.__subclasses__()}

    for shape_name, data in config.items():
        if shape_name not in shape_classes:
            Path.Log.warning(f"Unknown shape class '{shape_name}' in aliases file")
            continue

        shape_class = shape_classes[shape_name]

        # Expect dict format with aliases/subtypes
        if not isinstance(data, dict):
            Path.Log.warning(f"Invalid data format for {shape_name} - expected dict")
            continue

        aliases_list = data.get("aliases", [])
        subtypes_list = data.get("subtypes", [])
        shape_class.aliases = tuple(sorted(alias.lower() for alias in aliases_list))
        shape_class.subtypes = tuple(sorted(subtype.lower() for subtype in subtypes_list))
        Path.Log.info(f"Applied aliases to {shape_name}: {aliases_list}")
        Path.Log.info(f"Applied subtypes to {shape_name}: {subtypes_list}")


# Load and apply aliases immediately after imports
_apply_aliases_to_shape_classes()

# A list of the name of each ToolBitShape
TOOL_BIT_SHAPE_NAMES = sorted([cls.name for cls in ToolBitShape.__subclasses__()])

# Define __all__ for explicit public interface
__all__ = [
    "ToolBitShape",
    "ToolBitShapeBallend",
    "ToolBitShapeBullnose",
    "ToolBitShapeChamfer",
    "ToolBitShapeCustom",
    "ToolBitShapeDovetail",
    "ToolBitShapeDrill",
    "ToolBitShapeEndmill",
    "ToolBitShapeRadius",
    "ToolBitShapeProbe",
    "ToolBitShapeReamer",
    "ToolBitShapeSlittingSaw",
    "ToolBitShapeTap",
    "ToolBitShapeTaperedBallNose",
    "ToolBitShapeThreadMill",
    "ToolBitShapeVBit",
    "TOOL_BIT_SHAPE_NAMES",
    "ToolBitShapeIcon",
    "ToolBitShapeSvgIcon",
    "ToolBitShapePngIcon",
]
