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
from .model import BindingClass, BindingMethod, StubSignatureOverrides
from .module_merge import ensure_parent_package_stubs, module_stub_path, public_module_names
from .render import write_stub_file
from .stub_support import StubSupport, collect_stub_support
from .validation import validate_public_class_aliases


@dataclass(frozen=True)
class GenerationResult:
    """Result of one stub generation attempt, including merge diagnostics."""

    overlay_count: int = 0
    diagnostics: tuple[MergeDiagnostic, ...] = ()

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
    result = GenerationResult(diagnostics=diagnostics)
    if result.errors:
        return result

    out_dir.mkdir(parents=True, exist_ok=True)
    # Resolve the complete model before deleting the previous generated tree.
    shutil.rmtree(out_dir / "stubs", ignore_errors=True)

    api_model = merge_result.value
    module_names.update(module.name for module in api_model.modules)
    support = collect_stub_support(root, source_dir, api_model, tuple(classes), overlay_dir)
    write_public_module_stubs(out_dir / "stubs", module_names, api_model, support)
    return GenerationResult(
        overlay_count=len(tuple(overlay_dir.rglob("*.pyi"))) if overlay_dir is not None else 0,
        diagnostics=diagnostics,
    )
