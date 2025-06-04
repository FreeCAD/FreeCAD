# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
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
import uuid
import pathlib
from typing import Mapping, Union, Optional, List, Dict, cast
import Path
from ...assets import Asset, AssetUri
from ...toolbit import ToolBit


class Library(Asset):
    asset_type: str = "toolbitlibrary"
    API_VERSION = 1

    def __init__(self, label, id=None):
        self.id = id if id is not None else str(uuid.uuid4())
        self._label = label
        self._bits: List[ToolBit] = []
        self._bit_nos: Dict[int, ToolBit] = {}
        self._bit_urls: Dict[AssetUri, ToolBit] = {}

    @property
    def label(self) -> str:
        return self._label

    def get_id(self) -> str:
        """Returns the unique identifier for the Library instance."""
        return self.id

    @classmethod
    def resolve_name(cls, identifier: Union[str, AssetUri, pathlib.Path]) -> AssetUri:
        """
        Resolves various forms of library identifiers to a canonical AssetUri string.
        Handles direct AssetUri objects, URI strings, asset IDs, or legacy filenames.
        Returns the canonical URI string or None if resolution fails.
        """
        if isinstance(identifier, AssetUri):
            return identifier

        if isinstance(identifier, str) and AssetUri.is_uri(identifier):
            return AssetUri(identifier)

        if isinstance(identifier, pathlib.Path):  # Handle direct Path objects (legacy filenames)
            identifier = identifier.stem  # Use the filename stem as potential ID

        if not isinstance(identifier, str):
            raise ValueError("Failed to resolve {identifier} to a Uri")

        return AssetUri.build(asset_type=Library.asset_type, asset_id=identifier)

    def to_dict(self) -> dict:
        """Returns a dictionary representation of the Library in the specified format."""
        tools_list = []
        for tool_no, tool in self._bit_nos.items():
            tools_list.append(
                {"nr": tool_no, "path": f"{tool.get_id()}.fctb"}  # Tool ID with .fctb extension
            )
        return {"label": self.label, "tools": tools_list, "version": self.API_VERSION}

    @classmethod
    def from_dict(
        cls,
        data_dict: dict,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> "Library":
        """
        Creates a Library instance from a dictionary and resolved dependencies.
        If dependencies is None, it's a shallow load, and tools are not populated.
        """
        library = cls(data_dict.get("label", id or "Unnamed Library"), id=id)

        if dependencies is None:
            Path.Log.debug(
                f"Library.from_dict: Shallow load for library '{library.label}' (id: {id}). Tools not populated."
            )
            return library  # Only process tools if dependencies were resolved

        tools_list = data_dict.get("tools", [])
        for tool_data in tools_list:
            tool_no = tool_data["nr"]
            tool_id = pathlib.Path(tool_data["path"]).stem  # Extract tool ID
            tool_uri = AssetUri(f"toolbit://{tool_id}")
            bit = cast(ToolBit, dependencies.get(tool_uri))
            if bit:
                library.add_bit(bit, bit_no=tool_no)
            else:
                raise ValueError(f"Tool with id {tool_id} not found in dependencies")
        return library

    def __str__(self):
        return '{} "{}"'.format(self.id, self.label)

    def __eq__(self, other):
        return self.id == other.id

    def __iter__(self):
        return self._bits.__iter__()

    def get_next_bit_no(self):
        bit_nolist = sorted(self._bit_nos, reverse=True)
        return bit_nolist[0] + 1 if bit_nolist else 1

    def get_bit_no_from_bit(self, bit: ToolBit) -> Optional[int]:
        for bit_no, thebit in self._bit_nos.items():
            if bit == thebit:
                return bit_no
        return None

    def get_tool_by_uri(self, uri: AssetUri) -> Optional[ToolBit]:
        for tool in self._bit_nos.values():
            if tool.get_uri() == uri:
                return tool
        return None

    def assign_new_bit_no(self, bit: ToolBit, bit_no: Optional[int] = None) -> Optional[int]:
        if bit not in self._bits:
            return

        # If no specific bit_no was requested, assign a new one.
        if bit_no is None:
            bit_no = self.get_next_bit_no()
        elif self._bit_nos.get(bit_no) == bit:
            return

        # Otherwise, add the bit. Since the requested bit_no may already
        # be in use, we need to account for that. In this case, we will
        # add the removed bit into a new bit_no.
        old_bit = self._bit_nos.pop(bit_no, None)
        old_bit_no = self.get_bit_no_from_bit(bit)
        if old_bit_no:
            del self._bit_nos[old_bit_no]
        self._bit_nos[bit_no] = bit
        if old_bit:
            self.assign_new_bit_no(old_bit)
        return bit_no

    def add_bit(self, bit: ToolBit, bit_no: Optional[int] = None) -> Optional[int]:
        if bit not in self._bits:
            self._bits.append(bit)
        return self.assign_new_bit_no(bit, bit_no)

    def get_bits(self) -> List[ToolBit]:
        return self._bits

    def has_bit(self, bit: ToolBit) -> bool:
        for t in self._bits:
            if bit.id == t.id:
                return True
        return False

    def remove_bit(self, bit: ToolBit):
        self._bits = [t for t in self._bits if t.id != bit.id]
        self._bit_nos = {k: v for (k, v) in self._bit_nos.items() if v.id != bit.id}

    def dump(self, summarize: bool = False):
        title = 'Library "{}" ({}) (instance {})'.format(self.label, self.id, id(self))
        print("-" * len(title))
        print(title)
        print("-" * len(title))
        for bit in self._bits:
            print(f"- {bit.label} ({bit.get_id()})")
        print()
