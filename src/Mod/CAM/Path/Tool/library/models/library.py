# -*- coding: utf-8 -*-
import uuid
import pathlib
from typing import Mapping, Union, Optional
import Path
from ...assets import Asset, AssetUri


class Library(Asset):
    asset_type: str = "toolbitlibrary"
    API_VERSION = 1

    def __init__(self, label, id=None):
        self.id = id or str(uuid.uuid1())
        self.label = label
        self.tools = []
        self.tool_nos = {}  # Maps tool_no number to tool

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

        if isinstance(identifier, pathlib.Path): # Handle direct Path objects (legacy filenames)
            identifier = identifier.stem # Use the filename stem as potential ID

        if not isinstance(identifier, str):
            raise ValueError("Failed to resolve {identifier} to a Uri")
            
        return AssetUri.build(asset_type=Library.asset_type, asset_id=identifier)

    def to_dict(self) -> dict:
        """Returns a dictionary representation of the Library in the specified format."""
        tools_list = []
        for tool_no, tool in self.tool_nos.items():
            tools_list.append({
                "nr": tool_no,
                "path": f"{tool.get_id()}.fctb"  # Tool ID with .fctb extension
            })
        return {
            "label": self.label,
            "tools": tools_list,
            "version": self.API_VERSION
        }

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
            Path.Log.debug(f"Library.from_dict: Shallow load for library '{library.label}' (id: {id}). Tools not populated.")
            return library  # Only process tools if dependencies were resolved

        tools_list = data_dict.get("tools", [])
        for tool_data in tools_list:
            tool_no = tool_data["nr"]
            tool_id = pathlib.Path(tool_data["path"]).stem  # Extract tool ID
            tool_uri = AssetUri(f"toolbit://{tool_id}")
            tool = dependencies.get(tool_uri)
            if tool:
                library.add_tool(tool, tool_no=tool_no)
            else:
                raise ValueError(f"Tool with id {tool_id} not found in dependencies")
        return library

    def __str__(self):
        return '{} "{}"'.format(self.id, self.label)

    def __eq__(self, other):
        return self.id == other.id

    def __iter__(self):
        return self.tools.__iter__()

    def get_next_tool_no(self):
        tool_nolist = sorted(self.tool_nos, reverse=True)
        return tool_nolist[0]+1 if tool_nolist else 1

    def get_tool_no_from_tool(self, tool):
        for tool_no, thetool in self.tool_nos.items():
            if tool == thetool:
                return tool_no
        return None

    def get_tool_by_uri(self, uri: AssetUri):
        for tool in self.tool_nos.values():
            if tool.id == uri.asset_id:
                return tool
        return None

    def assign_new_tool_no(self, tool, tool_no=None):
        if tool not in self.tools:
            return

        # If no specific tool_no was requested, assign a new one.
        if tool_no is None:
            tool_no = self.get_next_tool_no()
        elif self.tool_nos.get(tool_no) == tool:
            return

        # Otherwise, add the tool. Since the requested tool_no may already
        # be in use, we need to account for that. In this case, we will
        # add the removed tool into a new tool_no.
        old_tool = self.tool_nos.pop(tool_no, None)
        old_tool_no = self.get_tool_no_from_tool(tool)
        if old_tool_no:
            del self.tool_nos[old_tool_no]
        self.tool_nos[tool_no] = tool
        if old_tool:
            self.assign_new_tool_no(old_tool)
        return tool_no

    def add_tool(self, tool, tool_no=None):
        if tool not in self.tools:
            self.tools.append(tool)
        return self.assign_new_tool_no(tool, tool_no)

    def get_tools(self):
        return self.tools

    def has_tool(self, tool):
        for t in self.tools:
            if tool.id == t.id:
                return True
        return False

    def remove_tool(self, tool):
        self.tools = [t for t in self.tools if t.id != tool.id]
        self.tool_nos = {k: v for (k, v) in self.tool_nos.items() if v.id != tool.id}

    def dump(self, summarize=False):
        title = 'Library "{}" ({}) (instance {})'.format(self.label, self.id, id(self))
        print("-"*len(title))
        print(title)
        print("-"*len(title))
        for tool in self.tools:
            tool.dump(summarize=summarize)
            print()
