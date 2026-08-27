# pyright: strict

"""Generate structural BIM protocols from literal property declarations."""

from __future__ import annotations

import ast
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path

from .module_merge import generated_stub_header
from .property_contracts import PropertyCatalog, property_contract
from .property_declarations import (
    BIM_GENERATED_PROTOCOL_CLASSES,
    BIM_PROTOCOL_CLASSES,
    discover_property_declarations,
)
from .property_hierarchy import PropertyHierarchy


@dataclass(frozen=True)
class GeneratedBIMProtocol:
    """One generated protocol and the property members discovered for it."""

    source: str
    protocol_name: str
    base_protocols: tuple[str, ...]
    properties: tuple[tuple[str, str, str], ...]


def _base_name(node: ast.expr) -> str | None:
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        return node.attr
    return None


def _class_bases(source: str) -> dict[str, tuple[str, ...]]:
    tree = ast.parse(source)
    return {
        node.name: tuple(
            base_name for base in node.bases if (base_name := _base_name(base)) is not None
        )
        for node in tree.body
        if isinstance(node, ast.ClassDef)
    }


def _owner_protocols(
    protocol_classes: Mapping[str, Mapping[str, str]],
) -> dict[str, str]:
    return {
        owner: protocol
        for class_map in protocol_classes.values()
        for owner, protocol in class_map.items()
    }


def _expand_aliases(expression: str, catalog: PropertyCatalog) -> str:
    aliases = {alias.name: alias.expression for alias in catalog.aliases}
    expanded = expression
    for _ in range(len(aliases) + 1):
        current = expanded
        for name, replacement in aliases.items():
            expanded = expanded.replace(name, replacement)
        if expanded == current:
            return expanded
    raise ValueError(f"cyclic Core property aliases while expanding {expression!r}")


def _protocol_properties(
    root: Path,
    source_name: str,
    owner: str,
    protocol_name: str,
    hierarchy: PropertyHierarchy,
    catalog: PropertyCatalog,
    protocol_classes: Mapping[str, str],
) -> tuple[tuple[str, str, str], ...]:
    declarations = discover_property_declarations(
        root,
        paths=(Path(source_name),),
        protocol_classes={source_name: protocol_classes},
    )
    properties: list[tuple[str, str, str]] = []
    seen: set[str] = set()
    for declaration in declarations:
        if declaration.owner_class != owner or declaration.protocol_class != protocol_name:
            continue
        if declaration.property_name in seen:
            continue
        if not declaration.property_name.isidentifier():
            raise ValueError(
                f"{declaration.source}:{declaration.line}: cannot generate a protocol "
                f"member for invalid property name {declaration.property_name!r}"
            )
        try:
            contract = property_contract(declaration.type_id, hierarchy, catalog)
        except KeyError as error:
            raise ValueError(
                f"{declaration.source}:{declaration.line}: no complete Core property "
                f"contract for {declaration.type_id}"
            ) from error
        properties.append(
            (
                declaration.property_name,
                _expand_aliases(contract.getter, catalog),
                _expand_aliases(contract.setter, catalog),
            )
        )
        seen.add(declaration.property_name)
    return tuple(properties)


def discover_generated_bim_protocols(
    root: Path,
    hierarchy: PropertyHierarchy,
    catalog: PropertyCatalog,
    generated_protocol_classes: Mapping[str, Mapping[str, str]] = BIM_GENERATED_PROTOCOL_CLASSES,
    inherited_protocol_classes: Mapping[str, Mapping[str, str]] = BIM_PROTOCOL_CLASSES,
) -> tuple[GeneratedBIMProtocol, ...]:
    """Discover generated BIM protocols and resolve their Core property contracts."""

    all_protocol_classes = {**inherited_protocol_classes, **generated_protocol_classes}
    owner_protocols = _owner_protocols(all_protocol_classes)
    protocols: list[GeneratedBIMProtocol] = []
    for source_name, class_map in generated_protocol_classes.items():
        source_path = root / source_name
        source = source_path.read_text(encoding="utf-8")
        class_bases = _class_bases(source)
        for owner, protocol_name in class_map.items():
            base_protocols = tuple(
                base_protocol
                for base in class_bases.get(owner, ())
                if (base_protocol := owner_protocols.get(base)) is not None
                and base_protocol != protocol_name
            )
            protocols.append(
                GeneratedBIMProtocol(
                    source=source_name,
                    protocol_name=protocol_name,
                    base_protocols=base_protocols or ("Protocol",),
                    properties=_protocol_properties(
                        root,
                        source_name,
                        owner,
                        protocol_name,
                        hierarchy,
                        catalog,
                        class_map,
                    ),
                )
            )
    return tuple(protocols)


def _render_property(name: str, getter: str, setter: str) -> list[str]:
    return [
        "    @property",
        f"    def {name}(self) -> {getter}: ...",
        "",
        f"    @{name}.setter",
        f"    def {name}(self, value: {setter}) -> None: ...",
        "",
    ]


def render_bim_protocols(
    root: Path,
    hierarchy: PropertyHierarchy,
    catalog: PropertyCatalog,
    generated_protocol_classes: Mapping[str, Mapping[str, str]] = BIM_GENERATED_PROTOCOL_CLASSES,
    inherited_protocol_classes: Mapping[str, Mapping[str, str]] = BIM_PROTOCOL_CLASSES,
) -> str:
    """Render the generated BIM protocol package source."""

    protocols = discover_generated_bim_protocols(
        root,
        hierarchy,
        catalog,
        generated_protocol_classes,
        inherited_protocol_classes,
    )
    generated_names = {protocol.protocol_name for protocol in protocols}
    external_bases = {
        base
        for protocol in protocols
        for base in protocol.base_protocols
        if base not in generated_names and base != "Protocol"
    }
    annotations = [
        annotation
        for protocol in protocols
        for _, getter, setter in protocol.properties
        for annotation in (getter, setter)
    ]

    lines = [generated_stub_header(), "", "from __future__ import annotations", ""]
    if any("Sequence[" in annotation for annotation in annotations):
        lines.append("from collections.abc import Sequence")
    lines.append("from typing import Protocol")
    if any("Base." in annotation for annotation in annotations):
        lines.append("from FreeCAD import Base")
    if "ArchComponentObject" in external_bases:
        lines.append("from ArchTypeHints import ArchComponentObject")
    lines.extend(["", ""])

    for index, protocol in enumerate(protocols):
        bases = ", ".join((*protocol.base_protocols, "Protocol"))
        lines.append(f"class {protocol.protocol_name}({bases}):")
        lines.append(f'    """Generated from {protocol.source} addProperty declarations."""')
        lines.append("")
        if not protocol.properties:
            lines.append("    pass")
        else:
            for property_name, getter, setter in protocol.properties:
                lines.extend(_render_property(property_name, getter, setter))
        if index != len(protocols) - 1:
            lines.extend(["", ""])
    return "\n".join(lines).rstrip() + "\n"


def write_bim_protocols(
    out_dir: Path,
    root: Path,
    hierarchy: PropertyHierarchy,
    catalog: PropertyCatalog,
) -> Path:
    """Write the generated BIM protocol package and return its init stub path."""

    package_dir = out_dir / "bim_typing"
    package_dir.mkdir(parents=True, exist_ok=True)
    target = package_dir / "__init__.pyi"
    target.write_text(render_bim_protocols(root, hierarchy, catalog), encoding="utf-8")
    return target
