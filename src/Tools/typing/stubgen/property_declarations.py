# SPDX-License-Identifier: LGPL-2.1-or-later

"""Discover and verify simple Python ``App::Property*`` declarations.

The first pass is intentionally conservative.  It recognizes calls whose
TypeId and property name are literal strings, records their runtime owner and
matching structural protocol, and checks only protocol properties with an
explicit getter/setter pair.  Dynamic declarations and protocol fields that
are not yet covered by the Core property catalog remain outside this
conservative validation pass without becoming false-positive failures.
"""

from __future__ import annotations

import ast
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Mapping, Sequence

from .property_contracts import (
    PropertyCatalog,
    ResolvedPropertyContract,
    load_property_catalog,
    property_contract,
)
from .property_hierarchy import PropertyHierarchy, discover_property_hierarchy

DRAFT_PROPERTY_SOURCES: tuple[Path, ...] = tuple(
    Path("src/Mod/Draft/draftobjects") / f"{name}.py"
    for name in (
        "circle",
        "ellipse",
        "polygon",
        "rectangle",
        "fillet",
        "clone",
        "shape2dview",
        "text",
        "point",
        "shapestring",
        "bezcurve",
        "bspline",
        "wire",
    )
)

BIM_PROPERTY_SOURCES: tuple[Path, ...] = tuple(
    Path("src/Mod/BIM") / name
    for name in (
        "ArchComponent.py",
        "ArchBuildingPart.py",
        "ArchSectionPlane.py",
        "ArchFrame.py",
        "ArchEquipment.py",
        "ArchRoof.py",
        "ArchWindow.py",
        "ArchWall.py",
        "ArchSpace.py",
    )
)

BIM_PROTOCOL_CLASSES: Mapping[str, Mapping[str, str]] = {
    "src/Mod/BIM/ArchComponent.py": {"Component": "ArchComponentObject"},
    "src/Mod/BIM/ArchBuildingPart.py": {"BuildingPart": "ArchBuildingPartObject"},
    "src/Mod/BIM/ArchSectionPlane.py": {
        "_SectionPlane": "ArchSectionPlaneObject",
        "_ViewProviderSectionPlane": "ArchSectionPlaneViewObject",
    },
    "src/Mod/BIM/ArchFrame.py": {"_Frame": "_FrameObject"},
    "src/Mod/BIM/ArchEquipment.py": {"_Equipment": "ArchEquipmentObject"},
    "src/Mod/BIM/ArchRoof.py": {"_Roof": "_RoofObject"},
    "src/Mod/BIM/ArchWindow.py": {"_Window": "_WindowObject"},
    "src/Mod/BIM/ArchWall.py": {"_Wall": "_WallObject"},
    "src/Mod/BIM/ArchSpace.py": {"_Space": "_SpaceObject"},
}


@dataclass(frozen=True)
class PropertyDeclaration:
    """One literal ``addProperty`` declaration in a Python source file."""

    source: str
    line: int
    owner_class: str | None
    protocol_class: str | None
    property_name: str
    type_id: str


@dataclass(frozen=True)
class ProtocolProperty:
    """Getter/setter annotations for one property in a structural protocol."""

    getter: str | None = None
    setter: str | None = None


@dataclass(frozen=True)
class ProtocolContractIssue:
    """A mismatch between a runtime property declaration and its protocol."""

    source: str
    line: int
    protocol_class: str
    property_name: str
    type_id: str
    direction: str
    expected: str
    actual: str

    def format(self) -> str:
        return (
            f"{self.source}:{self.line}: {self.protocol_class}.{self.property_name} "
            f"{self.direction} for {self.type_id} is {self.actual!r}; "
            f"expected {self.expected!r}"
        )


class _DeclarationVisitor(ast.NodeVisitor):
    def __init__(
        self,
        source: str,
        class_names: set[str],
        protocol_classes: Mapping[str, str] | None = None,
    ) -> None:
        self.source = source
        self.class_names = class_names
        self.protocol_classes = protocol_classes or {}
        self.class_stack: list[str] = []
        self.declarations: list[PropertyDeclaration] = []

    def visit_ClassDef(self, node: ast.ClassDef) -> None:
        self.class_stack.append(node.name)
        self.generic_visit(node)
        self.class_stack.pop()

    def visit_Call(self, node: ast.Call) -> None:
        if isinstance(node.func, ast.Attribute) and node.func.attr == "addProperty":
            type_id = _literal_string(node.args, 0)
            property_name = _literal_string(node.args, 1)
            if type_id is not None and property_name is not None:
                owner_class = self.class_stack[-1] if self.class_stack else None
                protocol_name = self.protocol_classes.get(owner_class or "")
                if protocol_name is None:
                    protocol_name = (
                        f"{owner_class}Object"
                        if owner_class is not None and f"{owner_class}Object" in self.class_names
                        else None
                    )
                self.declarations.append(
                    PropertyDeclaration(
                        source=self.source,
                        line=node.lineno,
                        owner_class=owner_class,
                        protocol_class=protocol_name,
                        property_name=property_name,
                        type_id=type_id,
                    )
                )
        self.generic_visit(node)


