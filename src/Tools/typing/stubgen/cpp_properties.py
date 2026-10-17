# SPDX-License-Identifier: LGPL-2.1-or-later

"""Discover C++-registered properties for public stub generation.

The binding ``.pyi`` inputs describe direct PyCXX members.  Properties on
``PropertyContainer`` objects are different: C++ registers them by name and
the Python wrapper resolves them dynamically through the property map.  This
module discovers that second kind of member from the existing C++ sources and
leaves direct binding generation untouched.
"""

from __future__ import annotations

import ast
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import re
from typing import Literal, Mapping

from .property_contracts import PropertyCatalog, property_contract
from .model import PublicPythonType
from .parsing import extract_balanced, line_number, split_top_level, strip_comments
from .type_hierarchy import TypeHierarchy

_CLASS_KEYWORD = re.compile(r"\bclass\b")
_CPP_IDENTIFIER = re.compile(r"[A-Za-z_]\w*")
_SCOPE_OPERATOR_WHITESPACE = re.compile(r"[ \t]*::[ \t]*")
_PROPERTY_MEMBER = re.compile(
    r"\b(?P<type>(?:[A-Za-z_]\w*::)*Property[A-Za-z_]\w*)\s+" r"(?P<declarators>[^;{}]+);"
)
_PROPERTY_REGISTRATION = re.compile(r"\b(?P<macro>ADD_PROPERTY(?:_TYPE)?)\s*\(")
_HEADER_INCLUDE = re.compile(r"^\s*#\s*include\s*[<\"]([^>\"]+)[>\"]", re.MULTILINE)
_CONSTRUCTOR = re.compile(r"\b(?P<name>[A-Za-z_]\w*)::(?P=name)\s*\(")


@dataclass(frozen=True)
class CppProperty:
    """One statically registered C++ property and its declared property type."""

    owner_type_id: str
    property_name: str
    property_type_id: str
    source: str
    line: int
    documentation: str | None = None


@dataclass(frozen=True)
class TypedCppProperty:
    """A discovered C++ property resolved to its public Python contract."""

    owner: PublicPythonType
    name: str
    getter: str
    setter: str | None
    source: str
    line: int
    documentation: str | None = None


CppPropertyDiagnosticKind = Literal[
    "missing_header",
    "missing_class",
    "missing_member",
    "unscoped_registration",
    "unresolved_owner",
    "unresolved_contract",
]


@dataclass(frozen=True)
class CppPropertyDiagnostic:
    """A discoverable C++ property that needs attention or was skipped."""

    kind: CppPropertyDiagnosticKind
    owner_type_id: str
    source: str
    line: int
    property_name: str | None = None
    detail: str = ""

    def format(self) -> str:
        target = self.owner_type_id
        if self.property_name:
            target += f".{self.property_name}"
        message = f"{self.source}:{self.line}: {target}: {self.kind.replace('_', ' ')}"
        return f"{message}: {self.detail}" if self.detail else message


@dataclass(frozen=True)
class CppPropertyReport:
    """Summary of C++ property discovery and public-stub generation."""

    discovered_count: int
    typed_count: int
    diagnostics: tuple[CppPropertyDiagnostic, ...]

    def summary(self) -> str:
        unresolved = self.discovered_count - self.typed_count
        lines = [
            f"C++ dynamic properties: {self.discovered_count} discovered, "
            f"{self.typed_count} generated, {unresolved} unresolved"
        ]
        if not self.diagnostics:
            return lines[0]

        counts = Counter(diagnostic.kind for diagnostic in self.diagnostics)
        lines.append("C++ dynamic property diagnostics:")
        for kind, count in sorted(counts.items()):
            examples = [
                diagnostic.format() for diagnostic in self.diagnostics if diagnostic.kind == kind
            ][:3]
            example_text = "; ".join(examples)
            suffix = f" (examples: {example_text})" if example_text else ""
            lines.append(f"  {kind.replace('_', ' ')}: {count}{suffix}")
        return "\n".join(lines)


