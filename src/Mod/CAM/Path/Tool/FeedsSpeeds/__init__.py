# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

from .resolver import resolve, default_providers
from .types import (
    FeedSpeedResult,
    MaterialContext,
    OpContext,
    PartialResult,
    ToolContext,
    OP_TYPES,
)
from .presets import (
    PRESETS_PROPERTY,
    derive_preset_label,
    get_presets,
    make_preset,
    set_presets,
)

__all__ = (
    "FeedSpeedResult",
    "MaterialContext",
    "OpContext",
    "OP_TYPES",
    "PartialResult",
    "PRESETS_PROPERTY",
    "ToolContext",
    "default_providers",
    "derive_preset_label",
    "get_presets",
    "make_preset",
    "resolve",
    "set_presets",
)