def _literal_string(args: list[ast.expr], index: int) -> str | None:
    if index >= len(args):
        return None
    value = args[index]
    return value.value if isinstance(value, ast.Constant) and isinstance(value.value, str) else None


def _source_name(root: Path, path: Path) -> str:
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        return path.as_posix()


def _parse_source(source: str) -> ast.Module:
    return ast.parse(source)


def parse_property_declarations(
    source: str,
    source_name: str,
    protocol_classes: Mapping[str, str] | None = None,
) -> tuple[PropertyDeclaration, ...]:
    """Parse literal property declarations from one source string."""

    tree = _parse_source(source)
    class_names = {node.name for node in ast.walk(tree) if isinstance(node, ast.ClassDef)}
    visitor = _DeclarationVisitor(source_name, class_names, protocol_classes)
    visitor.visit(tree)
    return tuple(visitor.declarations)


def discover_property_declarations(
    root: Path,
    paths: Sequence[Path] = DRAFT_PROPERTY_SOURCES,
    protocol_classes: Mapping[str, Mapping[str, str]] | None = None,
) -> tuple[PropertyDeclaration, ...]:
    """Discover declarations from repository-relative source paths."""

    declarations: list[PropertyDeclaration] = []
    for relative_path in paths:
        path = relative_path if relative_path.is_absolute() else root / relative_path
        if not path.exists():
            raise FileNotFoundError(f"Property declaration source does not exist: {path}")
        source_name = _source_name(root, path)
        declarations.extend(
            parse_property_declarations(
                path.read_text(encoding="utf-8"),
                source_name,
                (protocol_classes or {}).get(source_name),
            )
        )
    return tuple(declarations)


def _decorated_property_name(decorator: ast.expr) -> tuple[str, str] | None:
    if isinstance(decorator, ast.Name) and decorator.id == "property":
        return "getter", ""
    if (
        isinstance(decorator, ast.Attribute)
        and decorator.attr == "setter"
        and isinstance(decorator.value, ast.Name)
    ):
        return "setter", decorator.value.id
    return None


def _annotation(node: ast.expr | None) -> str | None:
    return ast.unparse(node) if node is not None else None


def protocol_properties(
    source: str,
    protocol_class: str,
    inherited_sources: Sequence[str] = (),
) -> dict[str, ProtocolProperty]:
    """Read explicit property getter/setter annotations from one protocol."""

    trees = [_parse_source(source)] + [_parse_source(item) for item in inherited_sources]
    visited: set[str] = set()

    def collect(name: str) -> dict[str, ProtocolProperty]:
        if name in visited:
            return {}
        visited.add(name)

        protocol: ast.ClassDef | None = None
        for tree in trees:
            protocol = next(
                (
                    node
                    for node in tree.body
                    if isinstance(node, ast.ClassDef) and node.name == name
                ),
                None,
            )
            if protocol is not None:
                break
        if protocol is None:
            return {}

        properties: dict[str, ProtocolProperty] = {}
        for base in protocol.bases:
            if isinstance(base, ast.Name):
                properties.update(collect(base.id))

        for node in protocol.body:
            if isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
                annotation = _annotation(node.annotation)
                properties[node.target.id] = ProtocolProperty(
                    getter=annotation,
                    setter=annotation,
                )
                continue
            if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                continue

            for decorator in node.decorator_list:
                decorated = _decorated_property_name(decorator)
                if decorated is None:
                    continue
                direction, property_name = decorated
                if direction == "getter":
                    properties[property_name or node.name] = ProtocolProperty(
                        getter=_annotation(node.returns)
                    )
                elif len(node.args.args) >= 2:
                    current = properties.get(property_name, ProtocolProperty())
                    properties[property_name] = ProtocolProperty(
                        getter=current.getter,
                        setter=_annotation(node.args.args[1].annotation),
                    )
        return properties

    return collect(protocol_class)


_PROTOCOL_DIRECT_NAMES = {
    "bool": "bool",
    "float": "float",
    "int": "int",
    "str": "str",
    "Base.Quantity": "Quantity",
    "Base.Placement": "App.Placement",
    "FreeCAD.Placement": "App.Placement",
    "Base.Vector": "Vector",
}


