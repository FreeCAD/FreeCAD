# pyright: strict

"""Orchestration layer for FreeCAD Python binding stub generation.

This module is the coordination point for the stub pipeline. It keeps the
public entrypoints stable while delegating the detailed work to narrower
modules:
- ``discovery`` inventories C++ registrations and PyCXX types
- ``source_inputs`` reads curated binding, module, and type stub inputs
- ``render`` turns normalized bindings into textual stub fragments
- ``module_merge`` assembles module bodies and support nodes
- ``class_merge`` normalizes binding classes and materializes model classes
  plus alias exports

Keep the command-facing pipeline wiring here. Discovery heuristics, source-input
parsing, and AST merge behavior should live in the specialized modules instead
of growing this file again.
"""

from __future__ import annotations

from collections import Counter
from pathlib import Path
import shutil

from .class_merge import (
    append_api_model_class_stubs,
    normalize_api_model_binding_class_headers,
    validate_public_class_aliases,
)
from .diagnostics import (
    MergeDiagnostic,
    discovered_model_diagnostics,
    generated_output_diagnostics,
)
from .module_merge import (
    copy_module_support_stubs,
    copy_overlay_stubs,
    copy_type_support_stubs,
    ensure_parent_package_stubs,
    merge_api_module_aliases_into_stubs,
    merge_api_module_attributes_into_stubs,
    module_stub_path,
    public_module_names,
)
from .model import BindingClass, BindingMethod, StubSignatureOverrides
from .python_api.extract import extract_curated_api_model_with_diagnostics
from .python_api.adapters import merge_discovered_bindings
from .python_api.model import ApiModel
from .render import write_stub_file


def write_public_module_stubs(
    out_dir: Path,
    module_names: set[str],
    stub_signature_overrides: StubSignatureOverrides,
    api_model: ApiModel,
) -> None:
    api_modules = {module.name: module for module in api_model.modules}
    ensure_parent_package_stubs(out_dir, module_names)
    module_names_to_write: set[str] = set()
    module_names_to_write.update(
        module.name
        for module in api_model.modules
        if module.functions or module.attributes or module.aliases
    )
    for module_name in sorted(module_names_to_write):
        write_stub_file(
            module_stub_path(out_dir, module_name, module_names),
            [],
            stub_signature_overrides=stub_signature_overrides,
            api_module=api_modules.get(module_name),
            module_name=module_name,
        )


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


def write_outputs(
    out_dir: Path,
    root: Path,
    source_dir: Path,
    methods: list[BindingMethod],
    classes: list[BindingClass],
    type_registrations: dict[str, list[str]],
    stub_signature_overrides: StubSignatureOverrides,
    overlay_dir: Path | None = None,
    diagnostics: list[MergeDiagnostic] | None = None,
) -> int:
    validate_public_class_aliases(classes)
    out_dir.mkdir(parents=True, exist_ok=True)
    for generated_dir in ("stubs",):
        shutil.rmtree(out_dir / generated_dir, ignore_errors=True)

    module_names = public_module_names(methods, classes, type_registrations, overlay_dir)
    api_model, model_diagnostics = extract_curated_api_model_with_diagnostics(
        root,
        source_dir,
        binding_classes=classes,
    )
    module_names.update(module.name for module in api_model.modules)
    if diagnostics is not None:
        diagnostics.extend(model_diagnostics)
    api_model = normalize_api_model_binding_class_headers(root, classes, api_model)
    api_model = merge_discovered_bindings(
        api_model,
        methods,
        type_registrations,
        stub_signature_overrides,
    )
    module_names.update(module.name for module in api_model.modules)
    if diagnostics is not None:
        diagnostics.extend(discovered_model_diagnostics(classes, api_model))
    write_public_module_stubs(
        out_dir / "stubs",
        module_names,
        stub_signature_overrides,
        api_model,
    )
    overlay_count = (
        copy_overlay_stubs(overlay_dir, out_dir / "stubs", module_names) if overlay_dir else 0
    )
    copy_module_support_stubs(root, source_dir, out_dir / "stubs", module_names)
    merge_api_module_attributes_into_stubs(out_dir / "stubs", api_model, module_names)
    append_api_model_class_stubs(out_dir / "stubs", api_model, module_names)
    copy_type_support_stubs(root, source_dir, out_dir / "stubs", module_names)
    merge_api_module_aliases_into_stubs(out_dir / "stubs", api_model, module_names)
    if diagnostics is not None:
        diagnostics.extend(generated_output_diagnostics(out_dir / "stubs", api_model, module_names))
    return overlay_count