def _diagnose(
    diagnostics: list[CppPropertyDiagnostic] | None,
    kind: CppPropertyDiagnosticKind,
    owner_type_id: str,
    source: str,
    line: int,
    property_name: str | None = None,
    detail: str = "",
) -> None:
    if diagnostics is not None:
        diagnostics.append(
            CppPropertyDiagnostic(
                kind,
                owner_type_id,
                source,
                line,
                property_name,
                detail,
            )
        )


def _class_members(header: str, class_name: str) -> dict[str, str] | None:
    source = strip_comments(header)
    for candidate_name, opening in _class_declarations(source):
        if candidate_name != class_name:
            continue
        _, end = extract_balanced(source, opening, "{", "}")
        body = _SCOPE_OPERATOR_WHITESPACE.sub("::", source[opening + 1 : end - 1])
        members: dict[str, str] = {}
        for member in _PROPERTY_MEMBER.finditer(body):
            property_type = member.group("type")
            for declarator in split_top_level(member.group("declarators")):
                match = re.fullmatch(r"\s*(?P<name>[A-Za-z_]\w*)\s*(?:=.*)?", declarator)
                if match:
                    members[match.group("name")] = property_type
        return members
    return None


def _class_declarations(source: str) -> tuple[tuple[str, int], ...]:
    """Return class names and opening-brace offsets from a header."""

    declarations: list[tuple[str, int]] = []
    for class_match in _CLASS_KEYWORD.finditer(source):
        cursor = class_match.end()
        opening = source.find("{", cursor)
        semicolon = source.find(";", cursor)
        if opening == -1 or (semicolon != -1 and semicolon < opening):
            continue

        declaration = source[cursor:opening].split(":", 1)[0]
        identifiers = list(_CPP_IDENTIFIER.finditer(declaration))
        identifiers = [identifier for identifier in identifiers if identifier.group() != "final"]
        if identifiers:
            declarations.append((identifiers[-1].group(), opening))
    return tuple(declarations)


def _header_path(root: Path, source_path: Path, source: str) -> Path | None:
    """Find the header defining the registered class in *source*.

    Most files use a matching ``.h`` name, but a number of FreeCAD classes
    keep a historical ``Feature*.cpp``/``Feature*.h`` pairing or include the
    declaration from a differently named header.  Prefer the matching stem,
    then resolve ordinary local and ``src``-relative includes.
    """

    for extension in (".h", ".hh", ".hpp", ".hxx"):
        candidate = source_path.with_suffix(extension)
        if candidate.exists():
            return candidate

    for include in _HEADER_INCLUDE.findall(source):
        include_path = Path(include)
        candidates = (
            source_path.parent / include_path,
            root / "src" / include_path,
            root / include_path,
        )
        for candidate in candidates:
            if candidate.exists() and candidate.is_file():
                return candidate
    return None


def _property_members(
    root: Path,
    hierarchy: TypeHierarchy,
    owner_type_id: str,
) -> tuple[dict[str, str], bool, bool]:
    """Collect direct property members from an owner and its C++ ancestors."""

    members: dict[str, str] = {}
    found_header = False
    found_class = False
    for type_id in hierarchy.chain(owner_type_id):
        node = hierarchy.nodes.get(type_id)
        if node is None:
            continue
        source_path = root / node.source
        if not source_path.exists():
            continue
        source = source_path.read_text(encoding="utf-8")
        header_path = _header_path(root, source_path, source)
        if header_path is None:
            continue
        found_header = True
        class_name = type_id.rsplit("::", 1)[-1]
        direct_members = _class_members(
            header_path.read_text(encoding="utf-8"),
            class_name,
        )
        if direct_members is None:
            continue
        found_class = True
        for name, property_type in direct_members.items():
            members.setdefault(name, property_type)
    return members, found_header, found_class


