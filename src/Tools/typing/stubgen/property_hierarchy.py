# SPDX-License-Identifier: LGPL-2.1-or-later

"""Discover the C++ inheritance graph for ``App::Property*`` classes.

The property conversion catalog intentionally contains only Python conversion
roots and overrides.  The C++ ``TYPESYSTEM_SOURCE`` registrations provide the
inheritance edges needed to resolve those contracts for concrete TypeIds.
"""

from __future__ import annotations

from dataclasses import dataclass, replace
from pathlib import Path
import re

from .type_hierarchy import TypeHierarchy, discover_type_hierarchy
from .parsing import extract_balanced, strip_comments

_CPP_HEADER_EXTENSIONS = {".h", ".hh", ".hpp", ".hxx"}
_PROPERTY_CLASS = re.compile(
    r"\bclass\s+(?:[A-Za-z_]\w*\s+)*(?P<name>Property[A-Za-z_]\w*)\b[^\{;]*\{"
)
_CONVERSION_METHOD = re.compile(r"\b(?P<method>getPyObject|setPyObject)\s*\(")
_OVERRIDE_KEYWORD = re.compile(r"\boverride\b")


@dataclass(frozen=True)
class ConversionOverride:
    """One C++ ``getPyObject`` or ``setPyObject`` declaration."""

    type_id: str
    direction: str
    source: str
    line: int


PropertyHierarchy = TypeHierarchy


def property_hierarchy_from(hierarchy: TypeHierarchy) -> TypeHierarchy:
    """Project the generic TypeId graph onto ``App::Property*`` classes."""

    nodes = {
        type_id: replace(
            node,
            parent=(
                node.parent
                if node.parent is not None and node.parent.startswith("App::Property")
                else None
            ),
        )
        for type_id, node in hierarchy.nodes.items()
        if type_id.startswith("App::Property")
    }
    return TypeHierarchy(nodes)


def discover_property_hierarchy(root: Path) -> TypeHierarchy:
    """Discover and project the C++ TypeId graph onto App properties."""

    return property_hierarchy_from(discover_type_hierarchy(root))


def _matching_brace(text: str, opening: int) -> int | None:
    depth = 0
    state = "code"
    index = opening
    while index < len(text):
        char = text[index]
        next_char = text[index + 1] if index + 1 < len(text) else ""
        if state == "code":
            if char == "/" and next_char == "/":
                state = "line_comment"
                index += 2
                continue
            if char == "/" and next_char == "*":
                state = "block_comment"
                index += 2
                continue
            if char == '"':
                state = "string"
            elif char == "'":
                state = "char"
            elif char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
                if depth == 0:
                    return index + 1
        elif state == "line_comment":
            if char == "\n":
                state = "code"
        elif state == "block_comment":
            if char == "*" and next_char == "/":
                state = "code"
                index += 2
                continue
        elif state in {"string", "char"}:
            if char == "\\":
                index += 2
                continue
            if (state == "string" and char == '"') or (state == "char" and char == "'"):
                state = "code"
        index += 1
    return None


def _conversion_override_methods(
    source: str,
    start: int,
    end: int,
) -> tuple[tuple[str, int], ...]:
    """Find conversion methods with an ``override`` specifier in a class body.

    The method name is easy to find lexically, but its parameter list can
    contain nested function types and default expressions. Parse that part
    with the shared balanced-expression helper, then inspect only the
    declaration suffix for the ``override`` keyword.
    """

    result: list[tuple[str, int]] = []
    for match in _CONVERSION_METHOD.finditer(source, start, end):
        opening = source.find("(", match.start(), match.end())
        try:
            _, parameters_end = extract_balanced(source, opening, "(", ")")
        except ValueError:
            continue

        terminators = [
            position
            for position in (
                source.find(";", parameters_end, end),
                source.find("{", parameters_end, end),
            )
            if position != -1
        ]
        declaration_end = min(terminators, default=end)
        if _OVERRIDE_KEYWORD.search(source, parameters_end, declaration_end):
            result.append((match.group("method"), match.start()))
    return tuple(result)


def discover_conversion_overrides(root: Path) -> tuple[ConversionOverride, ...]:
    """Find conversion overrides declared in the App property headers.

    This deliberately checks declarations rather than trying to infer Python
    types from C++ method bodies. The adjacent metadata remains the semantic
    source for those types.
    """

    overrides: dict[tuple[str, str], ConversionOverride] = {}
    for path in sorted(
        path
        for path in (root / "src/App").rglob("*")
        if path.is_file() and path.suffix in _CPP_HEADER_EXTENSIONS
    ):
        text = path.read_text(encoding="utf-8")
        source_text = strip_comments(text)
        source = path.relative_to(root).as_posix()
        for class_match in _PROPERTY_CLASS.finditer(source_text):
            end = _matching_brace(source_text, class_match.end() - 1)
            if end is None:
                continue
            class_name = class_match.group("name")
            for method_name, method_start in _conversion_override_methods(
                source_text,
                class_match.end(),
                end,
            ):
                direction = "getter" if method_name == "getPyObject" else "setter"
                key = (f"App::{class_name}", direction)
                overrides.setdefault(
                    key,
                    ConversionOverride(
                        type_id=key[0],
                        direction=direction,
                        source=source,
                        line=text.count("\n", 0, method_start) + 1,
                    ),
                )
    return tuple(sorted(overrides.values(), key=lambda item: (item.type_id, item.direction)))
