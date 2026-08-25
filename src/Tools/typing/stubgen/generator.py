# pyright: strict

"""Coordinate the end-to-end Python stub generation pipeline.

Discovery and curated inputs are normalized into ``PythonApiModel``, resolved
before rendering, and only then replace the previous generated output.
"""

from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import shutil

from python_api_model.diagnostics import MergeDiagnostic, MergeDiagnostics
from python_api_model.model import PythonApiModel
from python_api_model.resolve import merge_api_models

from .api_extract import extract_curated_api_model_with_diagnostics
from .binding_adapter import adapt_discovered_bindings
from .cpp_properties import (
    CppPropertyDiagnostic,
    CppPropertyReport,
    discover_cpp_properties,
    typed_cpp_properties,
)
from .document_object_types import direct_python_types, document_object_python_types
from .generated_api import (
    add_cpp_properties_to_model,
    add_document_overloads_to_model,
    generated_constant_model,
)
from .init_exports import load_init_exports
from .model import BindingClass, BindingMethod, StubSignatureOverrides
from .module_merge import ensure_parent_package_stubs, module_stub_path, public_module_names
from .project import Project
from .property_contracts import conversion_metadata_issues, load_property_catalog
from .property_hierarchy import property_hierarchy_from
from .render import write_stub_file
from .stub_support import StubSupport, collect_stub_support
from .type_hierarchy import discover_type_hierarchy
from .validation import validate_discovered_bindings, validate_public_class_aliases


@dataclass(frozen=True)
class GenerationResult:
    """Result of one stub generation attempt, including merge diagnostics."""

    overlay_count: int = 0
    diagnostics: tuple[MergeDiagnostic, ...] = ()
    cpp_property_report: CppPropertyReport = CppPropertyReport(0, 0, ())

    @property
    def errors(self) -> tuple[MergeDiagnostic, ...]:
        return MergeDiagnostics(self.diagnostics).errors


def write_public_module_stubs(
    out_dir: Path,
    module_names: set[str],
    api_model: PythonApiModel,
    support: StubSupport,
) -> None:
    """Write every normalized API module, including class-only modules."""

    ensure_parent_package_stubs(out_dir, module_names)
    for module in api_model.modules:
        write_stub_file(
            module_stub_path(out_dir, module.name, module_names),
            module=module,
            support=support,
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


def write_pep561_markers(out_dir: Path, module_names: set[str]) -> None:
    top_level = {name.split(".", 1)[0] for name in module_names}
    for pkg in sorted(top_level):
        (out_dir / pkg).mkdir(parents=True, exist_ok=True)
        (out_dir / pkg / "py.typed").touch()


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
    """Generate stubs from one resolved ``PythonApiModel``."""

    validate_public_class_aliases(classes)
    validate_discovered_bindings(methods, type_registrations)
    module_names = public_module_names(methods, classes, type_registrations, overlay_dir)
    api_model, diagnostics = extract_curated_api_model_with_diagnostics(
        root,
        source_dir,
        binding_classes=classes,
        overlay_dir=overlay_dir,
    )
    generated_model = adapt_discovered_bindings(
        methods,
        type_registrations,
        stub_signature_overrides,
    )
    merge_result = merge_api_models(api_model, generated_model)
    diagnostics = diagnostics + merge_result.diagnostics
    api_model = merge_result.value
    if MergeDiagnostics(diagnostics).errors:
        return GenerationResult(diagnostics=diagnostics)

    hierarchy = discover_type_hierarchy(root)
    property_catalog = load_property_catalog(root)
    conversion_issues = conversion_metadata_issues(
        root,
        property_hierarchy_from(hierarchy),
        property_catalog,
    )
    if conversion_issues:
        formatted = "\n".join(issue.format() for issue in conversion_issues)
        raise ValueError("Core property conversion metadata is incomplete:\n" + formatted)

    constants_model, generated_support = generated_constant_model(
        root,
        property_catalog,
        load_init_exports(root),
    )
    merge_result = merge_api_models(api_model, constants_model)
    diagnostics += merge_result.diagnostics
    api_model = merge_result.value

    property_diagnostics: list[CppPropertyDiagnostic] = []
    discovered_properties = discover_cpp_properties(root, hierarchy, property_diagnostics)
    typed_properties = typed_cpp_properties(
        discovered_properties,
        hierarchy,
        property_catalog,
        direct_python_types(
            classes, type_ids={item.owner_type_id for item in discovered_properties}
        ),
        property_diagnostics,
    )
    cpp_property_report = CppPropertyReport(
        len(discovered_properties),
        len(typed_properties),
        tuple(property_diagnostics),
    )
    api_model, property_support = add_cpp_properties_to_model(api_model, typed_properties)
    registrations = document_object_python_types(
        hierarchy,
        direct_python_types(classes, hierarchy),
    )
    api_model, document_support = add_document_overloads_to_model(api_model, registrations)

    result = GenerationResult(diagnostics=diagnostics, cpp_property_report=cpp_property_report)
    if result.errors:
        return result

    out_dir.mkdir(parents=True, exist_ok=True)
    # Resolve the complete model before deleting the previous generated tree.
    shutil.rmtree(out_dir / "stubs", ignore_errors=True)

    module_names.update(module.name for module in api_model.modules)
    support = collect_stub_support(root, source_dir, api_model, tuple(classes), overlay_dir)
    support = StubSupport(
        module_fragments=(
            *support.module_fragments,
            *generated_support,
            *property_support,
            *document_support,
        ),
        class_fragments=support.class_fragments,
    )
    write_public_module_stubs(out_dir / "stubs", module_names, api_model, support)
    write_pep561_markers(out_dir / "stubs", module_names)
    project = Project(root)
    project.write_pyproject(out_dir)
    project.write_readme(out_dir)
    return GenerationResult(
        overlay_count=len(tuple(overlay_dir.rglob("*.pyi"))) if overlay_dir is not None else 0,
        diagnostics=diagnostics,
        cpp_property_report=cpp_property_report,
    )