def _registered_properties(source: str) -> list[tuple[str, int, str | None]]:
    source_without_comments = strip_comments(source)
    result: list[tuple[str, int, str | None]] = []
    for match in _PROPERTY_REGISTRATION.finditer(source_without_comments):
        arguments, _ = extract_balanced(
            source_without_comments,
            match.end() - 1,
            "(",
            ")",
        )
        parts = split_top_level(arguments)
        if not parts:
            continue
        name = parts[0].strip()
        if not re.fullmatch(r"[A-Za-z_]\w*", name) or name.startswith("_"):
            continue

        documentation = None
        if match.group("macro") == "ADD_PROPERTY_TYPE":
            candidate = parts[-1].strip()
            try:
                value = ast.literal_eval(candidate)
            except (ValueError, SyntaxError):
                value = None
            if isinstance(value, str) and value:
                documentation = value
        result.append((name, line_number(source, match.start()), documentation))
    return result


def _property_type_id(cpp_type: str, owner_type_id: str) -> str | None:
    cpp_type = re.sub(r"\s+", "", cpp_type)
    name = cpp_type.rsplit("::", 1)[-1]
    if not name.startswith("Property"):
        return None
    namespace = (
        cpp_type.rsplit("::", 1)[0] if "::" in cpp_type else owner_type_id.rsplit("::", 1)[0]
    )
    return f"{namespace}::{name}"


def _constructor_body_opening(source: str, after_parameters: int) -> int | None:
    """Find a constructor body while skipping braced initializers."""

    opening = source.find("{", after_parameters)
    while opening != -1:
        _, end = extract_balanced(source, opening, "{", "}")
        next_index = end
        while next_index < len(source) and source[next_index].isspace():
            next_index += 1
        if next_index < len(source) and source[next_index] in ",{":
            opening = source.find("{", end)
            continue
        return opening
    return None


def _constructor_properties(
    source: str,
    class_name: str,
) -> tuple[bool, list[tuple[str, int, str | None]]]:
    """Return property registrations from a class's out-of-line constructor."""

    source_without_comments = strip_comments(source)
    result: list[tuple[str, int, str | None]] = []
    found_definition = False
    for match in _CONSTRUCTOR.finditer(source_without_comments):
        if match.group("name") != class_name:
            continue
        _, after_parameters = extract_balanced(
            source_without_comments,
            match.end() - 1,
            "(",
            ")",
        )
        opening = _constructor_body_opening(source_without_comments, after_parameters)
        if opening is None:
            continue
        semicolon = source_without_comments.find(";", after_parameters, opening)
        if semicolon != -1:
            continue
        found_definition = True
        body, _ = extract_balanced(source_without_comments, opening, "{", "}")
        base_line = line_number(source, opening) - 1
        result.extend(
            (name, base_line + line, documentation)
            for name, line, documentation in _registered_properties(body)
        )
    return found_definition, result


