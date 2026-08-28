# pyright: strict

"""Orchestration layer for FreeCAD Python binding stub generation.

This module is the coordination point for the stub pipeline. It keeps the
public entrypoints stable while delegating the detailed work to narrower
modules:
- ``discovery`` inventories C++ registrations and PyCXX types
- ``source_inputs`` reads curated binding, module, and type stub inputs
- ``render`` turns normalized bindings into textual stub fragments
- ``module_merge`` assembles module bodies and support nodes
- ``class_merge`` appends public class stubs and alias exports

Keep the command-facing pipeline wiring here. Discovery heuristics, source-input
parsing, and AST merge behavior should live in the specialized modules instead
of growing this file again.
"""

from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import shutil

from .class_merge import (
    append_class_stubs,
    validate_public_class_aliases,
)
from .cpp_properties import (
    CppPropertyDiagnostic,
    CppPropertyReport,
    TypedCppProperty,
    add_cpp_properties,
    discover_cpp_properties,
    typed_cpp_properties,
)
from .module_merge import (
    copy_module_support_stubs,
    copy_overlay_stubs,
    copy_type_support_stubs,
    ensure_parent_package_stubs,
    merge_module_support_nodes,
    module_stub_path,
    public_module_names,
    public_stub_symbols,
)
from .discovery import (
    group_methods,
    group_type_methods_by_public_module,
    module_names_from_type_methods,
)
from .document_object_types import (
    add_document_add_object_overloads,
    direct_python_types,
    document_object_python_types,
)
from .init_exports import ModuleExport, load_init_exports, render_init_exports
from .model import BindingClass, BindingMethod, PublicTypeGroup, StubSignatureOverrides
from .property_contracts import (
    PropertyCatalog,
    conversion_metadata_issues,
    load_property_catalog,
    render_property_aliases,
)
from .property_hierarchy import property_hierarchy_from
from .render import type_stub_lines, write_stub_file
from .type_hierarchy import TypeHierarchy, discover_type_hierarchy


@dataclass(frozen=True)
class GenerationResult:
    """Results from generating the public stub tree."""

    overlay_count: int
    cpp_property_report: CppPropertyReport


def write_public_module_stubs(
    out_dir: Path,
    methods: list[BindingMethod],
    module_names: set[str],
    stub_signature_overrides: StubSignatureOverrides,
) -> None:
    module_methods, _, _ = group_methods(methods)
    ensure_parent_package_stubs(out_dir, module_names)
    for module_name, group in sorted(module_methods.items()):
        write_stub_file(
            module_stub_path(out_dir, module_name, module_names),
            group,
            stub_signature_overrides=stub_signature_overrides,
        )


def append_type_stubs(
    out_dir: Path,
    methods: list[BindingMethod],
    type_registrations: dict[str, list[str]],
    stub_signature_overrides: StubSignatureOverrides,
    module_names: set[str] | None = None,
    supplemental_groups: dict[str, list[PublicTypeGroup]] | None = None,
) -> int:
    module_names = module_names or module_names_from_type_methods(methods, type_registrations)
    grouped = group_type_methods_by_public_module(methods, type_registrations)
    seen = {
        (module_name, type_group.class_symbol, type_group.variable_symbol)
        for module_name, type_groups in grouped.items()
        for type_group in type_groups
    }
    for module_name, type_groups in (supplemental_groups or {}).items():
        for type_group in type_groups:
            key = (module_name, type_group.class_symbol, type_group.variable_symbol)
            if key in seen:
                continue
            seen.add(key)
            grouped.setdefault(module_name, []).append(type_group)
    count = 0
    for module_name, type_groups in sorted(grouped.items()):
        path = module_stub_path(out_dir, module_name, module_names)
        path.parent.mkdir(parents=True, exist_ok=True)
        existing = path.read_text(encoding="utf-8") if path.exists() else ""
        existing_symbols = public_stub_symbols(existing)
        type_groups = [
            type_group
            for type_group in type_groups
            if type_group.class_symbol not in existing_symbols
            and (
                not type_group.variable_symbol or type_group.variable_symbol not in existing_symbols
            )
        ]
        if not type_groups:
            continue
        lines = "\n".join(
            type_stub_lines(
                type_groups,
                stub_signature_overrides,
                include_future_import=not existing.strip(),
            )
        ).rstrip()
        separator = "\n\n" if existing else ""
        path.write_text(existing.rstrip() + separator + lines + "\n", encoding="utf-8")
        count += len(type_groups)
    return count


def append_property_aliases(
    out_dir: Path,
    module_names: set[str],
    catalog: PropertyCatalog,
) -> None:
    """Add generated ``App::Property*`` aliases to the public FreeCAD stub."""

    target = module_stub_path(out_dir, "FreeCAD", module_names)
    if not target.exists():
        raise FileNotFoundError(f"Expected generated FreeCAD module stub: {target}")

    original = target.read_text(encoding="utf-8")
    merged = merge_module_support_nodes(original, render_property_aliases(catalog))
    if merged != original:
        target.write_text(merged, encoding="utf-8")


