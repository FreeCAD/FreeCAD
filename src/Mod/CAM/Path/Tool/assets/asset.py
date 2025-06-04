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
import abc
from abc import ABC
from typing import Mapping, List, Optional, Type, TYPE_CHECKING
from .uri import AssetUri

if TYPE_CHECKING:
    from .serializer import AssetSerializer


class Asset(ABC):
    asset_type: str

    def __init__(self, *args, **kwargs):
        if not hasattr(self, "asset_type"):
            raise ValueError("Asset subclasses must define 'asset_type'.")

    @property
    def label(self) -> str:
        return self.__class__.__name__

    @abc.abstractmethod
    def get_id(self) -> str:
        """Returns the unique ID of an asset object."""
        pass

    def get_uri(self) -> AssetUri:
        return AssetUri.build(asset_type=self.asset_type, asset_id=self.get_id())

    @classmethod
    def resolve_name(cls, identifier: str) -> AssetUri:
        """
        Resolves an identifier (id, name, or URI) to an AssetUri object.
        """
        # 1. If the input is a url string, return the Uri object for it.
        if AssetUri.is_uri(identifier):
            return AssetUri(identifier)

        # 2. Construct the Uri using Uri.build() and return it
        return AssetUri.build(
            asset_type=cls.asset_type,
            asset_id=identifier,
        )

    @classmethod
    def get_uri_from_id(cls, asset_id):
        return AssetUri.build(cls.asset_type, asset_id=asset_id)

    @classmethod
    def extract_dependencies(cls, data: bytes, serializer: Type[AssetSerializer]) -> List[AssetUri]:
        """Extracts URIs of dependencies from serialized data."""
        return serializer.extract_dependencies(data)

    @classmethod
    def from_bytes(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
        serializer: Type[AssetSerializer],
    ) -> Asset:
        """
        Creates an object from serialized data and resolved dependencies.
        If dependencies is None, it indicates a shallow load where dependencies were not resolved.
        """
        return serializer.deserialize(data, id, dependencies)

    def to_bytes(self, serializer: Type[AssetSerializer]) -> bytes:
        """Serializes an object into bytes."""
        return serializer.serialize(self)