def discover_cpp_properties(
    root: Path,
    hierarchy: TypeHierarchy,
    diagnostics: list[CppPropertyDiagnostic] | None = None,
) -> tuple[CppProperty, ...]:
    """Discover properties registered by C++ object classes.

    A source file with several ``PROPERTY_SOURCE`` classes is resolved through
    each class's out-of-line constructor. This avoids attributing one class's
    properties to another while still covering the common multi-registration
    source layout. Registrations outside a constructor remain intentionally
    conservative and are left for a later scoped parser.
    """

    by_source: dict[str, list[str]] = {}
    for type_id, node in hierarchy.nodes.items():
        by_source.setdefault(node.source, []).append(type_id)

    properties: dict[tuple[str, str], CppProperty] = {}
    member_cache: dict[str, tuple[dict[str, str], bool, bool]] = {}
    for source, type_ids in sorted(by_source.items()):
        source_path = root / source
        if source_path.suffix not in {".cc", ".cpp", ".cxx"} or not source_path.exists():
            continue
        source_text = source_path.read_text(encoding="utf-8")
        registrations_by_owner: dict[str, tuple[bool, list[tuple[str, int, str | None]]]] = {}
        for owner_type_id in type_ids:
            class_name = owner_type_id.rsplit("::", 1)[-1]
            has_constructor, registrations = _constructor_properties(source_text, class_name)
            if not has_constructor and len(type_ids) == 1:
                registrations = _registered_properties(source_text)
            if registrations:
                registrations_by_owner[owner_type_id] = (has_constructor, registrations)

        if not registrations_by_owner:
            if len(type_ids) > 1 and _PROPERTY_REGISTRATION.search(strip_comments(source_text)):
                owner_type_id = type_ids[0]
                _diagnose(
                    diagnostics,
                    "unscoped_registration",
                    owner_type_id,
                    source,
                    hierarchy.nodes[owner_type_id].line,
                    detail="could not associate registrations with a constructor",
                )
            continue

        for owner_type_id, (has_constructor, registrations) in registrations_by_owner.items():
            if owner_type_id not in member_cache:
                member_cache[owner_type_id] = _property_members(
                    root,
                    hierarchy,
                    owner_type_id,
                )
            members, found_header, found_class = member_cache[owner_type_id]
            if not found_header:
                _diagnose(
                    diagnostics,
                    "missing_header",
                    owner_type_id,
                    source,
                    hierarchy.nodes[owner_type_id].line,
                    detail="could not locate the class declaration header",
                )
                continue
            if not found_class:
                _diagnose(
                    diagnostics,
                    "missing_class",
                    owner_type_id,
                    source,
                    hierarchy.nodes[owner_type_id].line,
                    detail="could not find the class declaration in the available headers",
                )
                continue
            for name, line, documentation in registrations:
                if not has_constructor:
                    _diagnose(
                        diagnostics,
                        "unscoped_registration",
                        owner_type_id,
                        source,
                        line,
                        name,
                        "using source-wide registration fallback",
                    )
                cpp_type = members.get(name)
                if cpp_type is None:
                    _diagnose(
                        diagnostics,
                        "missing_member",
                        owner_type_id,
                        source,
                        line,
                        name,
                        "registration has no matching property member in the header",
                    )
                    continue
                property_type_id = _property_type_id(cpp_type, owner_type_id)
                if property_type_id is None:
                    _diagnose(
                        diagnostics,
                        "missing_member",
                        owner_type_id,
                        source,
                        line,
                        name,
                        f"unsupported property member type {cpp_type!r}",
                    )
                    continue
                key = (owner_type_id, name)
                property_ = CppProperty(
                    owner_type_id,
                    name,
                    property_type_id,
                    source,
                    line,
                    documentation,
                )
                previous = properties.get(key)
                if previous is not None and previous.property_type_id != property_type_id:
                    raise ValueError(
                        f"Conflicting C++ property types for {owner_type_id}.{name}: "
                        f"{previous.property_type_id} at {previous.source}:{previous.line} and "
                        f"{property_type_id} at {source}:{line}"
                    )
                properties[key] = property_
    return tuple(properties.values())


def typed_cpp_properties(
    properties: tuple[CppProperty, ...],
    hierarchy: TypeHierarchy,
    catalog: PropertyCatalog,
    python_types: Mapping[str, PublicPythonType],
    diagnostics: list[CppPropertyDiagnostic] | None = None,
) -> tuple[TypedCppProperty, ...]:
    """Resolve discoverable C++ properties through the Core catalog.

    Properties whose conversion family is not cataloged yet are intentionally
    omitted.  This makes the prototype conservative: missing metadata becomes
    an inventory item instead of an invented public type.
    """

    typed: list[TypedCppProperty] = []
    for property_ in properties:
        owner = python_types.get(property_.owner_type_id)
        if owner is None:
            _diagnose(
                diagnostics,
                "unresolved_owner",
                property_.owner_type_id,
                property_.source,
                property_.line,
                property_.property_name,
                "no public binding class is available for this C++ TypeId",
            )
            continue
        try:
            contract = property_contract(property_.property_type_id, hierarchy, catalog)
        except KeyError:
            _diagnose(
                diagnostics,
                "unresolved_contract",
                property_.owner_type_id,
                property_.source,
                property_.line,
                property_.property_name,
                f"no Python conversion contract for {property_.property_type_id}",
            )
            continue
        typed.append(
            TypedCppProperty(
                owner,
                property_.property_name,
                contract.getter,
                contract.setter,
                property_.source,
                property_.line,
                property_.documentation,
            )
        )
    return tuple(typed)