def append_cpp_properties(
    out_dir: Path,
    module_names: set[str],
    root: Path,
    classes: list[BindingClass],
    hierarchy: TypeHierarchy,
    property_catalog: PropertyCatalog,
) -> CppPropertyReport:
    """Add C++ property-container members to generated public class stubs."""

    diagnostics: list[CppPropertyDiagnostic] = []
    discovered_properties = discover_cpp_properties(root, hierarchy, diagnostics)
    public_types = direct_python_types(
        classes,
        type_ids={property_.owner_type_id for property_ in discovered_properties},
    )
    properties = typed_cpp_properties(
        discovered_properties,
        hierarchy,
        property_catalog,
        public_types,
        diagnostics,
    )
    by_module: dict[str, list[TypedCppProperty]] = {}
    for property_ in properties:
        by_module.setdefault(property_.owner.module_name, []).append(property_)

    for module_name, module_properties in sorted(by_module.items()):
        target = module_stub_path(out_dir, module_name, module_names)
        if not target.exists():
            raise FileNotFoundError(f"Expected generated {module_name} stub: {target}")
        original = target.read_text(encoding="utf-8")
        merged = add_cpp_properties(original, tuple(module_properties))
        if merged != original:
            target.write_text(merged, encoding="utf-8")
    return CppPropertyReport(len(discovered_properties), len(properties), tuple(diagnostics))


def append_init_exports(
    out_dir: Path,
    module_names: set[str],
    root: Path,
) -> None:
    """Add structured Python-bootstrap exports to their public module stub."""

    exports = load_init_exports(root)
    by_module: dict[str, list[ModuleExport]] = {}
    for export in exports:
        by_module.setdefault(export.module, []).append(export)

    for module_name, module_exports in by_module.items():
        target = module_stub_path(out_dir, module_name, module_names)
        if not target.exists():
            raise FileNotFoundError(f"Expected generated {module_name} stub: {target}")

        original = target.read_text(encoding="utf-8")
        merged = merge_module_support_nodes(original, render_init_exports(tuple(module_exports)))
        if merged != original:
            target.write_text(merged, encoding="utf-8")


def markdown_report(methods: list[BindingMethod]) -> str:
    by_family = Counter(method.family for method in methods)
    by_context = Counter(
        method.inferred_module or f"{method.context_kind}:{method.context_name}"
        for method in methods
    )
    generated_count = sum(method.generated_source for method in methods)

    lines = [
        "# FreeCAD Python Binding Inventory",
        "",
        f"Total registrations: {len(methods)}",
        f"Generated implementation sources included: {generated_count}",
        "",
        "## Families",
        "",
    ]
    for family, count in by_family.most_common():
        lines.append(f"- `{family}`: {count}")

    lines.extend(["", "## Contexts", ""])
    for context, count in by_context.most_common():
        lines.append(f"- `{context}`: {count}")

    lines.extend(["", "## Registrations", ""])
    for method in methods:
        context = method.inferred_module or f"{method.context_kind}:{method.context_name}"
        doc = method.doc.splitlines()[0].strip() if method.doc else ""
        doc_suffix = f" - {doc}" if doc else ""
        lines.append(
            f"- `{context}.{method.python_name}` "
            f"({method.family}, {method.method_kind}) "
            f"[`{method.source}:{method.line}`]{doc_suffix}"
        )

    return "\n".join(lines) + "\n"


def append_document_add_object_overloads(
    out_dir: Path,
    module_names: set[str],
    classes: list[BindingClass],
    hierarchy: TypeHierarchy,
) -> None:
    """Add TypeId-derived overloads to the generated public Document class."""

    target = module_stub_path(out_dir, "FreeCAD", module_names)
    if not target.exists():
        raise FileNotFoundError(f"Expected generated FreeCAD module stub: {target}")

    registrations = document_object_python_types(
        hierarchy,
        direct_python_types(classes, hierarchy),
    )
    original = target.read_text(encoding="utf-8")
    merged = add_document_add_object_overloads(original, registrations)
    if merged != original:
        target.write_text(merged, encoding="utf-8")


def write_outputs(
    out_dir: Path,
    root: Path,
    source_dir: Path,
    methods: list[BindingMethod],
    classes: list[BindingClass],
    type_registrations: dict[str, list[str]],
    stub_signature_overrides: StubSignatureOverrides,
    overlay_dir: Path | None = None,
) -> GenerationResult:
    validate_public_class_aliases(classes)
    out_dir.mkdir(parents=True, exist_ok=True)
    for generated_dir in ("stubs",):
        shutil.rmtree(out_dir / generated_dir, ignore_errors=True)

    module_names = public_module_names(methods, classes, type_registrations, overlay_dir)
    type_hierarchy = discover_type_hierarchy(root)
    property_catalog = load_property_catalog(root)
    property_hierarchy = property_hierarchy_from(type_hierarchy)
    conversion_issues = conversion_metadata_issues(root, property_hierarchy, property_catalog)
    if conversion_issues:
        formatted = "\n".join(issue.format() for issue in conversion_issues)
        raise ValueError("Core property conversion metadata is incomplete:\n" + formatted)
    write_public_module_stubs(out_dir / "stubs", methods, module_names, stub_signature_overrides)
    overlay_count = (
        copy_overlay_stubs(overlay_dir, out_dir / "stubs", module_names) if overlay_dir else 0
    )
    copy_module_support_stubs(root, source_dir, out_dir / "stubs", module_names)
    append_property_aliases(out_dir / "stubs", module_names, property_catalog)
    append_type_stubs(
        out_dir / "stubs",
        methods,
        type_registrations,
        stub_signature_overrides,
        module_names,
    )
    append_class_stubs(out_dir / "stubs", root, classes, module_names)
    copy_type_support_stubs(root, source_dir, out_dir / "stubs", module_names)
    cpp_property_report = append_cpp_properties(
        out_dir / "stubs",
        module_names,
        root,
        classes,
        type_hierarchy,
        property_catalog,
    )
    append_document_add_object_overloads(
        out_dir / "stubs",
        module_names,
        classes,
        type_hierarchy,
    )
    append_init_exports(
        out_dir / "stubs",
        module_names,
        root,
    )
    return GenerationResult(overlay_count, cpp_property_report)
