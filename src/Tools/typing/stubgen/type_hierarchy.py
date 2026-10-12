# pyright: strict

"""Discover the C++ TypeId inheritance graph used by stub generation.

FreeCAD uses several registration macros for ordinary classes, document
objects, and properties. They all contribute edges to the same runtime TypeId
graph. Keeping discovery here gives consumers such as document-object and
property typing one normalized source of truth.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
from typing import Mapping

from .model import SOURCE_EXTENSIONS
from .parsing import extract_balanced, split_top_level, strip_comments

_TYPE_REGISTRATION_MACROS = (
    "PROPERTY_SOURCE_ABSTRACT_WITH_EXTENSIONS",
    "PROPERTY_SOURCE_WITH_EXTENSIONS",
    "PROPERTY_SOURCE_ABSTRACT",
    "PROPERTY_SOURCE_TEMPLATE",
    "PROPERTY_SOURCE",
    "TYPESYSTEM_SOURCE_ABSTRACT",
    "TYPESYSTEM_SOURCE_TEMPLATE_T",
    "TYPESYSTEM_SOURCE",
)
_TYPE_REGISTRATION_MARKER = re.compile(
    rf"\b(?P<macro>{'|'.join(map(re.escape, _TYPE_REGISTRATION_MACROS))})\s*\("
)
_CPP_NAME = re.compile(r"^(?:[A-Za-z_]\w*::)*[A-Za-z_]\w*$")


@dataclass(frozen=True)
class TypeNode:
    """One C++ TypeId registration and its direct parent."""

    type_id: str
    parent: str | None
    source: str
    line: int
    is_abstract: bool = False


@dataclass(frozen=True)
class TypeHierarchy:
    """Lookup data for a C++ TypeId inheritance graph."""

    nodes: Mapping[str, TypeNode]

    def chain(self, type_id: str) -> tuple[str, ...]:
        """Return *type_id* followed by all registered ancestors."""

        result: list[str] = []
        current: str | None = type_id
        seen: set[str] = set()
        while current is not None:
            if current in seen:
                raise ValueError(f"Cycle in C++ TypeId inheritance at {current}")
            result.append(current)
            seen.add(current)
            node = self.nodes.get(current)
            current = node.parent if node is not None else None
        return tuple(result)

    def is_derived_from(self, type_id: str, ancestor: str) -> bool:
        return ancestor in self.chain(type_id)


def _normalize_cpp_name(value: str) -> str:
    return "::".join(part.strip() for part in value.strip().split("::"))


def _valid_cpp_name(value: str) -> bool:
    return bool(_CPP_NAME.fullmatch(value)) and not value.startswith("_")


def discover_type_hierarchy(root: Path) -> TypeHierarchy:
    """Parse all supported TypeId registrations below ``src``.

    ``PROPERTY_SOURCE`` is used by ``App::DocumentObject`` descendants while
    ``TYPESYSTEM_SOURCE`` covers ordinary ``BaseClass`` descendants. Both
    macros ultimately register the same TypeId graph. Macro definitions in
    headers are ignored by rejecting their placeholder arguments.
    """

    nodes: dict[str, TypeNode] = {}
    source_root = root / "src"
    for path in sorted(
        path
        for path in source_root.rglob("*")
        if path.is_file() and path.suffix in SOURCE_EXTENSIONS
    ):
        text = path.read_text(encoding="utf-8", errors="replace")
        source_text = strip_comments(text)
        source = path.relative_to(root).as_posix()
        for match in _TYPE_REGISTRATION_MARKER.finditer(source_text):
            opening = source_text.find("(", match.start(), match.end())
            try:
                arguments, _ = extract_balanced(source_text, opening, "(", ")")
            except ValueError:
                continue
            parts = split_top_level(arguments)
            if len(parts) < 2:
                continue
            type_id = _normalize_cpp_name(parts[0])
            parent = _normalize_cpp_name(parts[1])
            if not _valid_cpp_name(type_id) or not _valid_cpp_name(parent):
                continue
            node = TypeNode(
                type_id,
                parent,
                source,
                text.count("\n", 0, match.start()) + 1,
                is_abstract="_ABSTRACT" in match.group("macro"),
            )
            previous = nodes.get(type_id)
            if previous is not None:
                if (previous.parent, previous.is_abstract) != (node.parent, node.is_abstract):
                    raise ValueError(
                        f"Conflicting C++ registration for {type_id}: "
                        f"({previous.parent!r}, abstract={previous.is_abstract}) at "
                        f"{previous.source}:{previous.line} and "
                        f"({node.parent!r}, abstract={node.is_abstract}) at "
                        f"{node.source}:{node.line}"
                    )
                continue
            nodes[type_id] = node
    return TypeHierarchy(nodes)
