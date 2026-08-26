# SPDX-License-Identifier: LGPL-2.1-or-later

"""Resolve Python contracts for the core ``App::Property*`` hierarchy.

The source-adjacent ``src/App/PropertyPythonContracts.pyi`` file contains
only conversion roots and overrides. The C++ ``TYPESYSTEM_SOURCE`` and
``TYPESYSTEM_SOURCE_ABSTRACT`` registrations, discovered by
:mod:`property_hierarchy`, provide inheritance.
"""

from __future__ import annotations

import ast
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING, Mapping

if TYPE_CHECKING:
    from .type_hierarchy import TypeHierarchy

from .property_hierarchy import discover_conversion_overrides

PROPERTY_METADATA_PATH = Path("src/App/PropertyPythonContracts.pyi")


@dataclass(frozen=True)
class AliasSpec:
    """One generated private alias and its source-level expression."""

    name: str
    expression: str


@dataclass(frozen=True)
class TypeSpec:
    """One Python type expression, optionally exposed through an alias."""

    expression: str
    alias: AliasSpec | None = None


@dataclass(frozen=True)
class PropertyContract:
    """One explicit conversion root or override in the metadata catalog."""

    type_id: str
    getter: TypeSpec | None = None
    setter: TypeSpec | None = None


@dataclass(frozen=True)
class PropertyCatalog:
    """All aliases and conversion contracts from the source-adjacent input."""

    aliases: tuple[AliasSpec, ...]
    contracts: tuple[PropertyContract, ...]


@dataclass(frozen=True)
class ResolvedPropertyContract:
    """Complete getter/setter contract resolved for one concrete TypeId."""

    type_id: str
    getter: str
    setter: str
    getter_alias: AliasSpec | None = None
    setter_alias: AliasSpec | None = None


@dataclass(frozen=True)
class ConversionMetadataIssue:
    """A C++ conversion override without an explicit adjacent contract."""

    type_id: str
    direction: str
    source: str
    line: int

    def format(self) -> str:
        return (
            f"{self.source}:{self.line}: {self.type_id} overrides "
            f"{self.direction} conversion but has no explicit Python contract"
        )


def _metadata_aliases(tree: ast.Module) -> dict[str, AliasSpec]:
    aliases: dict[str, AliasSpec] = {}
    for node in tree.body:
        if not isinstance(node, ast.AnnAssign) or not isinstance(node.target, ast.Name):
            continue
        annotation = ast.unparse(node.annotation)
        if annotation not in {"TypeAlias", "typing.TypeAlias"}:
            continue
        aliases[node.target.id] = AliasSpec(node.target.id, ast.unparse(node.value))
    return aliases


def _annotation_spec(annotation: ast.expr, aliases: Mapping[str, AliasSpec]) -> TypeSpec:
    if isinstance(annotation, ast.Name) and annotation.id in aliases:
        alias = aliases[annotation.id]
        return TypeSpec(alias.expression, alias)
    return TypeSpec(ast.unparse(annotation))


def load_property_catalog(root: Path) -> PropertyCatalog:
    """Read aliases and conversion roots/overrides in one metadata parse."""

    path = root / PROPERTY_METADATA_PATH
    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    aliases_by_name = _metadata_aliases(tree)
    contracts: list[PropertyContract] = []
    for node in tree.body:
        if not isinstance(node, ast.ClassDef) or not node.name.startswith("Property"):
            continue

        getter: TypeSpec | None = None
        setter: TypeSpec | None = None
        for member in node.body:
            if not isinstance(member, (ast.FunctionDef, ast.AsyncFunctionDef)):
                continue
            if member.name == "get":
                if member.returns is None:
                    raise ValueError(f"{path}:{member.lineno}: getter metadata needs a return type")
                getter = _annotation_spec(member.returns, aliases_by_name)
            elif member.name == "set" and len(member.args.args) >= 2:
                value_annotation = member.args.args[1].annotation
                if value_annotation is None:
                    raise ValueError(f"{path}:{member.lineno}: setter metadata needs a value type")
                setter = _annotation_spec(value_annotation, aliases_by_name)

        contracts.append(PropertyContract(f"App::{node.name}", getter, setter))

    return PropertyCatalog(tuple(aliases_by_name.values()), tuple(contracts))


def _resolve_direction(
    chain: tuple[str, ...],
    direction: str,
    contracts: Mapping[str, PropertyContract],
) -> TypeSpec:
    for type_id in chain:
        contract = contracts.get(type_id)
        if contract is None:
            continue
        spec = getattr(contract, direction)
        if spec is not None:
            return spec
    raise KeyError(f"No Python {direction} contract is cataloged for {chain[0]!r}")


def property_contract(
    type_id: str,
    hierarchy: TypeHierarchy,
    catalog: PropertyCatalog,
) -> ResolvedPropertyContract:
    """Resolve the complete Python contract for one ``App::Property*`` TypeId."""

    chain = hierarchy.chain(type_id)
    contracts = {contract.type_id: contract for contract in catalog.contracts}
    getter = _resolve_direction(chain, "getter", contracts)
    setter = _resolve_direction(chain, "setter", contracts)
    return ResolvedPropertyContract(
        type_id,
        getter.expression,
        setter.expression,
        getter.alias,
        setter.alias,
    )


def conversion_metadata_issues(
    root: Path,
    hierarchy: TypeHierarchy,
    catalog: PropertyCatalog,
) -> tuple[ConversionMetadataIssue, ...]:
    """Find conversion overrides in families covered by the metadata input.

    The catalog is intentionally incremental while additional Core property
    families are migrated. Overrides in an unrelated, still-uncataloged
    family are left for that family's first metadata entry; once a family has
    a root contract, every direct getter/setter override must be explicit.
    """

    contracts = {contract.type_id: contract for contract in catalog.contracts}
    issues: list[ConversionMetadataIssue] = []
    for override in discover_conversion_overrides(root):
        chain = hierarchy.chain(override.type_id)
        if not any(type_id in contracts for type_id in chain):
            continue
        contract = contracts.get(override.type_id)
        if contract is None or getattr(contract, override.direction) is None:
            issues.append(
                ConversionMetadataIssue(
                    override.type_id,
                    override.direction,
                    override.source,
                    override.line,
                )
            )
    return tuple(issues)


def render_property_aliases(catalog: PropertyCatalog) -> str:
    """Render source metadata aliases as a support fragment for ``FreeCAD``."""

    lines = [
        "from collections.abc import Sequence",
        "from . import Base as Base",
        "from typing import TypeAlias",
        "",
    ]
    lines.extend(f"{spec.name}: TypeAlias = {spec.expression}" for spec in catalog.aliases)
    return "\n".join(lines) + "\n"
