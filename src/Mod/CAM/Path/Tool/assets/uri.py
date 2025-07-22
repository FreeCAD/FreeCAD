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
from __future__ import annotations
import urllib.parse
from typing import Dict, Any, Mapping


class AssetUri:
    """
    Represents an asset URI with components.

    The URI structure is: <asset_type>://<asset_id>[/version]
    """

    def __init__(self, uri_string: str):
        # Manually parse the URI string
        parts = uri_string.split("://", 1)
        if len(parts) != 2:
            raise ValueError(f"Invalid URI structure: {uri_string}")

        self.asset_type = parts[0]
        rest = parts[1]

        # Split asset_id, version, and params
        path_and_query = rest.split("?", 1)
        path_parts = path_and_query[0].split("/")

        if not path_parts or not path_parts[0]:
            raise ValueError(f"Invalid URI structure: {uri_string}")

        self.asset_id = path_parts[0]
        self.version = path_parts[1] if len(path_parts) > 1 else None

        if len(path_parts) > 2:
            raise ValueError(f"Invalid URI path structure: {uri_string}")

        self.params: Dict[str, list[str]] = {}
        if len(path_and_query) > 1:
            self.params = urllib.parse.parse_qs(path_and_query[1])

        if not self.asset_type or not self.asset_id:
            raise ValueError(f"Invalid URI structure: {uri_string}")

    def __str__(self) -> str:
        path = f"/{self.version}" if self.version else ""

        query = urllib.parse.urlencode(self.params, doseq=True) if self.params else ""

        uri_string = urllib.parse.urlunparse((self.asset_type, self.asset_id, path, "", query, ""))
        return uri_string

    def __repr__(self) -> str:
        return f"AssetUri('{str(self)}')"

    def __eq__(self, other: Any) -> bool:
        if not isinstance(other, AssetUri):
            return NotImplemented
        return (
            self.asset_type == other.asset_type
            and self.asset_id == other.asset_id
            and self.version == other.version
            and self.params == other.params
        )

    def __hash__(self) -> int:
        """Returns a hash value for the AssetUri."""
        return hash((self.asset_type, self.asset_id, self.version, frozenset(self.params.items())))

    @classmethod
    def is_uri(cls, uri: AssetUri | str) -> bool:
        """Checks if the given string is a valid URI."""
        if isinstance(uri, AssetUri):
            return True

        try:
            AssetUri(uri)
        except ValueError:
            return False
        return True

    @staticmethod
    def build(
        asset_type: str,
        asset_id: str,
        version: str | None = None,
        params: Mapping[str, str | list[str]] | None = None,
    ) -> AssetUri:
        """Builds a Uri object from components."""
        uri = AssetUri.__new__(AssetUri)  # Create a new instance without calling __init__
        uri.asset_type = asset_type
        uri.asset_id = asset_id
        uri.version = version
        uri.params = {}
        if params:
            for key, value in params.items():
                if isinstance(value, list):
                    uri.params[key] = value
                else:
                    uri.params[key] = [value]
        return uri
