# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Frank Martínez
# SPDX-FileNotice: Part of the FreeCAD project.

"""
Python API for Content Providers.

Provides a global registry for content providers.

Content Managers are components that manage some kind of
resources/assets associated to a specific schema. Managers do not
contain the resources in itself but uses content providers to discover
the resources from its sources like filesystem, databases, git repositories,
urls, etc... and knows how to fetch and provide them to the user.

Each content provider is associated with a schema, the schema is known
by the provider and by the content manager associated. there may be many content
managers capable of managing the same schema.

This API provides a global registry where any component can register or query
about available sources per schema. So an Addon can register new sources of
library components or sources of svg symbols to be consumable by sketcher for example.

Caching providers info is responsibility of the managers.
"""

from __future__ import annotations

from collections import defaultdict
from collections.abc import Mapping, Sequence
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from types import MappingProxyType
from typing import ClassVar, TypeAlias

Schema: TypeAlias = str

Immutable: TypeAlias = (
    str | int | float | Sequence["Immutable"] | Mapping[str, "Immutable"] | Path | None
)


class OnConflict(Enum):
    Ignore = 0
    Override = 1
    Raise = -1


def _freeze(value: Immutable) -> Immutable:
    """Enforce nested immutability of containers"""
    if isinstance(value, (str, int, float, Path, bytes)) or value is None:
        return value
    if isinstance(value, Mapping):
        return MappingProxyType({k: _freeze(v) for k, v in value.items()})
    if isinstance(value, Sequence):
        return tuple(_freeze(v) for v in value)
    msg = f"type '{type(value).__name__}' is not supported."
    raise TypeError(msg)


@dataclass(kw_only=True, frozen=True)
class ContentProvider:
    """
    Content provider record. Contains the information required to
    resolve the resource but knows nothing about the resource itself.
    """

    schema: Schema
    """Type of resource/asset. use URN spec: https://en.wikipedia.org/wiki/Uniform_Resource_Name"""

    name: str
    """Name of the content provider. Must be unique per schema."""

    label: str
    """Description of the provider, can be translated."""

    data: Mapping[str, Immutable]
    """Data required to locate the resource, schema dependent."""

    tags: tuple[str, ...] = field(default_factory=tuple)
    """Classification tags, useful for indexing."""

    def __eq__(self, other: object) -> bool:
        """Identity is (schema, name), data is checked in `register`."""
        if not isinstance(other, ContentProvider):
            return NotImplemented
        return (self.schema, self.name) == (other.schema, other.name)

    def __hash__(self) -> int:
        return hash((self.schema, self.name))

    def __post_init__(self) -> None:
        object.__setattr__(self, "data", _freeze(self.data))
        object.__setattr__(self, "tags", _freeze(self.tags))


class DuplicatedContentProviderError(RuntimeError):
    pass


class ContentProviderRegistry:
    """
    Global Content Provider Registry.

    Entry point to register and query for Content Providers.
    """

    _providers: ClassVar[dict[Schema, dict[str, ContentProvider]]] = defaultdict(dict)

    @classmethod
    def register(
        cls,
        provider: ContentProvider,
        on_conflict: OnConflict = OnConflict.Raise,
    ) -> None:
        """
        Register a content provider.
        """
        group = cls._providers[provider.schema]
        previous = group.get(provider.name)
        if previous is None:
            group[provider.name] = provider
            return

        match on_conflict:
            case OnConflict.Raise:
                if previous.data != provider.data:
                    msg = f"A different {provider.schema}/{provider.name} Already exists."
                    raise DuplicatedContentProviderError(msg)
            case OnConflict.Override:
                group[provider.name] = provider
            case OnConflict.Ignore:
                pass
            case _:
                msg = f"invalid on_conflict: {on_conflict!r}"
                raise TypeError(msg)

    @classmethod
    def unregister(cls, *, schema: Schema, name: str) -> bool:
        """
        Unregister a content provider if found.
        """
        group = cls._providers.get(schema)
        return bool(group and group.pop(name, False))

    @classmethod
    def find(
        cls,
        schema: Schema | Sequence[Schema] | None = None,
    ) -> list[ContentProvider]:
        """
        Query content providers by schema filter.
        """
        result: set[ContentProvider] = set()
        if schema is None:
            for items in cls._providers.values():
                result.update(items.values())
        elif isinstance(schema, str):
            if items := cls._providers.get(schema):
                result.update(items.values())
        else:
            for part in schema:
                if items := cls._providers.get(part):
                    result.update(items.values())
        return sorted(result, key=lambda p: (p.schema, p.name))


__all__ = (
    "ContentProvider",
    "ContentProviderRegistry",
    "DuplicatedContentProviderError",
    "Immutable",
    "OnConflict",
    "Schema",
)
