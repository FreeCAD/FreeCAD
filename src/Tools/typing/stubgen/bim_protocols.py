# pyright: strict

"""Generate BIM object stubs from literal property declarations."""

from __future__ import annotations

import ast
from collections.abc import Mapping
from dataclasses import dataclass
import json
import os
from pathlib import Path

from .module_merge import generated_stub_header
from .property_contracts import PropertyCatalog, property_contract
from .property_declarations import (
    BIM_GENERATED_OBJECT_CLASSES,
    BIM_GENERATED_RUNTIME_BASES,
    BIM_MANUAL_OBJECT_CLASSES,
    BIM_TYPE_CHECK_SOURCES,
    discover_property_declarations,
)
from .property_hierarchy import PropertyHierarchy


@dataclass(frozen=True)
class GeneratedBIMObject:
    """One generated BIM object stub and its discovered property members."""

    source: str
    object_name: str
    base_types: tuple[str, ...]
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


def _owner_objects(
    object_classes: Mapping[str, Mapping[str, str]],
) -> dict[str, str]:
    return {
        owner: object_name
        for class_map in object_classes.values()
        for owner, object_name in class_map.items()
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


def _object_properties(
    root: Path,
    source_name: str,
    owner: str,
    object_name: str,
    hierarchy: PropertyHierarchy,
    catalog: PropertyCatalog,
    object_classes: Mapping[str, str],
) -> tuple[tuple[str, str, str], ...]:
    declarations = discover_property_declarations(
        root,
        paths=(Path(source_name),),
        object_classes={source_name: object_classes},
    )
    properties: list[tuple[str, str, str]] = []
    seen: set[str] = set()
    for declaration in declarations:
        if declaration.owner_class != owner or declaration.object_class != object_name:
            continue
        if declaration.property_name in seen:
            continue
        if not declaration.property_name.isidentifier():
            raise ValueError(
                f"{declaration.source}:{declaration.line}: cannot generate an object "
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


def discover_generated_bim_objects(
    root: Path,
    hierarchy: PropertyHierarchy,
    catalog: PropertyCatalog,
    generated_object_classes: Mapping[str, Mapping[str, str]] = BIM_GENERATED_OBJECT_CLASSES,
    inherited_object_classes: Mapping[str, Mapping[str, str]] = BIM_MANUAL_OBJECT_CLASSES,
    runtime_base_types: Mapping[str, tuple[str, ...]] = BIM_GENERATED_RUNTIME_BASES,
) -> tuple[GeneratedBIMObject, ...]:
    """Discover generated BIM objects and resolve their Core property contracts."""

    all_object_classes = {**inherited_object_classes, **generated_object_classes}
    owner_objects = _owner_objects(all_object_classes)
    objects: list[GeneratedBIMObject] = []
    for source_name, class_map in generated_object_classes.items():
        source_path = root / source_name
        source = source_path.read_text(encoding="utf-8")
        class_bases = _class_bases(source)
        for owner, object_name in class_map.items():
            inherited_base_types = tuple(
                base_object
                for base in class_bases.get(owner, ())
                if (base_object := owner_objects.get(base)) is not None
                and base_object != object_name
            )
            base_types = tuple(
                dict.fromkeys((*runtime_base_types.get(object_name, ()), *inherited_base_types))
            )
            objects.append(
                GeneratedBIMObject(
                    source=source_name,
                    object_name=object_name,
                    base_types=base_types,
                    properties=_object_properties(
                        root,
                        source_name,
                        owner,
                        object_name,
                        hierarchy,
                        catalog,
                        class_map,
                    ),
                )
            )
    return tuple(objects)


def _base_import(base: str) -> str:
    if "." in base:
        module, _ = base.rsplit(".", 1)
        return f"import {module}"
    if base == "DocumentObject":
        return "from FreeCAD import DocumentObject"
    return f"from ArchTypeHints import {base}"


def _render_property(name: str, getter: str, setter: str) -> list[str]:
    return [
        "    @property",
        f"    def {name}(self) -> {getter}: ...",
        "",
        f"    @{name}.setter",
        f"    def {name}(self, value: {setter}) -> None: ...",
        "",
    ]


def render_bim_objects(
    root: Path,
    hierarchy: PropertyHierarchy,
    catalog: PropertyCatalog,
    generated_object_classes: Mapping[str, Mapping[str, str]] = BIM_GENERATED_OBJECT_CLASSES,
    inherited_object_classes: Mapping[str, Mapping[str, str]] = BIM_MANUAL_OBJECT_CLASSES,
    runtime_base_types: Mapping[str, tuple[str, ...]] = BIM_GENERATED_RUNTIME_BASES,
) -> str:
    """Render the generated BIM object package source."""

    objects = discover_generated_bim_objects(
        root,
        hierarchy,
        catalog,
        generated_object_classes,
        inherited_object_classes,
        runtime_base_types,
    )
    generated_names = {obj.object_name for obj in objects}
    base_types = {
        base
        for obj in objects
        for base in (obj.base_types or ("DocumentObject",))
        if base not in generated_names
    }
    annotations = [
        annotation
        for obj in objects
        for _, getter, setter in obj.properties
        for annotation in (getter, setter)
    ]

    lines = [generated_stub_header(), "", "from __future__ import annotations", ""]
    if any("Sequence[" in annotation for annotation in annotations):
        lines.append("from collections.abc import Sequence")
    if any("Base." in annotation for annotation in annotations):
        lines.append("from FreeCAD import Base")
    imports = {_base_import(base) for base in base_types}
    if any("DocumentObject" in annotation for annotation in annotations):
        imports.add("from FreeCAD import DocumentObject")
    lines.extend(sorted(imports))
    lines.extend(["", ""])

    for index, obj in enumerate(objects):
        bases = ", ".join(obj.base_types or ("DocumentObject",))
        lines.append(f"class {obj.object_name}({bases}):")
        lines.append(f'    """Generated from {obj.source} addProperty declarations."""')
        lines.append("")
        if not obj.properties:
            lines.append("    pass")
        else:
            for property_name, getter, setter in obj.properties:
                lines.extend(_render_property(property_name, getter, setter))
        if index != len(objects) - 1:
            lines.extend(["", ""])
    return "\n".join(lines).rstrip() + "\n"


def write_bim_objects(
    out_dir: Path,
    root: Path,
    hierarchy: PropertyHierarchy,
    catalog: PropertyCatalog,
) -> Path:
    """Write the generated BIM object package and return its init stub path."""

    package_dir = out_dir / "bim_typing"
    package_dir.mkdir(parents=True, exist_ok=True)
    target = package_dir / "__init__.pyi"
    target.write_text(render_bim_objects(root, hierarchy, catalog), encoding="utf-8")
    return target


def _toml_array(values: tuple[Path, ...]) -> str:
    return "[\n" + ",\n".join(f"    {json.dumps(str(value))}" for value in values) + "\n]"


def write_bim_checker_configs(stubs_dir: Path, root: Path) -> tuple[Path, Path]:
    """Write disposable checker configs using all registry-enabled BIM sources."""

    smoke_dir = root / "src/Tools/typing/smoke"
    generated_sources = tuple(root / source for source in BIM_TYPE_CHECK_SOURCES)
    included_paths = tuple(
        dict.fromkeys(
            (
                smoke_dir / "smoke.py",
                smoke_dir / "property_protocols.py",
                *generated_sources,
            )
        )
    )
    search_paths = (stubs_dir, root / "src/Mod/Draft", root / "src/Mod/BIM")

    def relative(path: Path) -> str:
        return os.path.relpath(path, stubs_dir)

    pyright_config = stubs_dir / "_bim_pyrightconfig.json"
    pyright_config.write_text(
        json.dumps(
            {
                "include": [relative(path) for path in included_paths],
                "extraPaths": [relative(path) for path in search_paths],
                "reportMissingImports": "error",
                "reportMissingModuleSource": "none",
                "typeCheckingMode": "basic",
            },
            indent=4,
        )
        + "\n",
        encoding="utf-8",
    )

    pyrefly_config = stubs_dir / "_bim_pyrefly.toml"
    pyrefly_config.write_text(
        "project-includes = "
        + _toml_array(tuple(Path(relative(path)) for path in included_paths))
        + "\nsearch-path = "
        + _toml_array(tuple(Path(relative(path)) for path in search_paths))
        + '\nignore-missing-imports = ["Arch_rc", "pivy"]\n',
        encoding="utf-8",
    )
    return pyright_config, pyrefly_config
