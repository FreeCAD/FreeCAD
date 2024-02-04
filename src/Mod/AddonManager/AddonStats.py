# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 FreeCAD Project Association                        *
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

""" Classes and structures related to Addon sidecar information """
from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime


def to_int_or_zero(inp: [str | int | None]):
    try:
        return int(inp)
    except TypeError:
        return 0


@dataclass
class AddonStats:
    """Statistics about an addon: not all stats apply to all addon types"""

    last_update_time: datetime | None = None
    stars: int = 0
    open_issues: int = 0
    forks: int = 0
    license: str = ""
    page_views_last_month: int = 0

    @classmethod
    def from_json(cls, json_dict: dict):
        new_stats = AddonStats()
        if "pushed_at" in json_dict:
            new_stats.last_update_time = datetime.fromisoformat(json_dict["pushed_at"])
        new_stats.stars = to_int_or_zero(json_dict["stargazers_count"])
        new_stats.forks = to_int_or_zero(json_dict["forks_count"])
        new_stats.open_issues = to_int_or_zero(json_dict["open_issues_count"])
        new_stats.license = json_dict["license"]  # Might be None or "NOASSERTION"
        return new_stats