_PROTOCOL_ALIAS_EXPRESSIONS = {
    "App.Placement": {"FreeCAD.Placement"},
    "DocumentObjectList": {"list[DocumentObject]", "list[DocumentObject | None]"},
    "DocumentObjectSubLinkListInput": {
        "_DocumentObjectSubLinkInput | Sequence[_DocumentObjectSubLinkListItemInput]"
    },
    "DocumentObjectSubLinkValue": {
        "tuple[DocumentObject, list[str]]",
        "tuple[DocumentObject, list[str]] | None",
    },
    "FloatList": {"list[float]"},
    "FloatListInput": {"Sequence[float]"},
    "IntegerConstraintInput": {"int | dict[str, int] | tuple[int, int, int, int]"},
    "IntegerList": {"list[int]"},
    "IntegerListInput": {"Sequence[int]"},
    "StringList": {"list[str]"},
    "StringListInput": {"Sequence[str]"},
    "VectorList": {"list[Vector]"},
    "VectorListInput": {"Sequence[VectorInput]"},
    "VectorValue": {"Vector"},
}


def _protocol_annotations_match(expected: str, actual: str) -> bool:
    return actual == expected or actual in _PROTOCOL_ALIAS_EXPRESSIONS.get(expected, set())


def _protocol_annotation(contract: ResolvedPropertyContract, direction: str) -> str | None:
    alias = contract.getter_alias if direction == "getter" else contract.setter_alias
    if alias is not None:
        return alias.name.removeprefix("_")
    expression = contract.getter if direction == "getter" else contract.setter
    return _PROTOCOL_DIRECT_NAMES.get(expression)


def validate_protocol_property_contracts(
    root: Path,
    paths: Sequence[Path] = DRAFT_PROPERTY_SOURCES,
    hierarchy: PropertyHierarchy | None = None,
    protocol_classes: Mapping[str, Mapping[str, str]] | None = None,
    inherited_source_paths: Sequence[Path] | None = None,
    catalog: PropertyCatalog | None = None,
    require_complete: bool = False,
) -> tuple[ProtocolContractIssue, ...]:
    """Verify cataloged declarations against explicit protocol members."""

    hierarchy = hierarchy or discover_property_hierarchy(root)
    catalog = catalog or load_property_catalog(root)
    issues: list[ProtocolContractIssue] = []
    source_cache: dict[str, str] = {}
    protocol_cache: dict[tuple[str, str], dict[str, ProtocolProperty]] = {}
    inherited_source_paths = inherited_source_paths or (
        Path("src/Mod/Draft/draftobjects/type_hints.py"),
    )
    inherited_sources = tuple(
        (path if path.is_absolute() else root / path).read_text(encoding="utf-8")
        for path in inherited_source_paths
    )
    for declaration in discover_property_declarations(root, paths, protocol_classes):
        if declaration.protocol_class is None:
            continue
        source_path = root / declaration.source
        source = source_cache.setdefault(
            declaration.source, source_path.read_text(encoding="utf-8")
        )
        cache_key = (declaration.source, declaration.protocol_class)
        properties = protocol_cache.setdefault(
            cache_key, protocol_properties(source, declaration.protocol_class, inherited_sources)
        )
        try:
            contract = property_contract(declaration.type_id, hierarchy, catalog)
        except KeyError:
            continue
        protocol_property = properties.get(declaration.property_name)
        if protocol_property is None:
            if require_complete:
                issues.append(
                    ProtocolContractIssue(
                        source=declaration.source,
                        line=declaration.line,
                        protocol_class=declaration.object_class,
                        property_name=declaration.property_name,
                        type_id=declaration.type_id,
                        direction="property",
                        expected="a declared protocol member",
                        actual="<missing>",
                    )
                )
            continue

        for direction, actual in (
            ("getter", protocol_property.getter),
            ("setter", protocol_property.setter),
        ):
            expected = _protocol_annotation(contract, direction)
            if expected is None or actual is None or _protocol_annotations_match(expected, actual):
                continue
            issues.append(
                ProtocolContractIssue(
                    source=declaration.source,
                    line=declaration.line,
                    protocol_class=declaration.protocol_class,
                    property_name=declaration.property_name,
                    type_id=declaration.type_id,
                    direction=direction,
                    expected=expected,
                    actual=actual,
                )
            )
    return tuple(issues)


def format_issues(issues: Iterable[ProtocolContractIssue]) -> str:
    return "\n".join(issue.format() for issue in issues)
