# SPDX-License-Identifier: LGPL-2.1-or-later
# (c) 2026 Frank Martínez <mnesarco> LGPL-2.1-or-later

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
from collections.abc import Sequence
from dataclasses import dataclass, field
from typing import Any, ClassVar, TypeAlias

Schema: TypeAlias = str


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

    data: dict[str, Any]
    """Data required to locate the resource, schema dependent."""

    tags: list[str] = field(default_factory=list)
    """Classification tags, useful for indexing."""

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, ContentProvider):
            return NotImplemented
        return (self.schema, self.name) == (other.schema, other.name)

    def __hash__(self) -> int:
        return hash((self.schema, self.name))


class DuplicatedContentProviderError(RuntimeError):
    pass


class ContentProviderRegistry:
    """
    Global Content Provider Registry.

    Entry point to register and query for Content Providers.
    """

    _providers: ClassVar[dict[Schema, set[ContentProvider]]] = defaultdict(set)

    @classmethod
    def register(cls, provider: ContentProvider) -> None:
        """
        Register a content provider.
        """
        group = cls._providers[provider.schema]
        if provider in group:
            msg = f"{provider.schema}/{provider.name} Already exists!!!"
            raise DuplicatedContentProviderError(msg)

        group.add(provider)

    @classmethod
    def unregister(cls, *, schema: Schema, name: str) -> bool:
        """
        Unregister a content provider if found.
        """
        group = cls._providers[schema]
        for provider in group:
            if provider.name == name:
                group.remove(provider)
                return True
        return False

    @classmethod
    def find(
        cls,
        schema: Schema | Sequence[Schema] | None = None,
    ) -> list[ContentProvider]:
        """
        Query content providers by schema filter.
        """
        if schema is None:
            result = []
            for items in cls._providers.values():
                result.extend(items)
            return result

        if isinstance(schema, str):
            return list(cls._providers[schema])

        result = []
        for part in schema:
            result.extend(cls._providers[part])
        return result


__all__ = (
    "ContentProvider",
    "ContentProviderRegistry",
    "DuplicatedContentProviderError",
    "Schema",
)
