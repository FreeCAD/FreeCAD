# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
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

"""Tool holders: everything between the spindle nose and the tool bit, taken as one. That is
the holder body and whatever grips the tool bit, such as a collet and nut, a shrink fit, a drill
chuck or an adapter.

ToolHolder holds what every kind of tool holder has; each kind is a subclass that adds its shape
and how it is stored. ToolHolderOutline is the kind FreeCAD has now: an outline of (radius,
height) points in mm, revolved about the tool's axis and measured upward from the exit face, the
face the tool bit comes out of. The outline is all a simulation or collision check needs, so no
model is opened or recomputed to use one.

A tool bit is set in zero or one tool holder at one stickout, the distance from the exit face to
the tip. Stickout is not gauge length, which runs from where the tool holder seats in the spindle.

Tool holders ship with FreeCAD as .fcholder files in Mod/CAM/Tools/ToolHolder. The CAM asset
folder's Tools/ToolHolder is read too; a tool holder there replaces the built-in one with the
same name.
"""

import json
from typing import Dict, List, Mapping, Optional, Sequence, Tuple

import Path
from ..assets import Asset, AssetSerializer, AssetUri

FORMAT_VERSION = 1


class ToolHolder(Asset):
    """What every kind of tool holder has: its id, name and details. A subclass adds its shape
    and sets the asset_type it is stored as."""

    def __init__(
        self, id: str, name: str, holder_type: str = "", notes: str = "", source: str = ""
    ):
        super().__init__()
        self.id = id
        self.name = name
        self.holder_type = holder_type
        self.notes = notes
        self.source = source

    def get_id(self) -> str:
        return self.id

    @property
    def label(self) -> str:
        return self.name

    def to_dict(self) -> dict:
        optional = {"type": self.holder_type, "notes": self.notes, "source": self.source}
        return {"name": self.name, **{key: value for key, value in optional.items() if value}}


class ToolHolderOutline(ToolHolder):
    """A tool holder drawn as an outline revolved about the tool's axis, stored as .fcholder."""

    asset_type: str = "toolholder"

    def __init__(self, id: str, name: str, profile: Sequence[Sequence[float]], **details):
        super().__init__(id, name, **details)
        self.profile: List[Tuple[float, float]] = [(float(r), float(z)) for r, z in profile]
        self.validate()

    @property
    def height(self) -> float:
        """How far the tool holder reaches above the exit face, in mm."""
        return max(z for _, z in self.profile)

    @property
    def diameter(self) -> float:
        """The tool holder's largest diameter, in mm."""
        return 2.0 * max(r for r, _ in self.profile)

    def validate(self):
        if len(self.profile) < 3:
            raise ValueError(f"Holder {self.id}: an outline needs at least three points")
        if self.profile[0][0] != 0 or self.profile[-1][0] != 0:
            raise ValueError(f"Holder {self.id}: the outline does not start and end on the axis")
        for r, z in self.profile:
            if r < 0:
                raise ValueError(f"Holder {self.id}: radius {r} is negative")
            if z < 0:
                raise ValueError(f"Holder {self.id}: height {z} is below the face the bit leaves")

    def to_dict(self) -> dict:
        data = super().to_dict()
        data.update(version=FORMAT_VERSION, units="mm", profile=[[r, z] for r, z in self.profile])
        return data

    @classmethod
    def from_dict(cls, id: str, data: dict) -> "ToolHolderOutline":
        units = data.get("units", "mm")
        if units != "mm":
            raise ValueError(f"Holder {id}: outlines are in mm, not {units}")
        return cls(
            id=id,
            name=data.get("name", id),
            profile=data["profile"],
            holder_type=data.get("type", ""),
            notes=data.get("notes", ""),
            source=data.get("source", ""),
        )


class ToolHolderOutlineSerializer(AssetSerializer):
    for_class = ToolHolderOutline
    extensions = (".fcholder",)
    mime_type = "application/x-freecad-toolholder"

    @classmethod
    def get_label(cls) -> str:
        return "FreeCAD Tool Holder"

    @classmethod
    def extract_dependencies(cls, data: bytes) -> List[AssetUri]:
        return []

    @classmethod
    def serialize(cls, asset: ToolHolderOutline) -> bytes:
        return json.dumps(asset.to_dict(), sort_keys=True, indent=2).encode("utf-8")

    @classmethod
    def deserialize(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> ToolHolderOutline:
        return ToolHolderOutline.from_dict(id, json.loads(data.decode("utf-8")))

    @classmethod
    def deep_deserialize(cls, data: bytes) -> ToolHolderOutline:
        attrs = json.loads(data.decode("utf-8"))
        return ToolHolderOutline.from_dict(attrs.get("id", attrs.get("name", "holder")), attrs)


def available_holders(asset_manager=None) -> Dict[str, ToolHolder]:
    """Return every holder by id: the built-in ones and those in the CAM asset folder, which
    replace a built-in one with the same id. A holder that cannot be read is skipped with a
    warning."""
    if asset_manager is None:
        from ..camassets import cam_assets as asset_manager

    holders: Dict[str, ToolHolder] = {}
    for store in ("builtin", "local"):
        for uri in asset_manager.list_assets(asset_type=ToolHolderOutline.asset_type, store=store):
            try:
                holder = asset_manager.get(uri, store=store)
            except Exception as e:
                Path.Log.warning(f"Tool holder {uri} could not be read: {e}")
                continue
            holders[uri.asset_id] = holder
    return holders
